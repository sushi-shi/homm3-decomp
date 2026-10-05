use super::*;
use homm3_resource::hdat::Container;

fn data(records: &[(u8, &[i32])]) -> Vec<u8> {
    let mut bytes = b"HDAT\x02\0\0\0".to_vec();
    bytes.extend_from_slice(&(records.len() as u32).to_le_bytes());
    for (id, integers) in records {
        let name = format!("terrain{id}");
        bytes.extend_from_slice(&(name.len() as u32).to_le_bytes());
        bytes.extend_from_slice(name.as_bytes());
        bytes.extend_from_slice(&[0; 9]); // empty source path, no strings, absent payload
        bytes.extend_from_slice(&(integers.len() as u32).to_le_bytes());
        for value in *integers {
            bytes.extend_from_slice(&value.to_le_bytes());
        }
    }
    bytes
}
fn patterns() -> Vec<u8> {
    (0..HOTA_PATTERN_COUNT)
        .flat_map(|index| [0_u32.to_le_bytes(), u32::from(index >= 2).to_le_bytes()].concat())
        .collect()
}
fn catalog() -> TerrainCatalog {
    let bytes = data(&[(10, &[0, 50]), (11, &[50])]);
    TerrainCatalog::parse_hota181(Container::parse(&bytes).unwrap(), &patterns()).unwrap()
}

#[test]
fn installed_frame_admission_rejects_bad_ranges_and_missing_records() {
    let bytes = data(&[(10, &[0, 50]), (11, &[50])]);
    let container = Container::parse(&bytes).unwrap();
    let mut pattern = patterns();
    assert!(matches!(
        TerrainCatalog::parse_hota181(container, &pattern[..8]),
        Err(TerrainDataError::PatternLength(8))
    ));
    pattern[16..20].copy_from_slice(&1_u32.to_le_bytes());
    assert!(matches!(
        TerrainCatalog::parse_hota181(container, &pattern),
        Err(TerrainDataError::Pattern(2))
    ));
    pattern = patterns();
    pattern[24..28].copy_from_slice(&2_u32.to_le_bytes());
    assert!(matches!(
        TerrainCatalog::parse_hota181(container, &pattern),
        Err(TerrainDataError::Pattern(4))
    ));
    let missing = data(&[(10, &[50])]);
    assert!(matches!(
        TerrainCatalog::parse_hota181(Container::parse(&missing).unwrap(), &patterns()),
        Err(TerrainDataError::Missing(11))
    ));
    let missing = data(&[(10, &[]), (11, &[50])]);
    assert!(matches!(
        TerrainCatalog::parse_hota181(Container::parse(&missing).unwrap(), &patterns()),
        Err(TerrainDataError::Record(_))
    ));
}

#[test]
fn frame_selection_excludes_duplicate_neighbours_and_restores_exhausted_ranges() {
    let catalog = catalog();
    let terrain = Terrain::Highlands;
    let first = catalog
        .parse_tile(terrain, 0, Reflection::default())
        .unwrap();
    let second = catalog
        .parse_tile(terrain, 1, Reflection::default())
        .unwrap();
    let strength = BrushStrength::parse(0).unwrap();
    for (neighbours, expected) in [
        (vec![first, first], 1),
        (vec![second, second], 0),
        (vec![first, second], 1),
    ] {
        let mut rng = RetailRng::new(1);
        let selected = catalog
            .select_base(terrain, strength, None, &neighbours, &mut rng)
            .unwrap();
        assert_eq!(selected.frame(), expected);
        assert!(!selected.is_special());
        assert_eq!(rng.draws(), 2); // The zero-percent chance still draws, then frame selection.
    }
    let mut rng = RetailRng::new(1);
    let special = catalog
        .select_base(
            terrain,
            BrushStrength::parse(8).unwrap(),
            None,
            &[],
            &mut rng,
        )
        .unwrap();
    assert!(special.is_special());
    assert_eq!(rng.draws(), 2);
    let before = rng.checkpoint();
    assert_eq!(
        catalog
            .select_base(terrain, strength, Some(special), &[special], &mut rng)
            .unwrap(),
        special
    );
    assert_eq!(rng.checkpoint(), before);
    assert!(matches!(
        catalog.parse_tile(terrain, 124, Reflection::default()),
        Err(TerrainRuleError::Frame { .. })
    ));
    assert!(matches!(
        TerrainCatalog::complete().parse_tile(terrain, 0, Reflection::default()),
        Err(TerrainRuleError::UnavailableTerrain(_))
    ));
}

#[test]
fn native_unsigned_probability_and_last_record_wins_are_preserved() {
    let bytes = data(&[(10, &[50]), (11, &[50]), (10, &[-1])]);
    let catalog =
        TerrainCatalog::parse_hota181(Container::parse(&bytes).unwrap(), &patterns()).unwrap();
    let mut rng = RetailRng::new(1);
    let selected = catalog
        .select_base(
            Terrain::Highlands,
            BrushStrength::parse(8).unwrap(),
            None,
            &[],
            &mut rng,
        )
        .unwrap();
    assert!(selected.is_special());
    assert_eq!(catalog.ruleset(), Ruleset::HotA181);
    assert_eq!(catalog.frame_count(Terrain::Highlands), Some(124));
}
