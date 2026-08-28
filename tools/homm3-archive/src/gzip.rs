//! Allocation-free validation of the gzip envelope used by maps and saves.

use core::fmt;

const FIXED_HEADER_SIZE: usize = 10;
const TRAILER_SIZE: usize = 8;
const FLAG_HEADER_CRC: u8 = 0x02;
const FLAG_EXTRA: u8 = 0x04;
const FLAG_NAME: u8 = 0x08;
const FLAG_COMMENT: u8 = 0x10;
const FLAG_RESERVED: u8 = 0xe0;

/// A malformed gzip member envelope.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    Short { needed: usize, available: usize },
    BadMagic,
    UnsupportedCompression(u8),
    ReservedFlags(u8),
    MissingTerminator { offset: usize },
    SizeOverflow,
    HeaderOverlapsTrailer { header_end: usize, trailer: usize },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Short { needed, available } => {
                write!(f, "gzip needs {needed} bytes, found {available}")
            }
            Self::BadMagic => write!(f, "gzip magic is not 1f 8b"),
            Self::UnsupportedCompression(method) => {
                write!(f, "gzip compression method {method} is not deflate")
            }
            Self::ReservedFlags(flags) => write!(f, "gzip uses reserved flags {flags:#04x}"),
            Self::MissingTerminator { offset } => {
                write!(f, "gzip string beginning at {offset:#x} has no terminator")
            }
            Self::SizeOverflow => write!(f, "gzip header size overflows the host"),
            Self::HeaderOverlapsTrailer {
                header_end,
                trailer,
            } => write!(
                f,
                "gzip header ends at {header_end:#x}, after trailer at {trailer:#x}"
            ),
        }
    }
}

impl core::error::Error for Error {}

/// A validated gzip member borrowing its raw-deflate stream and trailer.
#[derive(Clone, Copy, Debug)]
pub struct Member<'a> {
    flags: u8,
    modified_time: u32,
    extra_flags: u8,
    operating_system: u8,
    deflate: &'a [u8],
    crc32: u32,
    uncompressed_size: u32,
}

impl<'a> Member<'a> {
    /// Parse one gzip envelope, including all optional header fields.
    ///
    /// The compressed bitstream is deliberately left opaque; the `std`
    /// oracle owns inflation and CRC verification.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for invalid fixed fields, unterminated optional
    /// strings, missing extents, or a header that reaches the trailer.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        let minimum = FIXED_HEADER_SIZE + TRAILER_SIZE;
        if data.len() < minimum {
            return Err(Error::Short {
                needed: minimum,
                available: data.len(),
            });
        }
        if data[0..2] != [0x1f, 0x8b] {
            return Err(Error::BadMagic);
        }
        if data[2] != 8 {
            return Err(Error::UnsupportedCompression(data[2]));
        }
        let flags = data[3];
        if flags & FLAG_RESERVED != 0 {
            return Err(Error::ReservedFlags(flags));
        }

        let trailer = data.len() - TRAILER_SIZE;
        let mut position = FIXED_HEADER_SIZE;
        if flags & FLAG_EXTRA != 0 {
            let length_bytes = data.get(position..position + 2).ok_or(Error::Short {
                needed: position + 2,
                available: data.len(),
            })?;
            let length = usize::from(u16::from_le_bytes([length_bytes[0], length_bytes[1]]));
            position = position
                .checked_add(2)
                .and_then(|value| value.checked_add(length))
                .ok_or(Error::SizeOverflow)?;
        }
        if flags & FLAG_NAME != 0 {
            position = terminated_end(data, position, trailer)?;
        }
        if flags & FLAG_COMMENT != 0 {
            position = terminated_end(data, position, trailer)?;
        }
        if flags & FLAG_HEADER_CRC != 0 {
            position = position.checked_add(2).ok_or(Error::SizeOverflow)?;
        }
        if position > trailer {
            return Err(Error::HeaderOverlapsTrailer {
                header_end: position,
                trailer,
            });
        }

        Ok(Self {
            flags,
            modified_time: little_u32(data, 4),
            extra_flags: data[8],
            operating_system: data[9],
            deflate: &data[position..trailer],
            crc32: little_u32(data, trailer),
            uncompressed_size: little_u32(data, trailer + 4),
        })
    }

    /// Header flag byte.
    #[must_use]
    pub const fn flags(self) -> u8 {
        self.flags
    }

    /// Header modification time.
    #[must_use]
    pub const fn modified_time(self) -> u32 {
        self.modified_time
    }

    /// Deflate compressor hint byte.
    #[must_use]
    pub const fn extra_flags(self) -> u8 {
        self.extra_flags
    }

    /// Originating operating-system byte.
    #[must_use]
    pub const fn operating_system(self) -> u8 {
        self.operating_system
    }

    /// Raw deflate bitstream between header and trailer.
    #[must_use]
    pub const fn deflate(self) -> &'a [u8] {
        self.deflate
    }

    /// Trailer CRC-32 of the uncompressed bytes.
    #[must_use]
    pub const fn crc32(self) -> u32 {
        self.crc32
    }

    /// Trailer uncompressed size modulo 2^32.
    #[must_use]
    pub const fn uncompressed_size(self) -> u32 {
        self.uncompressed_size
    }
}

fn terminated_end(data: &[u8], start: usize, limit: usize) -> Result<usize, Error> {
    let bytes = data.get(start..limit).ok_or(Error::HeaderOverlapsTrailer {
        header_end: start,
        trailer: limit,
    })?;
    let relative = bytes
        .iter()
        .position(|&byte| byte == 0)
        .ok_or(Error::MissingTerminator { offset: start })?;
    start.checked_add(relative + 1).ok_or(Error::SizeOverflow)
}

fn little_u32(data: &[u8], at: usize) -> u32 {
    u32::from_le_bytes([data[at], data[at + 1], data[at + 2], data[at + 3]])
}
