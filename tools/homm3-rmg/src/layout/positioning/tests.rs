use super::*;
use crate::{
    behavior::Behavior,
    geometry::ZoneId,
    layout::{can_place, LayoutWorkspace},
    request::{default_record, MapSize, Town},
    rng::RetailRng,
    template::{TemplateCandidate, TemplateSource},
};

fn policies(rules: Ruleset, levels: Levels) -> Positioning {
    Positioning {
        rules,
        levels,
        map_size: 100,
    }
}

// Synthetic 1.8.1 pack; values intentionally distinguish circle sizes,
// enclosing radii, explicit level restrictions and repulsion.
fn pack(placement: &str, repulsion: bool, connection: &str) -> Vec<u8> {
    let mut text = String::from("Pack\r\nsections\r\ncolumns\r\n");
    for index in 0..3 {
        let mut row = vec![String::new(); 140];
        if index == 0 {
            for (column, value) in [12, 10, 4, 8, 10, 18, 4].into_iter().enumerate() {
                row[column] = value.to_string();
            }
            row[15] = "positioning".into();
            row[16] = "1".into();
            row[17] = "32".into();
        }
        for (column, value) in [
            (28, index + 1),
            (33, 10 * (index + 1)),
            (35, 8),
            (37, 8),
            (38, 1),
        ] {
            row[column] = value.to_string();
        }
        row[29] = "x".into(); // Human zone, to exercise the underground-town rule.
        row[48] = "x".into(); // All towns.
        row[75] = "x".into(); // Dirt.
        row[109] = placement.into();
        if repulsion {
            row[115] = "x".into();
        }
        if index < 2 {
            row[127] = (index + 1).to_string();
            row[128] = (index + 2).to_string();
            row[133] = connection.into();
            row[137] = "8".into();
            row[139] = "8".into();
        }
        text.push_str(&row.join("\t"));
        text.push_str("\r\n");
    }
    text.into_bytes()
}

fn with_template(
    placement: &str,
    repulsion: bool,
    connection: &str,
    f: impl FnOnce(&Template<'_>),
) {
    let bytes = pack(placement, repulsion, connection);
    let request = Request::parse(
        default_record(MapSize::Large, Levels::Underground),
        Behavior::Hotfix,
    )
    .unwrap();
    let candidates = TemplateSource::parse(&bytes)
        .unwrap()
        .prepare_for(&request, Water::None, Ruleset::HotA181)
        .unwrap();
    let TemplateCandidate::Ready(template) = &candidates[0] else {
        panic!("synthetic pack")
    };
    assert_eq!(template.zones().len(), 3);
    assert_eq!(template.zones()[0].options().zone_repulsion, repulsion);
    f(template);
}

#[test]
fn connection_preferences_and_candidate_seeding_are_distinct() {
    use ConnectionKind::{Automatic, Ground, Random, Teleport, Underground};
    for levels in [Levels::Surface, Levels::Underground] {
        let complete = policies(Ruleset::Complete, levels);
        let hota = policies(Ruleset::HotA181, levels);
        for (kind, same, other, seeds) in [
            (Automatic, 1, 1, true),
            (Ground, 101, 0, true),
            (
                Underground,
                usize::from(levels == Levels::Surface),
                101,
                true,
            ),
            (Teleport, 0, 0, false),
            (Random, 0, 0, false),
        ] {
            assert_eq!(hota.connection_weight(kind, true), same);
            assert_eq!(hota.connection_weight(kind, false), other);
            assert_eq!(hota.seeds_candidates(kind), seeds);
            assert_eq!(complete.connection_weight(kind, true), 1);
            assert_eq!(complete.connection_weight(kind, false), 1);
            assert!(complete.seeds_candidates(kind));
        }
    }
}

#[test]
fn spacing_uses_sizes_planes_islands_and_sparseness() {
    for (levels, water, multiplier, expected) in [
        (Levels::Surface, Water::None, 1.0, 114),
        (Levels::Surface, Water::Normal, 1.0, 114),
        (Levels::Underground, Water::None, 1.0, 92),
        (Levels::Surface, Water::Islands, 0.75, 171),
        (Levels::Underground, Water::Islands, 0.75, 138),
    ] {
        assert_eq!(
            spacing([10, 20, 30].into_iter(), levels, water, multiplier).unwrap(),
            expected
        );
    }
    // Squaring before widening matters: 65536^2 is zero in the native DWORD.
    assert_eq!(
        spacing([65536].into_iter(), Levels::Surface, Water::None, 1.0).unwrap(),
        131072
    );
    assert!(spacing([10].into_iter(), Levels::Surface, Water::None, f64::NAN).is_err());
}

#[test]
fn origin_uses_integer_centroid_then_source_ordered_clipping() {
    let zones = [(Point::new(0, 0), 10), (Point::new(100, 0), 20)];
    assert_eq!(
        weighted_origin(zones.into_iter(), 1000).unwrap(),
        Point::new(-420, -500)
    );
    let opposing = [(Point::new(-100, 0), 10), (Point::new(100, 0), 10)];
    assert_eq!(
        weighted_origin(opposing.into_iter(), 100).unwrap(),
        Point::new(9, -50)
    );
    assert_eq!(
        weighted_origin(opposing.into_iter().rev(), 100).unwrap(),
        Point::new(-108, -50)
    );
}

#[test]
fn explicit_placement_replaces_the_player_alignment_rule_only_with_two_planes() {
    with_template("underground", false, "ground", |template| {
        let zones = template.zones();
        assert_eq!(zones[0].options().placement, Some(Level::Underground));
        let mut current = PositionedZone {
            id: zones[0].id(),
            alignment: Some(Town::parse(0).unwrap()),
            position: WorldPosition {
                point: Point::default(),
                level: Level::Underground,
            },
        };
        assert!(can_place(
            current,
            &[],
            zones,
            policies(Ruleset::HotA181, Levels::Underground)
        )
        .unwrap());
        assert!(!can_place(
            current,
            &[],
            zones,
            policies(Ruleset::Complete, Levels::Underground)
        )
        .unwrap());
        assert!(!can_place(
            current,
            &[],
            zones,
            policies(Ruleset::HotA181, Levels::Surface)
        )
        .unwrap());
        current.position.level = Level::Surface;
        assert!(!can_place(
            current,
            &[],
            zones,
            policies(Ruleset::HotA181, Levels::Underground)
        )
        .unwrap());
        assert!(can_place(
            current,
            &[],
            zones,
            policies(Ruleset::HotA181, Levels::Surface)
        )
        .unwrap());
    });
}

#[test]
fn repulsion_keeps_the_three_unit_tolerance_in_candidate_order() {
    with_template("", true, "teleport", |template| {
        let zone = |index, x| PositionedZone {
            id: ZoneId::new(index),
            alignment: None,
            position: WorldPosition {
                point: Point::new(x, 0),
                level: Level::Surface,
            },
        };
        let current = zone(0, 0);
        let placed = [current, zone(1, 0)];
        let mut candidates: Vec<_> = [97, 100, 96, 99].map(|x| zone(0, x).position).into();
        let hota = policies(Ruleset::HotA181, Levels::Underground);
        hota.repel(&current, &placed, template.zones(), &mut candidates)
            .unwrap();
        assert_eq!(
            candidates.iter().map(|c| c.point.x).collect::<Vec<_>>(),
            [97, 100, 99]
        );
        // Separation is squared only for the additional cross-plane term.
        let other = WorldPosition {
            point: Point::new(3, 4),
            level: Level::Underground,
        };
        assert_eq!(
            repulsion_score(current.position, 20, [(other, 30)].into_iter()).unwrap(),
            10005
        );
    });
}

#[test]
fn first_surface_candidate_still_bypasses_explicit_placement() {
    with_template("underground", false, "", |template| {
        let mut workspace = LayoutWorkspace::default();
        let mut current = PositionedZone {
            id: ZoneId::new(0),
            alignment: Some(Town::parse(0).unwrap()),
            position: WorldPosition {
                point: Point::default(),
                level: Level::Surface,
            },
        };
        let mut rng = RetailRng::new(0); // First draw 38 selects surface from the two candidates.
        workspace
            .position(
                &mut current,
                template.zones(),
                policies(Ruleset::HotA181, Levels::Underground),
                false,
                &mut rng,
            )
            .unwrap();
        assert_eq!(current.position.level, Level::Surface);
        assert_eq!(rng.draws(), 1);
        assert_eq!(workspace.candidates.len(), 2);
    });
}

// DLL 0x1be611 keeps the accumulated coordinates when signed, wrapped
// square weights cancel. The recovered C++ approximation loses this case.
#[test]
fn zero_weight_keeps_the_native_accumulated_coordinates() {
    let zones = [
        (Point::new(1, 0), 65535), // square wraps to -131071
        (Point::default(), 362),   // 131044
        (Point::default(), 5),
        (Point::default(), 1),
        (Point::default(), 1),
    ];
    assert_eq!(
        weighted_origin(zones.into_iter(), 1_000_000).unwrap(),
        Point::new(-631071, -500000)
    );
}
