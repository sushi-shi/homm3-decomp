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
