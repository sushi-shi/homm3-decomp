use super::*;
use crate::{
    behavior::Behavior, line::Reflection, placement_rules::PlacementRules,
    terrain_rules::TerrainCatalog,
};

/// One `HotA` rule; `scores` are the twelve terrain columns (Rock ignored).
fn rules(scores: [i32; 12]) -> PlacementRules {
    let mut row = vec![
        "rule".to_owned(),
        "0".into(),
        "0".into(),
        raw::MONSTER.to_string(),
        "0".into(),
        "0".into(),
        "0".into(),
    ];
    row.extend(scores[..9].iter().map(ToString::to_string));
    row.extend(scores[10..].iter().map(ToString::to_string));
    row.extend(["0".into(), "0".into()]);
    let text = format!("header\r\nheader\r\nheader\r\n{}\r\n", row.join("\t"));
    PlacementRules::parse_for(text.as_bytes(), Behavior::Hotfix, Ruleset::HotA181).unwrap()
}

fn rule(rules: &PlacementRules) -> &PlacementRule {
    &rules.rules()[0]
}

/// A Complete tile of `terrain` whose frame has `shape`.
fn tile(terrain: Terrain, shape: u32) -> TerrainTile {
    let catalog = TerrainCatalog::complete();
    (0..=u8::MAX)
        .map_while(|frame| {
            catalog
                .parse_tile(terrain, frame, Reflection::default())
                .ok()
        })
        .find(|tile| tile.shape() == shape)
        .unwrap_or_else(|| panic!("{terrain:?} has no shape {shape}"))
}

fn scores(base: i32) -> [i32; 12] {
    [base; 12]
}

#[test]
fn transition_frames_count_their_edge_terrain() {
    let mut values = scores(raw::RMG_PLACEMENT_INVALID - 1);
    values[Terrain::Grass.index()] = 10;
    values[Terrain::Dirt.index()] = 20;
    values[Terrain::Sand.index()] = 30;
    let rules = rules(values);
    for (shape, dirt, sand) in [
        (raw::SHAPE_FILL, false, false),
        (raw::SHAPE_N_W_BLEND, true, false),
        (raw::SHAPE_N_W_HARD, false, true),
        (raw::SHAPE_NW_BLEND_SE_HARD, true, true),
        // RVA 0x1c7f60's default arm: shape 28 counts only its own terrain.
        (raw::SHAPE_E_S_BLEND_NE_SW_HARD, false, false),
    ] {
        let mut tally = TerrainTally::default();
        tally
            .mark(rule(&rules), tile(Terrain::Grass, shape))
            .unwrap();
        let grass = TerrainTally::COUNTED | TerrainTally::OCCUPIED;
        assert_eq!(tally.seen[Terrain::Grass.index()], grass, "{shape}");
        assert_eq!(tally.seen[Terrain::Dirt.index()] != 0, dirt, "{shape}");
        assert_eq!(tally.seen[Terrain::Sand.index()] != 0, sand, "{shape}");
        let gate = tally.gate(rule(&rules), Ruleset::HotA181, 2).unwrap();
        let expected = 10 + if dirt { 20 } else { 0 } + if sand { 30 } else { 0 };
        assert_eq!(gate.score, Some(expected), "{shape}");
    }
}

#[test]
fn neutral_band_terrains_are_not_counted_for_transitions() {
    let mut values = scores(5);
    values[Terrain::Grass.index()] = -4500;
    values[Terrain::Dirt.index()] = -4000;
    let rules = rules(values);
    let mut tally = TerrainTally::default();
    tally
        .mark(rule(&rules), tile(Terrain::Grass, raw::SHAPE_N_W_BLEND))
        .unwrap();
    // Neither Grass nor Dirt counts; only the occupied bit remains.
    assert_eq!(tally.seen[Terrain::Grass.index()], TerrainTally::OCCUPIED);
    assert_eq!(tally.seen[Terrain::Dirt.index()], 0);
    // A plain frame counts its own terrain unconditionally; with no counted
    // positive terrain, even a single-cell object is then rejected.
    tally
        .mark(rule(&rules), tile(Terrain::Grass, raw::SHAPE_FILL))
        .unwrap();
    assert_eq!(
        tally.gate(rule(&rules), Ruleset::HotA181, 1).unwrap(),
        TerrainGate {
            score: None,
            terrain_ok: false,
        }
    );
}

#[test]
fn forbidden_terrains_reject_multi_cell_objects_and_forbid_single_cells() {
    let mut values = scores(0);
    values[Terrain::Grass.index()] = 50;
    values[Terrain::Dirt.index()] = -4000;
    let rules = rules(values);
    let mut tally = TerrainTally::default();
    tally
        .mark(rule(&rules), tile(Terrain::Grass, raw::SHAPE_FILL))
        .unwrap();
    tally
        .mark(rule(&rules), tile(Terrain::Dirt, raw::SHAPE_FILL))
        .unwrap();
    let gate = tally.gate(rule(&rules), Ruleset::HotA181, 2).unwrap();
    assert_eq!(gate.score, None);
    assert!(!gate.terrain_ok);
    let gate = tally.gate(rule(&rules), Ruleset::HotA181, 1).unwrap();
    // Grass was summed before Dirt forbade the object; later terrains no
    // longer add, but positive occupied Grass keeps it valid.
    assert_eq!(gate.score, Some(FORBIDDEN_SCORE));

    let mut values = scores(0);
    values[Terrain::Grass.index()] = 10;
    values[Terrain::Dirt.index()] = -1011;
    let rules = super::tests::rules(values);
    let mut tally = TerrainTally::default();
    tally
        .mark(rule(&rules), tile(Terrain::Grass, raw::SHAPE_FILL))
        .unwrap();
    tally
        .mark(rule(&rules), tile(Terrain::Dirt, raw::SHAPE_FILL))
        .unwrap();
    // -1001 < -1000: the total, rather than any one terrain, rejects it.
    let gate = tally.gate(rule(&rules), Ruleset::HotA181, 3).unwrap();
    assert_eq!(gate.score, None);
    assert!(!gate.terrain_ok);
}

#[test]
fn a_positive_score_must_come_from_an_occupied_terrain() {
    let mut values = scores(0);
    values[Terrain::Dirt.index()] = 40;
    values[Terrain::Grass.index()] = -3;
    let rules = rules(values);
    let mut tally = TerrainTally::default();
    // Dirt is counted only through Grass's blending edge.
    tally
        .mark(rule(&rules), tile(Terrain::Grass, raw::SHAPE_N_W_BLEND))
        .unwrap();
    let gate = tally.gate(rule(&rules), Ruleset::HotA181, 4).unwrap();
    assert_eq!(gate.score, None);
    assert!(gate.terrain_ok);
    tally
        .mark(rule(&rules), tile(Terrain::Dirt, raw::SHAPE_FILL))
        .unwrap();
    let gate = tally.gate(rule(&rules), Ruleset::HotA181, 4).unwrap();
    assert_eq!(gate.score, Some(37));
}

#[test]
fn decoration_filter_requires_edge_terrains_and_moves_shape_28() {
    let mut values = scores(0);
    values[Terrain::Grass.index()] = -4000;
    let rules = rules(values);
    let grass = |shape| allows_decoration(rule(&rules), tile(Terrain::Grass, shape)).unwrap();
    // Plain frames exclude the neutral band; transition frames only -5000.
    assert!(!grass(raw::SHAPE_FILL));
    assert!(grass(raw::SHAPE_N_W_BLEND));
    assert!(grass(raw::SHAPE_E_S_BLEND_NE_SW_HARD));

    let mut values = scores(0);
    values[Terrain::Sand.index()] = raw::RMG_PLACEMENT_INVALID;
    let rules = super::tests::rules(values);
    let grass = |shape| allows_decoration(rule(&rules), tile(Terrain::Grass, shape)).unwrap();
    assert!(grass(raw::SHAPE_FILL));
    assert!(grass(raw::SHAPE_N_W_BLEND));
    assert!(!grass(raw::SHAPE_N_W_HARD));
    assert!(!grass(raw::SHAPE_E_BLEND_SW_HARD));
    // Shape 28 joins the mixed family here, unlike the seen-terrain switch.
    assert!(!grass(raw::SHAPE_E_S_BLEND_NE_SW_HARD));
}

#[test]
fn same_prototype_distance_keeps_native_gap_asymmetry() {
    let mut bounds = BlockedBounds::default();
    bounds.include(Point::new(10, 10));
    bounds.include(Point::new(11, 10));
    let at = |target: Point| {
        bounds
            .same_prototype_distance(36, |cell| Ok::<_, ()>(cell == target))
            .unwrap()
    };
    // Blocked box is [10, 12) x [10, 11).
    assert_eq!(at(Point::new(9, 9)), Some(1));
    assert_eq!(at(Point::new(12, 11)), Some(1));
    assert_eq!(at(Point::new(8, 10)), Some(2));
    assert_eq!(at(Point::new(13, 10)), Some(1));
    assert_eq!(at(Point::new(11, 8)), Some(2));
    assert_eq!(at(Point::new(11, 12)), Some(1));
    // Diagonals take the smaller axis gap.
    assert_eq!(at(Point::new(5, 8)), Some(2));
    assert_eq!(at(Point::new(17, 16)), Some(5));
    // The scan reaches seven cells beyond the box and no further.
    assert_eq!(at(Point::new(18, 10)), Some(6));
    assert_eq!(at(Point::new(19, 10)), None);
    assert_eq!(at(Point::new(3, 10)), Some(7));
    assert_eq!(at(Point::new(2, 10)), None);
    // An empty box scans nothing.
    assert_eq!(
        BlockedBounds::default()
            .same_prototype_distance(36, |_| Ok::<_, ()>(true))
            .unwrap(),
        None
    );
}

#[test]
fn distance_scan_is_clamped_to_the_map() {
    let mut bounds = BlockedBounds::default();
    bounds.include(Point::new(0, 0));
    let mut visited = Vec::new();
    bounds
        .same_prototype_distance(4, |cell| {
            visited.push(cell);
            Ok::<_, ()>(false)
        })
        .unwrap();
    assert_eq!(visited.len(), 16);
    assert_eq!(visited.first(), Some(&Point::new(0, 0)));
    assert_eq!(visited.last(), Some(&Point::new(3, 3)));
    assert_eq!(clamp_rectangle(4, 5, 1, 9, 3), (5, 1, 5, 1));
}

#[test]
fn finishing_adds_blocked_cells_and_spacing_penalty() {
    assert_eq!(finish(100, false, 4, None).unwrap(), 103);
    assert_eq!(finish(100, true, 4, None).unwrap(), 100);
    assert_eq!(finish(100, false, 0, Some(1)).unwrap(), 100 - 9 - 140);
    assert_eq!(finish(100, true, 0, Some(7)).unwrap(), 80);
    assert_eq!(finish(100, true, 0, Some(8)).unwrap(), 100);
    assert_eq!(
        finish(FORBIDDEN_SCORE, false, 4, Some(1)).unwrap(),
        FORBIDDEN_SCORE
    );
    assert!(matches!(
        finish(i32::MAX - 20, false, 10, None),
        Err(PlacementError::Arithmetic)
    ));
}
