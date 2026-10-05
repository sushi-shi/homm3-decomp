//! Installed-data comparison against the initialized native trait/rule tables.

use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    domain::Terrain,
    object::ObjectKind,
    placement_rules::{NeighbourScore, PlacementRules},
    raw,
};
use std::{fmt::Write, path::PathBuf};

#[test]
#[ignore = "requires HOMM3_RMG_PLACEMENT and HOMM3_RMG_ORACLE loader checkpoints"]
fn installed_rules_and_object_traits_match_the_native_loader() {
    let bytes = std::fs::read(std::env::var_os("HOMM3_RMG_PLACEMENT").unwrap()).unwrap();
    let root = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    for (mode, behavior) in [
        ("retail", Behavior::Retail(RetailProfile::default())),
        ("hotfix", Behavior::Hotfix),
    ] {
        let directory = root.join(format!("{mode}-layout/case-0-candidate"));
        let rules = PlacementRules::parse(&bytes, behavior).unwrap();
        // Shipped rules have no rejected rows, so this compares every column.
        assert_eq!(rules.rules().len(), rules.source_rows());
        let mut actual = String::new();
        for (id, rule) in rules.iter() {
            write!(
                actual,
                "{} {} {} {}",
                rule.object().index(),
                rule.subtype(),
                rule.terrain() as i32,
                rule.source_row()
            )
            .unwrap();
            for index in 0..raw::RMG_TERRAIN_COUNT {
                let terrain = Terrain::parse(i32::try_from(index).unwrap()).unwrap();
                write!(actual, " {}", rule.terrain_score(terrain).unwrap()).unwrap();
            }
            actual.push_str(" | ");
            for kind in [NeighbourScore::Adjacent, NeighbourScore::Blocked] {
                if kind == NeighbourScore::Blocked {
                    actual.push_str("| ");
                }
                for (neighbour, _) in rules.iter() {
                    write!(
                        actual,
                        "{} ",
                        rules.neighbour_score(id, neighbour, kind).unwrap()
                    )
                    .unwrap();
                }
            }
            actual.push('\n');
        }
        let expected = std::fs::read_to_string(directory.join("placement-rules.txt"))
            .unwrap()
            .replace("\r\n", "\n");
        assert_eq!(actual, expected, "{mode} placement rules");
        actual.clear();
        for index in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
            let kind = ObjectKind::parse(i32::try_from(index).unwrap()).unwrap();
            let traits = kind.traits();
            writeln!(
                actual,
                "{} {} {} {} {} {}",
                index,
                kind.family().index(),
                u8::from(traits.is_decoration()),
                u8::from(traits.cleared_on_visit()),
                u8::from(traits.blocks_landing()),
                u8::from(traits.enterable_from_north())
            )
            .unwrap();
        }
        let expected = std::fs::read_to_string(directory.join("object-traits.txt"))
            .unwrap()
            .replace("\r\n", "\n");
        assert_eq!(actual, expected, "{mode} object traits");
        eprintln!(
            "{mode}: {} placement rules, {} object traits matched",
            rules.rules().len(),
            raw::ADVENTURE_OBJECT_TRAIT_COUNT
        );
    }
}
