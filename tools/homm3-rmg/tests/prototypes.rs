//! Compare installed prototypes and masks against real C++ loader checkpoints.

use flate2::read::ZlibDecoder;
use homm3_lod::{Archive, Payload};
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    placement_rules::PlacementRules,
    prototype::{ImageMask, PrototypeSource},
    request::MapVersion,
};
use std::{error::Error, io::Read, path::PathBuf};

#[path = "support/prototype_snapshot.rs"]
mod prototype_snapshot;
use prototype_snapshot::snapshot;

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
