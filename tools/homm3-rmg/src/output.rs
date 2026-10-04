//! Streaming `RoE`, AB and `SoD` H3M output, continuing the generation RNG.
//!
//! Compression belongs to the caller. A reusable slot vector is the only
//! map-sized serialization scratch; tiles and objects are written by borrowing
//! the generated map. Repeated writes start from the same final generation RNG.
#![expect(
    clippy::cast_possible_truncation,
    clippy::cast_sign_loss,
    reason = "wire fields intentionally retain native low-byte/word narrowing; domain indexes are bounded at parsing"
)]

use crate::{
    generation::GeneratedMap,
    object::ObjectKind,
    placement::{ObjectId, TreasureGenerationError},
    prototype::{MaskCell, PreparedPrototype, PrototypeId},
    raw,
    request::MapVersion,
    rng::{RetailRng, RngCheckpoint},
};
use std::{
    collections::TryReserveError,
    error::Error,
    fmt,
    io::{self, Write},
};
mod header;
mod objects;

/// Output boundary reached when a write fails.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum OutputStage {
    /// Description, players, teams and availability masks.
    Header,
    /// Flat terrain/river/road tiles.
    Tiles,
    /// Reserved and referenced object prototypes.
    Prototypes,
    /// Decoration pass followed by gameplay objects.
    Objects,
    /// Empty global-event trailer.
    Trailer,
}
/// Serialization cannot complete safely.
#[derive(Debug)]
pub enum OutputFault {
    /// Destination failed, possibly after accepting a prefix.
    Io(io::Error),
    /// Reusable prototype slots could not grow.
    Allocation(TryReserveError),
    /// A native signed count cannot represent the output.
    Count,
    /// Native unchecked description construction exceeds its stack buffer.
    DescriptionOverflow,
    /// Retail's overlapping player masks exhaust the remaining team quotas.
    EmptyTeamPool,
    /// A mandatory reserved prototype family is empty.
    MissingPrototype(ObjectKind),
    /// An object references a prototype absent from this generation.
    Prototype(PrototypeId),
    /// A registered object is absent or has no position.
    Object(ObjectId),
    /// Ordered spell payload lookup failed.
    Treasure(TreasureGenerationError),
}
impl fmt::Display for OutputFault {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Io(e) => e.fmt(f),
            Self::Allocation(e) => e.fmt(f),
            Self::Count => f.write_str("H3M count exceeds native signed range"),
            Self::DescriptionOverflow => f.write_str("native map description buffer overflows"),
            Self::EmptyTeamPool => {
                f.write_str("retail team assignment divides by zero after its draw")
            }
            Self::MissingPrototype(k) => {
                write!(f, "missing reserved prototype family {}", k.index())
            }
            Self::Prototype(p) => write!(f, "missing output prototype {}", p.index()),
            Self::Object(o) => write!(f, "missing placed object {}", o.index()),
            Self::Treasure(e) => e.fmt(f),
        }
    }
}
impl Error for OutputFault {}
impl From<io::Error> for OutputFault {
    fn from(e: io::Error) -> Self {
        Self::Io(e)
    }
}
impl From<TreasureGenerationError> for OutputFault {
    fn from(e: TreasureGenerationError) -> Self {
        Self::Treasure(e)
    }
}
/// Failure with exact RNG position; the destination may contain a partial map.
#[derive(Debug)]
pub struct OutputError {
    /// Source error.
    pub fault: OutputFault,
    /// Last started output section.
    pub stage: OutputStage,
    /// Includes team draws made before failure.
    pub rng: RngCheckpoint,
}
impl fmt::Display for OutputError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{:?} at RNG draw {}: {}",
            self.stage, self.rng.draws, self.fault
        )
    }
}
impl Error for OutputError {
    fn source(&self) -> Option<&(dyn Error + 'static)> {
        Some(&self.fault)
    }
}
/// Completed output diagnostics, including header team-assignment draws.
#[derive(Clone, Copy, Debug)]
pub struct OutputReport {
    /// State after the entire uncompressed H3M stream was accepted.
    pub rng: RngCheckpoint,
}
/// Reusable catalog-index-to-wire-slot storage. No terrain or payload copies.
#[derive(Default)]
pub struct OutputWorkspace {
    slots: Vec<Option<u32>>,
}
impl OutputWorkspace {
    /// Write an uncompressed H3M stream in native serialization order.
    ///
    /// # Errors
    /// Returns the reached output section and RNG state. IO is not transactional;
    /// callers publishing files should finish compression before renaming them.
    pub fn write(
        &mut self,
        map: &GeneratedMap<'_>,
        destination: &mut impl Write,
    ) -> Result<OutputReport, OutputError> {
        let mut rng = map.report().rng.clone();
        let mut stage = OutputStage::Header;
        self.write_inner(map, &mut Writer(destination), &mut rng, &mut stage)
            .map_err(|fault| OutputError {
                fault,
                stage,
                rng: rng.checkpoint(),
            })?;
        Ok(OutputReport {
            rng: rng.checkpoint(),
        })
    }
    fn write_inner(
        &mut self,
        map: &GeneratedMap<'_>,
        out: &mut Writer<'_, impl Write>,
        rng: &mut RetailRng,
        stage: &mut OutputStage,
    ) -> Result<(), OutputFault> {
        header::write(map, out, rng)?;
        *stage = OutputStage::Tiles;
        out.u32(0)?; // no rumors
        for (tile, cell) in map.map().terrain().tiles().iter().zip(map.map().cells()) {
            let river = cell.river();
            let road = cell.road();
            let reflection =
                |r: crate::line::Reflection| u8::from(r.flip_x) | (u8::from(r.flip_y) << 1);
            let flags = reflection(tile.reflection())
                | (reflection(river.reflection()) << 2)
                | (reflection(road.reflection()) << 4)
                | (u8::from(cell.is_coastal()) << 6);
            out.bytes(&[
                tile.terrain().index() as u8,
                tile.frame(),
                river.kind().map_or(0, |k| k as u8),
                river.frame(),
                road.kind().map_or(0, |k| k as u8),
                road.frame(),
                flags,
            ])?;
        }
        *stage = OutputStage::Prototypes;
        let catalog = map.treasures().ready().catalog().prototypes();
        self.slots
            .try_reserve(catalog.entries().len().saturating_sub(self.slots.len()))
            .map_err(OutputFault::Allocation)?;
        self.slots.resize(catalog.entries().len(), None);
        self.slots.fill(None);
        // Every live arena record owns one reference, including unplaced pending
        // children of replacement artifacts and native retained/leaked records.
        for id in map.objects().referenced_prototypes() {
            catalog.get(id).ok_or(OutputFault::Prototype(id))?;
            self.slots[id.index()] = Some(0);
        }
        let mut count = 2_u32;
        for slot in self.slots.iter_mut().filter(|slot| slot.is_some()) {
            *slot = Some(count);
            count = count
                .checked_add(1)
                .filter(|&n| i32::try_from(n).is_ok())
                .ok_or(OutputFault::Count)?;
        }
        out.u32(count)?;
        // Reserved slots stay separate even if these entries also own references.
        for family in [raw::RANDOM_MONSTER, raw::TERRAIN_HOLE] {
            let kind = ObjectKind::parse(i32::try_from(family).unwrap()).unwrap();
            let entry = catalog
                .family(kind)
                .first()
                .ok_or(OutputFault::MissingPrototype(kind))?;
            prototype(out, entry)?;
        }
        for (slot, entry) in self.slots.iter().zip(catalog.entries()) {
            if slot.is_some() {
                prototype(out, entry)?;
            }
        }
        *stage = OutputStage::Objects;
        out.count(map.map().active_objects().len())?;
        for decoration in [true, false] {
            for &id in map.map().active_objects() {
                let object = map.objects().get(id).ok_or(OutputFault::Object(id))?;
                if object.kind().traits().is_decoration() == decoration {
                    let slot = self
                        .slots
                        .get(object.prototype().index())
                        .copied()
                        .flatten()
                        .ok_or(OutputFault::Prototype(object.prototype()))?;
                    objects::write(map, id, slot, out)?;
                }
            }
        }
        *stage = OutputStage::Trailer;
        out.u32(0) // no global events
    }
}
struct Writer<'a, W>(&'a mut W);
impl<W: Write> Writer<'_, W> {
    fn bytes(&mut self, bytes: &[u8]) -> Result<(), OutputFault> {
        self.0.write_all(bytes)?;
        Ok(())
    }
    fn byte(&mut self, value: u8) -> Result<(), OutputFault> {
        self.bytes(&[value])
    }
    fn u16(&mut self, value: u16) -> Result<(), OutputFault> {
        self.bytes(&value.to_le_bytes())
    }
    fn u32(&mut self, value: u32) -> Result<(), OutputFault> {
        self.bytes(&value.to_le_bytes())
    }
    fn i32(&mut self, value: i32) -> Result<(), OutputFault> {
        self.bytes(&value.to_le_bytes())
    }
    fn count(&mut self, value: usize) -> Result<(), OutputFault> {
        self.i32(i32::try_from(value).map_err(|_| OutputFault::Count)?)
    }
    fn string(&mut self, value: &[u8]) -> Result<(), OutputFault> {
        self.count(value.len())?;
        self.bytes(value)
    }
    fn zero(&mut self, mut count: usize) -> Result<(), OutputFault> {
        const ZERO: [u8; 32] = [0; 32];
        while count != 0 {
            let n = count.min(ZERO.len());
            self.bytes(&ZERO[..n])?;
            count -= n;
        }
        Ok(())
    }
    fn position(&mut self, position: crate::domain::WorldPosition) -> Result<(), OutputFault> {
        // Native writes low bytes, including coordinates from modeled flat aliases.
        self.bytes(&[
            position.point.x as u8,
            position.point.y as u8,
            position.level.index() as u8,
        ])
    }
    fn bits(
        &mut self,
        count: usize,
        mut bit: impl FnMut(usize) -> bool,
    ) -> Result<(), OutputFault> {
        for first in (0..count).step_by(8) {
            let mut byte = 0;
            for index in first..(first + 8).min(count) {
                byte |= u8::from(bit(index)) << (index - first);
            }
            self.byte(byte)?;
        }
        Ok(())
    }
}
fn expansion(version: MapVersion) -> bool {
    !matches!(version, MapVersion::Restoration)
}
fn prototype(
    out: &mut Writer<'_, impl Write>,
    entry: &PreparedPrototype<'_>,
) -> Result<(), OutputFault> {
    let p = entry.prototype();
    out.string(entry.image_name())?;
    // Fixed 8x6 footprint in reverse native row/column order, packed LSB first.
    // North is up; bit 47 is the object's map anchor P:
    //    0  1 ..  6  7
    //    8  9 .. 14 15
    //   ..
    //   40 41 .. 46  P
    for trigger in [false, true] {
        out.bits(raw::OBJECT_MASK_CELLS as usize, |bit| {
            let cell = MaskCell::parse(
                (raw::OBJECT_MASK_WIDTH as usize - 1 - bit % raw::OBJECT_MASK_WIDTH as usize) as u8,
                (raw::OBJECT_MASK_HEIGHT as usize - 1 - bit / raw::OBJECT_MASK_WIDTH as usize)
                    as u8,
            )
            .unwrap();
            if trigger {
                p.is_trigger(cell)
            } else {
                p.is_passable(cell)
            }
        })?;
    }
    for recommended in [false, true] {
        out.bits(raw::RMG_TERRAIN_COUNT as usize, |index| {
            let t = crate::domain::Terrain::parse(i32::try_from(index).unwrap()).unwrap();
            if recommended {
                p.recommends(t)
            } else {
                p.allows_terrain(t)
            }
        })?;
    }
    out.count(p.kind().index())?;
    out.i32(p.subtype())?;
    out.byte(p.category() as u8)?;
    out.byte(u8::from(p.underlay()))?;
    out.zero(16)
}
