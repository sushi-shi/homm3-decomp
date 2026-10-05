//! Borrowed `HDAT` version 2 records from `HotA.dat`.
//!
//! Layout follows the pinned `HotA` 1.8.1 loader at DLL RVA `0x127030` and
//! its length-prefixed string reader at `0x127920`. All lengths/counts are
//! signed little-endian words; negative lengths are rejected at this boundary.
//! Text retains its original encoding. Payloads are opaque bytes, not host
//! structures: some contain stale native pointers which must never be used.

use core::{fmt, iter::FusedIterator};

/// Malformed container with its absolute byte offset.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error {
    /// Start of the rejected field.
    pub offset: usize,
    /// Reason the field cannot be read.
    pub kind: ErrorKind,
}

/// Container framing failure.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ErrorKind {
    /// Missing bytes for a field or its declared contents.
    Truncated,
    /// The first four bytes are not `HDAT`.
    Signature,
    /// Only version 2 is supported by the pinned loader.
    Version(i32),
    /// A count or byte length is negative.
    NegativeLength(i32),
    /// Bytes remain after the declared records.
    TrailingBytes,
}
impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "HDAT at byte {}: {:?}", self.offset, self.kind)
    }
}
impl core::error::Error for Error {}

#[derive(Clone, Copy)]
struct Cursor<'a> {
    data: &'a [u8],
    position: usize,
}
impl<'a> Cursor<'a> {
    fn error(&self, kind: ErrorKind) -> Error {
        Error {
            offset: self.position,
            kind,
        }
    }
    fn take(&mut self, size: usize) -> Result<&'a [u8], Error> {
        let end = self
            .position
            .checked_add(size)
            .ok_or_else(|| self.error(ErrorKind::Truncated))?;
        let value = self
            .data
            .get(self.position..end)
            .ok_or_else(|| self.error(ErrorKind::Truncated))?;
        self.position = end;
        Ok(value)
    }
    fn word(&mut self) -> Result<i32, Error> {
        let b = self.take(4)?;
        Ok(i32::from_le_bytes([b[0], b[1], b[2], b[3]]))
    }
    fn count(&mut self) -> Result<usize, Error> {
        let offset = self.position;
        let value = self.word()?;
        usize::try_from(value).map_err(|_| Error {
            offset,
            kind: ErrorKind::NegativeLength(value),
        })
    }
    fn string(&mut self) -> Result<&'a [u8], Error> {
        let size = self.count()?;
        self.take(size)
    }
    fn entry(&mut self) -> Result<Entry<'a>, Error> {
        let offset = self.position;
        let name = self.string()?;
        let source_path = self.string()?;
        let string_count = self.count()?;
        let mut strings = *self;
        // Each iteration must consume at least a four-byte length. This also
        // bounds work on malformed counts before entering the loop.
        if string_count > (self.data.len() - self.position) / 4 {
            return Err(self.error(ErrorKind::Truncated));
        }
        for _ in 0..string_count {
            self.string()?;
        }
        strings.data = &strings.data[..self.position];
        let payload_flag = self.take(1)?[0];
        let payload = if payload_flag == 0 {
            None
        } else {
            Some(self.string()?)
        };
        let integer_count = self.count()?;
        let integer_bytes = integer_count
            .checked_mul(4)
            .ok_or_else(|| self.error(ErrorKind::Truncated))?;
        let integers = self.take(integer_bytes)?;
        Ok(Entry {
            offset,
            name,
            source_path,
            strings,
            string_count,
            payload_flag,
            payload,
            integers,
        })
    }
}

/// Validated allocation-free view of a complete container.
#[derive(Clone, Copy)]
pub struct Container<'a> {
    records: Cursor<'a>,
    count: usize,
}
impl<'a> Container<'a> {
    /// Validate every record and require exact consumption of the file.
    ///
    /// # Errors
    /// Rejects unsupported headers, negative lengths, truncated fields and
    /// trailing bytes. No record can borrow outside the supplied slice.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        let mut cursor = Cursor { data, position: 0 };
        if cursor.take(4)? != b"HDAT" {
            return Err(Error {
                offset: 0,
                kind: ErrorKind::Signature,
            });
        }
        let version = cursor.word()?;
        if version != 2 {
            return Err(Error {
                offset: 4,
                kind: ErrorKind::Version(version),
            });
        }
        let count = cursor.count()?;
        let records = cursor;
        if count > (data.len() - cursor.position) / 17 {
            return Err(cursor.error(ErrorKind::Truncated));
        }
        for _ in 0..count {
            cursor.entry()?;
        }
        if cursor.position != data.len() {
            return Err(cursor.error(ErrorKind::TrailingBytes));
        }
        Ok(Self { records, count })
    }
    /// Number of named records, including records unrelated to RMG.
    #[must_use]
    pub const fn len(self) -> usize {
        self.count
    }
    /// Whether the container contains no records.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.count == 0
    }
    /// Records in file order, without copying their data.
    #[must_use]
    pub const fn entries(self) -> Entries<'a> {
        Entries {
            cursor: self.records,
            remaining: self.count,
        }
    }
}

/// One named resource record. Its data borrows the original file.
#[derive(Clone, Copy)]
pub struct Entry<'a> {
    offset: usize,
    name: &'a [u8],
    source_path: &'a [u8],
    strings: Cursor<'a>,
    string_count: usize,
    payload_flag: u8,
    payload: Option<&'a [u8]>,
    integers: &'a [u8],
}
impl<'a> Entry<'a> {
    /// Absolute offset of the record's name length.
    #[must_use]
    pub const fn offset(self) -> usize {
        self.offset
    }
    /// Native record name, such as `monst151` or `rmgobjects0`.
    #[must_use]
    pub const fn name(self) -> &'a [u8] {
        self.name
    }
    /// Original authoring path, often empty; never a filesystem instruction.
    #[must_use]
    pub const fn source_path(self) -> &'a [u8] {
        self.source_path
    }
    /// String fields in source order, preserving empty and non-UTF-8 values.
    #[must_use]
    pub const fn strings(self) -> Strings<'a> {
        Strings {
            cursor: self.strings,
            remaining: self.string_count,
        }
    }
    /// Original presence byte; the native loader treats any nonzero value as true.
    #[must_use]
    pub const fn payload_flag(self) -> u8 {
        self.payload_flag
    }
    /// Binary data, retaining the distinction between absent and present-empty.
    #[must_use]
    pub const fn payload(self) -> Option<&'a [u8]> {
        self.payload
    }
    /// Signed integer fields in source order.
    #[must_use]
    pub fn integers(self) -> impl ExactSizeIterator<Item = i32> + DoubleEndedIterator + 'a {
        self.integers
            .chunks_exact(4)
            .map(|b| i32::from_le_bytes([b[0], b[1], b[2], b[3]]))
    }
}

/// Infallible iterator over previously validated records.
#[derive(Clone)]
pub struct Entries<'a> {
    cursor: Cursor<'a>,
    remaining: usize,
}
impl<'a> Iterator for Entries<'a> {
    type Item = Entry<'a>;
    fn next(&mut self) -> Option<Self::Item> {
        if self.remaining == 0 {
            return None;
        }
        let entry = self
            .cursor
            .entry()
            .expect("container framing was validated");
        self.remaining -= 1;
        Some(entry)
    }
    fn size_hint(&self) -> (usize, Option<usize>) {
        (self.remaining, Some(self.remaining))
    }
}
impl ExactSizeIterator for Entries<'_> {}
impl FusedIterator for Entries<'_> {}

/// Infallible iterator over previously validated string fields.
#[derive(Clone)]
pub struct Strings<'a> {
    cursor: Cursor<'a>,
    remaining: usize,
}
impl<'a> Iterator for Strings<'a> {
    type Item = &'a [u8];
    fn next(&mut self) -> Option<Self::Item> {
        if self.remaining == 0 {
            return None;
        }
        let string = self.cursor.string().expect("string framing was validated");
        self.remaining -= 1;
        Some(string)
    }
    fn size_hint(&self) -> (usize, Option<usize>) {
        (self.remaining, Some(self.remaining))
    }
}
impl ExactSizeIterator for Strings<'_> {}
impl FusedIterator for Strings<'_> {}
