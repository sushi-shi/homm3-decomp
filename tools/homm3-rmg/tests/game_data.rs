//! Installed game data checks; game assets remain outside the repository.

use homm3_resource::hdat::Container;
use homm3_rmg::treasure::{ObjectRecipe, ObjectRecipes};

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
