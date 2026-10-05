//! Whole-map generation, independent parsing, and workspace reuse.

use homm3_map::{MapBody, MapHeader, ObjectTable, Terrain, WorldPrefix};
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    generation::{Assets, GenerationFault, GenerationWorkspace, PreparedGeneration, Stage},
    output::{OutputFault, OutputStage, OutputWorkspace},
    placement_rules::PlacementRules,
    prototype::PrototypeSource,
    request::{default_record, Levels, MapSize, Request, RequestOptions},
    rng::RngCheckpoint,
    rules::Ruleset,
    template::TemplateSource,
    traits::{ArtifactCatalog, CreatureCatalog, SpellCatalog},
};
use homm3_rmg_cli::resources::Installation;
use std::{
    io::{self, Write},
    path::{Path, PathBuf},
};

const CASES: [(u32, MapSize, Levels, u32, u32); 4] = [
    (1, MapSize::Small, Levels::Surface, 2, 0),
    (42, MapSize::Medium, Levels::Underground, 1, 1),
    (100, MapSize::Large, Levels::Surface, 0, 2),
    (17, MapSize::ExtraLarge, Levels::Underground, 2, 3),
];
const RETAIL: Behavior = Behavior::Retail(RetailProfile {
    initial_key_tent_color: Some(0),
    stack_word: 0,
    heap_byte: 0,
    water_zone_towns: None,
    water_guards_match_alignment: false,
});

fn with_assets(data: &Path, run: impl FnOnce(Assets<'_>, Assets<'_>)) {
    let mut installation = Installation::open(data).unwrap();
    let mut scratch = Vec::new();
    installation.text("rand_trn.txt", &mut scratch).unwrap();
    let hotfix_rules = PlacementRules::parse(&scratch, Behavior::Hotfix).unwrap();
    let retail_rules = PlacementRules::parse(&scratch, RETAIL).unwrap();
    installation.text("crtraits.txt", &mut scratch).unwrap();
    let creatures = CreatureCatalog::parse(&scratch).unwrap();
    installation.text("sptraits.txt", &mut scratch).unwrap();
    let spells = SpellCatalog::parse(&scratch).unwrap();
    installation.text("artraits.txt", &mut scratch).unwrap();
    let artifacts = ArtifactCatalog::parse(&scratch).unwrap();
    let mut template_bytes = Vec::new();
    installation.text("rmg.txt", &mut template_bytes).unwrap();
    let templates = TemplateSource::parse(&template_bytes).unwrap();
    let mut prototype_bytes = Vec::new();
    installation
        .text("objects.txt", &mut prototype_bytes)
        .unwrap();
    let prototypes =
        PrototypeSource::parse(&prototype_bytes, |name| installation.mask(name)).unwrap();
    drop(installation);
    let hotfix = Assets {
        templates: &templates,
        prototypes: &prototypes,
        placement: &hotfix_rules,
        creatures: &creatures,
        spells: &spells,
        artifacts: &artifacts,
    };
    let retail = Assets {
        placement: &retail_rules,
        ..hotfix
    };
    run(hotfix, retail);
}

fn prepare(assets: Assets<'_>, case: usize, behavior: Behavior) -> PreparedGeneration<'_> {
    let (seed, size, levels, version, water) = CASES[case];
    let mut record = default_record(size, levels);
    record.m_mapVersion = version;
    record.m_waterContent = water;
    assets
        .prepare(Request::parse(record, behavior).unwrap(), seed)
        .unwrap()
}

#[test]
fn hota_inputs_cannot_silently_generate_with_complete_assets() {
    let Some(data) = std::env::var_os("HOMM3_RMG_DATA") else {
        eprintln!("skipping asset-backed admission: set HOMM3_RMG_DATA");
        return;
    };
    with_assets(&PathBuf::from(data), |_, retail| {
        let record = default_record(MapSize::Giant, Levels::Underground);
        let request = Request::parse_with_options(
            record,
            RETAIL,
            RequestOptions {
                ruleset: Ruleset::HotA181,
                ..RequestOptions::default()
            },
        )
        .unwrap();
        let Err(failure) = retail.prepare(request, 42) else {
            panic!("unsupported HotA pipeline admitted request")
        };
        assert!(matches!(
            failure.fault,
            GenerationFault::UnsupportedRuleset(Ruleset::HotA181)
        ));
        assert_eq!(failure.report.stage(), Stage::Assets);
        assert_eq!(
            failure.report.rng(),
            RngCheckpoint {
                state: 42,
                draws: 0
            }
        );
        assert_eq!(failure.report.request().repaired_record(), record);

        let rules =
            PlacementRules::parse_for(b"header\r\nheader\r\nheader\r\n", RETAIL, Ruleset::HotA181)
                .unwrap();
        let assets = Assets {
            placement: &rules,
            ..retail
        };
        let request =
            Request::parse(default_record(MapSize::Small, Levels::Surface), RETAIL).unwrap();
        let Err(failure) = assets.prepare(request, 42) else {
            panic!("Complete generation admitted HotA placement rules")
        };
        assert!(matches!(
            failure.fault,
            GenerationFault::UnsupportedRuleset(Ruleset::HotA181)
        ));
        assert_eq!(failure.report.stage(), Stage::Assets);
        assert_eq!(
            failure.report.rng(),
            RngCheckpoint {
                state: 42,
                draws: 0
            }
        );
    });
}

fn parse_map(bytes: &[u8]) {
    let header = MapHeader::parse(bytes).unwrap();
    let prefix = WorldPrefix::parse(header.world(), header.version).unwrap();
    let terrain = Terrain::parse(prefix.map(), header.size, header.two_layers).unwrap();
    let table = ObjectTable::parse(terrain.objects()).unwrap();
    let body = MapBody::parse(table, header.version).unwrap();
    assert!(table.object_count() > 0);
    assert!(body.trailing().is_empty());
    assert!(body.padding().is_empty()); // RMG does not write the editor trailer.
    assert_eq!(body.objects().count(), table.object_count() as usize);
}

#[derive(Debug, PartialEq, Eq)]
struct Snapshot {
    bytes: Vec<u8>,
    generation_rng: RngCheckpoint,
    output_rng: RngCheckpoint,
}

struct LimitedWriter {
    bytes: Vec<u8>,
    fail_after: usize,
}
impl Write for LimitedWriter {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        let remaining = self.fail_after - self.bytes.len();
        if remaining == 0 {
            return Err(io::Error::other("injected destination failure"));
        }
        let accepted = bytes.len().min(remaining).min(3);
        self.bytes.extend_from_slice(&bytes[..accepted]);
        Ok(accepted)
    }
    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}

fn check_partial_writers(
    prepared: &PreparedGeneration<'_>,
    generation: &mut GenerationWorkspace,
    output: &mut OutputWorkspace,
    expected: &Snapshot,
) {
    let map = generation.generate(prepared).unwrap();
    for (fail_after, stage) in [
        (0, OutputStage::Header),
        (expected.bytes.len() - 1, OutputStage::Trailer),
    ] {
        let mut destination = LimitedWriter {
            bytes: Vec::new(),
            fail_after,
        };
        let failure = output.write(&map, &mut destination).unwrap_err();
        assert!(matches!(failure.fault, OutputFault::Io(_)));
        assert_eq!(failure.stage, stage);
        assert_eq!(destination.bytes, expected.bytes[..fail_after]);
        assert_eq!(map.report().rng(), expected.generation_rng);
    }
    // Reuse the output workspace after both failures. Every write accepts at
    // most three bytes, exercising Write::write_all throughout serialization.
    let mut destination = LimitedWriter {
        bytes: Vec::new(),
        fail_after: usize::MAX,
    };
    let report = output.write(&map, &mut destination).unwrap();
    assert_eq!(destination.bytes, expected.bytes);
    assert_eq!(report.rng, expected.output_rng);
}

fn generate(
    prepared: &PreparedGeneration<'_>,
    generation: &mut GenerationWorkspace,
    output: &mut OutputWorkspace,
    second_write: &mut Vec<u8>,
) -> Snapshot {
    let map = generation.generate(prepared).unwrap();
    assert_eq!(map.report().stage(), Stage::Complete);
    let mut bytes = Vec::new();
    let report = output.write(&map, &mut bytes).unwrap();
    second_write.clear();
    let repeated = output.write(&map, second_write).unwrap();
    assert_eq!(bytes, *second_write);
    assert_eq!(report.rng, repeated.rng);
    parse_map(&bytes);
    Snapshot {
        bytes,
        generation_rng: map.report().rng(),
        output_rng: report.rng,
    }
}

#[test]
fn complete_maps_are_parseable_and_independent_of_workspace_history() {
    let Some(data) = std::env::var_os("HOMM3_RMG_DATA") else {
        eprintln!("skipping asset-backed generation: set HOMM3_RMG_DATA");
        return;
    };
    with_assets(&PathBuf::from(data), |hotfix, retail| {
        let prepared = vec![
            prepare(hotfix, 0, Behavior::Hotfix),
            prepare(hotfix, 1, Behavior::Hotfix),
            prepare(hotfix, 2, Behavior::Hotfix),
            prepare(hotfix, 3, Behavior::Hotfix),
            prepare(retail, 1, RETAIL),
            prepare(retail, 3, RETAIL),
        ];
        let mut generation = GenerationWorkspace::default();
        let mut output = OutputWorkspace::default();
        let mut second_write = Vec::new();
        let mut expected = Vec::new();
        for case in &prepared {
            let first = generate(case, &mut generation, &mut output, &mut second_write);
            let repeated = generate(case, &mut generation, &mut output, &mut second_write);
            assert_eq!(first, repeated);
            expected.push(first);
        }
        // A/B/A also crosses behavior, format and shape boundaries and shrinks
        // retained buffers after the largest two-level map.
        for index in [0, 5, 0] {
            let actual = generate(
                &prepared[index],
                &mut generation,
                &mut output,
                &mut second_write,
            );
            assert_eq!(
                actual, expected[index],
                "case {index} after workspace reuse"
            );
        }
        check_partial_writers(&prepared[0], &mut generation, &mut output, &expected[0]);
    });
}
