//! Installed game data checks; game assets remain outside the repository.

use homm3_resource::hdat::Container;
use homm3_rmg::treasure::{ObjectRecipe, ObjectRecipes};
use homm3_rmg::{rules::Ruleset, traits::CreatureCatalog};

#[test]
#[ignore = "requires HOMM3_HOTA_DAT and HOMM3_RMG_CREATURES from the pinned HotA installation"]
fn installed_creature_catalog_loads_expanded_factions_and_native_numeric_fields() {
    let bytes = std::fs::read(std::env::var_os("HOMM3_HOTA_DAT").unwrap()).unwrap();
    let base = std::fs::read(std::env::var_os("HOMM3_RMG_CREATURES").unwrap()).unwrap();
    let catalog = CreatureCatalog::parse_hota181(&base, Container::parse(&bytes).unwrap()).unwrap();
    assert_eq!(catalog.ruleset(), Ruleset::HotA181);
    assert_eq!(catalog.entries().len(), 200);
    for (id, faction, tier, ai, growth, wandering) in [
        (151, 9, 2, 602, 7, (12, 20)),
        (185, 10, 6, 6433, 1, (3, 8)),
        (199, 11, 6, 6694, 1, (3, 8)),
    ] {
        let row = catalog.get(catalog.id(id).unwrap()).unwrap();
        assert_eq!(row.town().unwrap().index(), faction);
        assert_eq!(row.tier().unwrap().index(), tier);
        assert_eq!(row.ai_value().unwrap().get(), ai);
        assert_eq!(row.growth(), growth);
        assert_eq!(row.wandering_counts(), wandering);
    }
    assert_eq!(catalog.entries()[138].town().unwrap().index(), 10);
    assert!(catalog.entries()[152].tier().is_none());
    assert!(catalog.entries()[167].town().is_none());
    assert!(catalog.id(200).is_none());
}

#[test]
#[ignore = "requires HOMM3_HOTA_DAT pointing to the pinned HotA 1.8.1 data file"]
fn installed_object_recipes_match_recovered_native_lists() {
    let bytes = std::fs::read(std::env::var_os("HOMM3_HOTA_DAT").unwrap()).unwrap();
    let recipes = ObjectRecipes::parse(Container::parse(&bytes).unwrap()).unwrap();
    assert_eq!(recipes.additional().len(), 42);
    assert_eq!(recipes.defaults().len(), 13);
    assert_eq!(
        recipes.additional()[0],
        ObjectRecipe {
            object_type: 213,
            subtype: 0,
            value: 100,
            density: 50,
            maximum_per_map: -1,
            maximum_per_zone: -1,
        }
    );
    assert_eq!(recipes.additional().last().unwrap().subtype, 12);
    assert!(recipes.additional().contains(&ObjectRecipe {
        object_type: 145,
        subtype: 1,
        value: 500,
        density: 400,
        maximum_per_map: -1,
        maximum_per_zone: -1,
    }));
    assert!(recipes.additional().contains(&ObjectRecipe {
        object_type: 88,
        subtype: 3,
        value: 7000,
        density: 100,
        maximum_per_map: 32,
        maximum_per_zone: 1,
    }));
    assert_eq!(
        recipes.defaults()[11],
        ObjectRecipe {
            object_type: 145,
            subtype: 0,
            value: 5000,
            density: 200,
            maximum_per_map: -1,
            maximum_per_zone: -1,
        }
    );
    assert_eq!(
        recipes.defaults()[12],
        ObjectRecipe {
            object_type: 146,
            subtype: 4,
            value: 20000,
            density: 20,
            maximum_per_map: -1,
            maximum_per_zone: 1,
        }
    );
}

#[test]
#[ignore = "requires HOMM3_HOTA_DAT pointing to the pinned HotA 1.8.1 data file"]
fn installed_heroes_have_expanded_classes_and_water_replacements() {
    use homm3_rmg::{
        hero::{HeroCatalog, HeroPool},
        request::{MapVersion, Water},
    };
    let bytes = std::fs::read(std::env::var_os("HOMM3_HOTA_DAT").unwrap()).unwrap();
    let catalog = HeroCatalog::parse_hota181(Container::parse(&bytes).unwrap()).unwrap();
    assert_eq!(catalog.entries().len(), 215);
    assert_eq!(catalog.ruleset(), Ruleset::HotA181);
    for (id, class) in [
        (156, 18),
        (171, 19),
        (179, 20),
        (194, 21),
        (198, 22),
        (214, 23),
    ] {
        let hero = catalog.get(catalog.id(id).unwrap()).unwrap();
        assert_eq!(hero.class().index(), class);
        assert!(hero.available(MapVersion::ShadowOfDeath));
    }
    for id in [20, 71, 144, 159, 172, 195, 210] {
        assert!(!catalog
            .get(catalog.id(id).unwrap())
            .unwrap()
            .available(MapVersion::ShadowOfDeath));
    }
    for water in [Water::None, Water::Normal, Water::Islands] {
        let pool =
            HeroPool::prepare(&catalog, MapVersion::ShadowOfDeath, water, &[], 8, &[]).unwrap();
        for id in [3, 122, 174, 214] {
            assert_eq!(
                pool.is_disabled(catalog.id(id).unwrap()),
                water == Water::None
            );
        }
        for id in [175, 176, 159, 210] {
            assert_eq!(
                pool.is_disabled(catalog.id(id).unwrap()),
                water != Water::None
            );
        }
        assert_eq!(pool.remaining_by_class(), &[8; 24]);
        assert_eq!(pool.prison_limit(), Some(64));
    }
}

#[test]
#[ignore = "requires HOMM3_HOTA_DAT and HOMM3_RMG_ARTIFACTS from the pinned HotA installation"]
fn installed_artifacts_load_new_classes_and_all_combination_memberships() {
    use homm3_rmg::traits::{ArtifactCatalog, ArtifactClass};
    let bytes = std::fs::read(std::env::var_os("HOMM3_HOTA_DAT").unwrap()).unwrap();
    let base = std::fs::read(std::env::var_os("HOMM3_RMG_ARTIFACTS").unwrap()).unwrap();
    let catalog = ArtifactCatalog::parse_hota181(&base, Container::parse(&bytes).unwrap()).unwrap();
    assert_eq!(catalog.ruleset(), Ruleset::HotA181);
    assert_eq!(catalog.entries().len(), 166);
    for (id, combo) in [(141, 12), (142, 13), (160, 14), (143, 15)] {
        let row = catalog.get(catalog.id(id).unwrap()).unwrap();
        assert_eq!(row.combination().unwrap().index(), combo);
        assert!(!row.disabled());
    }
    for (id, combo) in [(66, 12), (57, 13), (115, 14), (10, 15)] {
        assert_eq!(
            catalog
                .get(catalog.id(id).unwrap())
                .unwrap()
                .component_of()
                .unwrap()
                .index(),
            combo
        );
    }
    for id in [144, 145] {
        let row = catalog.get(catalog.id(id).unwrap()).unwrap();
        assert_eq!(row.class(), ArtifactClass::Unused);
        assert!(row.disabled());
        assert!(row.combination().is_none());
    }
    let last = catalog.get(catalog.id(165).unwrap()).unwrap();
    assert_eq!(last.class(), ArtifactClass::Relic);
    assert!(!last.disabled());
    assert!(catalog
        .get(catalog.id(152).unwrap())
        .unwrap()
        .quest_eligible());
}

#[test]
#[ignore = "requires HOMM3_HOTA_DAT and HOMM3_RMG_ARTIFACTS from the pinned HotA installation"]
fn installed_quest_pool_respects_template_combination_bans() {
    use homm3_rmg::{
        artifact::ArtifactPool, request::Water, rng::RetailRng, template::MapOptions,
        traits::ArtifactCatalog,
    };
    let bytes = std::fs::read(std::env::var_os("HOMM3_HOTA_DAT").unwrap()).unwrap();
    let base = std::fs::read(std::env::var_os("HOMM3_RMG_ARTIFACTS").unwrap()).unwrap();
    let catalog = ArtifactCatalog::parse_hota181(&base, Container::parse(&bytes).unwrap()).unwrap();
    let exhaust = |mut pool: ArtifactPool<'_>| {
        let mut seen = Vec::new();
        let mut rng = RetailRng::new(42);
        while let Some(artifact) = pool.select_quest(&mut rng).artifact {
            assert!(!seen.contains(&artifact.index()));
            seen.push(artifact.index());
            assert!(pool.exclude(artifact));
        }
        assert_eq!(rng.draws(), u64::try_from(seen.len()).unwrap());
        seen
    };
    for water in [Water::None, Water::Normal, Water::Islands] {
        let default_pool = ArtifactPool::new(&catalog, water);
        let available = exhaust(default_pool.clone());
        assert!(available.contains(&54)); // Amulet of the Undertaker: Cloak recipe is banned.
        assert!(available.contains(&152)); // Expanded treasure class, no permitted combination.
        assert!(available.contains(&153));
        assert!(!available.contains(&37)); // Quiet Eye: Power of the Dragon Father is permitted.
        let mut changed = default_pool;
        changed
            .apply_template(&MapOptions {
                combination_artifacts: Some(b"-5 +1".to_vec()),
                ..MapOptions::default()
            })
            .unwrap();
        let available = exhaust(changed);
        assert!(available.contains(&37));
        assert!(!available.contains(&54));
    }
}

#[test]
#[ignore = "requires HOMM3_HOTA_DAT and HOMM3_HOTA_TERRAIN_PATTERNS (992 bytes at pinned DLL RVA 0x25f860)"]
fn installed_terrain_catalog_admits_native_frames_and_expanded_transitions() {
    use homm3_rmg::{
        domain::Terrain,
        line::Reflection,
        rng::RetailRng,
        terrain_rules::{BrushStrength, TerrainCatalog, TerrainShape, TerrainTransition},
    };
    let bytes = std::fs::read(std::env::var_os("HOMM3_HOTA_DAT").unwrap()).unwrap();
    let patterns = std::fs::read(std::env::var_os("HOMM3_HOTA_TERRAIN_PATTERNS").unwrap()).unwrap();
    let catalog =
        TerrainCatalog::parse_hota181(Container::parse(&bytes).unwrap(), &patterns).unwrap();
    assert_eq!(catalog.ruleset(), Ruleset::HotA181);
    for terrain in [Terrain::Highlands, Terrain::Wasteland] {
        assert_eq!(catalog.frame_count(terrain), Some(124));
        for frame in 0..124 {
            let tile = catalog
                .parse_tile(terrain, frame, Reflection::default())
                .unwrap();
            assert_eq!(tile.is_special(), (102..118).contains(&frame));
        }
        assert!(catalog
            .parse_tile(terrain, 124, Reflection::default())
            .is_err());
        let mut rng = RetailRng::new(1);
        let fill = catalog
            .select_base(
                terrain,
                BrushStrength::parse(8).unwrap(),
                None,
                &[],
                &mut rng,
            )
            .unwrap();
        assert_eq!(fill.frame(), 105); // 41 < 50; 102 + 18467 % 16.
        assert_eq!(rng.draws(), 2);
        let frames: Vec<_> = [6, 7, 8, 9, 10, 11, 12, 12]
            .into_iter()
            .map(|frame| {
                catalog
                    .parse_tile(terrain, frame, Reflection::default())
                    .unwrap()
            })
            .collect();
        let transition = TerrainTransition {
            shape: TerrainShape::WestBlend,
            reflection: Reflection {
                flip_x: true,
                flip_y: false,
            },
        };
        let selected = catalog
            .select_transition(terrain, transition, None, &frames, &mut rng)
            .unwrap();
        assert_eq!(selected.frame(), 13); // The last unused variant of this edge.
        assert_eq!(selected.reflection(), transition.reflection);
        assert_eq!(rng.draws(), 3);
    }
    for index in 0..10 {
        let terrain = Terrain::parse(index).unwrap();
        assert_eq!(
            catalog.frame_count(terrain),
            TerrainCatalog::complete().frame_count(terrain)
        );
    }
}
