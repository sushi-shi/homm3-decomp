use super::*;

fn domain<const N: usize>(values: &[usize]) -> [bool; N] {
    std::array::from_fn(|index| values.contains(&index))
}

#[test]
fn independent_mt_stream_crosses_refill_boundaries() {
    // Checked against std::mt19937 independently of the recovered implementation.
    for (seed, expected) in [
        (
            0,
            [
                2_357_136_044,
                2_546_248_239,
                3_071_714_933,
                3_791_854_820,
                341_544_762,
                1_145_454_359,
                4_192_857_288,
            ],
        ),
        (
            5489,
            [
                3_499_211_612,
                581_869_302,
                3_890_346_734,
                4_020_325_887,
                4_178_893_912,
                2_538_210_759,
                358_555_951,
            ],
        ),
        (
            u32::MAX,
            [
                419_326_371,
                479_346_978,
                3_918_654_476,
                1_027_084_080,
                3_860_652_269,
                3_512_076_445,
                2_400_582_258,
            ],
        ),
    ] {
        let mut random = random::Random::new(seed);
        let mut observed = Vec::new();
        for index in 0..=1248 {
            let value = random.next();
            if [0, 1, 2, 623, 624, 1247, 1248].contains(&index) {
                observed.push(value);
            }
        }
        assert_eq!(observed, expected);
        assert_eq!(random.draws, 1249);
    }
    let mut random = random::Random::new(7);
    random.shuffle(&mut []);
    random.shuffle(&mut [1]);
    assert_eq!(random.draws, 0);
}

#[test]
fn matching_terrain_does_not_merge_distinct_town_types() {
    let mut problem = Problem::new().unwrap();
    let a = problem.add_town(domain(&[0, 1]), None).unwrap();
    let b = problem.add_town(domain(&[0, 1]), None).unwrap();
    let ground = problem.add_terrain(domain(&[2]));
    problem
        .relate_town_terrain(a, ground, Relation::Same)
        .unwrap();
    problem
        .relate_town_terrain(b, ground, Relation::Same)
        .unwrap();
    problem.relate_towns(a, b, Relation::Different).unwrap();
    let mut reads = Vec::new();
    let mut solution = problem
        .solve(42, |read| {
            reads.push(read);
            Some(0)
        })
        .unwrap();
    // Native source-order merge absorbs only the first town matching the terrain.
    assert!(solution.issues().is_empty());
    assert_eq!(solution.town(a).unwrap(), Some(0));
    assert_eq!(solution.town(b).unwrap(), Some(1));
    assert_eq!(solution.terrain(ground).unwrap(), Some(2));
    assert_eq!(solution.draws(), Some(2));
    assert_eq!(solution.random.as_mut().unwrap().next(), 4_083_286_876);
    assert_eq!(
        reads,
        [ClockRead {
            component: 0,
            steps: 0
        }]
    );
}

#[test]
fn components_preserve_native_order_and_keep_searching_after_failure() {
    let mut problem = Problem::new().unwrap();
    let a = problem.add_town(domain(&[0]), None).unwrap();
    let b = problem.add_town(domain(&[1]), None).unwrap();
    let independent = problem.add_town(domain(&[0, 1]), None).unwrap();
    let ground = problem.add_terrain([true; 10]);
    problem.relate_towns(a, b, Relation::Same).unwrap();
    let mut reads = Vec::new();
    let mut solution = problem
        .solve(42, |read| {
            reads.push(read);
            Some(0)
        })
        .unwrap();
    assert_eq!(solution.issues(), [Issue::NoSolution]);
    assert_eq!(solution.town(a).unwrap(), Some(0));
    assert_eq!(solution.town(b).unwrap(), Some(0));
    assert_eq!(solution.town(independent).unwrap(), Some(1));
    assert_eq!(solution.terrain(ground).unwrap(), Some(0));
    assert_eq!(solution.draws(), Some(10));
    assert_eq!(solution.random.as_mut().unwrap().next(), 669_991_378);
    // Empty-domain component exits before its first clock read.
    assert_eq!(
        reads,
        [
            ClockRead {
                component: 1,
                steps: 0
            },
            ClockRead {
                component: 2,
                steps: 0
            }
        ]
    );
}

#[test]
fn graph_merging_order_controls_both_values_and_shuffle_draws() {
    let mut problem = Problem::new().unwrap();
    let towns: Vec<_> = (0..3)
        .map(|_| problem.add_town([true; 12], None).unwrap())
        .collect();
    let terrain: Vec<_> = (0..3).map(|_| problem.add_terrain([true; 10])).collect();
    problem
        .relate_town_terrain(towns[0], terrain[0], Relation::Same)
        .unwrap();
    problem
        .relate_town_terrain(towns[1], terrain[0], Relation::Same)
        .unwrap();
    problem
        .relate_towns(towns[0], towns[1], Relation::Different)
        .unwrap();
    problem
        .relate_town_terrain(towns[2], terrain[1], Relation::Same)
        .unwrap();
    problem
        .relate_terrains(terrain[1], terrain[2], Relation::Different)
        .unwrap();
    let mut solution = problem.solve(7, |_| Some(0)).unwrap();
    assert!(solution.issues().is_empty());
    assert_eq!(
        towns
            .iter()
            .map(|&id| solution.town(id).unwrap().unwrap())
            .collect::<Vec<_>>(),
        [1, 0, 9]
    );
    assert_eq!(
        terrain
            .iter()
            .map(|&id| solution.terrain(id).unwrap().unwrap())
            .collect::<Vec<_>>(),
        [2, 4, 3]
    );
    assert_eq!(solution.draws(), Some(42));
    assert_eq!(solution.random.as_mut().unwrap().next(), 4_080_775_104);
}

#[test]
fn admission_contradictions_do_not_seed_and_fixed_choices_survive() {
    let mut problem = Problem::new().unwrap();
    let a = problem.add_town([false; 12], Some(11)).unwrap();
    let b = problem.add_town([true; 12], None).unwrap();
    problem.restrict_town(a, [false; 12]).unwrap();
    problem.relate_towns(a, b, Relation::Same).unwrap();
    problem.relate_towns(a, b, Relation::Different).unwrap();
    let solution = problem
        .solve(99, |_| panic!("no search for a pre-existing fatal error"))
        .unwrap();
    assert_eq!(solution.issues(), [Issue::IncompatibleTowns(a, b)]);
    assert_eq!(solution.draws(), None);
    assert_eq!(solution.town(a).unwrap(), Some(11));
    assert_eq!(solution.town(b).unwrap(), None);
}

#[test]
fn timeout_checks_at_native_interval_and_continues_other_components() {
    let mut problem = Problem::new().unwrap();
    let towns: Vec<_> = (0..12)
        .map(|_| {
            problem
                .add_town(std::array::from_fn(|i| i < 11), None)
                .unwrap()
        })
        .collect();
    for (i, &a) in towns.iter().enumerate() {
        for &b in &towns[i + 1..] {
            problem.relate_towns(a, b, Relation::Different).unwrap();
        }
    }
    let independent = problem.add_town(domain(&[0, 1]), None).unwrap();
    let mut reads = Vec::new();
    let mut solution = problem
        .solve(42, |read| {
            reads.push(read);
            Some(if read.steps == 0 { 0 } else { 100_000_001 })
        })
        .unwrap();
    assert_eq!(solution.issues(), [Issue::Timeout]);
    assert_eq!(solution.draws(), Some(121));
    assert_eq!(solution.town(independent).unwrap(), Some(0));
    assert_eq!(
        towns
            .iter()
            .map(|&id| solution.town(id).unwrap().unwrap())
            .collect::<Vec<_>>(),
        [10, 5, 6, 3, 2, 7, 0, 10, 4, 8, 1, 9]
    );
    assert_eq!(solution.random.as_mut().unwrap().next(), 3_209_715_436);
    assert_eq!(
        reads,
        [
            ClockRead {
                component: 0,
                steps: 0
            },
            ClockRead {
                component: 0,
                steps: 500_000
            },
            ClockRead {
                component: 1,
                steps: 0
            }
        ]
    );
}

#[test]
fn handles_and_required_clock_observations_are_checked() {
    let mut first = Problem::new().unwrap();
    let mut second = Problem::new().unwrap();
    let a = first.add_town([true; 12], None).unwrap();
    let b = second.add_town([true; 12], None).unwrap();
    let t = second.add_terrain([true; 10]);
    assert_eq!(
        first.relate_towns(a, b, Relation::Same),
        Err(Fault::ForeignHandle)
    );
    assert_eq!(
        first.relate_town_terrain(a, t, Relation::Same),
        Err(Fault::ForeignHandle)
    );
    assert_eq!(
        first.add_town([true; 12], Some(12)),
        Err(Fault::TownType(12))
    );
    let failure = first.solve(1, |_| None).unwrap_err();
    assert_eq!(
        failure,
        Fault::MissingClock(ClockRead {
            component: 0,
            steps: 0
        })
    );
    let solution = second.solve(1, |_| Some(0)).unwrap();
    assert_eq!(solution.town(a), Err(Fault::ForeignHandle));
}

#[test]
fn backjump_exhaustion_retains_native_fallback_assignment() {
    let mut problem = Problem::new().unwrap();
    let towns: Vec<_> = (0..3)
        .map(|_| problem.add_town(domain(&[0, 1]), None).unwrap())
        .collect();
    for (a, b) in [(0, 1), (1, 2), (2, 0)] {
        problem
            .relate_towns(towns[a], towns[b], Relation::Different)
            .unwrap();
    }
    let mut solution = problem.solve(42, |_| Some(0)).unwrap();
    assert_eq!(solution.issues(), [Issue::NoSolution]);
    assert_eq!(
        towns
            .iter()
            .map(|&id| solution.town(id).unwrap().unwrap())
            .collect::<Vec<_>>(),
        [1, 0, 0]
    );
    assert_eq!(solution.draws(), Some(3));
    assert_eq!(solution.random.as_mut().unwrap().next(), 787_846_414);
}

#[test]
fn contradiction_found_during_merging_still_seeds_and_searches() {
    let mut problem = Problem::new().unwrap();
    let town = problem.add_town(domain(&[0, 1]), None).unwrap();
    let terrain = problem.add_terrain([true; 10]);
    problem
        .relate_town_terrain(town, terrain, Relation::Same)
        .unwrap();
    problem
        .relate_town_terrain(town, terrain, Relation::Different)
        .unwrap();
    let solution = problem.solve(42, |_| Some(0)).unwrap();
    assert_eq!(
        solution.issues(),
        [Issue::IncompatibleTownTerrain(town, terrain)]
    );
    assert_eq!(solution.town(town).unwrap(), Some(1));
    assert_eq!(solution.terrain(terrain).unwrap(), Some(2));
    assert_eq!(solution.draws(), Some(1));
}
