#![no_std]
#![forbid(unsafe_code)]
//! Allocation-free readers for Heroes III SND and VID resource archives.
//!
//! Both formats begin with a little-endian entry count. SND records contain
//! `name[40], offset, size`; VID records contain `name[40], offset`, with the
//! next record's offset (or end of file) delimiting the payload. The readers
//! validate every extent before returning borrowed entries and bytes.

pub mod gzip;

use core::fmt;

/// Bytes in the shared count field.
pub const HEADER_SIZE: usize = 4;
/// Bytes in one SND directory record.
pub const SND_ENTRY_SIZE: usize = 48;
/// Bytes in one VID directory record.
pub const VID_ENTRY_SIZE: usize = 44;
/// Bytes in an SND or VID member name.
pub const NAME_SIZE: usize = 40;

/// The archive family being parsed.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    /// A sound archive with explicit payload sizes.
    Sound,
    /// A video archive whose payload sizes are derived from offsets.
    Video,
}

/// A malformed SND or VID archive.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    ShortHeader {
        available: usize,
    },
    DirectoryOverflow {
        kind: Kind,
        entries: u32,
    },
    ShortDirectory {
        kind: Kind,
        needed: usize,
        available: usize,
    },
    PayloadBeforeDirectory {
        kind: Kind,
        index: usize,
        offset: u32,
        directory_end: usize,
    },
    PayloadOutOfBounds {
        kind: Kind,
        index: usize,
        offset: u32,
        size: u32,
    },
    OffsetsOutOfOrder {
        index: usize,
        offset: u32,
        next_offset: u32,
    },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::ShortHeader { available } => {
                write!(f, "archive count needs 4 bytes, found {available}")
            }
            Self::DirectoryOverflow { kind, entries } => {
                write!(f, "{kind:?} directory overflows for {entries} entries")
            }
            Self::ShortDirectory {
                kind,
                needed,
                available,
            } => write!(
                f,
                "{kind:?} directory ends at {needed:#x}, archive has {available:#x} bytes"
            ),
            Self::PayloadBeforeDirectory {
                kind,
                index,
                offset,
                directory_end,
            } => write!(
                f,
                "{kind:?} entry {index} payload starts at {offset:#x}, before directory end {directory_end:#x}"
            ),
            Self::PayloadOutOfBounds {
                kind,
                index,
                offset,
                size,
            } => write!(
                f,
                "{kind:?} entry {index} payload {offset:#x}+{size:#x} is outside the archive"
            ),
            Self::OffsetsOutOfOrder {
                index,
                offset,
                next_offset,
            } => write!(
                f,
                "video entry {index} offset {offset:#x} follows next offset {next_offset:#x}"
            ),
        }
    }
}

impl core::error::Error for Error {}

macro_rules! name_helpers {
    () => {
        /// The member name as UTF-8 when its DOS-era byte field is valid text.
        #[must_use]
        pub fn name_str(&self) -> Option<&str> {
            core::str::from_utf8(self.name).ok()
        }

        /// Case-insensitive ASCII name comparison, matching retail lookup.
        #[must_use]
        pub fn matches(&self, name: &str) -> bool {
            self.name.eq_ignore_ascii_case(name.as_bytes())
        }

        /// Case-insensitive extension check.
        #[must_use]
        pub fn has_extension(&self, extension: &str) -> bool {
            self.name
                .len()
                .checked_sub(extension.len())
                .is_some_and(|at| self.name[at..].eq_ignore_ascii_case(extension.as_bytes()))
        }
    };
}

/// One borrowed sound member.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SoundEntry<'a> {
    /// NUL-trimmed filename from the 40-byte directory field.
    pub name: &'a [u8],
    /// Absolute payload offset.
    pub offset: u32,
    /// Payload length recorded in the directory.
    pub size: u32,
}

impl SoundEntry<'_> {
    name_helpers!();
}

/// One borrowed video member.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct VideoEntry<'a> {
    /// NUL-trimmed filename from the 40-byte directory field.
    pub name: &'a [u8],
    /// Absolute payload offset.
    pub offset: u32,
    /// Payload length derived from the following offset or archive end.
    pub size: u32,
}

impl VideoEntry<'_> {
    name_helpers!();
}

/// A validated SND archive borrowing its complete image.
#[derive(Clone, Copy, Debug)]
pub struct SoundArchive<'a> {
    data: &'a [u8],
    directory: &'a [u8],
    count: usize,
}

impl<'a> SoundArchive<'a> {
    /// Parse the count and validate every explicit payload extent.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] when the directory or a member extent is malformed.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        let (count, directory, directory_end) = directory(data, Kind::Sound, SND_ENTRY_SIZE)?;
        let archive = Self {
            data,
            directory,
            count,
        };
        for index in 0..count {
            let entry = archive.entry(index).ok_or(Error::DirectoryOverflow {
                kind: Kind::Sound,
                entries: word(data, 0),
            })?;
            validate_start(Kind::Sound, index, entry.offset, directory_end)?;
            slice(data, Kind::Sound, index, entry.offset, entry.size)?;
        }
        Ok(archive)
    }

    /// Number of sound members.
    #[must_use]
    pub const fn len(&self) -> usize {
        self.count
    }

    /// Whether the archive has no members.
    #[must_use]
    pub const fn is_empty(&self) -> bool {
        self.count == 0
    }

    /// Decode one sound directory record.
    #[must_use]
    pub fn entry(&self, index: usize) -> Option<SoundEntry<'a>> {
        let record = record(self.directory, index, SND_ENTRY_SIZE)?;
        Some(SoundEntry {
            name: name(record),
            offset: word(record, NAME_SIZE),
            size: word(record, NAME_SIZE + 4),
        })
    }

    /// Every sound record in on-disk order.
    #[must_use = "iterators are lazy"]
    pub fn entries(&self) -> impl Iterator<Item = SoundEntry<'a>> + use<'a> {
        let archive = *self;
        (0..archive.count).filter_map(move |index| archive.entry(index))
    }

    /// Find a sound by case-insensitive ASCII name.
    #[must_use]
    pub fn find(&self, name: &str) -> Option<SoundEntry<'a>> {
        self.entries().find(|entry| entry.matches(name))
    }

    /// Borrow a sound member's bytes.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] if the entry did not originate from this archive or
    /// its extent is invalid.
    pub fn payload(&self, entry: SoundEntry<'a>) -> Result<&'a [u8], Error> {
        let index = self
            .entries()
            .position(|candidate| candidate == entry)
            .unwrap_or(self.count);
        slice(self.data, Kind::Sound, index, entry.offset, entry.size)
    }
}

/// A validated VID archive borrowing its complete image.
#[derive(Clone, Copy, Debug)]
pub struct VideoArchive<'a> {
    data: &'a [u8],
    directory: &'a [u8],
    count: usize,
}

impl<'a> VideoArchive<'a> {
    /// Parse the count, derive member sizes, and validate every payload.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] when offsets are out of order or outside the image.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        let (count, directory, directory_end) = directory(data, Kind::Video, VID_ENTRY_SIZE)?;
        let archive = Self {
            data,
            directory,
            count,
        };
        for index in 0..count {
            let current =
                record(directory, index, VID_ENTRY_SIZE).ok_or(Error::DirectoryOverflow {
                    kind: Kind::Video,
                    entries: word(data, 0),
                })?;
            let offset = word(current, NAME_SIZE);
            let next_offset = if index + 1 < count {
                let next = record(directory, index + 1, VID_ENTRY_SIZE).ok_or(
                    Error::DirectoryOverflow {
                        kind: Kind::Video,
                        entries: word(data, 0),
                    },
                )?;
                word(next, NAME_SIZE)
            } else {
                u32::try_from(data.len()).map_err(|_| Error::PayloadOutOfBounds {
                    kind: Kind::Video,
                    index,
                    offset,
                    size: u32::MAX,
                })?
            };
            let size = next_offset
                .checked_sub(offset)
                .ok_or(Error::OffsetsOutOfOrder {
                    index,
                    offset,
                    next_offset,
                })?;
            let entry = VideoEntry {
                name: name(current),
                offset,
                size,
            };
            validate_start(Kind::Video, index, entry.offset, directory_end)?;
            slice(data, Kind::Video, index, entry.offset, entry.size)?;
        }
        Ok(archive)
    }

    /// Number of video members.
    #[must_use]
    pub const fn len(&self) -> usize {
        self.count
    }

    /// Whether the archive has no members.
    #[must_use]
    pub const fn is_empty(&self) -> bool {
        self.count == 0
    }

    /// Decode one video directory record and derive its payload size.
    #[must_use]
    pub fn entry(&self, index: usize) -> Option<VideoEntry<'a>> {
        let current = record(self.directory, index, VID_ENTRY_SIZE)?;
        let offset = word(current, NAME_SIZE);
        let end = if index + 1 < self.count {
            let next = record(self.directory, index + 1, VID_ENTRY_SIZE)?;
            word(next, NAME_SIZE)
        } else {
            u32::try_from(self.data.len()).ok()?
        };
        Some(VideoEntry {
            name: name(current),
            offset,
            size: end.checked_sub(offset)?,
        })
    }

    /// Every video record in on-disk order.
    #[must_use = "iterators are lazy"]
    pub fn entries(&self) -> impl Iterator<Item = VideoEntry<'a>> + use<'a> {
        let archive = *self;
        (0..archive.count).filter_map(move |index| archive.entry(index))
    }

    /// Find a video by case-insensitive ASCII name.
    #[must_use]
    pub fn find(&self, name: &str) -> Option<VideoEntry<'a>> {
        self.entries().find(|entry| entry.matches(name))
    }

    /// Borrow a video member's bytes.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] if the entry did not originate from this archive or
    /// its extent is invalid.
    pub fn payload(&self, entry: VideoEntry<'a>) -> Result<&'a [u8], Error> {
        let index = self
            .entries()
            .position(|candidate| candidate == entry)
            .unwrap_or(self.count);
        slice(self.data, Kind::Video, index, entry.offset, entry.size)
    }
}

fn directory(data: &[u8], kind: Kind, entry_size: usize) -> Result<(usize, &[u8], usize), Error> {
    if data.len() < HEADER_SIZE {
        return Err(Error::ShortHeader {
            available: data.len(),
        });
    }
    let count_word = word(data, 0);
    let count = usize::try_from(count_word).map_err(|_| Error::DirectoryOverflow {
        kind,
        entries: count_word,
    })?;
    let directory_end = count
        .checked_mul(entry_size)
        .and_then(|bytes| HEADER_SIZE.checked_add(bytes))
        .ok_or(Error::DirectoryOverflow {
            kind,
            entries: count_word,
        })?;
    let directory = data
        .get(HEADER_SIZE..directory_end)
        .ok_or(Error::ShortDirectory {
            kind,
            needed: directory_end,
            available: data.len(),
        })?;
    Ok((count, directory, directory_end))
}

fn validate_start(
    kind: Kind,
    index: usize,
    offset: u32,
    directory_end: usize,
) -> Result<(), Error> {
    if usize::try_from(offset)
        .ok()
        .is_none_or(|at| at < directory_end)
    {
        return Err(Error::PayloadBeforeDirectory {
            kind,
            index,
            offset,
            directory_end,
        });
    }
    Ok(())
}

fn slice(data: &[u8], kind: Kind, index: usize, offset: u32, size: u32) -> Result<&[u8], Error> {
    let start = usize::try_from(offset).map_err(|_| Error::PayloadOutOfBounds {
        kind,
        index,
        offset,
        size,
    })?;
    let len = usize::try_from(size).map_err(|_| Error::PayloadOutOfBounds {
        kind,
        index,
        offset,
        size,
    })?;
    data.get(
        start..start.checked_add(len).ok_or(Error::PayloadOutOfBounds {
            kind,
            index,
            offset,
            size,
        })?,
    )
    .ok_or(Error::PayloadOutOfBounds {
        kind,
        index,
        offset,
        size,
    })
}

fn record(directory: &[u8], index: usize, size: usize) -> Option<&[u8]> {
    let start = index.checked_mul(size)?;
    directory.get(start..start.checked_add(size)?)
}

fn name(record: &[u8]) -> &[u8] {
    let raw = &record[..NAME_SIZE];
    let end = raw.iter().position(|&byte| byte == 0).unwrap_or(NAME_SIZE);
    &raw[..end]
}

fn word(data: &[u8], at: usize) -> u32 {
    u32::from_le_bytes([data[at], data[at + 1], data[at + 2], data[at + 3]])
}
