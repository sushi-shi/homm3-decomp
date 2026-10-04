//! Native `objects.txt` loading, shared image masks, and prepared prototypes.
//!
//! Object coordinates grow west and north from the bottom-right anchor P.
//! Serialized mask bits run the other way, with bit 47 at P:
//! ```text
//!    0  1 ..  6  7
//!    8  9 .. 14 15
//!   ..
//!   40 41 .. 46  P   (bit 47)
//! ```

use crate::{
    behavior::Behavior,
    domain::Terrain,
    object::ObjectKind,
    parse,
    placement_rules::{PlacementRuleId, PlacementRules},
    raw,
    request::MapVersion,
    rng::RetailRng,
    traits::CreatureId,
};
use homm3_resource::{Mask, Text};
use std::{borrow::Cow, collections::TryReserveError, error::Error, fmt, num::NonZeroU32};

const MASK_BITS: u64 = (1 << raw::OBJECT_MASK_CELLS) - 1;
const MASK_BYTES: usize = 2 + 2 * (raw::OBJECT_MASK_CELLS as usize / u8::BITS as usize);
const KINDS: usize = raw::ADVENTURE_OBJECT_TRAIT_COUNT as usize;

/// A coordinate inside the fixed object mask, measured west/north from its anchor.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MaskCell {
    x: u8,
    y: u8,
}
impl MaskCell {
    /// Parse a fixed-frame coordinate independently of the image's dimensions.
    #[must_use]
    pub const fn parse(x: u8, y: u8) -> Option<Self> {
        if (x as u32) < raw::OBJECT_MASK_WIDTH && (y as u32) < raw::OBJECT_MASK_HEIGHT {
            Some(Self { x, y })
        } else {
            None
        }
    }
    /// Westward offset from the object's anchor.
    #[must_use]
    pub const fn x(self) -> u8 {
        self.x
    }
    /// Northward offset from the object's anchor.
    #[must_use]
    pub const fn y(self) -> u8 {
        self.y
    }
    const fn bit(self) -> u32 {
        raw::OBJECT_MASK_CELLS - 1 - self.y as u32 * raw::OBJECT_MASK_WIDTH - self.x as u32
    }
}

/// Nonempty image dimensions fitting the fixed mask frame.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct FootprintSize {
    width: u8,
    height: u8,
}
impl FootprintSize {
    /// Validate dimensions before footprint iteration or outline traversal.
    #[must_use]
    pub const fn parse(width: u8, height: u8) -> Option<Self> {
        if width > 0
            && height > 0
            && (width as u32) <= raw::OBJECT_MASK_WIDTH
            && (height as u32) <= raw::OBJECT_MASK_HEIGHT
        {
            Some(Self { width, height })
        } else {
            None
        }
    }
    /// Width in cells.
    #[must_use]
    pub const fn width(self) -> u8 {
        self.width
    }
    /// Height in cells.
    #[must_use]
    pub const fn height(self) -> u8 {
        self.height
    }
}

/// A known unsafe retail footprint or creature index, deferred until use.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PrototypeFault {
    /// Signed dimensions from the mask do not fit the fixed nonempty frame.
    Dimensions {
        /// Signed retail width.
        width: i8,
        /// Signed retail height.
        height: i8,
    },
    /// Outline tracing has no occupied cell in its starting row.
    EmptyBottomRow,
    /// Monster subtype cannot index the creature table.
    CreatureSubtype(i32),
}
impl fmt::Display for PrototypeFault {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Dimensions { width, height } => write!(
                f,
                "prototype dimensions {width}x{height} do not fit the mask frame"
            ),
            Self::EmptyBottomRow => {
                f.write_str("prototype outline has no occupied bottom-row cell")
            }
            Self::CreatureSubtype(subtype) => write!(
                f,
                "prototype creature subtype {subtype} is outside the creature table"
            ),
        }
    }
}
impl Error for PrototypeFault {}

/// Parsed mask data, copied once per distinct image name (14 input bytes).
#[derive(Clone, Copy, Debug)]
pub struct ImageMask {
    size: Result<FootprintSize, PrototypeFault>,
    draw: u64,
    shadow: u64,
}
impl ImageMask {
    /// Parse the engine's mask header while recording unusable signed dimensions.
    /// Trailing resource bytes are ignored, matching the C++ reader's fixed reads.
    ///
    /// # Errors
    /// Reports a truncated mask header.
    pub fn parse(bytes: &[u8]) -> Result<Self, homm3_resource::Error> {
        let mask = Mask::parse(bytes.get(..MASK_BYTES).unwrap_or(bytes))?;
        let size =
            FootprintSize::parse(mask.width(), mask.height()).ok_or(PrototypeFault::Dimensions {
                width: i8::from_ne_bytes([mask.width()]),
                height: i8::from_ne_bytes([mask.height()]),
            });
        let mut draw = 0;
        let mut shadow = 0;
        for bit in 0..raw::OBJECT_MASK_CELLS as usize {
            if mask.draw(bit) == Some(true) {
                draw |= 1 << bit;
            }
            if mask.shadow(bit) == Some(true) {
                shadow |= 1 << bit;
            }
        }
        Ok(Self { size, draw, shadow })
    }
    fn missing() -> Self {
        Self {
            size: Err(PrototypeFault::Dimensions {
                width: 0,
                height: 0,
            }),
            draw: 0,
            shadow: 0,
        }
    }
    /// Parsed dimensions or the explicit retail fault they would cause.
    ///
    /// # Errors
    /// Returns the signed dimensions when they do not fit the mask frame.
    pub const fn size(self) -> Result<FootprintSize, PrototypeFault> {
        self.size
    }
    /// Whether the image draws this cell.
    #[must_use]
    pub const fn draws(self, cell: MaskCell) -> bool {
        self.draw & (1 << cell.bit()) != 0
    }
    /// Whether the image casts a shadow here.
    #[must_use]
    pub const fn shadows(self, cell: MaskCell) -> bool {
        self.shadow & (1 << cell.bit()) != 0
    }
}

#[derive(Debug)]
struct Image<'a> {
    name: Cow<'a, [u8]>,
    mask: ImageMask,
}

/// Parsed object row; all mask bits and object identities are in their domains.
#[derive(Debug)]
pub struct Prototype {
    source_row: usize,
    image: usize,
    kind: ObjectKind,
    subtype: i32,
    category: i32,
    underlay: bool,
    passable: u64,
    trigger: u64,
    terrain: u16,
    recommended: u16,
    entrance: Option<MaskCell>,
}
impl Prototype {
    /// Original zero-based data-row ordinal, excluding the count line.
    #[must_use]
    pub const fn source_row(&self) -> usize {
        self.source_row
    }
    /// Actual object type, before family aliasing.
    #[must_use]
    pub const fn kind(&self) -> ObjectKind {
        self.kind
    }
    /// Family-dependent subtype; monster eligibility checks its creature bound.
    #[must_use]
    pub const fn subtype(&self) -> i32 {
        self.subtype
    }
    /// Editor slot identity, also used by RMG selection and H3M serialization.
    #[must_use]
    pub const fn category(&self) -> i32 {
        self.category
    }
    /// Underlay metadata, normalized from the source integer.
    #[must_use]
    pub const fn underlay(&self) -> bool {
        self.underlay
    }
    /// First trigger cell in anchor-relative row/column order; no sentinel.
    #[must_use]
    pub const fn entrance(&self) -> Option<MaskCell> {
        self.entrance
    }
    /// Passability includes every cell outside the image's draw mask.
    #[must_use]
    pub const fn is_passable(&self, cell: MaskCell) -> bool {
        self.passable & (1 << cell.bit()) != 0
    }
    /// Only blocked cells can be triggers.
    #[must_use]
    pub const fn is_trigger(&self, cell: MaskCell) -> bool {
        self.trigger & (1 << cell.bit()) != 0
    }
    /// Cells occupied by placement or outline logic.
    #[must_use]
    pub const fn occupies(&self, cell: MaskCell) -> bool {
        !self.is_passable(cell) || self.is_trigger(cell)
    }
    /// Legal terrain mask from the source row.
    #[must_use]
    pub const fn allows_terrain(&self, terrain: Terrain) -> bool {
        self.terrain & (1 << terrain.index()) != 0
    }
    /// Recommended mask, set after the legal mask without intersecting the two.
    #[must_use]
    pub const fn recommends(&self, terrain: Terrain) -> bool {
        self.recommended & (1 << terrain.index()) != 0
    }
    /// Original selector policy, without consuming randomness.
    #[must_use]
    pub fn selectable(&self, subtype: i32, terrain: Terrain) -> bool {
        self.subtype == subtype
            && if matches!(
                u32::try_from(self.category),
                Ok(raw::OBJECT_SLOT_CATEGORY_4 | raw::OBJECT_SLOT_CATEGORY_5)
            ) {
                terrain != Terrain::Water
            } else {
                self.recommends(terrain)
            }
    }
    fn available(&self, version: MapVersion) -> bool {
        let kind = self.kind.index();
        if version != MapVersion::ShadowOfDeath && kind >= raw::CLOVER_FIELD_2 as usize {
            return false;
        }
        if version == MapVersion::Restoration && kind >= raw::MAX_EVENT_TYPE as usize {
            return false;
        }
        !(version != MapVersion::ShadowOfDeath
            && [
                raw::LITH_TWOWAY,
                raw::LITH_ONEWAY_ENTRANCE,
                raw::LITH_ONEWAY_EXIT,
            ]
            .iter()
            .any(|&monolith| monolith as usize == kind)
            && i64::from(self.subtype) >= i64::from(raw::RMG_PRE_SOD_MONOLITH_SUBTYPE_COUNT))
    }
    fn footprint(&self, mask: ImageMask) -> Result<FootprintSize, PrototypeFault> {
        let size = mask.size?;
        if self.kind.index() == raw::TERRAIN_HOLE as usize {
            return Ok(size);
        }
        if !(0..size.width).any(|x| self.occupies(MaskCell { x, y: 0 })) {
            return Err(PrototypeFault::EmptyBottomRow);
        }
        if self.kind.family().index() == raw::MONSTER as usize
            && CreatureId::parse(self.subtype).is_none()
        {
            return Err(PrototypeFault::CreatureSubtype(self.subtype));
        }
        Ok(size)
    }
}

/// Resource or parsing failure before an object catalog can be prepared.
#[derive(Debug)]
pub enum PrototypeLoadError<E> {
    /// Text resource encoding is malformed.
    Text(homm3_resource::Error),
    /// A row cannot be interpreted without relying on failed stream extraction.
    Field {
        /// Zero-based resource row (zero is the count).
        row: usize,
        /// Failed field.
        field: &'static str,
    },
    /// Mask provider failed (missing resources use `Ok(None)` instead).
    Resource(E),
    /// Storage reservation failed.
    Allocation(TryReserveError),
}
impl<E: fmt::Display> fmt::Display for PrototypeLoadError<E> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Text(error) => error.fmt(f),
            Self::Field { row, field } => write!(f, "objects.txt row {row}: unsupported {field}"),
            Self::Resource(error) => error.fmt(f),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl<E: Error + 'static> Error for PrototypeLoadError<E> {}
impl<E> From<TryReserveError> for PrototypeLoadError<E> {
    fn from(error: TryReserveError) -> Self {
        Self::Allocation(error)
    }
}

/// Object rows and interned image metadata, independent of mode and map format.
#[derive(Debug)]
pub struct PrototypeSource<'a> {
    images: Vec<Image<'a>>,
    rows: Vec<Prototype>,
}
impl<'a> PrototypeSource<'a> {
    /// Read every declared row and load each distinct image's mask once. The
    /// provider receives the derived `.msk` name, then `default.msk` on absence.
    /// Names borrow the text unless quote normalization requires owned bytes.
    ///
    /// # Errors
    /// Reports malformed rows, mask-provider errors or allocation failure.
    pub fn parse<E>(
        bytes: &'a [u8],
        mut mask: impl FnMut(&[u8]) -> Result<Option<ImageMask>, E>,
    ) -> Result<Self, PrototypeLoadError<E>> {
        let text = Text::parse(bytes).map_err(PrototypeLoadError::Text)?;
        let mut lines = text.lines();
        let count = lines
            .next()
            .and_then(|field| parse::integer(field.decoded()))
            .and_then(|count| usize::try_from(count).ok())
            .filter(|&count| count < text.len())
            .ok_or(PrototypeLoadError::Field {
                row: 0,
                field: "row count",
            })?;
        let mut source = Self {
            images: Vec::new(),
            rows: Vec::new(),
        };
        source.rows.try_reserve(count)?;
        // Sorted indices intern names without allocating tree nodes or copying strings.
        let mut names = Vec::<usize>::new();
        let mut mask_name = Vec::new();
        for (index, line) in lines.take(count).enumerate() {
            let mut row = RowReader::new(line.encoded(), index + 1);
            let name = row.name()?;
            let passable = row.bits(raw::OBJECT_MASK_CELLS, "passable mask")?;
            let trigger = row.bits(raw::OBJECT_MASK_CELLS, "trigger mask")?;
            let terrain = u16::try_from(row.bits(raw::eTerrainWater as u32 + 1, "terrain mask")?)
                .map_err(|_| row.error("terrain mask"))?;
            let recommended =
                u16::try_from(row.bits(raw::eTerrainWater as u32 + 1, "recommended mask")?)
                    .map_err(|_| row.error("recommended mask"))?;
            let kind = row.integer("object type")?;
            let subtype = row.integer("subtype")?;
            let category = row.integer("slot category")?;
            let underlay = row.integer("underlay")? != 0;
            let image = match names
                .binary_search_by(|&image| source.images[image].name.as_ref().cmp(name.as_ref()))
            {
                Ok(found) => names[found],
                Err(at) => {
                    mask_name.clear();
                    let stem = name
                        .iter()
                        .rposition(|&byte| byte == b'.')
                        .unwrap_or(name.len());
                    mask_name.try_reserve(stem.saturating_add(b".msk".len()))?;
                    mask_name.extend_from_slice(&name[..stem]);
                    mask_name.extend_from_slice(b".msk");
                    let loaded = match mask(&mask_name).map_err(PrototypeLoadError::Resource)? {
                        Some(mask) => mask,
                        None => mask(b"default.msk")
                            .map_err(PrototypeLoadError::Resource)?
                            .unwrap_or_else(ImageMask::missing),
                    };
                    let image = source.images.len();
                    source.images.try_reserve(1)?;
                    names.try_reserve(1)?;
                    source.images.push(Image { name, mask: loaded });
                    names.insert(at, image);
                    image
                }
            };
            // C++ loads masks before rejecting an out-of-domain object identity.
            let Some(kind) = ObjectKind::parse(kind) else {
                continue;
            };
            let passable = passable | (!source.images[image].mask.draw & MASK_BITS);
            let trigger = trigger & !passable;
            let entrance = (0..raw::OBJECT_MASK_HEIGHT)
                .flat_map(|y| (0..raw::OBJECT_MASK_WIDTH).map(move |x| (x, y)))
                .find_map(|(x, y)| {
                    let cell = MaskCell::parse(u8::try_from(x).ok()?, u8::try_from(y).ok()?)?;
                    (trigger & (1 << cell.bit()) != 0).then_some(cell)
                });
            source.rows.push(Prototype {
                source_row: index,
                image,
                kind,
                subtype,
                category,
                underlay,
                passable,
                trigger,
                terrain,
                recommended,
                entrance,
            });
        }
        Ok(source)
    }
    /// Rows with bounded object identities, in original order.
    #[must_use]
    pub fn rows(&self) -> &[Prototype] {
        &self.rows
    }
    /// Number of distinct image names loaded (case-sensitive, like the source registry).
    #[must_use]
    pub fn image_count(&self) -> usize {
        self.images.len()
    }
    /// Prepare format/mode filtering, monster exchange order, and rule bindings.
    ///
    /// # Errors
    /// Reports retail's empty-monster sort fault or allocation failure.
    pub fn prepare<'s>(
        &'s self,
        rules: &'s PlacementRules,
        version: MapVersion,
        behavior: Behavior,
    ) -> Result<PrototypeCatalog<'s>, CatalogError> {
        let mut entries = Vec::new();
        let mut offsets = [0; KINDS + 1];
        for prototype in &self.rows {
            if !prototype.available(version) {
                continue;
            }
            let image = &self.images[prototype.image];
            let footprint = prototype.footprint(image.mask);
            if behavior.is_hotfix() && footprint.is_err() {
                continue;
            }
            entries.try_reserve(1)?;
            entries.push(PreparedPrototype {
                prototype,
                image,
                footprint,
                preferred: None,
                rule: None,
            });
            offsets[prototype.kind.family().index() + 1] += 1;
        }
        for kind in 1..offsets.len() {
            offsets[kind] += offsets[kind - 1];
        }
        // The original row breaks ties, preserving per-family insertion order without sort scratch storage.
        entries.sort_unstable_by_key(|entry| {
            (
                entry.prototype.kind.family().index(),
                entry.prototype.source_row,
            )
        });
        let monster = raw::MONSTER as usize;
        let monsters = &mut entries[offsets[monster]..offsets[monster + 1]];
        if monsters.is_empty() && !behavior.is_hotfix() {
            return Err(CatalogError::RetailEmptyMonsters);
        }
        // Exchange sort is intentionally not stable for equal subtypes.
        for first in 0..monsters.len().saturating_sub(1) {
            for second in first + 1..monsters.len() {
                if monsters[first].prototype.subtype > monsters[second].prototype.subtype {
                    monsters.swap(first, second);
                }
            }
        }
        for entry in &mut entries {
            entry.preferred = (raw::eTerrainDirt..raw::eTerrainRock)
                .filter_map(Terrain::parse)
                .find(|&terrain| entry.prototype.recommends(terrain));
            entry.rule = entry.preferred.and_then(|terrain| {
                rules.find(
                    entry.prototype.kind.family().family(),
                    entry.prototype.subtype,
                    terrain,
                )
            });
        }
        Ok(PrototypeCatalog { entries, offsets })
    }
}

/// Prepared metadata retains invalid retail candidates without permitting unsafe use.
#[derive(Debug)]
pub struct PreparedPrototype<'a> {
    prototype: &'a Prototype,
    image: &'a Image<'a>,
    footprint: Result<FootprintSize, PrototypeFault>,
    preferred: Option<Terrain>,
    rule: Option<PlacementRuleId>,
}
impl PreparedPrototype<'_> {
    /// Source row and masks.
    #[must_use]
    pub const fn prototype(&self) -> &Prototype {
        self.prototype
    }
    /// Interned image name; no clone is needed for serialization.
    #[must_use]
    pub fn image_name(&self) -> &[u8] {
        &self.image.name
    }
    /// Image metadata shared by prototypes with identical names.
    #[must_use]
    pub const fn image_mask(&self) -> ImageMask {
        self.image.mask
    }
    /// Footprint checked before placement or outline traversal.
    ///
    /// # Errors
    /// Returns a deferred retail footprint/creature fault.
    pub const fn footprint(&self) -> Result<FootprintSize, PrototypeFault> {
        self.footprint
    }
    /// First recommended dirt-through-water terrain, or no preference.
    #[must_use]
    pub const fn preferred(&self) -> Option<Terrain> {
        self.preferred
    }
    /// Last matching placement rule.
    #[must_use]
    pub const fn rule(&self) -> Option<PlacementRuleId> {
        self.rule
    }
}

/// Filtering/sorting failure before map generation.
#[derive(Debug)]
pub enum CatalogError {
    /// Retail underflows `monster_count - 1` and accesses an absent prototype.
    RetailEmptyMonsters,
    /// Storage reservation failed.
    Allocation(TryReserveError),
}
impl fmt::Display for CatalogError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::RetailEmptyMonsters => f.write_str("retail sorts an empty monster list"),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for CatalogError {}
impl From<TryReserveError> for CatalogError {
    fn from(error: TryReserveError) -> Self {
        Self::Allocation(error)
    }
}

/// Immutable family buckets in one vector, with no per-family allocations.
#[derive(Debug)]
pub struct PrototypeCatalog<'a> {
    entries: Vec<PreparedPrototype<'a>>,
    offsets: [usize; KINDS + 1],
}

/// Identity of a prepared prototype, stable for the lifetime of its catalog.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct PrototypeId(usize);
impl PrototypeId {
    /// Dense catalog ordinal, distinct from the source row or H3M prototype slot.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0
    }
}
impl PrototypeCatalog<'_> {
    /// All prepared prototypes in family/selection order.
    #[must_use]
    pub fn entries(&self) -> &[PreparedPrototype<'_>] {
        &self.entries
    }
    /// One family, preserving source ordering and the monster exchange sort.
    #[must_use]
    pub fn family(&self, family: ObjectKind) -> &[PreparedPrototype<'_>] {
        &self.entries[self.offsets[family.index()]..self.offsets[family.index() + 1]]
    }
    /// Resolve a catalog identity without exposing an unchecked index.
    #[must_use]
    pub fn get(&self, id: PrototypeId) -> Option<&PreparedPrototype<'_>> {
        self.entries.get(id.0)
    }
    /// Direct family indexing used for towns, portals and reserved output slots.
    #[must_use]
    pub fn at(&self, family: ObjectKind, index: usize) -> Option<PrototypeId> {
        self.family(family)
            .get(index)
            .map(|_| PrototypeId(self.offsets[family.index()] + index))
    }
    /// Choose a matching prototype in source order using one draw, even for a
    /// singleton. An empty candidate set consumes none. Counting and selecting
    /// use borrowed iterators, with no temporary candidate allocation.
    #[expect(
        clippy::missing_panics_doc,
        reason = "parsed source row count is bounded by i32 and filtering cannot grow it"
    )]
    pub fn choose(
        &self,
        family: ObjectKind,
        subtype: i32,
        terrain: Terrain,
        rng: &mut RetailRng,
    ) -> Option<PrototypeId> {
        let mut candidates = self
            .family(family)
            .iter()
            .enumerate()
            .filter(|(_, entry)| entry.prototype.selectable(subtype, terrain));
        let count = NonZeroU32::new(
            u32::try_from(candidates.clone().count()).expect("source row count fits i32"),
        )?;
        candidates
            .nth(rng.below(count) as usize)
            .map(|(index, _)| PrototypeId(self.offsets[family.index()] + index))
    }
}

struct RowReader<'a> {
    bytes: &'a [u8],
    position: usize,
    row: usize,
}
impl<'a> RowReader<'a> {
    fn new(bytes: &'a [u8], row: usize) -> Self {
        Self {
            bytes: bytes.split(|&byte| byte == 0).next().unwrap_or_default(),
            position: 0,
            row,
        }
    }
    fn whitespace(&mut self) {
        while self
            .bytes
            .get(self.position)
            .is_some_and(u8::is_ascii_whitespace)
        {
            self.position += 1;
        }
    }
    fn error<E>(&self, field: &'static str) -> PrototypeLoadError<E> {
        PrototypeLoadError::Field {
            row: self.row,
            field,
        }
    }
    fn name<E>(&mut self) -> Result<Cow<'a, [u8]>, PrototypeLoadError<E>> {
        self.whitespace();
        let start = self.position;
        while self
            .bytes
            .get(self.position)
            .is_some_and(|byte| !byte.is_ascii_whitespace())
        {
            self.position += 1;
        }
        if start == self.position {
            return Err(self.error("image name"));
        }
        let name = &self.bytes[start..self.position];
        Ok(if name.windows(2).any(|pair| pair == b"\"\"") {
            Cow::Owned(
                name.iter()
                    .copied()
                    .enumerate()
                    .filter_map(|(index, byte)| {
                        (byte != b'"' || index == 0 || name[index - 1] != b'"').then_some(byte)
                    })
                    .collect(),
            )
        } else {
            Cow::Borrowed(name)
        })
    }
    fn bits<E>(&mut self, count: u32, field: &'static str) -> Result<u64, PrototypeLoadError<E>> {
        self.whitespace();
        let start = self.position;
        let mut value = 0;
        for _ in 0..count {
            match self.bytes.get(self.position) {
                Some(b'0') => value <<= 1,
                Some(b'1') => value = value << 1 | 1,
                _ => break,
            }
            self.position += 1;
        }
        if start == self.position {
            return Err(self.error(field));
        }
        Ok(value)
    }
    fn integer<E>(&mut self, field: &'static str) -> Result<i32, PrototypeLoadError<E>> {
        self.whitespace();
        let start = self.position;
        if matches!(self.bytes.get(self.position), Some(b'+' | b'-')) {
            self.position += 1;
        }
        let digits = self.position;
        while self
            .bytes
            .get(self.position)
            .is_some_and(u8::is_ascii_digit)
        {
            self.position += 1;
        }
        if digits == self.position {
            return Err(self.error(field));
        }
        parse::integer(self.bytes[start..self.position].iter().copied())
            .ok_or_else(|| self.error(field))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::behavior::RetailProfile;
    use std::convert::Infallible;

    fn mask() -> ImageMask {
        ImageMask {
            size: Ok(FootprintSize::parse(2, 2).unwrap()),
            draw: (1 << 47) | (1 << 46) | (1 << 39),
            shadow: 1 << 46,
        }
    }
    fn row(name: &str, kind: u32, subtype: i32) -> String {
        format!(
            "{name} {:048b} {:048b} {:09b} {:09b} {kind} {subtype} 0 0",
            0_u64,
            1_u64 << 47,
            1,
            1
        )
    }
    fn table(rows: &[String]) -> Vec<u8> {
        format!("{}\r\n{}\r\n", rows.len(), rows.join("\r\n")).into_bytes()
    }
    fn rules() -> PlacementRules {
        PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", Behavior::Hotfix).unwrap()
    }
    fn monster() -> ObjectKind {
        ObjectKind::parse(i32::try_from(raw::MONSTER).unwrap()).unwrap()
    }

    #[test]
    fn repeated_image_names_are_borrowed_and_loaded_once_and_monster_exchange_is_not_stable() {
        let bytes = table(&[
            row("shared.def", raw::MONSTER, 2),
            row("shared.def", raw::MONSTER, 2),
            row("shared.def", raw::MONSTER, 1),
        ]);
        let mut calls = 0;
        let source = PrototypeSource::parse(&bytes, |name| {
            calls += 1;
            assert_eq!(name, b"shared.msk");
            Ok::<_, Infallible>(Some(mask()))
        })
        .unwrap();
        assert_eq!(calls, 1);
        assert_eq!(source.image_count(), 1);
        assert!(matches!(source.images[0].name, Cow::Borrowed(_)));
        let rules = rules();
        let catalog = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        let rows: Vec<_> = catalog
            .family(monster())
            .iter()
            .map(|entry| entry.prototype().source_row())
            .collect();
        assert_eq!(rows, [2, 1, 0]);
        assert!(catalog
            .entries()
            .iter()
            .all(|entry| std::ptr::eq(entry.image, &raw const source.images[0])));
    }

    #[test]
    fn mask_setters_clip_triggers_to_blocking_art_and_keep_recommendations_independent() {
        let occupied = (1_u64 << 47) | (1 << 46) | (1 << 39);
        let bytes = table(&[format!(
            "art.def {:048b} {occupied:048b} {:09b} {:09b} {} 0 {} 2",
            1_u64 << 47,
            1,
            1 << raw::eTerrainGrass,
            raw::MONSTER,
            raw::OBJECT_SLOT_CATEGORY_4
        )]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let prototype = &source.rows()[0];
        assert_eq!(prototype.entrance(), MaskCell::parse(1, 0));
        assert!(prototype.is_passable(MaskCell::parse(0, 0).unwrap()));
        assert!(prototype.is_passable(MaskCell::parse(7, 5).unwrap()));
        assert!(!prototype.is_trigger(MaskCell::parse(0, 0).unwrap()));
        assert!(prototype.is_trigger(MaskCell::parse(1, 0).unwrap()));
        assert!(!prototype.allows_terrain(Terrain::Grass));
        assert!(prototype.recommends(Terrain::Grass));
        assert!(prototype.selectable(0, Terrain::Snow));
        assert!(!prototype.selectable(0, Terrain::Water));
        assert!(prototype.underlay());
    }

    #[test]
    fn missing_mask_falls_back_once_per_image_and_signed_dimensions_are_deferred() {
        let bytes = table(&[
            row("missing.def", raw::MONSTER, 0),
            row("missing.def", raw::MONSTER, 1),
        ]);
        let mut calls = Vec::new();
        let mut data = [0; 14];
        data[0] = 255;
        data[1] = 6;
        let invalid = ImageMask::parse(&data).unwrap();
        let source = PrototypeSource::parse(&bytes, |name| {
            calls.push(name.to_vec());
            Ok::<_, Infallible>((name == b"default.msk").then_some(invalid))
        })
        .unwrap();
        assert_eq!(calls, [b"missing.msk".to_vec(), b"default.msk".to_vec()]);
        let rules = rules();
        let retail = source
            .prepare(
                &rules,
                MapVersion::ShadowOfDeath,
                Behavior::Retail(RetailProfile::default()),
            )
            .unwrap();
        assert_eq!(retail.entries().len(), 2);
        assert_eq!(
            retail.entries()[0].footprint(),
            Err(PrototypeFault::Dimensions {
                width: -1,
                height: 6
            })
        );
        assert!(source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap()
            .entries()
            .is_empty());
    }

    #[test]
    fn hotfix_keeps_reserved_holes_and_filters_unusable_creatures() {
        let mut hole = row("hole.def", raw::TERRAIN_HOLE, 0);
        hole = hole.replacen(&format!("{:048b}", 0), &format!("{MASK_BITS:048b}"), 1);
        let bytes = table(&[hole, row("monster.def", raw::MONSTER, -1)]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let rules = rules();
        let hotfix = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        assert_eq!(hotfix.entries().len(), 1);
        assert_eq!(
            hotfix.entries()[0].prototype().kind().index(),
            raw::TERRAIN_HOLE as usize
        );
        assert!(hotfix.entries()[0].footprint().is_ok());
        let retail = source
            .prepare(
                &rules,
                MapVersion::ShadowOfDeath,
                Behavior::Retail(RetailProfile::default()),
            )
            .unwrap();
        assert_eq!(
            retail.family(monster())[0].footprint(),
            Err(PrototypeFault::CreatureSubtype(-1))
        );
    }

    #[test]
    fn malformed_streams_fault_before_exposing_a_catalog_and_empty_monsters_are_mode_specific() {
        for bytes in [
            b"-1\r\n".as_slice(),
            b"2\r\nshort\r\n",
            b"1\r\nname.def xyz\r\n",
        ] {
            assert!(PrototypeSource::parse(bytes, |_| Ok::<_, Infallible>(Some(mask()))).is_err());
        }
        let bytes = table(&[row("town.def", raw::TOWN, 0)]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let rules = rules();
        assert!(matches!(
            source.prepare(
                &rules,
                MapVersion::ShadowOfDeath,
                Behavior::Retail(RetailProfile::default())
            ),
            Err(CatalogError::RetailEmptyMonsters)
        ));
        assert_eq!(
            source
                .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
                .unwrap()
                .entries()
                .len(),
            1
        );
    }

    #[test]
    fn selection_counts_borrowed_candidates_and_preserves_singleton_draws() {
        let bytes = table(&[row("monster.def", raw::MONSTER, 2)]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let rules = rules();
        let catalog = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        let mut rng = RetailRng::new(1);
        let initial = rng.checkpoint();
        assert!(catalog
            .choose(monster(), 3, Terrain::Dirt, &mut rng)
            .is_none());
        assert_eq!(rng.checkpoint(), initial);
        let selected = catalog
            .choose(monster(), 2, Terrain::Dirt, &mut rng)
            .unwrap();
        assert_eq!(Some(selected), catalog.at(monster(), 0));
        assert_eq!(catalog.get(selected).unwrap().prototype().subtype(), 2);
        assert_eq!(rng.checkpoint().draws, initial.draws + 1);
        assert!(catalog.at(monster(), 1).is_none());
    }

    #[test]
    fn mask_header_ignores_trailing_bytes_but_rejects_truncation() {
        let mut bytes = [0; MASK_BYTES + 3];
        bytes[0] = 1;
        bytes[1] = 1;
        bytes[MASK_BYTES..].fill(255);
        assert_eq!(
            ImageMask::parse(&bytes).unwrap().size(),
            Ok(FootprintSize::parse(1, 1).unwrap())
        );
        assert!(ImageMask::parse(&bytes[..MASK_BYTES - 1]).is_err());
    }
}
