//! Native scratch geometry without guard assembly or world completion.
use super::treasure_factories::{row, tag, write_reward};
use homm3_rmg::{
    domain::Terrain,
    geometry::Point,
    hero::HeroId,
    placement::{
        KeyTentColor, KeyTentCursor, ObjectArena, ObjectId, ObjectPayload, PathReservation,
        PlacementError, TreasureGeneration, TreasureGenerationError, TreasureGroupWorkspace,
        TreasuresReady,
    },
    raw,
    rng::RetailRng,
    traits::{ArtifactCatalog, ArtifactId, SpellCatalog},
    treasure::DefinitionId,
};
use homm3_rmg_cli::resources::Installation;
use std::{fmt::Write, path::PathBuf};

pub fn snapshot(
    ready: TreasuresReady<'_>,
    objects: &mut ObjectArena,
    rng: &RetailRng,
    colors: &[KeyTentColor],
) -> String {
    let mut installation =
        Installation::open(&PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap())).unwrap();
    let mut bytes = Vec::new();
    installation.text("sptraits.txt", &mut bytes).unwrap();
    let spells = SpellCatalog::parse(&bytes).unwrap();
    installation.text("artraits.txt", &mut bytes).unwrap();
    let artifacts = ArtifactCatalog::parse(&bytes).unwrap();
    let mut generation = ready
        .begin_generation(objects, &spells, &artifacts)
        .unwrap();
    let mut rng = rng.clone();
    let mut group = TreasureGroupWorkspace::default();
    let zone = generation.ready().map().coverage().map().zones()[0];
    let ids: Vec<_> = generation.ready().catalog().ids().collect();
    let mut text = format!(
        "registry 0\ngeometry {} 12 {}\n",
        ids.len(),
        zone.terrain().index()
    );
    state(&mut text, "initial", &generation, &rng, colors);
    let mut world_counts = String::new();
    super::placement_snapshot::write_counts(&mut world_counts, generation.ready().map());
    let mut fits = [0; 2];
    for (batch, definitions) in ids.chunks(12).enumerate() {
        writeln!(text, "batch {batch}").unwrap();
        let mut members = Vec::new();
        for &definition in definitions {
            generate(
                &mut text,
                &mut generation,
                &mut group,
                objects,
                &mut rng,
                definition,
                &mut members,
                &mut fits,
            );
            state(&mut text, "step", &generation, &rng, colors);
        }
        if group.objects().next().is_some() {
            group.prepare_placement().unwrap();
        }
        state(&mut text, "state", &generation, &rng, colors);
        write_group(&mut text, &generation, &group, objects, &members);
        let checkpoint = rng.checkpoint();
        let mut foreign = ObjectArena::default();
        assert!(matches!(
            generation.discard_group(&mut group, &mut foreign),
            Err(TreasureGenerationError::Placement(
                PlacementError::ArenaContext
            ))
        ));
        assert!(group
            .objects()
            .eq(members.iter().map(|&(object, _)| object)));
        generation.discard_group(&mut group, objects).unwrap();
        assert_eq!(rng.checkpoint(), checkpoint);
        assert!(members
            .iter()
            .all(|&(object, _)| objects.get(object).is_none()));
        assert_reset(&group);
        let mut after_counts = String::new();
        super::placement_snapshot::write_counts(&mut after_counts, generation.ready().map());
        assert_eq!(world_counts, after_counts);
        state(&mut text, "cleanup", &generation, &rng, colors);
        text.push_str("checks 0 0 0\n");
    }
    assert!(fits.into_iter().all(|n| n > 0));
    eprintln!("treasure fits rejected/accepted: {fits:?}");
    text
}
#[expect(
    clippy::too_many_arguments,
    reason = "isolated native checkpoint state"
)]
fn generate(
    text: &mut String,
    generation: &mut TreasureGeneration<'_>,
    group: &mut TreasureGroupWorkspace,
    objects: &mut ObjectArena,
    rng: &mut RetailRng,
    definition: DefinitionId,
    members: &mut Vec<(ObjectId, DefinitionId)>,
    fits: &mut [usize; 2],
) {
    let zone = generation.ready().map().coverage().map().zones()[0].id();
    let selected = generation.choose_prototype(definition, zone, rng).unwrap();
    writeln!(
        text,
        "generate {} {} {} {} {}",
        definition.index(),
        definition.index(),
        selected.map_or(-1, |s| row(generation, s.prototype())),
        rng.state(),
        generation.ready().map().next_object_id()
    )
    .unwrap();
    let pending =
        selected.and_then(|selected| generation.generate(selected, objects, rng).unwrap());
    let Some(pending) = pending else {
        text.push_str("null\n");
        return;
    };
    let object = pending.object();
    let centered = group.objects().next().is_none();
    let rejected = if centered {
        generation
            .add_centered_to_group(group, pending, objects)
            .unwrap();
        None
    } else {
        generation
            .try_add_to_group(group, pending, objects, rng)
            .unwrap()
    };
    fits[usize::from(rejected.is_none())] += 1;
    writeln!(
        text,
        "fit {} {}",
        u8::from(centered),
        u8::from(rejected.is_none())
    )
    .unwrap();
    write_object(
        text,
        generation,
        objects,
        object,
        definition,
        definition.index(),
    );
    if let Some(pending) = rejected {
        generation.discard(pending, objects).unwrap();
    } else {
        members.push((object, definition));
    }
}
fn write_object(
    text: &mut String,
    generation: &TreasureGeneration<'_>,
    objects: &ObjectArena,
    object: ObjectId,
    definition: DefinitionId,
    ordinal: usize,
) {
    let geometry = objects.get(object).unwrap();
    let (x, y, z) = geometry.position().map_or((-1, -1, -1), |p| {
        (
            p.point.x,
            p.point.y,
            i32::try_from(p.level.index()).unwrap(),
        )
    });
    writeln!(
        text,
        "object {ordinal} {} {} {} {x} {y} {z}",
        tag(generation
            .ready()
            .catalog()
            .get(definition)
            .unwrap()
            .reward()),
        definition.index(),
        row(generation, geometry.prototype())
    )
    .unwrap();
    text.push_str("marks 0 0 0 0 0\n");
    write_reward(text, generation, objects, object, true);
    // Geometry's native groupPrintObject emits a marker even without extra
    // payload fields. The separate factory checkpoint intentionally omits it.
    if matches!(
        objects.payload(object).unwrap(),
        ObjectPayload::Base
            | ObjectPayload::Artifact
            | ObjectPayload::Ownable
            | ObjectPayload::Resource
            | ObjectPayload::Scholar
            | ObjectPayload::Shrine
            | ObjectPayload::WitchHut
    ) {
        text.push_str("unit\n");
    }
}
fn write_group(
    text: &mut String,
    generation: &TreasureGeneration<'_>,
    group: &TreasureGroupWorkspace,
    objects: &ObjectArena,
    members: &[(ObjectId, DefinitionId)],
) {
    writeln!(
        text,
        "group {} 0 {}",
        members.len(),
        u8::from(!members.is_empty())
    )
    .unwrap();
    if let Some(bounds) = group.bounds() {
        writeln!(
            text,
            "bounds 1 {} {} {} {}",
            bounds.minimum().x,
            bounds.minimum().y,
            bounds.maximum().x,
            bounds.maximum().y
        )
        .unwrap();
    } else {
        text.push_str("bounds 0\n");
    }
    write!(text, "outline {}", group.outline().len()).unwrap();
    for point in group.outline() {
        write!(text, " {} {}", point.x, point.y).unwrap();
    }
    text.push('\n');
    for (ordinal, &(object, definition)) in members.iter().enumerate() {
        write_object(text, generation, objects, object, definition, ordinal);
    }
    let side = i32::try_from(raw::RMG_TREASURE_GROUP_MAP_SIZE).unwrap();
    for index in 0..side * side {
        let point = Point::new(index % side, index / side);
        let cell = group.cell(point).unwrap();
        write!(
            text,
            "cell {index} {} 0 {} {} {} {} {} -1 {} {} {} {} {} {}",
            Terrain::Dirt.index(),
            u8::from(cell.passable()),
            cell.entrance()
                .map_or(-1, |k| i32::try_from(k.index()).unwrap()),
            u8::from(cell.reservation() == PathReservation::Obstacle),
            u8::from(cell.reservation() == PathReservation::Open),
            u8::from(group.is_outline(point).unwrap()),
            cell.object_distance(),
            cell.movement().cost(),
            cell.zone_distance().cost(),
            cell.zone_distance()
                .connection()
                .map_or(-1, |(z, _)| i32::try_from(z.index()).unwrap()),
            cell.zone_distance().direction().index(),
            group.objects_at(point).unwrap().count()
        )
        .unwrap();
        for object in group.objects_at(point).unwrap() {
            write!(
                text,
                " {}",
                members.iter().position(|&(id, _)| id == object).unwrap()
            )
            .unwrap();
        }
        text.push('\n');
    }
}
fn state(
    text: &mut String,
    label: &str,
    generation: &TreasureGeneration<'_>,
    rng: &RetailRng,
    colors: &[KeyTentColor],
) {
    let ready = generation.ready();
    let KeyTentCursor::Value(cursor) = ready.map().key_tent_cursor() else {
        panic!("replayed cursor")
    };
    write!(
        text,
        "{label} {} {} {} {} {cursor} ",
        rng.state(),
        ready.map().next_object_id(),
        ready.next_seer_ordinal(),
        u8::from(ready.quest_pool_low())
    )
    .unwrap();
    for hero in 0..i32::try_from(raw::RMG_HERO_COUNT).unwrap() {
        text.push(if generation.hero_disabled(HeroId::parse(hero).unwrap()) {
            '1'
        } else {
            '0'
        });
    }
    text.push(' ');
    for artifact in 0..raw::ARTIFACT_COUNT {
        text.push(
            if ready.quest_artifact_used(ArtifactId::parse(artifact).unwrap()) {
                '1'
            } else {
                '0'
            },
        );
    }
    text.push(' ');
    for &color in colors {
        text.push(if ready.map().key_tent_disabled(color).unwrap() {
            '1'
        } else {
            '0'
        });
    }
    text.push('\n');
}
fn assert_reset(group: &TreasureGroupWorkspace) {
    assert!(group.objects().next().is_none());
    assert!(group.outline().is_empty());
    let side = i32::try_from(raw::RMG_TREASURE_GROUP_MAP_SIZE).unwrap();
    for y in 0..side {
        for x in 0..side {
            let point = Point::new(x, y);
            let cell = group.cell(point).unwrap();
            assert!(cell.passable());
            assert!(cell.entrance().is_none());
            assert_eq!(cell.reservation(), PathReservation::Open);
            assert!(!group.is_outline(point).unwrap());
            assert_eq!(group.objects_at(point).unwrap().count(), 0);
        }
    }
}
