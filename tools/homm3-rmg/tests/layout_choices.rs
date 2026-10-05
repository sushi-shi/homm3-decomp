//! Shared layout admission and lifetime of the separate constraint stream.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    boundaries::BoundaryWorkspace,
    domain::Terrain,
    layout::{LayoutError, LayoutWorkspace},
    request::{default_record, Levels, MapSize, Request, RequestOptions, Water},
    rng::RetailRng,
    rules::Ruleset,
    selection::SelectedTemplate,
    template::TemplateSource,
};

fn pack() -> Vec<u8> {
    let mut text = String::from("Pack\r\nsections\r\ncolumns\r\n");
    for index in 0..2 {
        let mut row = vec![String::new(); 140];
        if index == 0 {
            for (column, value) in [12, 10, 4, 8, 10, 18, 4].into_iter().enumerate() {
                row[column] = value.to_string();
            }
            row[15] = "choice lifetime".into();
            row[16] = "1".into();
            row[17] = "32".into();
        }
        for (column, value) in [
            (28, index + 1),
            (33, 30),
            (35, 8),
            (37, 8),
            (38, index + 1),
            (40, 1),
            (44, 1),
            (45, 1),
        ] {
            row[column] = value.to_string();
        }
        row[29] = "x".into();
        row[48..60].fill("x".into());
        row[84 - index] = "x".into(); // Wasteland, Highlands.
        row[116] = "ni000000000001".into(); // All neutral towns must be Bulwark.
        if index == 0 {
            row[124] = "100000000000".into(); // Creature faction Castle overrides town alignment.
            row[127] = "1".into();
            row[128] = "2".into();
            row[137] = "8".into();
            row[139] = "8".into();
        } else {
            row[113] = "x".into(); // Force neutral creatures overrides alignment.
        }
        text.push_str(&row.join("\t"));
        text.push_str("\r\n");
    }
    text.into_bytes()
}
fn request(mirror: bool) -> Request {
    let mut record = default_record(MapSize::Large, Levels::Surface);
    record.m_townChoices[0] = 10;
    record.m_townChoices[1] = 11;
    Request::parse_with_options(
        record,
        Behavior::Retail(RetailProfile::default()),
        RequestOptions {
            ruleset: Ruleset::HotA181,
            mirror,
            ..RequestOptions::default()
        },
    )
    .unwrap()
}

#[test]
fn layout_retains_hint_choices_and_resets_query_counters_on_workspace_reuse() {
    let bytes = pack();
    let request = request(false);
    let source = TemplateSource::parse(&bytes).unwrap();
    let templates = source.prepare(&request, Water::None).unwrap();
    let mut workspace = LayoutWorkspace::default();
    let mut first = None;
    for _ in 0..2 {
        let mut rng = RetailRng::new(42);
        let selected = SelectedTemplate::select(&templates, &request, &mut rng).unwrap();
        let before = rng.checkpoint();
        assert!(matches!(
            workspace.generate(&selected, &request, Water::None, &mut rng),
            Err(LayoutError::HintsRequired)
        ));
        assert_eq!(rng.checkpoint(), before);
        let mut layout = workspace
            .generate_with_hints(&selected, &request, Water::None, 42, &mut rng, |_| Some(0))
            .unwrap();
        assert!(layout.hints().unwrap().diagnostics().is_empty());
        assert_eq!(rng.draws() - before.draws, 6); // Three positioning draws per zone, no town/terrain draws.
        let zones = layout.zones().to_vec();
        assert_eq!(zones[0].alignment().unwrap().index(), 10);
        assert_eq!(zones[0].terrain(), Terrain::Wasteland);
        assert_eq!(zones[0].creature_town().unwrap().index(), 0);
        assert_eq!(zones[1].alignment().unwrap().index(), 11);
        assert_eq!(zones[1].terrain(), Terrain::Highlands);
        assert_eq!(zones[1].creature_town(), None);
        if let Some(previous) = &first {
            assert_eq!(&zones, previous);
        } else {
            first = Some(zones.clone());
        }
        let before = rng.checkpoint();
        assert_eq!(
            layout
                .select_zone_town(zones[0].id(), &mut rng)
                .unwrap()
                .unwrap()
                .index(),
            11
        );
        assert_eq!(
            layout
                .select_zone_town(zones[0].id(), &mut rng)
                .unwrap()
                .unwrap()
                .index(),
            11
        );
        assert_eq!(rng.checkpoint(), before);
        let mut boundary_workspace = BoundaryWorkspace::default();
        let mut boundaries = boundary_workspace.generate(layout, &mut rng).unwrap();
        assert!(boundaries.hints().unwrap().diagnostics().is_empty());
        let before = rng.checkpoint();
        assert_eq!(
            boundaries
                .select_zone_town(zones[0].id(), &mut rng)
                .unwrap()
                .unwrap()
                .index(),
            11
        );
        assert_eq!(rng.checkpoint(), before);
        let coverage = boundaries.prepare_terrain(&mut rng).unwrap();
        assert!(coverage
            .map()
            .zones()
            .iter()
            .all(|zone| zone.saved_center().is_some()));
        assert_eq!(rng.checkpoint(), before);
    }
}

#[test]
fn layout_rejects_a_missing_solver_clock_before_consuming_the_crt_stream() {
    let bytes = pack();
    let request = request(false);
    let source = TemplateSource::parse(&bytes).unwrap();
    let templates = source.prepare(&request, Water::None).unwrap();
    let mut rng = RetailRng::new(42);
    let selected = SelectedTemplate::select(&templates, &request, &mut rng).unwrap();
    let before = rng.checkpoint();
    assert!(matches!(
        LayoutWorkspace::default().generate_with_hints(
            &selected,
            &request,
            Water::None,
            42,
            &mut rng,
            |_| None
        ),
        Err(LayoutError::Hints(_))
    ));
    assert_eq!(rng.checkpoint(), before);
}

#[test]
fn mirror_layout_uses_compacted_fixed_towns_and_constructor_planes() {
    let bytes = pack();
    let mut record = default_record(MapSize::Large, Levels::Underground);
    record.m_humanPlayerCount = 4;
    record.m_townChoices[0] = 10;
    record.m_townChoices[1] = 0; // Odd original colour must not become the second internal player.
    record.m_townChoices[2] = 11;
    let request = Request::parse_with_options(
        record,
        Behavior::Retail(RetailProfile::default()),
        RequestOptions {
            ruleset: Ruleset::HotA181,
            mirror: true,
            ..RequestOptions::default()
        },
    )
    .unwrap();
    let source = TemplateSource::parse(&bytes).unwrap();
    let templates = source.prepare(&request, Water::None).unwrap();
    let mut rng = RetailRng::new(42);
    let selected = SelectedTemplate::select(&templates, &request, &mut rng).unwrap();
    let mut workspace = LayoutWorkspace::default();
    let layout = workspace
        .generate_with_hints(&selected, &request, Water::None, 42, &mut rng, |_| Some(0))
        .unwrap();
    assert_eq!(
        layout
            .zones()
            .iter()
            .map(|z| z.alignment().unwrap().index())
            .collect::<Vec<_>>(),
        [10, 11]
    );
    assert!(layout
        .zones()
        .iter()
        .all(|z| z.position().level == homm3_rmg::domain::Level::Surface));
    let mut boundary_workspace = BoundaryWorkspace::default();
    let boundaries = boundary_workspace.generate(layout, &mut rng).unwrap();
    assert_eq!(boundaries.raster().cells().len(), 108 * 108);
    assert!(boundaries
        .level_rng(homm3_rmg::domain::Level::Surface)
        .is_some());
    assert!(boundaries
        .level_rng(homm3_rmg::domain::Level::Underground)
        .is_none());
}

#[test]
fn native_terrain_and_underground_substitution_share_the_expanded_domain() {
    let bytes = String::from_utf8(pack()).unwrap();
    for levels in [Levels::Surface, Levels::Underground] {
        let mut rows: Vec<Vec<String>> = bytes
            .lines()
            .map(|line| line.split('\t').map(str::to_owned).collect())
            .collect();
        for row in &mut rows[3..] {
            row[74] = "x".into(); // Native terrain overrides the terrain mask.
            row[109] = "underground".into(); // Ignored for a single-level request.
        }
        let bytes = rows
            .iter()
            .map(|r| r.join("\t"))
            .collect::<Vec<_>>()
            .join("\r\n")
            + "\r\n";
        let mut record = default_record(MapSize::Large, levels);
        record.m_townChoices[0] = 10;
        record.m_townChoices[1] = 11;
        let request = Request::parse_with_options(
            record,
            Behavior::Retail(RetailProfile::default()),
            RequestOptions {
                ruleset: Ruleset::HotA181,
                ..RequestOptions::default()
            },
        )
        .unwrap();
        let source = TemplateSource::parse(bytes.as_bytes()).unwrap();
        let templates = source.prepare(&request, Water::None).unwrap();
        let mut rng = RetailRng::new(42);
        let selected = SelectedTemplate::select(&templates, &request, &mut rng).unwrap();
        let mut workspace = LayoutWorkspace::default();
        let layout = workspace
            .generate_with_hints(&selected, &request, Water::None, 42, &mut rng, |_| Some(0))
            .unwrap();
        assert!(layout.hints().unwrap().diagnostics().is_empty());
        let expected = if levels == Levels::Surface {
            [Terrain::Wasteland, Terrain::Snow]
        } else {
            [Terrain::Subterranean; 2]
        };
        assert_eq!(
            layout
                .zones()
                .iter()
                .map(|z| z.terrain())
                .collect::<Vec<_>>(),
            expected
        );
    }
}
