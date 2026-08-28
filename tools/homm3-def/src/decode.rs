//! Packet-level DEF decoders.

use crate::sprite::word;
use crate::{Encoding, Error, Frame, Result};

/// One decoded run from a scanline.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Run<'a> {
    /// A repeated palette/control index.
    Fill {
        /// Repeated index.
        color: u8,
        /// Pixel count.
        len: usize,
    },
    /// Literal indices borrowed directly from the encoded frame.
    Literal(&'a [u8]),
}

impl Run<'_> {
    /// Number of decoded pixels.
    #[must_use]
    pub const fn len(self) -> usize {
        match self {
            Self::Fill { len, .. } => len,
            Self::Literal(bytes) => bytes.len(),
        }
    }

    /// Whether this run contains no pixels.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.len() == 0
    }
}

/// Iterator over one stored scanline's runs.
///
/// Raw frames yield one literal. General RLE yields the retail
/// `(code,count-minus-one)` packets. Tileset/adventure encodings yield the
/// shared high-three-bit packet grammar; adventure rows restart at each
/// 32-pixel segment offset.
#[derive(Clone, Debug)]
pub struct RowRuns<'a> {
    stream: &'a [u8],
    encoding: Encoding,
    row: usize,
    at: usize,
    remaining: usize,
    segments: usize,
    segment: usize,
    segment_remaining: usize,
    done: bool,
}

impl<'a> RowRuns<'a> {
    pub(crate) fn new(frame: Frame<'a>, row: usize) -> Result<Self> {
        let height = frame.cropped_height();
        if row >= height {
            return Err(Error::BadDimension {
                what: "scanline",
                value: u32::try_from(row).unwrap_or(u32::MAX),
            });
        }
        let width = frame.cropped_width();
        let stream = frame.stream();
        let encoding = frame.encoding();
        match encoding {
            Encoding::Raw => {
                let at = row
                    .checked_mul(width)
                    .ok_or(Error::SizeOverflow { what: "raw row" })?;
                let end = at
                    .checked_add(width)
                    .ok_or(Error::SizeOverflow { what: "raw row" })?;
                if end > stream.len() {
                    return Err(Error::Truncated {
                        what: "raw scanline",
                        at,
                        needed: width,
                        available: stream.len().saturating_sub(at),
                    });
                }
                Ok(Self {
                    stream,
                    encoding,
                    row,
                    at,
                    remaining: width,
                    segments: 1,
                    segment: 0,
                    segment_remaining: width,
                    done: false,
                })
            }
            Encoding::GeneralRle => {
                let table_at = row.checked_mul(4).ok_or(Error::SizeOverflow {
                    what: "RLE row table",
                })?;
                let offset = offset32(stream, table_at, row)?;
                Ok(Self {
                    stream,
                    encoding,
                    row,
                    at: offset,
                    remaining: width,
                    segments: 1,
                    segment: 0,
                    segment_remaining: width,
                    done: false,
                })
            }
            Encoding::TilesetRle => {
                let table_at = row.checked_mul(2).ok_or(Error::SizeOverflow {
                    what: "tile row table",
                })?;
                let offset = offset16(stream, table_at, row)?;
                Ok(Self {
                    stream,
                    encoding,
                    row,
                    at: offset,
                    remaining: width,
                    segments: 1,
                    segment: 0,
                    segment_remaining: width,
                    done: false,
                })
            }
            Encoding::AdventureRle => {
                if width % 32 != 0 {
                    return Err(Error::BadDimension {
                        what: "adventure row width",
                        value: u32::try_from(width).unwrap_or(u32::MAX),
                    });
                }
                let segments = width / 32;
                let table_index = row.checked_mul(segments).ok_or(Error::SizeOverflow {
                    what: "adventure row table",
                })?;
                let table_at = table_index.checked_mul(2).ok_or(Error::SizeOverflow {
                    what: "adventure row table",
                })?;
                let offset = offset16(stream, table_at, row)?;
                Ok(Self {
                    stream,
                    encoding,
                    row,
                    at: offset,
                    remaining: width,
                    segments,
                    segment: 0,
                    segment_remaining: width.min(32),
                    done: false,
                })
            }
        }
    }

    fn byte(&mut self, what: &'static str) -> Result<u8> {
        let byte = self.stream.get(self.at).copied().ok_or(Error::Truncated {
            what,
            at: self.at,
            needed: 1,
            available: 0,
        })?;
        self.at += 1;
        Ok(byte)
    }

    fn literal(&mut self, len: usize, what: &'static str) -> Result<&'a [u8]> {
        let end = self
            .at
            .checked_add(len)
            .ok_or(Error::SizeOverflow { what })?;
        let bytes = self.stream.get(self.at..end).ok_or(Error::Truncated {
            what,
            at: self.at,
            needed: len,
            available: self.stream.len().saturating_sub(self.at),
        })?;
        self.at = end;
        Ok(bytes)
    }

    fn account(&mut self, len: usize) -> Result<()> {
        let limit = if self.encoding == Encoding::AdventureRle {
            self.segment_remaining
        } else {
            self.remaining
        };
        if len > limit {
            return Err(Error::RunOverrun {
                row: self.row,
                run: len,
                remaining: limit,
            });
        }
        self.remaining -= len;
        self.segment_remaining -= len;
        Ok(())
    }

    fn advance_adventure_segment(&mut self) -> Result<bool> {
        if self.encoding != Encoding::AdventureRle || self.segment_remaining != 0 {
            return Ok(false);
        }
        self.segment += 1;
        if self.segment >= self.segments {
            return Ok(false);
        }
        let table_index = self
            .row
            .checked_mul(self.segments)
            .and_then(|base| base.checked_add(self.segment))
            .ok_or(Error::SizeOverflow {
                what: "adventure segment table",
            })?;
        let table_at = table_index.checked_mul(2).ok_or(Error::SizeOverflow {
            what: "adventure segment table",
        })?;
        self.at = offset16(self.stream, table_at, self.row)?;
        self.segment_remaining = self.remaining.min(32);
        Ok(true)
    }

    fn next_run(&mut self) -> Result<Option<Run<'a>>> {
        if self.done {
            return Ok(None);
        }
        if self.remaining == 0 {
            self.done = true;
            return Ok(None);
        }
        if self.encoding == Encoding::AdventureRle && self.segment_remaining == 0 {
            self.advance_adventure_segment()?;
        }

        match self.encoding {
            Encoding::Raw => {
                let bytes = self.literal(self.remaining, "raw scanline")?;
                let len = bytes.len();
                self.account(len)?;
                Ok(Some(Run::Literal(bytes)))
            }
            Encoding::GeneralRle => {
                let color = self.byte("general-RLE colour")?;
                let len = usize::from(self.byte("general-RLE count")?) + 1;
                self.account(len)?;
                if color == 0xff {
                    Ok(Some(Run::Literal(
                        self.literal(len, "general-RLE literal")?,
                    )))
                } else {
                    Ok(Some(Run::Fill { color, len }))
                }
            }
            Encoding::TilesetRle | Encoding::AdventureRle => {
                let tag = self.byte("packed-RLE tag")?;
                let color = tag >> 5;
                let len = usize::from(tag & 0x1f) + 1;
                self.account(len)?;
                if color == 7 {
                    Ok(Some(Run::Literal(self.literal(len, "packed-RLE literal")?)))
                } else {
                    Ok(Some(Run::Fill { color, len }))
                }
            }
        }
    }
}

impl<'a> Iterator for RowRuns<'a> {
    type Item = Result<Run<'a>>;

    fn next(&mut self) -> Option<Self::Item> {
        match self.next_run() {
            Ok(Some(run)) => Some(Ok(run)),
            Ok(None) => None,
            Err(error) => {
                self.done = true;
                Some(Err(error))
            }
        }
    }
}

fn offset32(stream: &[u8], at: usize, row: usize) -> Result<usize> {
    let end = at.checked_add(4).ok_or(Error::SizeOverflow {
        what: "RLE row offset",
    })?;
    let bytes = stream.get(at..end).ok_or(Error::Truncated {
        what: "RLE row-offset table",
        at,
        needed: 4,
        available: stream.len().saturating_sub(at),
    })?;
    let offset = word(bytes, 0);
    let value = usize::try_from(offset).map_err(|_| Error::RowOffset {
        row,
        offset,
        available: stream.len(),
    })?;
    if value >= stream.len() {
        return Err(Error::RowOffset {
            row,
            offset,
            available: stream.len(),
        });
    }
    Ok(value)
}

fn offset16(stream: &[u8], at: usize, row: usize) -> Result<usize> {
    let end = at.checked_add(2).ok_or(Error::SizeOverflow {
        what: "packed row offset",
    })?;
    let bytes = stream.get(at..end).ok_or(Error::Truncated {
        what: "packed row-offset table",
        at,
        needed: 2,
        available: stream.len().saturating_sub(at),
    })?;
    let offset = u16::from_le_bytes([bytes[0], bytes[1]]);
    let value = usize::from(offset);
    if value >= stream.len() {
        return Err(Error::RowOffset {
            row,
            offset: u32::from(offset),
            available: stream.len(),
        });
    }
    Ok(value)
}
