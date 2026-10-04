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
