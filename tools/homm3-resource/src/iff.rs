//! Allocation-free IFF chunk and XMIDI envelope validation.
//!
//! Heroes III hands the event stream to Miles Sound System. This module owns
//! only the byte structure visible before that handoff: big-endian IFF chunk
//! extents, the `FORM XDIR` track count, and `CAT XMID` track forms.

use core::fmt;

/// An IFF four-character identifier.
pub type FourCc = [u8; 4];

/// A malformed IFF chunk sequence.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IffError {
    /// Fewer than eight bytes remain for a chunk header.
    ShortHeader { offset: usize, available: usize },
    /// A declared chunk payload or its word-alignment byte is absent.
    ChunkOutOfBounds {
        offset: usize,
        declared: u32,
        available: usize,
    },
    /// Host-size arithmetic overflowed while checking a chunk.
    SizeOverflow { offset: usize },
    /// A container chunk does not contain its four-byte form type.
    ShortContainer { offset: usize, size: usize },
}

impl fmt::Display for IffError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::ShortHeader { offset, available } => write!(
                f,
                "IFF chunk at {offset:#x} has {available} header bytes, needs 8"
            ),
            Self::ChunkOutOfBounds {
                offset,
                declared,
                available,
            } => write!(
                f,
                "IFF chunk at {offset:#x} declares {declared} bytes, only {available} remain"
            ),
            Self::SizeOverflow { offset } => {
                write!(f, "IFF chunk size at {offset:#x} overflows the host")
            }
            Self::ShortContainer { offset, size } => write!(
                f,
                "IFF container at {offset:#x} has {size} data bytes, needs a four-byte type"
            ),
        }
    }
}

impl core::error::Error for IffError {}

/// One validated IFF chunk borrowing its unpadded payload.
#[derive(Clone, Copy, Debug)]
pub struct Chunk<'a> {
    offset: usize,
    id: FourCc,
    data: &'a [u8],
}

impl<'a> Chunk<'a> {
    /// Offset of this chunk header within the sequence supplied to [`Chunks`].
    #[must_use]
    pub const fn offset(self) -> usize {
        self.offset
    }

    /// Four-character chunk identifier.
    #[must_use]
    pub const fn id(self) -> FourCc {
        self.id
    }

    /// Declared chunk payload, excluding a possible word-alignment byte.
    #[must_use]
    pub const fn data(self) -> &'a [u8] {
        self.data
    }

    /// Split a `FORM`, `CAT `, or `LIST` payload into type and child bytes.
    ///
    /// # Errors
    ///
    /// Returns [`IffError::ShortContainer`] when the payload has no form type.
    pub fn container(self) -> Result<(FourCc, &'a [u8]), IffError> {
        let kind = self.data.get(..4).ok_or(IffError::ShortContainer {
            offset: self.offset,
            size: self.data.len(),
        })?;
        Ok(([kind[0], kind[1], kind[2], kind[3]], &self.data[4..]))
    }
}

/// Iterator over consecutive word-aligned IFF chunks.
#[derive(Clone, Debug)]
pub struct Chunks<'a> {
    data: &'a [u8],
    position: usize,
    failed: bool,
}

impl<'a> Chunks<'a> {
    /// Begin reading a complete IFF chunk sequence.
    #[must_use]
    pub const fn new(data: &'a [u8]) -> Self {
        Self {
            data,
            position: 0,
            failed: false,
        }
    }
}

impl<'a> Iterator for Chunks<'a> {
    type Item = Result<Chunk<'a>, IffError>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.failed || self.position == self.data.len() {
            return None;
        }
        let offset = self.position;
        let remaining = &self.data[offset..];
        if remaining.len() < 8 {
            self.failed = true;
            return Some(Err(IffError::ShortHeader {
                offset,
                available: remaining.len(),
            }));
        }
        let id = [remaining[0], remaining[1], remaining[2], remaining[3]];
        let declared = u32::from_be_bytes([remaining[4], remaining[5], remaining[6], remaining[7]]);
        let Ok(size) = usize::try_from(declared) else {
            self.failed = true;
            return Some(Err(IffError::SizeOverflow { offset }));
        };
        let Some(padded) = size.checked_add(size & 1) else {
            self.failed = true;
            return Some(Err(IffError::SizeOverflow { offset }));
        };
        let Some(total) = 8usize.checked_add(padded) else {
            self.failed = true;
            return Some(Err(IffError::SizeOverflow { offset }));
        };
        if total > remaining.len() {
            self.failed = true;
            return Some(Err(IffError::ChunkOutOfBounds {
                offset,
                declared,
                available: remaining.len().saturating_sub(8),
            }));
        }
        self.position += total;
        Some(Ok(Chunk {
            offset,
            id,
            data: &remaining[8..8 + size],
        }))
    }
}

/// A malformed XMIDI envelope.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum XmidiError {
    /// The underlying IFF chunk stream is malformed.
    Iff(IffError),
    /// The two required top-level chunks are absent or reordered.
    TopLevelShape,
    /// The XDIR form is missing its sole two-byte INFO track count.
    DirectoryShape,
    /// The CAT payload is not an XMID form collection.
    CatalogShape,
    /// A catalog member is not a `FORM XMID` track.
    TrackShape { track: usize },
    /// A track contains no event stream or more than one event stream.
    EventShape { track: usize },
    /// XDIR's track count differs from the number of track forms.
    TrackCount { declared: u16, actual: usize },
}

impl From<IffError> for XmidiError {
    fn from(value: IffError) -> Self {
        Self::Iff(value)
    }
}

impl fmt::Display for XmidiError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Iff(error) => error.fmt(f),
            Self::TopLevelShape => write!(f, "XMIDI needs exactly FORM XDIR then CAT XMID"),
            Self::DirectoryShape => write!(f, "XMIDI XDIR needs exactly one two-byte INFO chunk"),
            Self::CatalogShape => write!(f, "XMIDI catalog type is not XMID"),
            Self::TrackShape { track } => {
                write!(f, "XMIDI catalog member {track} is not FORM XMID")
            }
            Self::EventShape { track } => {
                write!(f, "XMIDI track {track} needs exactly one EVNT chunk")
            }
            Self::TrackCount { declared, actual } => write!(
                f,
                "XMIDI XDIR declares {declared} tracks but catalog contains {actual}"
            ),
        }
    }
}

impl core::error::Error for XmidiError {}

/// A validated XMIDI file borrowing its track catalog.
#[derive(Clone, Copy, Debug)]
pub struct Xmidi<'a> {
    catalog: &'a [u8],
    track_count: u16,
}

impl<'a> Xmidi<'a> {
    /// Validate the IFF/XMIDI envelope used by the shipped `DEFAULT.XMI`.
    ///
    /// # Errors
    ///
    /// Returns [`XmidiError`] for malformed chunk extents, envelope ordering,
    /// track counts, track forms, or event-stream multiplicity.
    pub fn parse(data: &'a [u8]) -> Result<Self, XmidiError> {
        let mut top = Chunks::new(data);
        let directory = top.next().ok_or(XmidiError::TopLevelShape)??;
        let catalog = top.next().ok_or(XmidiError::TopLevelShape)??;
        if top.next().is_some() || directory.id() != *b"FORM" || catalog.id() != *b"CAT " {
            return Err(XmidiError::TopLevelShape);
        }

        let (directory_type, directory_children) = directory.container()?;
        if directory_type != *b"XDIR" {
            return Err(XmidiError::TopLevelShape);
        }
        let mut directory_chunks = Chunks::new(directory_children);
        let info = directory_chunks
            .next()
            .ok_or(XmidiError::DirectoryShape)??;
        if info.id() != *b"INFO" || info.data().len() != 2 || directory_chunks.next().is_some() {
            return Err(XmidiError::DirectoryShape);
        }
        let track_count = u16::from_le_bytes([info.data()[0], info.data()[1]]);

        let (catalog_type, catalog_children) = catalog.container()?;
        if catalog_type != *b"XMID" {
            return Err(XmidiError::CatalogShape);
        }
        let mut actual = 0usize;
        for track in Chunks::new(catalog_children) {
            let track = track?;
            if track.id() != *b"FORM" {
                return Err(XmidiError::TrackShape { track: actual });
            }
            let (track_type, track_children) = track.container()?;
            if track_type != *b"XMID" {
                return Err(XmidiError::TrackShape { track: actual });
            }
            let mut events = 0usize;
            for child in Chunks::new(track_children) {
                if child?.id() == *b"EVNT" {
                    events += 1;
                }
            }
            if events != 1 {
                return Err(XmidiError::EventShape { track: actual });
            }
            actual += 1;
        }
        if actual != usize::from(track_count) {
            return Err(XmidiError::TrackCount {
                declared: track_count,
                actual,
            });
        }
        Ok(Self {
            catalog: catalog_children,
            track_count,
        })
    }

    /// Number of tracks declared by XDIR and present in the catalog.
    #[must_use]
    pub const fn track_count(self) -> u16 {
        self.track_count
    }

    /// Iterate the validated `FORM XMID` tracks.
    #[must_use]
    pub const fn tracks(self) -> XmidiTracks<'a> {
        XmidiTracks {
            chunks: Chunks::new(self.catalog),
        }
    }
}

/// Iterator over validated XMIDI tracks.
#[derive(Clone, Debug)]
pub struct XmidiTracks<'a> {
    chunks: Chunks<'a>,
}

impl<'a> Iterator for XmidiTracks<'a> {
    type Item = XmidiTrack<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        let chunk = self.chunks.next()?.ok()?;
        let (_, children) = chunk.container().ok()?;
        Some(XmidiTrack { children })
    }
}

/// One validated XMIDI track.
#[derive(Clone, Copy, Debug)]
pub struct XmidiTrack<'a> {
    children: &'a [u8],
}

impl<'a> XmidiTrack<'a> {
    /// Iterate all track chunks, including `TIMB`, `EVNT`, and extensions.
    #[must_use]
    pub const fn chunks(self) -> Chunks<'a> {
        Chunks::new(self.children)
    }

    /// Borrow the sole validated event stream.
    #[must_use]
    pub fn events(self) -> &'a [u8] {
        for chunk in self.chunks().flatten() {
            if chunk.id() == *b"EVNT" {
                return chunk.data();
            }
        }
        &[]
    }

    /// Borrow the optional instrument/timbre map.
    #[must_use]
    pub fn timbres(self) -> Option<&'a [u8]> {
        self.chunks()
            .flatten()
            .find(|chunk| chunk.id() == *b"TIMB")
            .map(Chunk::data)
    }
}
