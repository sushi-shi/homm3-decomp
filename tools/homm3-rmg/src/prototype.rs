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
    identity::OwnerId,
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

mod readiness;
pub use readiness::{GenerationPrototypes, RequiredPrototypeError};
mod geometry;
pub(crate) use geometry::{advance_outline, outline_probe};
pub use geometry::{OutlineError, OutlineWorkspace, OverlapPriorities};
mod guard;
pub use guard::{GuardError, GuardFactions, GuardStack};

const MASK_BITS: u64 = (1 << raw::OBJECT_MASK_CELLS) - 1;
const MASK_BYTES: usize = 2 + 2 * (raw::OBJECT_MASK_CELLS as usize / u8::BITS as usize);
const KINDS: usize = raw::ADVENTURE_OBJECT_TRAIT_COUNT as usize;

/// A coordinate inside the fixed object mask, measured west/north from its anchor.
///
/// For a 3x2 image, `P` is mask coordinate `(0, 0)` at the bottom-right anchor.
/// North is up; mask coordinates increase in the opposite directions to world
/// coordinates. A mask cell `(x, y)` maps to world position `P - (x, y)`.
///
/// ```text
///   (2,1) (1,1) (0,1)
///   (2,0) (1,0)   P
/// ```
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
    /// Cells inside these admitted dimensions, in native row/column order.
    pub fn cells(self) -> impl Iterator<Item = MaskCell> {
        (0..self.height).flat_map(move |y| (0..self.width).map(move |x| MaskCell { x, y }))
    }
}

/// Invalid image dimensions, creature identity, or hotfix admission geometry.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PrototypeFault {
    /// Signed dimensions from the mask do not fit the fixed nonempty frame.
    Dimensions {
        /// Signed retail width.
        width: i8,
        /// Signed retail height.
        height: i8,
    },
    /// Hotfix admission requires an occupied bottom row; retail permits an empty one.
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
                f.write_str("prototype fails hotfix admission: no occupied bottom-row cell")
            }
            Self::CreatureSubtype(subtype) => write!(
                f,
                "prototype creature subtype {subtype} is outside the creature table"
            ),
        }
    }
}
impl Error for PrototypeFault {}

// Keep the recorded dimensions even when they cannot describe a footprint.
// Some native operations read just one signed extent without traversing cells.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum ImageDimensions {
    Footprint(FootprintSize),
    Unusable { width: i8, height: i8 },
}

/// Parsed mask data, copied once per distinct image name (14 input bytes).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ImageMask {
    size: ImageDimensions,
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
        let size = FootprintSize::parse(mask.width(), mask.height()).map_or(
            ImageDimensions::Unusable {
                width: i8::from_ne_bytes([mask.width()]),
                height: i8::from_ne_bytes([mask.height()]),
            },
            ImageDimensions::Footprint,
        );
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
            size: ImageDimensions::Unusable {
                width: 0,
                height: 0,
            },
            draw: 0,
            shadow: 0,
        }
    }
    /// Parsed dimensions or the explicit retail fault they would cause.
    ///
    /// # Errors
    /// Returns the signed dimensions when they do not fit the mask frame.
    pub const fn size(self) -> Result<FootprintSize, PrototypeFault> {
        match self.size {
            ImageDimensions::Footprint(size) => Ok(size),
            ImageDimensions::Unusable { width, height } => {
                Err(PrototypeFault::Dimensions { width, height })
            }
        }
    }
    /// Recorded signed width, without requiring a usable footprint or height.
    /// Mine resource strips use this from an unselected prototype in retail.
    #[must_use]
    pub const fn signed_width(self) -> i32 {
        match self.size {
            ImageDimensions::Footprint(size) => size.width() as i32,
            ImageDimensions::Unusable { width, .. } => width as i32,
        }
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
    ///
    /// Search ordinals across the full 8x6 mask, north up and anchor `P` at 0:
    /// bottom row first, east to west, then the next row north. These ordinals
    /// are the reverse of the serialized mask bit numbers.
    ///
    /// ```text
    ///   47 46 45 44 43 42 41 40
    ///               ...
    ///   15 14 13 12 11 10  9  8
    ///    7  6  5  4  3  2  1  P
    /// ```
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
    fn hotfix_admission(&self, mask: ImageMask) -> Result<FootprintSize, PrototypeFault> {
        let size = mask.size()?;
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
    /// Reports retail's empty-monster sort fault, allocation failure or exhausted ownership tags.
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
            let hotfix_admission = prototype.hotfix_admission(image.mask);
            if behavior.is_hotfix() && hotfix_admission.is_err() {
                continue;
            }
            entries.try_reserve(1)?;
            entries.push(PreparedPrototype {
                prototype,
                image,
                hotfix_admission,
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
        Ok(PrototypeCatalog {
            owner: OwnerId::new().ok_or(CatalogError::IdentityExhausted)?,
            entries,
            offsets,
            version,
            behavior,
        })
    }
}

/// Prepared metadata retains invalid retail candidates without permitting unsafe use.
#[derive(Debug)]
pub struct PreparedPrototype<'a> {
    prototype: &'a Prototype,
    image: &'a Image<'a>,
    hotfix_admission: Result<FootprintSize, PrototypeFault>,
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
    /// Footprint satisfying the hotfix catalog admission policy.
    /// Retail operations that only read dimensions use [`ImageMask::size`];
    /// an empty bottom row does not prevent scoring or placing decorations.
    ///
    /// # Errors
    /// Returns the reason a retained retail prototype fails hotfix admission.
    pub const fn hotfix_admission(&self) -> Result<FootprintSize, PrototypeFault> {
        self.hotfix_admission
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
    /// Process-local ownership tags have been exhausted; no tag is reused.
    IdentityExhausted,
    /// Retail underflows `monster_count - 1` and accesses an absent prototype.
    RetailEmptyMonsters,
    /// Storage reservation failed.
    Allocation(TryReserveError),
}
impl fmt::Display for CatalogError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::IdentityExhausted => f.write_str("catalog ownership identities exhausted"),
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
    owner: OwnerId,
    entries: Vec<PreparedPrototype<'a>>,
    offsets: [usize; KINDS + 1],
    version: MapVersion,
    behavior: Behavior,
}

/// Identity of a prepared prototype, stable for the lifetime of its catalog.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct PrototypeId {
    index: usize,
    owner: OwnerId,
}
impl PrototypeId {
    /// Dense catalog ordinal, distinct from the source row or H3M prototype slot.
    #[must_use]
    pub const fn index(self) -> usize {
        self.index
    }
}
impl PrototypeCatalog<'_> {
    pub(crate) const fn owner(&self) -> OwnerId {
        self.owner
    }
    pub(crate) const fn behavior(&self) -> Behavior {
        self.behavior
    }
    pub(crate) const fn version(&self) -> MapVersion {
        self.version
    }
    fn id(&self, index: usize) -> PrototypeId {
        PrototypeId {
            index,
            owner: self.owner,
        }
    }
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
        (id.owner == self.owner)
            .then(|| self.entries.get(id.index))
            .flatten()
    }
    /// Direct family indexing used for towns, portals and reserved output slots.
    #[must_use]
    pub fn at(&self, family: ObjectKind, index: usize) -> Option<PrototypeId> {
        self.family(family)
            .get(index)
            .map(|_| self.id(self.offsets[family.index()] + index))
    }
    /// First matching subtype in native family order, without terrain filtering
    /// or randomness. Used by key tents and border guards.
    #[must_use]
    pub fn first_subtype(&self, family: ObjectKind, subtype: i32) -> Option<PrototypeId> {
        let index = self
            .family(family)
            .iter()
            .position(|entry| entry.prototype().subtype() == subtype)?;
        self.at(family, index)
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
            .map(|(index, _)| self.id(self.offsets[family.index()] + index))
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
            size: ImageDimensions::Footprint(FootprintSize::parse(2, 2).unwrap()),
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
    fn creature_traits(edits: &[(usize, i32, i32, i32)]) -> crate::traits::CreatureCatalog {
        use std::fmt::Write;
        let mut text = String::new();
        for row in 0..185 {
            let (_, ai, low, high) = edits
                .iter()
                .find(|&&(r, _, _, _)| r == row)
                .copied()
                .unwrap_or((row, 0, 0, 0));
            for column in 0..24 {
                if column > 0 {
                    text.push('\t');
                }
                let value = match column {
                    10 => ai,
                    21 => low,
                    22 => high,
                    _ => 0,
                };
                write!(text, "{value}").unwrap();
            }
            text.push_str("\r\n");
        }
        crate::traits::CreatureCatalog::parse(text.as_bytes()).unwrap()
    }

    #[test]
    fn signed_width_does_not_require_a_usable_height() {
        for (width, height, expected) in [(6, 0, 6), (255, 6, -1), (2, 2, 2)] {
            let mut bytes = [0; 14];
            bytes[0] = width;
            bytes[1] = height;
            let mask = ImageMask::parse(&bytes).unwrap();
            assert_eq!(mask.signed_width(), expected);
            assert_eq!(mask.size().is_ok(), width == 2 && height == 2);
        }
    }

    #[test]
    fn object_arena_rejects_foreign_and_reset_identities() {
        use crate::placement::{ObjectArena, PlacementError};
        let bytes = table(&[row("monster.def", raw::MONSTER, 0)]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let rules = rules();
        let catalog = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        let other = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        let prototype = catalog.at(ObjectKind::MONSTER, 0).unwrap();
        let mut first = ObjectArena::default();
        let mut second = ObjectArena::default();
        assert!(matches!(
            first.create(&other, prototype),
            Err(PlacementError::UnknownPrototype(_))
        ));
        let a = first.create(&catalog, prototype).unwrap();
        let b = second.create(&catalog, prototype).unwrap();
        assert_eq!(a.index(), b.index());
        assert!(first.get(b).is_none());
        assert!(second.get(a).is_none());
        assert!(first.get(a).unwrap().position().is_none());
        first.discard_unplaced(a).unwrap();
        assert!(first.get(a).is_none());
        assert!(first.payload(a).is_none());
        assert!(matches!(
            first.discard_unplaced(a),
            Err(PlacementError::UnknownObject(_))
        ));
        let reused = first.create(&catalog, prototype).unwrap();
        assert_eq!(reused.index(), a.index());
        assert_ne!(reused, a);
        assert!(first.get(a).is_none());
        assert!(first.payload(a).is_none());
        assert!(first.get(reused).is_some());
        assert!(matches!(
            first.discard_unplaced(a),
            Err(PlacementError::UnknownObject(_))
        ));
        assert!(matches!(
            first.discard_unplaced(b),
            Err(PlacementError::UnknownObject(_))
        ));
        first.reset();
        assert!(first.get(reused).is_none());
        let replacement = first.create(&catalog, prototype).unwrap();
        assert_eq!(replacement.index(), a.index());
        assert!(first.get(a).is_none());
        assert!(first.get(replacement).is_some());
    }

    #[test]
    fn handles_reject_other_catalogs_and_survive_owner_moves() {
        let bytes = table(&[row("monster.def", raw::MONSTER, 0)]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let rules = rules();
        let first = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        let second = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        let id = first.at(ObjectKind::MONSTER, 0).unwrap();
        assert!(second.get(id).is_none());
        let moved = first;
        assert!(moved.get(id).is_some());
        drop(moved);
        let replacement = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        assert!(replacement.get(id).is_none());
    }

    #[test]
    fn guard_missing_prototypes_fault_after_selection_only_in_retail() {
        let bytes = table(&[row("monster.def", raw::MONSTER, 0)]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let rules = rules();
        let creatures = creature_traits(&[(2, 10, 1, 1), (3, 10, 1, 1)]);
        for behavior in [Behavior::Retail(RetailProfile::default()), Behavior::Hotfix] {
            let catalog = source
                .prepare(&rules, MapVersion::ShadowOfDeath, behavior)
                .unwrap();
            let mut rng = RetailRng::new(1);
            let result = catalog.select_guard(
                100,
                GuardFactions::Allowed(&[true; 10]),
                &creatures,
                &mut rng,
            );
            assert_eq!(
                result,
                if behavior.is_hotfix() {
                    Ok(None)
                } else {
                    Err(GuardError::MissingSelectedPrototype)
                }
            );
            assert_eq!(rng.draws(), 1);
            let end = rng.checkpoint();
            assert_eq!(
                catalog.select_guard(9, GuardFactions::Allowed(&[true; 10]), &creatures, &mut rng),
                Ok(None)
            );
            assert_eq!(rng.checkpoint(), end);
        }
    }

    #[test]
    fn roe_guard_117_escapes_value_and_faction_checks_and_last_prototype_wins() {
        let bytes = table(&[
            row("zero.def", raw::MONSTER, 0),
            row("first.def", raw::MONSTER, 117),
            row("last.def", raw::MONSTER, 117),
        ]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let rules = rules();
        let creatures = creature_traits(&[(2, 10, 1, 1), (143, 20, 1000, 1000)]);
        let factions =
            GuardFactions::Matching(crate::request::Town::parse(raw::TOWN_CASTLE).unwrap());
        for version in [MapVersion::Restoration, MapVersion::ArmageddonsBlade] {
            let catalog = source.prepare(&rules, version, Behavior::Hotfix).unwrap();
            let mut rng = RetailRng::new(1);
            let guard = catalog
                .select_guard(20, factions, &creatures, &mut rng)
                .unwrap()
                .unwrap();
            if version == MapVersion::Restoration {
                assert_eq!(guard.creature().index(), 117);
                assert_eq!(guard.count(), 1);
                assert_eq!(
                    catalog
                        .get(guard.prototype())
                        .unwrap()
                        .prototype()
                        .source_row(),
                    2
                );
            } else {
                assert_eq!(guard.creature().index(), 0);
                assert_eq!(guard.count(), 2);
            }
            assert_eq!(rng.draws(), 1);
        }
    }

    #[test]
    fn guard_faults_keep_rng_position_at_invalid_index_arithmetic_and_division() {
        let rules = rules();
        let bytes = table(&[row("bad.def", raw::MONSTER, -1)]);
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let catalog = source
            .prepare(
                &rules,
                MapVersion::ShadowOfDeath,
                Behavior::Retail(RetailProfile::default()),
            )
            .unwrap();
        let mut rng = RetailRng::new(1);
        assert_eq!(
            catalog.select_guard(
                0,
                GuardFactions::Allowed(&[true; 10]),
                &creature_traits(&[]),
                &mut rng
            ),
            Err(GuardError::CreatureSubtype(-1))
        );
        assert_eq!(rng.draws(), 0);
        let bytes = table(
            &(0..14)
                .map(|id| row("valid.def", raw::MONSTER, id))
                .collect::<Vec<_>>(),
        );
        let source = PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
        let catalog = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        let factions =
            GuardFactions::Matching(crate::request::Town::parse(raw::TOWN_CASTLE).unwrap());
        assert!(matches!(
            catalog.select_guard(
                0,
                factions,
                &creature_traits(&[(176, 0, i32::MAX, i32::MAX)]),
                &mut rng
            ),
            Err(GuardError::Arithmetic { .. })
        ));
        assert_eq!(rng.draws(), 0);
        assert_eq!(
            catalog.select_guard(0, factions, &creature_traits(&[]), &mut rng),
            Err(GuardError::ZeroAiValue(
                crate::traits::CreatureId::parse(0).unwrap()
            ))
        );
        assert_eq!(rng.draws(), 1);
    }

    fn required_rows() -> Vec<String> {
        let mut rows = [
            raw::MONSTER,
            raw::RANDOM_MONSTER,
            raw::TERRAIN_HOLE,
            raw::SHIPYARD,
            raw::LITH_TWOWAY,
        ]
        .map(|kind| row("object.def", kind, 0))
        .to_vec();
        rows.extend((0..=raw::TOWN_CONFLUX).map(|town| row("town.def", raw::TOWN, town)));
        rows
    }

    #[test]
    fn hotfix_admission_checks_relationships_while_retail_defers_faults() {
        use crate::{request::Levels, traits::ArtifactCatalog};
        let artifact_bytes = format!(
            "header\r\nheader\r\n{}",
            format!("{}\r\n", "\t".repeat(22)).repeat(raw::ARTIFACT_COUNT as usize)
        );
        let artifacts = ArtifactCatalog::parse(artifact_bytes.as_bytes()).unwrap();
        let rules = rules();
        let base = required_rows;
        let admit = |rows: &[String], levels, behavior| {
            let bytes = table(rows);
            let source =
                PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
            source
                .prepare(&rules, MapVersion::ShadowOfDeath, behavior)
                .unwrap()
                .into_generation(levels, &artifacts)
                .map(|_| ())
        };
        assert!(admit(&base(), Levels::Surface, Behavior::Hotfix).is_ok());
        for (index, kind) in [
            raw::MONSTER,
            raw::RANDOM_MONSTER,
            raw::TERRAIN_HOLE,
            raw::SHIPYARD,
        ]
        .into_iter()
        .enumerate()
        {
            let mut rows = base();
            rows.remove(index);
            assert_eq!(
                admit(&rows, Levels::Surface, Behavior::Hotfix),
                Err(RequiredPrototypeError::Family(
                    ObjectKind::parse(i32::try_from(kind).unwrap()).unwrap()
                ))
            );
        }
        assert_eq!(
            admit(&base(), Levels::Underground, Behavior::Hotfix),
            Err(RequiredPrototypeError::Family(ObjectKind::UNDERGROUND_GATE))
        );
        let mut rows = base();
        rows.push(row("gate.def", raw::UNDERGROUND_GATE, 0));
        assert!(admit(&rows, Levels::Underground, Behavior::Hotfix).is_ok());
        let mut rows = base();
        rows[5] = row("town.def", raw::TOWN, 1);
        assert_eq!(
            admit(&rows, Levels::Surface, Behavior::Hotfix),
            Err(RequiredPrototypeError::Town { index: 0 })
        );
        let mut rows = base();
        rows.remove(4);
        assert_eq!(
            admit(&rows, Levels::Surface, Behavior::Hotfix),
            Err(RequiredPrototypeError::Portals)
        );
        rows.push(row("entry.def", raw::LITH_ONEWAY_ENTRANCE, 1));
        rows.push(row("exit.def", raw::LITH_ONEWAY_EXIT, 2));
        assert_eq!(
            admit(&rows, Levels::Surface, Behavior::Hotfix),
            Err(RequiredPrototypeError::PortalPair { index: 0 })
        );
        let mut rows = base();
        rows.push(row("tent.def", raw::BORDER_TENT, 2));
        assert_eq!(
            admit(&rows, Levels::Surface, Behavior::Hotfix),
            Err(RequiredPrototypeError::BorderGuard { subtype: 2 })
        );
        rows.push(row("guard.def", raw::BORDER_GUARD, 2));
        assert!(admit(&rows, Levels::Surface, Behavior::Hotfix).is_ok());
        rows.push(row("seer.def", raw::SEER, 0));
        assert_eq!(
            admit(&rows, Levels::Surface, Behavior::Hotfix),
            Err(RequiredPrototypeError::RandomArtifact)
        );
        rows.push(row("random.def", raw::RANDOM_ARTIFACT, 0));
        assert!(admit(&rows, Levels::Surface, Behavior::Hotfix).is_ok());
        assert!(admit(
            &[row("monster.def", raw::MONSTER, 0)],
            Levels::Underground,
            Behavior::Retail(RetailProfile::default())
        )
        .is_ok());
    }

    #[test]
    fn hotfix_seer_admission_requires_every_eligible_quest_artifact() {
        use crate::{request::Levels, traits::ArtifactCatalog};
        let rules = rules();
        let blank = format!("{}\r\n", "\t".repeat(22));
        let artifact_bytes = format!(
            "header\r\nheader\r\n{}",
            blank.repeat(raw::ARTIFACT_COUNT as usize)
        );
        let mut rows = required_rows();
        rows.push(row("seer.def", raw::SEER, 0));
        rows.push(row("random.def", raw::RANDOM_ARTIFACT, 0));
        let mut quest_bytes = artifact_bytes.as_bytes().to_vec();
        quest_bytes.insert(b"header\r\nheader\r\n".len() + 21, b'T');
        let quest_artifacts = ArtifactCatalog::parse(&quest_bytes).unwrap();
        for present in [false, true] {
            if present {
                rows.push(row("artifact.def", raw::ARTIFACT, 0));
            }
            let bytes = table(&rows);
            let source =
                PrototypeSource::parse(&bytes, |_| Ok::<_, Infallible>(Some(mask()))).unwrap();
            let result = source
                .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
                .unwrap()
                .into_generation(Levels::Surface, &quest_artifacts)
                .map(|_| ());
            let expected = if present {
                Ok(())
            } else {
                Err(RequiredPrototypeError::QuestArtifact(
                    crate::traits::ArtifactId::parse(0).unwrap(),
                ))
            };
            assert_eq!(result, expected);
        }
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
            .family(ObjectKind::MONSTER)
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
            retail.entries()[0].hotfix_admission(),
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
        assert!(hotfix.entries()[0].hotfix_admission().is_ok());
        let retail = source
            .prepare(
                &rules,
                MapVersion::ShadowOfDeath,
                Behavior::Retail(RetailProfile::default()),
            )
            .unwrap();
        assert_eq!(
            retail.family(ObjectKind::MONSTER)[0].hotfix_admission(),
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
            .choose(ObjectKind::MONSTER, 3, Terrain::Dirt, &mut rng)
            .is_none());
        assert_eq!(rng.checkpoint(), initial);
        let selected = catalog
            .choose(ObjectKind::MONSTER, 2, Terrain::Dirt, &mut rng)
            .unwrap();
        assert_eq!(Some(selected), catalog.at(ObjectKind::MONSTER, 0));
        assert_eq!(catalog.get(selected).unwrap().prototype().subtype(), 2);
        assert_eq!(rng.checkpoint().draws, initial.draws + 1);
        assert!(catalog.at(ObjectKind::MONSTER, 1).is_none());
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
