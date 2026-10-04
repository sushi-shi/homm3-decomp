//! Native ordered ground crossings and per-edge shipyard fallback.
#[path = "support/connections.rs"]
mod connections;
#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE ground checkpoints"]
fn native_ground_crossings_preserve_guards_entrances_cells_and_rng() {
    connections::compare_native(connections::Attempts::Ground);
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE shipyard checkpoints"]
fn native_shipyard_fallback_preserves_water_visits_entrances_and_rng() {
    connections::compare_native(connections::Attempts::GroundAndShipyard);
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE direct connection checkpoints"]
fn native_first_pass_preserves_ground_shipyard_and_gate_dispatch() {
    connections::compare_native(connections::Attempts::Direct);
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE complete connection checkpoints"]
fn native_both_passes_preserve_portal_lists_protection_and_rng() {
    connections::compare_native(connections::Attempts::BothPasses);
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE junction checkpoints"]
fn native_junctions_preserve_routes_reservations_movement_and_rng() {
    connections::compare_native(connections::Attempts::Junctions);
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE mine checkpoints"]
fn native_mines_preserve_sites_resources_guards_and_rng() {
    connections::compare_native(connections::Attempts::Mines);
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE post-mine path checkpoints"]
fn native_treasure_paths_preserve_faction_counts_border_opening_and_rng() {
    connections::compare_native(connections::Attempts::TreasurePaths);
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE treasure value checkpoints"]
fn native_treasure_values_preserve_factions_ordinals_and_rng() {
    connections::compare_native(connections::Attempts::TreasureValues);
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE treasure factory checkpoints"]
fn native_treasure_factories_preserve_payloads_reservations_ids_and_rng() {
    connections::compare_native(connections::Attempts::TreasureFactories);
}
