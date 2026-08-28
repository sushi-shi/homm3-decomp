//! Deterministic hostile-input coverage for the scenario-header parser.

use homm3_map::{
    campaign::Campaign, MapBody, MapHeader, ObjectTable, Terrain, Version, WorldPrefix,
};

#[test]
fn deterministic_byte_corpus_never_panics() {
    for seed in 0..32u32 {
        let mut random = seed ^ 0x4833_4d50;
        for len in 0..=2048usize {
            let mut data = vec![0u8; len];
            for byte in &mut data {
                random = random.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
                *byte = random.to_le_bytes()[1];
            }
            let _ = MapHeader::parse(&data);
            let _ = Campaign::parse(&data);
            let _ = WorldPrefix::parse(&data, Version::Restoration);
            let _ = WorldPrefix::parse(&data, Version::ArmageddonsBlade);
            let _ = WorldPrefix::parse(&data, Version::ShadowOfDeath);
            let _ = Terrain::parse(&data, 36, false);
            let _ = Terrain::parse(&data, 144, true);
            if let Ok(table) = ObjectTable::parse(&data) {
                let _ = MapBody::parse(table, Version::Restoration);
                let _ = MapBody::parse(table, Version::ArmageddonsBlade);
                let _ = MapBody::parse(table, Version::ShadowOfDeath);
            }
        }
    }
}
