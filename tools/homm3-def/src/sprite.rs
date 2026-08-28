use crate::{Error, Result, RowRuns};

/// Bytes in the fixed DEF header (`type`, extent, group count, RGB palette).
pub const DEF_HEADER_SIZE: usize = 0x310;
/// Bytes in one group header before its names and frame offsets.
pub const GROUP_HEADER_SIZE: usize = 0x10;
/// Bytes in the ordinary cropped frame header.
pub const FRAME_HEADER_SIZE: usize = 0x20;
/// Bytes in a compact frame header.
pub const COMPACT_FRAME_HEADER_SIZE: usize = 0x10;
const FRAME_NAME_SIZE: usize = 13;
const PALETTE_SIZE: usize = 256 * 3;

/// Placement of per-frame metadata within a DEF member.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Dialect {
    /// Retail `GetSprite`: cropped types use 32-byte headers at offsets;
    /// compact types keep a 16-byte header band after the group tables.
    #[default]
    Retail,
    /// Official-media anomaly present since `RoE` 1.0: each directory offset
    /// points at a 16-byte compact header immediately followed by its payload.
    InterleavedCompactFrames,
}

/// Retail resource-type value from the DEF header.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct DefType(pub u32);

impl DefType {
    /// Generic sprite.
    pub const SPRITE: Self = Self(64);
    /// Sprite-definition record family using compact frame headers.
    pub const SPRITE_DEFINITION: Self = Self(65);
    /// Creature animation.
    pub const CREATURE: Self = Self(66);
    /// Adventure-map object.
    pub const ADVENTURE_OBJECT: Self = Self(67);
    /// Hero animation.
    pub const HERO: Self = Self(68);
    /// Terrain/road/river tileset.
    pub const TILESET: Self = Self(69);
    /// Mouse pointer.
    pub const POINTER: Self = Self(70);
    /// Interface sprite.
    pub const INTERFACE: Self = Self(71);
    /// Standalone frame record family using compact frame headers.
    pub const SPRITE_FRAME: Self = Self(72);
    /// Combat hero animation.
    pub const COMBAT_HERO: Self = Self(73);
    /// Adventure-mask record family using compact frame headers.
    pub const ADVENTURE_MASK: Self = Self(79);

    /// Whether `GetSprite` reads a 32-byte cropped header at each frame offset.
    ///
    /// The exact domain is retail's eight-arm test at `0x55c7b0`.
    #[must_use]
    pub const fn has_cropped_frames(self) -> bool {
        matches!(self.0, 64 | 66 | 67 | 68 | 69 | 70 | 71 | 73)
    }
}

/// Frame payload encoding, retail `TEncodingMethod` 0..=3.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(u32)]
pub enum Encoding {
    /// Cropped pixels stored row-major.
    Raw = 0,
    /// Dword offset per row; `(colour, count-minus-one)` packets and colour
    /// 255 literal packets.
    GeneralRle = 1,
    /// Word offset per row; high-three-bit fill/literal packets.
    TilesetRle = 2,
    /// Word offset per 32-pixel row segment; high-three-bit packets.
    AdventureRle = 3,
}

impl TryFrom<u32> for Encoding {
    type Error = Error;

    fn try_from(value: u32) -> Result<Self> {
        match value {
            0 => Ok(Self::Raw),
            1 => Ok(Self::GeneralRle),
            2 => Ok(Self::TilesetRle),
            3 => Ok(Self::AdventureRle),
            _ => Err(Error::BadEncoding(value)),
        }
    }
}

/// Borrowed 256-entry RGB palette.
#[derive(Clone, Copy, Debug)]
pub struct Palette<'a> {
    data: &'a [u8],
}

impl<'a> Palette<'a> {
    pub(crate) const fn new(data: &'a [u8]) -> Self {
        Self { data }
    }

    /// Raw `256 * RGB` bytes.
    #[must_use]
    pub const fn bytes(self) -> &'a [u8] {
        self.data
    }

    /// One RGB triple.
    #[must_use]
    pub fn rgb(self, index: u8) -> [u8; 3] {
        let at = usize::from(index) * 3;
        [self.data[at], self.data[at + 1], self.data[at + 2]]
    }
}

/// Fixed fields at the head of a DEF.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SpriteHeader {
    /// Resource-family discriminator.
    pub kind: DefType,
    /// Full sprite width.
    pub width: u32,
    /// Full sprite height.
    pub height: u32,
    /// Number of sequence/group records.
    pub group_count: usize,
}

/// A parsed DEF borrowing its archive member.
#[derive(Clone, Copy, Debug)]
pub struct Sprite<'a> {
    data: &'a [u8],
    header: SpriteHeader,
    palette: Palette<'a>,
    groups_end: usize,
    total_frames: usize,
    dialect: Dialect,
}

impl<'a> Sprite<'a> {
    /// Parse and validate the variable-length group table.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] when a fixed header, group names, or offset table is
    /// truncated or when count arithmetic overflows.
    pub fn parse(data: &'a [u8]) -> Result<Self> {
        Self::parse_with_dialect(data, Dialect::Retail)
    }

    /// Parse using an explicitly selected frame-metadata dialect.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] under the same conditions as [`Sprite::parse`].
    pub fn parse_with_dialect(data: &'a [u8], dialect: Dialect) -> Result<Self> {
        let fixed = take(data, 0, DEF_HEADER_SIZE, "DEF header")?;
        let group_word = word(fixed, 12);
        let group_count = usize::try_from(group_word).map_err(|_| Error::BadDimension {
            what: "group count",
            value: group_word,
        })?;
        let header = SpriteHeader {
            kind: DefType(word(fixed, 0)),
            width: word(fixed, 4),
            height: word(fixed, 8),
            group_count,
        };
        let palette = Palette::new(&fixed[16..16 + PALETTE_SIZE]);

        let mut at = DEF_HEADER_SIZE;
        let mut total_frames = 0usize;
        for _ in 0..group_count {
            let group = take(data, at, GROUP_HEADER_SIZE, "DEF group header")?;
            let frame_word = word(group, 4);
            let frames = usize::try_from(frame_word).map_err(|_| Error::BadDimension {
                what: "group frame count",
                value: frame_word,
            })?;
            total_frames = total_frames
                .checked_add(frames)
                .ok_or(Error::SizeOverflow {
                    what: "frame count",
                })?;
            let names = frames
                .checked_mul(FRAME_NAME_SIZE)
                .ok_or(Error::SizeOverflow {
                    what: "frame-name table",
                })?;
            let offsets = frames.checked_mul(4).ok_or(Error::SizeOverflow {
                what: "frame-offset table",
            })?;
            let group_size = GROUP_HEADER_SIZE
                .checked_add(names)
                .and_then(|size| size.checked_add(offsets))
                .ok_or(Error::SizeOverflow {
                    what: "group table",
                })?;
            take(data, at, group_size, "DEF group table")?;
            at = at.checked_add(group_size).ok_or(Error::SizeOverflow {
                what: "group position",
            })?;
        }

        if dialect == Dialect::Retail && !header.kind.has_cropped_frames() {
            let compact_bytes =
                total_frames
                    .checked_mul(COMPACT_FRAME_HEADER_SIZE)
                    .ok_or(Error::SizeOverflow {
                        what: "compact frame headers",
                    })?;
            take(data, at, compact_bytes, "compact frame headers")?;
        }

        Ok(Self {
            data,
            header,
            palette,
            groups_end: at,
            total_frames,
            dialect,
        })
    }

    /// Fixed DEF fields.
    #[must_use]
    pub const fn header(self) -> SpriteHeader {
        self.header
    }

    /// Borrowed RGB palette.
    #[must_use]
    pub const fn palette(self) -> Palette<'a> {
        self.palette
    }

    /// Number of sequence/group records.
    #[must_use]
    pub const fn group_count(self) -> usize {
        self.header.group_count
    }

    /// Total frame references across all groups.
    #[must_use]
    pub const fn total_frames(self) -> usize {
        self.total_frames
    }

    /// Decode a group by table index.
    ///
    /// # Errors
    ///
    /// Returns [`Error::GroupOutOfRange`] for an invalid index.
    pub fn group(self, index: usize) -> Result<Group<'a>> {
        if index >= self.header.group_count {
            return Err(Error::GroupOutOfRange {
                index,
                count: self.header.group_count,
            });
        }
        let mut at = DEF_HEADER_SIZE;
        let mut global_frame = 0usize;
        for current in 0..=index {
            let header = take(self.data, at, GROUP_HEADER_SIZE, "DEF group header")?;
            let frame_count =
                usize::try_from(word(header, 4)).map_err(|_| Error::SizeOverflow {
                    what: "group frame count",
                })?;
            let names_at = at + GROUP_HEADER_SIZE;
            let offsets_at = names_at + frame_count * FRAME_NAME_SIZE;
            if current == index {
                return Ok(Group {
                    data: self.data,
                    kind: self.header.kind,
                    groups_end: self.groups_end,
                    id: word(header, 0),
                    unknown_a: word(header, 8),
                    unknown_b: word(header, 12),
                    frame_count,
                    names_at,
                    offsets_at,
                    global_frame,
                    dialect: self.dialect,
                });
            }
            at = offsets_at + frame_count * 4;
            global_frame += frame_count;
        }
        unreachable!("range checked group walk")
    }

    /// Find the first group carrying a particular sequence id.
    #[must_use]
    pub fn find_group(self, id: u32) -> Option<Group<'a>> {
        self.groups().find(|group| group.id() == id)
    }

    /// Every group in table order.
    #[must_use = "iterators are lazy"]
    pub const fn groups(self) -> Groups<'a> {
        Groups {
            sprite: self,
            index: 0,
        }
    }
}

/// Iterator over DEF groups.
#[derive(Clone, Debug)]
pub struct Groups<'a> {
    sprite: Sprite<'a>,
    index: usize,
}

impl<'a> Iterator for Groups<'a> {
    type Item = Group<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index >= self.sprite.group_count() {
            return None;
        }
        let group = self.sprite.group(self.index).ok()?;
        self.index += 1;
        Some(group)
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        let remaining = self.sprite.group_count() - self.index;
        (remaining, Some(remaining))
    }
}

impl ExactSizeIterator for Groups<'_> {}

/// One DEF sequence/group record.
#[derive(Clone, Copy, Debug)]
pub struct Group<'a> {
    data: &'a [u8],
    kind: DefType,
    groups_end: usize,
    id: u32,
    unknown_a: u32,
    unknown_b: u32,
    frame_count: usize,
    names_at: usize,
    offsets_at: usize,
    global_frame: usize,
    dialect: Dialect,
}

impl<'a> Group<'a> {
    /// Sequence id used by callers (movement/attack/etc. for animated DEFs).
    #[must_use]
    pub const fn id(self) -> u32 {
        self.id
    }

    /// First unknown group-header dword, preserved without invented meaning.
    #[must_use]
    pub const fn unknown_a(self) -> u32 {
        self.unknown_a
    }

    /// Second unknown group-header dword, preserved without invented meaning.
    #[must_use]
    pub const fn unknown_b(self) -> u32 {
        self.unknown_b
    }

    /// Number of frame references.
    #[must_use]
    pub const fn len(self) -> usize {
        self.frame_count
    }

    /// Whether the sequence is empty.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.frame_count == 0
    }

    /// Parse one frame and borrow its encoded payload.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for an invalid index, frame offset, header, encoding,
    /// dimension, or payload extent.
    pub fn frame(self, index: usize) -> Result<Frame<'a>> {
        if index >= self.frame_count {
            return Err(Error::FrameOutOfRange {
                index,
                count: self.frame_count,
            });
        }
        let name_at = self.names_at + index * FRAME_NAME_SIZE;
        let raw_name = take(self.data, name_at, FRAME_NAME_SIZE, "frame name")?;
        let end = raw_name
            .iter()
            .position(|&byte| byte == 0)
            .unwrap_or(FRAME_NAME_SIZE);
        let name = &raw_name[..end];
        let offset = word(self.data, self.offsets_at + index * 4);
        let offset_at = usize::try_from(offset).map_err(|_| Error::FrameOffset {
            offset,
            available: self.data.len(),
        })?;
        if offset_at >= self.data.len() {
            return Err(Error::FrameOffset {
                offset,
                available: self.data.len(),
            });
        }

        if self.dialect == Dialect::InterleavedCompactFrames {
            let stream_at =
                offset_at
                    .checked_add(COMPACT_FRAME_HEADER_SIZE)
                    .ok_or(Error::SizeOverflow {
                        what: "interleaved frame position",
                    })?;
            compact_frame(
                self.data,
                name,
                offset_at,
                stream_at,
                "interleaved compact frame header",
            )
        } else if self.kind.has_cropped_frames() {
            let header = take(self.data, offset_at, FRAME_HEADER_SIZE, "frame header")?;
            let data_size = size_field(header, 0, "frame data size")?;
            let encoding = Encoding::try_from(word(header, 4))?;
            let width = dimension(header, 8, "frame width")?;
            let height = dimension(header, 12, "frame height")?;
            let cropped_width = dimension(header, 16, "cropped width")?;
            let cropped_height = dimension(header, 20, "cropped height")?;
            let cropped_x = signed_word(header, 24);
            let cropped_y = signed_word(header, 28);
            let stream_at = offset_at + FRAME_HEADER_SIZE;
            let stream = take(self.data, stream_at, data_size, "frame payload")?;
            Ok(Frame {
                name,
                data_size,
                encoding,
                width,
                height,
                cropped_width,
                cropped_height,
                cropped_x,
                cropped_y,
                stream,
            })
        } else {
            let global = self
                .global_frame
                .checked_add(index)
                .ok_or(Error::SizeOverflow {
                    what: "compact frame index",
                })?;
            let header_at = global
                .checked_mul(COMPACT_FRAME_HEADER_SIZE)
                .and_then(|relative| self.groups_end.checked_add(relative))
                .ok_or(Error::SizeOverflow {
                    what: "compact frame position",
                })?;
            compact_frame(
                self.data,
                name,
                header_at,
                offset_at,
                "compact frame header",
            )
        }
    }
}

fn compact_frame<'a>(
    data: &'a [u8],
    name: &'a [u8],
    header_at: usize,
    stream_at: usize,
    header_what: &'static str,
) -> Result<Frame<'a>> {
    let header = take(data, header_at, COMPACT_FRAME_HEADER_SIZE, header_what)?;
    let data_size = size_field(header, 0, "compact frame data size")?;
    let encoding = Encoding::try_from(word(header, 4))?;
    let width = dimension(header, 8, "frame width")?;
    let height = dimension(header, 12, "frame height")?;
    let stream = take(data, stream_at, data_size, "frame payload")?;
    Ok(Frame {
        name,
        data_size,
        encoding,
        width,
        height,
        cropped_width: width,
        cropped_height: height,
        cropped_x: 0,
        cropped_y: 0,
        stream,
    })
}

/// One parsed frame and its borrowed encoded payload.
#[derive(Clone, Copy, Debug)]
pub struct Frame<'a> {
    name: &'a [u8],
    data_size: usize,
    encoding: Encoding,
    width: usize,
    height: usize,
    cropped_width: usize,
    cropped_height: usize,
    cropped_x: i32,
    cropped_y: i32,
    stream: &'a [u8],
}

impl<'a> Frame<'a> {
    /// Fixed 13-byte frame name with NUL padding removed.
    #[must_use]
    pub const fn name(self) -> &'a [u8] {
        self.name
    }

    /// Frame name as text when valid UTF-8.
    #[must_use]
    pub fn name_str(self) -> Option<&'a str> {
        core::str::from_utf8(self.name).ok()
    }

    /// Advertised encoded byte count.
    #[must_use]
    pub const fn data_size(self) -> usize {
        self.data_size
    }

    /// Payload encoding.
    #[must_use]
    pub const fn encoding(self) -> Encoding {
        self.encoding
    }

    /// Full uncropped width.
    #[must_use]
    pub const fn width(self) -> usize {
        self.width
    }

    /// Full uncropped height.
    #[must_use]
    pub const fn height(self) -> usize {
        self.height
    }

    /// Stored rectangle width.
    #[must_use]
    pub const fn cropped_width(self) -> usize {
        self.cropped_width
    }

    /// Stored rectangle height.
    #[must_use]
    pub const fn cropped_height(self) -> usize {
        self.cropped_height
    }

    /// Stored rectangle x offset in full-frame coordinates.
    #[must_use]
    pub const fn cropped_x(self) -> i32 {
        self.cropped_x
    }

    /// Stored rectangle y offset in full-frame coordinates.
    #[must_use]
    pub const fn cropped_y(self) -> i32 {
        self.cropped_y
    }

    /// Borrowed encoded bytes, including any row-offset table.
    #[must_use]
    pub const fn stream(self) -> &'a [u8] {
        self.stream
    }

    /// Number of stored pixels after decoding.
    ///
    /// # Errors
    ///
    /// Returns [`Error::SizeOverflow`] if the area does not fit `usize`.
    pub fn pixel_len(self) -> Result<usize> {
        self.cropped_width
            .checked_mul(self.cropped_height)
            .ok_or(Error::SizeOverflow { what: "frame area" })
    }

    /// Iterate the encoded runs of one stored scanline.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for a row outside the crop or a malformed offset.
    pub fn runs(self, row: usize) -> Result<RowRuns<'a>> {
        RowRuns::new(self, row)
    }

    /// Validate and decode into an exactly sized caller-owned index buffer.
    ///
    /// Fill/control codes are preserved as their palette indices. Use
    /// [`Frame::runs`] when the distinction between an encoded fill and a
    /// literal byte matters.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for a bad destination length or malformed row.
    pub fn decode_into(self, destination: &mut [u8]) -> Result<()> {
        let needed = self.pixel_len()?;
        if destination.len() != needed {
            return Err(Error::BadDestination {
                needed,
                available: destination.len(),
            });
        }
        let width = self.cropped_width;
        for row in 0..self.cropped_height {
            let mut at = row * width;
            for run in self.runs(row)? {
                match run? {
                    crate::Run::Fill { color, len } => {
                        destination[at..at + len].fill(color);
                        at += len;
                    }
                    crate::Run::Literal(bytes) => {
                        destination[at..at + bytes.len()].copy_from_slice(bytes);
                        at += bytes.len();
                    }
                }
            }
            if at != (row + 1) * width {
                return Err(Error::ShortRow {
                    row,
                    decoded: at - row * width,
                    expected: width,
                });
            }
        }
        Ok(())
    }

    /// Validate every scanline without allocating a destination.
    ///
    /// # Errors
    ///
    /// Returns the first malformed row or packet.
    pub fn validate(self) -> Result<()> {
        for row in 0..self.cropped_height {
            let mut decoded = 0usize;
            for run in self.runs(row)? {
                decoded += run?.len();
            }
            if decoded != self.cropped_width {
                return Err(Error::ShortRow {
                    row,
                    decoded,
                    expected: self.cropped_width,
                });
            }
        }
        Ok(())
    }
}

fn take<'a>(data: &'a [u8], at: usize, len: usize, what: &'static str) -> Result<&'a [u8]> {
    let end = at.checked_add(len).ok_or(Error::SizeOverflow { what })?;
    data.get(at..end).ok_or(Error::Truncated {
        what,
        at,
        needed: len,
        available: data.len().saturating_sub(at),
    })
}

pub(crate) fn word(data: &[u8], at: usize) -> u32 {
    u32::from_le_bytes([data[at], data[at + 1], data[at + 2], data[at + 3]])
}

fn signed_word(data: &[u8], at: usize) -> i32 {
    i32::from_le_bytes([data[at], data[at + 1], data[at + 2], data[at + 3]])
}

fn size_field(data: &[u8], at: usize, what: &'static str) -> Result<usize> {
    let value = word(data, at);
    usize::try_from(value).map_err(|_| Error::BadDimension { what, value })
}

fn dimension(data: &[u8], at: usize, what: &'static str) -> Result<usize> {
    let value = word(data, at);
    if value == 0 || value > i32::MAX as u32 {
        return Err(Error::BadDimension { what, value });
    }
    usize::try_from(value).map_err(|_| Error::BadDimension { what, value })
}
