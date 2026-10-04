//! Compare installed prototypes and masks against real C++ loader checkpoints.

use flate2::read::ZlibDecoder;
use homm3_lod::{Archive, Payload};
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    domain::Terrain,
    placement_rules::PlacementRules,
    prototype::{ImageMask, MaskCell, PrototypeFault, PrototypeSource},
    raw,
    request::MapVersion,
};
use std::{error::Error, fmt::Write, io::Read, path::PathBuf};

fn mask(archives: &[Archive<'_>], name: &[u8]) -> Result<Option<ImageMask>, Box<dyn Error>> {
    let name = std::str::from_utf8(name)?;
    for archive in archives {
        if let Some(payload) = archive.get(name) {
            return Ok(Some(match payload {
                Payload::Stored(bytes) => ImageMask::parse(bytes)?,
                Payload::Compressed {
                    stream,
                    unpacked_size,
                } => {
                    let mut bytes = [0; 14];
                    assert_eq!(unpacked_size, bytes.len());
                    ZlibDecoder::new(stream).read_exact(&mut bytes)?;
                    ImageMask::parse(&bytes)?
                }
            }));
        }
    }
    Ok(None)
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA, HOMM3_RMG_OBJECTS, HOMM3_RMG_PLACEMENT and HOMM3_RMG_ORACLE"]
fn installed_prototype_catalog_matches_cpp_in_all_formats_and_modes() {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let root = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    // Complete's resource context searches base sprites before expansion sprites.
    let images =
        ["H3sprite.lod", "H3ab_spr.lod"].map(|name| std::fs::read(directory.join(name)).unwrap());
    let archives = images
        .each_ref()
        .map(|image| Archive::parse(image).unwrap());
    let objects = std::fs::read(std::env::var_os("HOMM3_RMG_OBJECTS").unwrap()).unwrap();
    let placements = std::fs::read(std::env::var_os("HOMM3_RMG_PLACEMENT").unwrap()).unwrap();
    let source = PrototypeSource::parse(&objects, |name| mask(&archives, name)).unwrap();
    eprintln!(
        "{} source rows, {} distinct image masks",
        source.rows().len(),
        source.image_count()
    );
    for (mode, behavior) in [
        ("retail", Behavior::Retail(RetailProfile::default())),
        ("hotfix", Behavior::Hotfix),
    ] {
        let rules = PlacementRules::parse(&placements, behavior).unwrap();
        for (case, version) in [
            MapVersion::ShadowOfDeath,
            MapVersion::ArmageddonsBlade,
            MapVersion::Restoration,
        ]
        .into_iter()
        .enumerate()
        {
            let catalog = source.prepare(&rules, version, behavior).unwrap();
            let actual = snapshot(&catalog, &rules);
            let expected = std::fs::read_to_string(root.join(format!(
                "{mode}-layout/case-{case}-candidate/prototypes.txt"
            )))
            .unwrap();
            assert_eq!(
                actual.lines().count(),
                expected.lines().count(),
                "{mode} {version:?} prototype count"
            );
            for (line, (actual, expected)) in actual.lines().zip(expected.lines()).enumerate() {
                assert_eq!(actual, expected, "{mode} {version:?} line {}", line + 1);
            }
            eprintln!(
                "{mode} {version:?}: {} prototypes matched",
                catalog.entries().len()
            );
        }
    }
}

fn snapshot(
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
