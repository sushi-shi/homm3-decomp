use core::fmt;

/// A malformed DEF or invalid decode/draw request.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    /// A fixed record or payload ran off the available bytes.
    Truncated {
        what: &'static str,
        at: usize,
        needed: usize,
        available: usize,
    },
    /// Size arithmetic overflowed the host address space.
    SizeOverflow { what: &'static str },
    /// A count or dimension cannot be represented by the model.
    BadDimension { what: &'static str, value: u32 },
    /// A frame named an encoding outside the retail 0..=3 domain.
    BadEncoding(u32),
    /// A group index was outside the DEF's group table.
    GroupOutOfRange { index: usize, count: usize },
    /// A frame index was outside one group.
    FrameOutOfRange { index: usize, count: usize },
    /// A frame offset did not point inside the DEF.
    FrameOffset { offset: u32, available: usize },
    /// A scanline offset did not point inside the encoded frame.
    RowOffset {
        row: usize,
        offset: u32,
        available: usize,
    },
    /// A run crossed the width declared for its row or 32-pixel segment.
    RunOverrun {
        row: usize,
        run: usize,
        remaining: usize,
    },
    /// A row ended before its declared width was decoded.
    ShortRow {
        row: usize,
        decoded: usize,
        expected: usize,
    },
    /// A caller-owned decode destination has the wrong length.
    BadDestination { needed: usize, available: usize },
    /// A canvas width/height/stride/pixel slice combination is inconsistent.
    BadCanvas,
    /// A source rectangle had negative dimensions or overflowed.
    BadRectangle,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Truncated {
                what,
                at,
                needed,
                available,
            } => write!(
                f,
                "{what} at {at:#x} needs {needed} bytes, only {available} remain"
            ),
            Self::SizeOverflow { what } => write!(f, "{what} size overflows"),
            Self::BadDimension { what, value } => {
                write!(f, "invalid {what} value {value}")
            }
            Self::BadEncoding(value) => write!(f, "unknown DEF encoding {value}"),
            Self::GroupOutOfRange { index, count } => {
                write!(f, "group index {index} is outside 0..{count}")
            }
            Self::FrameOutOfRange { index, count } => {
                write!(f, "frame index {index} is outside 0..{count}")
            }
            Self::FrameOffset { offset, available } => write!(
                f,
                "frame offset {offset:#x} is outside {available:#x} DEF bytes"
            ),
            Self::RowOffset {
                row,
                offset,
                available,
            } => write!(
                f,
                "row {row} offset {offset:#x} is outside {available:#x} frame bytes"
            ),
            Self::RunOverrun {
                row,
                run,
                remaining,
            } => write!(
                f,
                "row {row} run of {run} pixels exceeds its {remaining}-pixel remainder"
            ),
            Self::ShortRow {
                row,
                decoded,
                expected,
            } => write!(f, "row {row} decoded {decoded} of {expected} pixels"),
            Self::BadDestination { needed, available } => write!(
                f,
                "decode destination needs {needed} pixels, found {available}"
            ),
            Self::BadCanvas => f.write_str("invalid canvas dimensions, stride, or pixel slice"),
            Self::BadRectangle => f.write_str("invalid or overflowing rectangle"),
        }
    }
}

impl core::error::Error for Error {}
