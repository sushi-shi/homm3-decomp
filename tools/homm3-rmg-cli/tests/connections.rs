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
