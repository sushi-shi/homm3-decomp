//! Shared rendering of native prototype checkpoints.
use homm3_rmg::{
    domain::Terrain,
    placement_rules::PlacementRules,
    prototype::{MaskCell, PrototypeFault},
    raw,
};
use std::fmt::Write;

pub(crate) fn snapshot(
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
    rules: &PlacementRules,
) -> String {
    let mut actual = String::new();
    for entry in catalog.entries() {
        let prototype = entry.prototype();
        let image = entry.image_mask();
        let (width, height) = match image.size() {
            Ok(size) => (i32::from(size.width()), i32::from(size.height())),
            Err(PrototypeFault::Dimensions { width, height }) => {
                (i32::from(width), i32::from(height))
            }
            Err(error) => panic!("unexpected image fault {error:?}"),
        };
        let entrance = prototype
            .entrance()
            .map_or((raw::OBJECT_MASK_WIDTH, raw::OBJECT_MASK_HEIGHT), |cell| {
                (u32::from(cell.x()), u32::from(cell.y()))
            });
        let terrain_bits = |recommended| {
            (raw::eTerrainDirt..=raw::eTerrainRock).fold(0_u16, |bits, value| {
                let terrain = Terrain::parse(value).unwrap();
                let included = if recommended {
                    prototype.recommends(terrain)
                } else {
                    prototype.allows_terrain(terrain)
                };
                bits | (u16::from(included) << value)
            })
        };
        write!(
            actual,
            "{} {} {} {width} {height} {} {} {} {} {} {} {} {} {} {} ",
            prototype.kind().family().index(),
            prototype.source_row(),
            std::str::from_utf8(entry.image_name()).unwrap(),
            prototype.kind().index(),
            prototype.subtype(),
            prototype.category(),
            u8::from(prototype.underlay()),
            entrance.0,
            entrance.1,
            entry
                .preferred()
                .map_or(raw::eTerrainRock, |terrain| terrain as i32),
            entry.rule().map_or(-1, |rule| i64::try_from(
                rules.get(rule).unwrap().source_row()
            )
            .unwrap()),
            terrain_bits(false),
            terrain_bits(true)
        )
        .unwrap();
        for y in 0..u8::try_from(raw::OBJECT_MASK_HEIGHT).unwrap() {
            for x in 0..u8::try_from(raw::OBJECT_MASK_WIDTH).unwrap() {
                let cell = MaskCell::parse(x, y).unwrap();
                let bits = u8::from(image.draws(cell)) * 8
                    + u8::from(prototype.is_passable(cell)) * 4
                    + u8::from(image.shadows(cell)) * 2
                    + u8::from(prototype.is_trigger(cell));
                write!(actual, "{bits:x}").unwrap();
            }
        }
        actual.push('\n');
    }
    actual
}
