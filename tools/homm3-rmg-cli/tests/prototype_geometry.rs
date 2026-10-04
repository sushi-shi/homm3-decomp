//! Full installed-prototype outlines and priorities captured from the native C++.

use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    placement_rules::PlacementRules,
    prototype::{MaskCell, OutlineWorkspace, OverlapPriorities, PrototypeSource},
    raw,
    request::MapVersion,
};
use homm3_rmg_cli::resources::Installation;
use std::{fmt::Write, path::PathBuf};

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE geometry checkpoints"]
fn installed_outline_order_and_drawn_priorities_match_cpp() {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let oracle = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let mut objects = Vec::new();
    installation.text("objects.txt", &mut objects).unwrap();
    let source = PrototypeSource::parse(&objects, |name| installation.mask(name)).unwrap();
    let mut bytes = Vec::new();
    installation.text("rand_trn.txt", &mut bytes).unwrap();
    let mut workspace = OutlineWorkspace::default();
    let mut checked = 0;
    for (mode, behavior) in [
        ("retail", Behavior::Retail(RetailProfile::default())),
        ("hotfix", Behavior::Hotfix),
    ] {
        let rules = PlacementRules::parse(&bytes, behavior).unwrap();
        for (case, version) in [
            MapVersion::ShadowOfDeath,
            MapVersion::ArmageddonsBlade,
            MapVersion::Restoration,
        ]
        .into_iter()
        .enumerate()
        {
            let catalog = source.prepare(&rules, version, behavior).unwrap();
            let mut actual = String::new();
            for entry in catalog.entries() {
                if entry.image_mask().size().is_err() {
                    continue;
                }
                let outline = workspace.trace(entry).unwrap();
                write!(
                    actual,
                    "{} {}",
                    entry.prototype().source_row(),
                    outline.len()
                )
                .unwrap();
                for point in outline {
                    write!(actual, " {} {}", point.x, point.y).unwrap();
                }
                let priorities = OverlapPriorities::build(entry).unwrap();
                for y in 0..raw::OBJECT_MASK_HEIGHT {
                    for x in 0..raw::OBJECT_MASK_WIDTH {
                        let cell =
                            MaskCell::parse(u8::try_from(x).unwrap(), u8::try_from(y).unwrap())
                                .unwrap();
                        write!(actual, " {}", priorities.get(cell).map_or(-1, i32::from)).unwrap();
                    }
                }
                actual.push('\n');
                checked += 1;
            }
            let expected = std::fs::read_to_string(oracle.join(format!(
                "{mode}-layout/case-{case}-candidate/prototype-geometry.txt"
            )))
            .unwrap();
            assert_eq!(actual, expected.replace("\r\n", "\n"), "{mode} {version:?}");
        }
    }
    eprintln!("checked {checked} prototype outlines and priority masks");
}
