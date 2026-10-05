//! `HotA`'s hashed LOD directory (pinned 1.8.1 lookup/read RVAs 0x170ba0,
//! 0x171e40 and 0x170e20). Payload compression is exposed to the caller.

use crate::{word, Error, Header, ENTRY_SIZE, HEADER_SIZE};

/// Directory member identified by a folded-name hash instead of stored text.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct HashedEntry {
    /// FNV-1a of the lowercase resource name.
    pub name_hash: u32,
    /// Decoded payload offset.
    pub offset: u32,
    /// Decoded uncompressed length.
    pub size: u32,
    /// Decoded compressed length; zero means stored bytes.
    pub compressed_size: u32,
    /// Native codec tag (1 custom, 2 LZMA, 3 zlib); ignored for stored bytes.
    pub codec: u8,
}
impl HashedEntry {
    /// Bytes physically stored in the archive.
    #[must_use]
    pub const fn stored_size(self) -> u32 {
        if self.compressed_size == 0 {
            self.size
        } else {
            self.compressed_size
        }
    }
    /// Match the supplied ASCII resource name using native case folding/hash.
    #[must_use]
    pub fn matches(self, name: &[u8]) -> bool {
        self.name_hash == resource_name_hash(name)
    }
}

/// Native FNV-1a name hash, after ASCII lowercase conversion (RVA 0x174050).
/// Native lookup stops at a NUL, even when a caller's byte slice is longer.
#[must_use]
pub fn resource_name_hash(name: &[u8]) -> u32 {
    name.iter()
        .copied()
        .take_while(|&b| b != 0)
        .fold(0x811c_9dc5_u32, |hash, byte| {
            (hash ^ u32::from(byte.to_ascii_lowercase())).wrapping_mul(0x0100_0193)
        })
}

/// Borrowed hashed directory, with all decoded payload extents checked.
#[derive(Clone, Copy, Debug)]
pub struct HashedDirectory<'a> {
    records: &'a [u8],
    key: u32,
}
impl<'a> HashedDirectory<'a> {
    /// Read hashed records using the key in the common header's reserved word.
    ///
    /// # Errors
    /// Rejects named-format headers, short directories and out-of-file payloads.
    pub fn parse(data: &'a [u8], file_len: u64) -> Result<Self, Error> {
        let header = Header::parse(data)?;
        let key = header.hashed_key().ok_or(Error::DirectoryEncoding)?;
        let available = data
            .len()
            .min(usize::try_from(file_len).unwrap_or(usize::MAX));
        let records = data
            .get(..available)
            .and_then(|b| b.get(HEADER_SIZE..header.directory_end()))
            .ok_or(Error::ShortDirectory {
                needed: header.directory_end(),
                available,
            })?;
        let result = Self { records, key };
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
    /// Number of directory records.
    #[must_use]
    pub const fn len(self) -> usize {
        self.records.len() / ENTRY_SIZE
    }
    /// Whether this directory has no members.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.records.is_empty()
    }
    /// Decode a checked record. Unused bytes are native padding/noise.
    #[must_use]
    pub fn entry(self, index: usize) -> Option<HashedEntry> {
        let start = index.checked_mul(ENTRY_SIZE)?;
        let record = self.records.get(start..start.checked_add(ENTRY_SIZE)?)?;
        Some(HashedEntry {
            name_hash: word(record, 0),
            offset: word(record, 4) ^ self.key,
            size: word(record, 8) ^ self.key,
            compressed_size: word(record, 12) ^ self.key,
            codec: record[16],
        })
    }
    /// Every record in native lookup order; hash collisions retain the first.
    #[must_use = "iterators are lazy"]
    pub fn entries(self) -> impl Iterator<Item = HashedEntry> + use<'a> {
        (0..self.len()).filter_map(move |index| self.entry(index))
    }
    /// Find the first matching resource-name hash.
    #[must_use]
    pub fn find(self, name: &[u8]) -> Option<HashedEntry> {
        let hash = resource_name_hash(name);
        self.entries().find(|entry| entry.name_hash == hash)
    }
}
