#![no_std]
#![forbid(unsafe_code)]
//! Allocation-free readers for engine-owned Heroes III LOD payloads.
//!
//! This crate models the exact byte surfaces consumed by the retail resource
//! manager: wrapped indexed/24-bit bitmaps, RGBA palette files, font specs and
//! glyph bytes, adventure-object masks, the CRLF/tab text grammars, and the
//! IFF/XMIDI envelope handed to Miles.

pub mod iff;

use core::fmt;

/// Bytes in the archived bitmap wrapper header.
pub const BITMAP_HEADER_SIZE: usize = 12;
/// RGB bytes following an indexed archived bitmap.
pub const BITMAP_PALETTE_SIZE: usize = 256 * 3;
/// Opaque bytes preceding the RGBA records in a PAL resource.
pub const PALETTE_HEADER_SIZE: usize = 24;
/// RGBA bytes in a PAL resource.
pub const PALETTE_DATA_SIZE: usize = 256 * 4;
/// Total bytes in a PAL resource.
pub const PALETTE_FILE_SIZE: usize = PALETTE_HEADER_SIZE + PALETTE_DATA_SIZE;
/// Bytes in the fixed font specification.
pub const FONT_SPEC_SIZE: usize = 0x1020;
/// Bytes in an adventure-object MSK resource.
pub const MASK_SIZE: usize = 14;

/// Resource format associated with a parse error.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Format {
    /// Archived bitmap payload conventionally named `.pcx`.
    Bitmap,
    /// RGBA palette resource.
    Palette,
    /// Font specification and glyph payload.
    Font,
    /// Adventure-object placement/shadow mask.
    Mask,
    /// CRLF-delimited line resource.
    Text,
    /// CRLF/tab-delimited spreadsheet resource.
    Spreadsheet,
}

/// A malformed resource payload.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    Short {
        format: Format,
        needed: usize,
        available: usize,
    },
    Length {
        format: Format,
        expected: usize,
        actual: usize,
    },
    SizeOverflow {
        format: Format,
    },
    BadBitmapDimensions {
        width: u32,
        height: u32,
    },
    BadBitmapDataSize {
        data_size: u32,
        indexed_size: usize,
        packed24_size: usize,
        trailing: usize,
    },
    BadFontRange {
        first: u8,
        last: u8,
    },
    GlyphOutOfBounds {
        character: u16,
        offset: u32,
        size: usize,
        available: usize,
    },
    BadLineEnding {
        format: Format,
        offset: usize,
    },
    MalformedQuotedField {
        format: Format,
        row: usize,
        column: usize,
    },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Short {
                format,
                needed,
                available,
            } => write!(
                f,
                "{format:?} needs at least {needed} bytes, found {available}"
            ),
            Self::Length {
                format,
                expected,
                actual,
            } => write!(f, "{format:?} needs {expected} bytes, found {actual}"),
            Self::SizeOverflow { format } => write!(f, "{format:?} dimensions overflow"),
            Self::BadBitmapDimensions { width, height } => {
                write!(f, "bitmap dimensions must be nonzero, found {width}x{height}")
            }
            Self::BadBitmapDataSize {
                data_size,
                indexed_size,
                packed24_size,
                trailing,
            } => write!(
                f,
                "bitmap data size {data_size} with {trailing} trailing bytes is neither indexed {indexed_size} nor packed-24 {packed24_size}"
            ),
            Self::BadFontRange { first, last } => {
                write!(f, "font first character {first} exceeds last character {last}")
            }
            Self::GlyphOutOfBounds {
                character,
                offset,
                size,
                available,
            } => write!(
                f,
                "font glyph {character} payload {offset:#x}+{size:#x} exceeds {available:#x} bytes"
            ),
            Self::BadLineEnding { format, offset } => {
                write!(f, "{format:?} CR at {offset:#x} is not followed by LF")
            }
            Self::MalformedQuotedField {
                format,
                row,
                column,
            } => write!(f, "{format:?} row {row} column {column} has a short quoted field"),
        }
    }
}

impl core::error::Error for Error {}

/// Pixel representation in an archived bitmap.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BitmapKind {
    /// One palette index per pixel followed by 256 RGB triples.
    Indexed8,
    /// Three packed color bytes per pixel and no trailing palette.
    Packed24,
}

/// A validated archived bitmap borrowing pixels and an optional palette.
#[derive(Clone, Copy, Debug)]
pub struct Bitmap<'a> {
    width: usize,
    height: usize,
    kind: BitmapKind,
    pixels: &'a [u8],
    palette: Option<&'a [u8]>,
}

impl<'a> Bitmap<'a> {
    /// Parse the retail 12-byte wrapper and validate its complete payload.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for zero/overflowing dimensions or inconsistent
    /// pixel and palette lengths.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        if data.len() < BITMAP_HEADER_SIZE {
            return Err(Error::Short {
                format: Format::Bitmap,
                needed: BITMAP_HEADER_SIZE,
                available: data.len(),
            });
        }
        let data_size_word = word(data, 0);
        let width_word = word(data, 4);
        let height_word = word(data, 8);
        if width_word == 0 || height_word == 0 {
            return Err(Error::BadBitmapDimensions {
                width: width_word,
                height: height_word,
            });
        }
        let width = usize::try_from(width_word).map_err(|_| Error::SizeOverflow {
            format: Format::Bitmap,
        })?;
        let height = usize::try_from(height_word).map_err(|_| Error::SizeOverflow {
            format: Format::Bitmap,
        })?;
        let indexed_size = width.checked_mul(height).ok_or(Error::SizeOverflow {
            format: Format::Bitmap,
        })?;
        let packed24_size = indexed_size.checked_mul(3).ok_or(Error::SizeOverflow {
            format: Format::Bitmap,
        })?;
        let data_size = usize::try_from(data_size_word).map_err(|_| Error::SizeOverflow {
            format: Format::Bitmap,
        })?;
        let payload_end = BITMAP_HEADER_SIZE
            .checked_add(data_size)
            .ok_or(Error::SizeOverflow {
                format: Format::Bitmap,
            })?;
        let pixels = data
            .get(BITMAP_HEADER_SIZE..payload_end)
            .ok_or(Error::Short {
                format: Format::Bitmap,
                needed: payload_end,
                available: data.len(),
            })?;
        let trailing = data.len() - payload_end;
        let (kind, palette) = if data_size == indexed_size && trailing == BITMAP_PALETTE_SIZE {
            (BitmapKind::Indexed8, Some(&data[payload_end..]))
        } else if data_size == packed24_size && trailing == 0 {
            (BitmapKind::Packed24, None)
        } else {
            return Err(Error::BadBitmapDataSize {
                data_size: data_size_word,
                indexed_size,
                packed24_size,
                trailing,
            });
        };
        Ok(Self {
            width,
            height,
            kind,
            pixels,
            palette,
        })
    }

    /// Image width in pixels.
    #[must_use]
    pub const fn width(self) -> usize {
        self.width
    }

    /// Image height in pixels.
    #[must_use]
    pub const fn height(self) -> usize {
        self.height
    }

    /// Stored pixel representation.
    #[must_use]
    pub const fn kind(self) -> BitmapKind {
        self.kind
    }

    /// Borrowed pixel bytes.
    #[must_use]
    pub const fn pixels(self) -> &'a [u8] {
        self.pixels
    }

    /// Borrow the 768 RGB palette bytes for an indexed bitmap.
    #[must_use]
    pub const fn palette(self) -> Option<&'a [u8]> {
        self.palette
    }

    /// One indexed palette RGB triple.
    #[must_use]
    pub fn palette_rgb(self, index: u8) -> Option<[u8; 3]> {
        let palette = self.palette?;
        let at = usize::from(index) * 3;
        Some([palette[at], palette[at + 1], palette[at + 2]])
    }
}

/// A validated PAL payload borrowing its header and RGBA records.
#[derive(Clone, Copy, Debug)]
pub struct Palette<'a> {
    header: &'a [u8],
    rgba: &'a [u8],
    trailing: &'a [u8],
}

impl<'a> Palette<'a> {
    /// Parse the fixed 24-byte header and 256 four-byte records.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] when the 1,048 bytes consumed by retail do not fit.
    /// Any later bytes are retained as an opaque ignored tail.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        if data.len() < PALETTE_FILE_SIZE {
            return Err(Error::Short {
                format: Format::Palette,
                needed: PALETTE_FILE_SIZE,
                available: data.len(),
            });
        }
        Ok(Self {
            header: &data[..PALETTE_HEADER_SIZE],
            rgba: &data[PALETTE_HEADER_SIZE..PALETTE_FILE_SIZE],
            trailing: &data[PALETTE_FILE_SIZE..],
        })
    }

    /// The opaque header bytes retail reads and otherwise ignores.
    #[must_use]
    pub const fn header(self) -> &'a [u8] {
        self.header
    }

    /// One RGBA record. Retail's 24-bit conversion consumes RGB and drops A.
    #[must_use]
    pub fn rgba(self, index: u8) -> [u8; 4] {
        let at = usize::from(index) * 4;
        [
            self.rgba[at],
            self.rgba[at + 1],
            self.rgba[at + 2],
            self.rgba[at + 3],
        ]
    }

    /// Bytes after the records retail reads. Their semantics remain opaque.
    #[must_use]
    pub const fn trailing(self) -> &'a [u8] {
        self.trailing
    }
}

/// One font glyph record and its borrowed pixel bytes.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Glyph<'a> {
    /// Signed left bearing.
    pub left: i32,
    /// Inked width in pixels.
    pub width: usize,
    /// Signed right bearing.
    pub right: i32,
    /// Byte offset into the font's glyph payload.
    pub offset: u32,
    /// Row-major glyph bytes, `width * font.height()` long.
    pub pixels: &'a [u8],
}

/// A validated FNT resource borrowing its fixed spec and glyph payload.
#[derive(Clone, Copy, Debug)]
pub struct Font<'a> {
    spec: &'a [u8],
    data: &'a [u8],
}

impl<'a> Font<'a> {
    /// Parse the 0x1020-byte spec and validate all 256 glyph extents.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for a short spec, inverted character range, or a
    /// glyph offset/extent outside the trailing byte payload.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        let spec = data.get(..FONT_SPEC_SIZE).ok_or(Error::Short {
            format: Format::Font,
            needed: FONT_SPEC_SIZE,
            available: data.len(),
        })?;
        let first = spec[0];
        let last = spec[1];
        if first > last {
            return Err(Error::BadFontRange { first, last });
        }
        let font = Self {
            spec,
            data: &data[FONT_SPEC_SIZE..],
        };
        for character in 0..=u8::MAX {
            font.glyph(character)?;
        }
        Ok(font)
    }

    /// First advertised character code.
    #[must_use]
    pub const fn first(self) -> u8 {
        self.spec[0]
    }

    /// Last advertised character code.
    #[must_use]
    pub const fn last(self) -> u8 {
        self.spec[1]
    }

    /// Stored glyph bit depth byte.
    #[must_use]
    pub const fn depth(self) -> u8 {
        self.spec[2]
    }

    /// Signed horizontal spacing byte.
    #[must_use]
    pub const fn x_spacing(self) -> i8 {
        i8::from_ne_bytes([self.spec[3]])
    }

    /// Signed vertical spacing byte.
    #[must_use]
    pub const fn y_spacing(self) -> i8 {
        i8::from_ne_bytes([self.spec[4]])
    }

    /// Glyph row count.
    #[must_use]
    pub const fn height(self) -> u8 {
        self.spec[5]
    }

    /// Signed vertical bearing.
    #[must_use]
    pub const fn base_y_offset(self) -> i8 {
        i8::from_ne_bytes([self.spec[6]])
    }

    /// Palette-slot count stored in the spec.
    #[must_use]
    pub fn palette_count(self) -> u32 {
        word(self.spec, 8)
    }

    /// One of the five raw palette-slot dwords copied by retail.
    #[must_use]
    pub fn palette_slot(self, index: usize) -> Option<u32> {
        (index < 5).then(|| word(self.spec, 12 + index * 4))
    }

    /// Decode one ABC/offset record and borrow its glyph pixels.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] when its byte extent is outside the font payload.
    pub fn glyph(self, character: u8) -> Result<Glyph<'a>, Error> {
        let index = usize::from(character);
        let abc = 0x20 + index * 12;
        let left = signed_word(self.spec, abc);
        let width_word = word(self.spec, abc + 4);
        let right = signed_word(self.spec, abc + 8);
        let offset_at = 0xc20 + index * 4;
        let offset = word(self.spec, offset_at);
        let width = usize::try_from(width_word).map_err(|_| Error::SizeOverflow {
            format: Format::Font,
        })?;
        let size = width
            .checked_mul(usize::from(self.height()))
            .ok_or(Error::SizeOverflow {
                format: Format::Font,
            })?;
        let start = usize::try_from(offset).map_err(|_| Error::GlyphOutOfBounds {
            character: u16::from(character),
            offset,
            size,
            available: self.data.len(),
        })?;
        let pixels = self
            .data
            .get(
                start..start.checked_add(size).ok_or(Error::GlyphOutOfBounds {
                    character: u16::from(character),
                    offset,
                    size,
                    available: self.data.len(),
                })?,
            )
            .ok_or(Error::GlyphOutOfBounds {
                character: u16::from(character),
                offset,
                size,
                available: self.data.len(),
            })?;
        Ok(Glyph {
            left,
            width,
            right,
            offset,
            pixels,
        })
    }

    /// Complete trailing glyph payload.
    #[must_use]
    pub const fn data(self) -> &'a [u8] {
        self.data
    }
}

/// A 6x8-bit object footprint.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Mask<'a> {
    width: u8,
    height: u8,
    draw: &'a [u8],
    shadow: &'a [u8],
}

impl<'a> Mask<'a> {
    /// Parse `width, height, draw[6], shadow[6]`.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] unless the input is exactly 14 bytes.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        if data.len() != MASK_SIZE {
            return Err(Error::Length {
                format: Format::Mask,
                expected: MASK_SIZE,
                actual: data.len(),
            });
        }
        Ok(Self {
            width: data[0],
            height: data[1],
            draw: &data[2..8],
            shadow: &data[8..14],
        })
    }

    /// Footprint width recorded by the resource.
    #[must_use]
    pub const fn width(self) -> u8 {
        self.width
    }

    /// Footprint height recorded by the resource.
    #[must_use]
    pub const fn height(self) -> u8 {
        self.height
    }

    /// Whether one of the 48 cells is drawn.
    #[must_use]
    pub fn draw(self, cell: usize) -> Option<bool> {
        bit(self.draw, cell)
    }

    /// Whether one of the 48 cells casts a shadow.
    #[must_use]
    pub fn shadow(self, cell: usize) -> Option<bool> {
        bit(self.shadow, cell)
    }
}

/// A borrowed text field after retail's wrapper/terminator removal.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Field<'a> {
    encoded: &'a [u8],
}

impl<'a> Field<'a> {
    /// Bytes before collapsing adjacent quote runs.
    #[must_use]
    pub const fn encoded(self) -> &'a [u8] {
        self.encoded
    }

    /// Iterate retail-normalized bytes without allocating.
    #[must_use = "iterators are lazy"]
    pub const fn decoded(self) -> Decoded<'a> {
        Decoded {
            bytes: self.encoded,
            position: 0,
        }
    }
}

/// Iterator implementing retail's adjacent-quote collapse.
#[derive(Clone, Debug)]
pub struct Decoded<'a> {
    bytes: &'a [u8],
    position: usize,
}

impl Iterator for Decoded<'_> {
    type Item = u8;

    fn next(&mut self) -> Option<Self::Item> {
        let byte = *self.bytes.get(self.position)?;
        self.position += 1;
        if byte == b'"' {
            while self.bytes.get(self.position) == Some(&b'"') {
                self.position += 1;
            }
        }
        Some(byte)
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        (0, Some(self.bytes.len().saturating_sub(self.position)))
    }
}

/// A validated CRLF line resource.
#[derive(Clone, Copy, Debug)]
pub struct Text<'a> {
    data: &'a [u8],
    rows: usize,
}

impl<'a> Text<'a> {
    /// Validate line endings and quoted-line bounds.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for a CR not followed by LF or a one-byte quoted
    /// row that retail would underflow while removing its terminator.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        validate_rows(data, Format::Text, |row, row_index| {
            let trimmed = trim_tabs(row);
            if trimmed.first() == Some(&b'"') && trimmed.len() < 2 {
                return Err(Error::MalformedQuotedField {
                    format: Format::Text,
                    row: row_index,
                    column: 0,
                });
            }
            Ok(())
        })
        .map(|rows| Self { data, rows })
    }

    /// Number of CR-terminated rows. Retail ignores an unterminated tail.
    #[must_use]
    pub const fn len(self) -> usize {
        self.rows
    }

    /// Whether no CR-terminated rows exist.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.rows == 0
    }

    /// Every parsed line in order.
    #[must_use = "iterators are lazy"]
    pub const fn lines(self) -> TextLines<'a> {
        TextLines {
            rows: RawRows::new(self.data),
        }
    }
}

/// Iterator over normalized text lines.
#[derive(Clone, Debug)]
pub struct TextLines<'a> {
    rows: RawRows<'a>,
}

impl<'a> Iterator for TextLines<'a> {
    type Item = Field<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        let raw = self.rows.next()?;
        let trimmed = trim_tabs(raw);
        let encoded = if trimmed.first() == Some(&b'"') {
            &trimmed[1..trimmed.len() - 1]
        } else {
            trimmed
        };
        Some(Field { encoded })
    }
}

/// A validated CRLF/tab spreadsheet resource.
#[derive(Clone, Copy, Debug)]
pub struct Spreadsheet<'a> {
    data: &'a [u8],
    rows: usize,
}

impl<'a> Spreadsheet<'a> {
    /// Validate line endings and quoted-cell bounds.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for bad CRLF or a one-byte quoted cell.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        validate_rows(data, Format::Spreadsheet, |row, row_index| {
            for (column, cell) in raw_cells(row).enumerate() {
                if cell.first() == Some(&b'"') && cell.len() < 2 {
                    return Err(Error::MalformedQuotedField {
                        format: Format::Spreadsheet,
                        row: row_index,
                        column,
                    });
                }
            }
            Ok(())
        })
        .map(|rows| Self { data, rows })
    }

    /// Number of CR-terminated spreadsheet rows.
    #[must_use]
    pub const fn len(self) -> usize {
        self.rows
    }

    /// Whether no CR-terminated rows exist.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.rows == 0
    }

    /// Every spreadsheet row in order.
    #[must_use = "iterators are lazy"]
    pub const fn rows(self) -> SpreadsheetRows<'a> {
        SpreadsheetRows {
            rows: RawRows::new(self.data),
        }
    }
}

/// One spreadsheet row.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SpreadsheetRow<'a> {
    raw: &'a [u8],
}

impl<'a> SpreadsheetRow<'a> {
    /// Number of tab-delimited columns, including empty cells.
    #[must_use]
    pub fn len(self) -> usize {
        self.raw.split(|&byte| byte == b'\t').count()
    }

    /// A row always has at least one cell.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        false
    }

    /// Every normalized cell in the row.
    #[must_use = "iterators are lazy"]
    pub const fn cells(self) -> Cells<'a> {
        Cells {
            raw: self.raw,
            position: 0,
            done: false,
        }
    }
}

/// Iterator over spreadsheet rows.
#[derive(Clone, Debug)]
pub struct SpreadsheetRows<'a> {
    rows: RawRows<'a>,
}

impl<'a> Iterator for SpreadsheetRows<'a> {
    type Item = SpreadsheetRow<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        self.rows.next().map(|raw| SpreadsheetRow { raw })
    }
}

/// Iterator over normalized spreadsheet cells.
#[derive(Clone, Debug)]
pub struct Cells<'a> {
    raw: &'a [u8],
    position: usize,
    done: bool,
}

impl<'a> Iterator for Cells<'a> {
    type Item = Field<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.done {
            return None;
        }
        let tail = &self.raw[self.position..];
        let length = tail
            .iter()
            .position(|&byte| byte == b'\t')
            .unwrap_or(tail.len());
        let raw = &tail[..length];
        if length == tail.len() {
            self.done = true;
        } else {
            self.position += length + 1;
        }
        let encoded = if raw.first() == Some(&b'"') {
            &raw[1..raw.len() - 1]
        } else {
            raw
        };
        Some(Field { encoded })
    }
}

#[derive(Clone, Debug)]
struct RawRows<'a> {
    data: &'a [u8],
    position: usize,
}

impl<'a> RawRows<'a> {
    const fn new(data: &'a [u8]) -> Self {
        Self { data, position: 0 }
    }
}

impl<'a> Iterator for RawRows<'a> {
    type Item = &'a [u8];

    fn next(&mut self) -> Option<Self::Item> {
        let tail = self.data.get(self.position..)?;
        let length = tail.iter().position(|&byte| byte == b'\r')?;
        let row = &tail[..length];
        self.position = self.position.saturating_add(length + 2);
        Some(row)
    }
}

fn validate_rows(
    data: &[u8],
    format: Format,
    mut validate: impl FnMut(&[u8], usize) -> Result<(), Error>,
) -> Result<usize, Error> {
    let mut rows = 0usize;
    let mut position = 0usize;
    while let Some(relative) = data[position..].iter().position(|&byte| byte == b'\r') {
        let end = position + relative;
        if data.get(end + 1) != Some(&b'\n') {
            return Err(Error::BadLineEnding {
                format,
                offset: end,
            });
        }
        validate(&data[position..end], rows)?;
        rows += 1;
        position = end + 2;
    }
    Ok(rows)
}

fn raw_cells(row: &[u8]) -> impl Iterator<Item = &[u8]> {
    row.split(|&byte| byte == b'\t')
}

fn trim_tabs(mut bytes: &[u8]) -> &[u8] {
    while bytes.last() == Some(&b'\t') {
        bytes = &bytes[..bytes.len() - 1];
    }
    bytes
}

fn bit(bytes: &[u8], cell: usize) -> Option<bool> {
    (cell < 48).then(|| bytes[cell / 8] & (1 << (cell % 8)) != 0)
}

fn word(data: &[u8], at: usize) -> u32 {
    u32::from_le_bytes([data[at], data[at + 1], data[at + 2], data[at + 3]])
}

fn signed_word(data: &[u8], at: usize) -> i32 {
    i32::from_le_bytes([data[at], data[at + 1], data[at + 2], data[at + 3]])
}
