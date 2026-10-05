#![no_std]
#![forbid(unsafe_code)]
//! Allocation-free reader for the Heroes III LOD resource archive.
//!
//! The format model follows retail `LODFile::open`/`pointAt`/`read` at
//! `0x4fa8a0`, `0x4faa70`, and `0x4fab20`: a 92-byte header, followed by
//! fixed 32-byte directory records. Stored members borrow the archive bytes;
//! packed members expose their zlib stream and advertised output size so a
//! `std` caller can choose how to inflate them.

mod hashed;
pub use hashed::{resource_name_hash, HashedDirectory, HashedEntry};

use core::fmt;

/// Bytes in the fixed LOD header.
pub const HEADER_SIZE: usize = 0x5c;
/// Bytes in one directory record.
pub const ENTRY_SIZE: usize = 0x20;
/// Bytes in a directory member name.
pub const NAME_SIZE: usize = 16;

/// A malformed archive.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    /// The fixed header does not fit.
    ShortHeader { available: usize },
    /// The first three bytes are not `LOD`.
    BadSignature([u8; 4]),
    /// Named and hashed directory readers cannot interpret each other's records.
    DirectoryEncoding,
    /// The directory size overflowed the host address space.
    DirectoryOverflow { entries: u32 },
    /// The declared directory does not fit.
    ShortDirectory { needed: usize, available: usize },
    /// A member's stored or compressed payload is outside the archive.
    PayloadOutOfBounds {
        index: usize,
        offset: u32,
        size: u32,
    },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::DirectoryEncoding => f.write_str("LOD directory encoding does not match reader"),
            Self::ShortHeader { available } => {
                write!(f, "LOD header needs {HEADER_SIZE} bytes, found {available}")
            }
            Self::BadSignature(signature) => write!(
                f,
                "bad LOD signature {:02x} {:02x} {:02x} {:02x}",
                signature[0], signature[1], signature[2], signature[3]
            ),
            Self::DirectoryOverflow { entries } => {
                write!(f, "LOD directory size overflows for {entries} entries")
            }
            Self::ShortDirectory { needed, available } => write!(
                f,
                "LOD directory ends at {needed:#x}, archive has {available:#x} bytes"
            ),
            Self::PayloadOutOfBounds {
                index,
                offset,
                size,
            } => write!(
                f,
                "LOD entry {index} payload {offset:#x}+{size:#x} is outside the archive"
            ),
        }
    }
}

impl core::error::Error for Error {}

/// One decoded 32-byte LOD directory record.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Entry<'a> {
    /// NUL-trimmed DOS member name, borrowed from the directory.
    pub name: &'a [u8],
    /// Byte offset of the member payload.
    pub offset: u32,
    /// Uncompressed byte length.
    pub size: u32,
    /// Retail attribute word. Its bit meanings are not needed by the reader.
    pub attributes: u32,
    /// Compressed length, or zero for a stored member.
    pub compressed_size: u32,
}

impl Entry<'_> {
    /// The name as UTF-8 when the fixed DOS field contains valid text.
    #[must_use]
    pub fn name_str(&self) -> Option<&str> {
        core::str::from_utf8(self.name).ok()
    }

    /// Case-insensitive ASCII name comparison, matching the resource lookup.
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

    /// Number of bytes physically stored in the archive.
    #[must_use]
    pub const fn stored_size(&self) -> u32 {
        if self.compressed_size == 0 {
            self.size
        } else {
            self.compressed_size
        }
    }

    /// Whether retail inflates this entry before serving it.
    #[must_use]
    pub const fn is_compressed(&self) -> bool {
        self.compressed_size != 0
    }
}

/// Borrowed member bytes.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Payload<'a> {
    /// Bytes can be consumed directly.
    Stored(&'a [u8]),
    /// A zlib stream and the exact output length allocated by retail.
    Compressed {
        /// Borrowed zlib stream.
        stream: &'a [u8],
        /// Advertised uncompressed byte length.
        unpacked_size: usize,
    },
}

/// Parsed header with a checked directory extent.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Header {
    version: u32,
    key: u32,
    count: usize,
    directory_end: usize,
}
impl Header {
    /// Parse only the fixed header, before reading the directory or payloads.
    ///
    /// # Errors
    /// Rejects a short header, bad signature or directory-size overflow.
    pub fn parse(data: &[u8]) -> Result<Self, Error> {
        let header = data.get(..HEADER_SIZE).ok_or(Error::ShortHeader {
            available: data.len(),
        })?;
        let signature = [header[0], header[1], header[2], header[3]];
        if &signature[..3] != b"LOD" {
            return Err(Error::BadSignature(signature));
        }
        let count_word = word(header, 8);
        let count = usize::try_from(count_word).map_err(|_| Error::DirectoryOverflow {
            entries: count_word,
        })?;
        let directory_end = count
            .checked_mul(ENTRY_SIZE)
            .and_then(|size| HEADER_SIZE.checked_add(size))
            .ok_or(Error::DirectoryOverflow {
                entries: count_word,
            })?;
        Ok(Self {
            version: word(header, 4),
            key: word(header, 12),
            count,
            directory_end,
        })
    }
    /// Key for `HotA`'s hashed directory, or `None` for the named format.
    /// Native treats both zero and `0x007e0213` as named-format markers.
    #[must_use]
    pub const fn hashed_key(self) -> Option<u32> {
        if self.key == 0 || self.key == 0x007e_0213 {
            None
        } else {
            Some(self.key)
        }
    }
    /// Bytes needed for the header and complete directory, excluding payloads.
    #[must_use]
    pub const fn directory_end(self) -> usize {
        self.directory_end
    }
}

/// Validated directory borrowing only header/directory bytes. Payloads can stay
/// on disk; their extents are checked against the supplied file length.
#[derive(Clone, Copy, Debug)]
pub struct Directory<'a> {
    records: &'a [u8],
    version: u32,
    count: usize,
}
impl<'a> Directory<'a> {
    /// Parse a directory and check every stored/compressed payload extent.
    /// Bytes beyond the directory are permitted but need not be present.
    ///
    /// # Errors
    /// Rejects malformed headers, short directories and out-of-file payloads.
    pub fn parse(data: &'a [u8], file_len: u64) -> Result<Self, Error> {
        let header = Header::parse(data)?;
        if header.hashed_key().is_some() {
            return Err(Error::DirectoryEncoding);
        }
        let available = data
            .len()
            .min(usize::try_from(file_len).unwrap_or(usize::MAX));
        let directory = data
            .get(..available)
            .and_then(|data| data.get(HEADER_SIZE..header.directory_end))
            .ok_or(Error::ShortDirectory {
                needed: header.directory_end,
                available,
            })?;
        let result = Self {
            records: directory,
            version: header.version,
            count: header.count,
        };
        for (index, entry) in result.entries().enumerate() {
            if u64::from(entry.offset) + u64::from(entry.stored_size()) > file_len {
                return Err(Error::PayloadOutOfBounds {
                    index,
                    offset: entry.offset,
                    size: entry.stored_size(),
                });
            }
        }
        Ok(result)
    }
    /// Version dword from the archive header.
    #[must_use]
    pub const fn version(&self) -> u32 {
        self.version
    }

    /// Number of directory entries.
    #[must_use]
    pub const fn len(&self) -> usize {
        self.count
    }

    /// Whether the directory is empty.
    #[must_use]
    pub const fn is_empty(&self) -> bool {
        self.count == 0
    }

    /// Decode one directory record.
    #[must_use]
    pub fn entry(&self, index: usize) -> Option<Entry<'a>> {
        let start = index.checked_mul(ENTRY_SIZE)?;
        let record = self.records.get(start..start.checked_add(ENTRY_SIZE)?)?;
        let raw_name = record.get(..NAME_SIZE)?;
        let name_end = raw_name
            .iter()
            .position(|&byte| byte == 0)
            .unwrap_or(NAME_SIZE);
        Some(Entry {
            name: &raw_name[..name_end],
            offset: word(record, 16),
            size: word(record, 20),
            attributes: word(record, 24),
            compressed_size: word(record, 28),
        })
    }

    /// Every directory record in on-disk order.
    #[must_use = "iterators are lazy"]
    pub fn entries(&self) -> impl Iterator<Item = Entry<'a>> + use<'a> {
        let archive = *self;
        (0..archive.count).filter_map(move |index| archive.entry(index))
    }

    /// Find a member by case-insensitive ASCII name.
    #[must_use]
    pub fn find(&self, name: &str) -> Option<Entry<'a>> {
        self.entries().find(|entry| entry.matches(name))
    }
}

/// A validated archive borrowing its complete image.
#[derive(Clone, Copy, Debug)]
pub struct Archive<'a> {
    data: &'a [u8],
    directory: Directory<'a>,
}
impl<'a> Archive<'a> {
    /// Parse the header and validate every directory payload extent.
    ///
    /// # Errors
    /// Rejects malformed headers, directories and out-of-image payloads.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        Ok(Self {
            data,
            directory: Directory::parse(data, data.len() as u64)?,
        })
    }
    /// Version dword from the archive header.
    #[must_use]
    pub const fn version(&self) -> u32 {
        self.directory.version()
    }
    /// Number of directory entries.
    #[must_use]
    pub const fn len(&self) -> usize {
        self.directory.len()
    }
    /// Whether the directory is empty.
    #[must_use]
    pub const fn is_empty(&self) -> bool {
        self.directory.is_empty()
    }
    /// Decode one directory record.
    #[must_use]
    pub fn entry(&self, index: usize) -> Option<Entry<'a>> {
        self.directory.entry(index)
    }
    /// Every directory record in on-disk order.
    #[must_use = "iterators are lazy"]
    pub fn entries(&self) -> impl Iterator<Item = Entry<'a>> + use<'a> {
        self.directory.entries()
    }
    /// Find a member by case-insensitive ASCII name.
    #[must_use]
    pub fn find(&self, name: &str) -> Option<Entry<'a>> {
        self.directory.find(name)
    }
    /// Borrow bytes for a decoded entry, checking its extent even if it came
    /// from another archive.
    ///
    /// # Errors
    /// Reports an entry extent outside this archive.
    pub fn payload(&self, entry: Entry<'a>) -> Result<Payload<'a>, Error> {
        let index = self
            .entries()
            .position(|candidate| candidate == entry)
            .unwrap_or(self.len());
        self.payload_at(index, entry)
    }
    /// Find a member and borrow its stored representation.
    #[must_use]
    pub fn get(&self, name: &str) -> Option<Payload<'a>> {
        self.find(name).and_then(|entry| self.payload(entry).ok())
    }
    fn payload_at(&self, index: usize, entry: Entry<'a>) -> Result<Payload<'a>, Error> {
        let start = usize::try_from(entry.offset).map_err(|_| Error::PayloadOutOfBounds {
            index,
            offset: entry.offset,
            size: entry.stored_size(),
        })?;
        let stored_word = entry.stored_size();
        let stored = usize::try_from(stored_word).map_err(|_| Error::PayloadOutOfBounds {
            index,
            offset: entry.offset,
            size: stored_word,
        })?;
        let bytes = self
            .data
            .get(
                start..start.checked_add(stored).ok_or(Error::PayloadOutOfBounds {
                    index,
                    offset: entry.offset,
                    size: stored_word,
                })?,
            )
            .ok_or(Error::PayloadOutOfBounds {
                index,
                offset: entry.offset,
                size: stored_word,
            })?;
        if entry.is_compressed() {
            let unpacked_size =
                usize::try_from(entry.size).map_err(|_| Error::PayloadOutOfBounds {
                    index,
                    offset: entry.offset,
                    size: entry.size,
                })?;
            Ok(Payload::Compressed {
                stream: bytes,
                unpacked_size,
            })
        } else {
            Ok(Payload::Stored(bytes))
        }
    }
}

fn word(data: &[u8], at: usize) -> u32 {
    u32::from_le_bytes([data[at], data[at + 1], data[at + 2], data[at + 3]])
}
