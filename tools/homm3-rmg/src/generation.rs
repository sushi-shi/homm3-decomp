//! Native generation from immutable parsed assets into reusable map storage.
//!
//! Prepare a request once, then generate while borrowing that prepared context
//! and a workspace. Independent workspaces and RNGs may share the same assets;
//! generation decisions use one run-local RNG and do not depend on global state.

use crate::{
    boundaries::{BoundaryError, BoundaryWorkspace},
    layout::{LayoutError, LayoutWorkspace},
    placement::{
        ConnectionError, MineError, ObjectArena, ObstacleWorkspace, PlacementError, PlacementMap,
        PlacementWorkspace, SelectedTreasure, TownError, TreasureGeneration,
        TreasureGenerationError, TreasureGroupWorkspace,
    },
    placement_rules::PlacementRules,
    prototype::{CatalogError, PrototypeCatalog, PrototypeSource, RequiredPrototypeError},
    request::{Request, Water},
    rng::{RetailRng, RngCheckpoint},
    selection::{resolve_water, SelectedTemplate, SelectionError},
    template::{TemplateCandidate, TemplateError, TemplateSource},
    terrain::{TerrainError, TerrainWorkspace},
    traits::{ArtifactCatalog, CreatureCatalog, SpellCatalog},
    treasure::{TreasureError, TreasureWorkspace},
};
use std::{error::Error, fmt};

/// Parsed resources shared by runs. Resource bytes and catalogs remain with the
/// caller; constructing this view neither copies data nor resolves randomness.
#[derive(Clone, Copy)]
pub struct Assets<'a> {
    /// Parsed template rows.
    pub templates: &'a TemplateSource<'a>,
    /// Source prototypes and loaded image masks.
    pub prototypes: &'a PrototypeSource<'a>,
    /// Placement rules parsed in the requested retail/hotfix mode.
    pub placement: &'a PlacementRules,
    /// Creature traits shared by guard and reward generation.
    pub creatures: &'a CreatureCatalog,
    /// Spell eligibility and payloads.
    pub spells: &'a SpellCatalog,
    /// Artifact eligibility and payloads.
    pub artifacts: &'a ArtifactCatalog,
}

/// Source-order generation boundaries, also used to locate failures.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(usize)]
pub enum Stage {
    /// Version/mode prototype filtering.
    Assets,
    /// Water resolution and request-dependent template admission.
    Templates,
    /// Eager treasure definition initialization, before template selection.
    Definitions,
    /// Hotfix required-prototype gate after constructor work, including with no templates.
    Admission,
    /// Template choice and player-slot assignment.
    Selection,
    /// Zone positions and terrain choices.
    Layout,
    /// Zone boundaries and water-zone graph.
    Boundaries,
    /// Zone raster coverage and islands.
    Coverage,
    /// Terrain tiles and placement storage initialization.
    Terrain,
    /// Primary and additional towns.
    Towns,
    /// Branching paths and connection borders.
    Borders,
    /// Water-zone islands.
    Islands,
    /// Initial connection path construction.
    Paths,
    /// Water-border repair.
    WaterBorders,
    /// Ground/shipyard/gate connections.
    DirectConnections,
    /// Remaining portal connections.
    Portals,
    /// Junction-zone floor preparation.
    Junctions,
    /// Mines and associated resources/guards.
    Mines,
    /// Town-faction tally and post-mine path rebuild.
    TreasurePaths,
    /// Treasure groups and their completion callbacks.
    Treasures,
    /// Underground rock and occupied-floor repainting.
    Underground,
    /// Coastal tile flags.
    Coasts,
    /// Obstacle placement.
    Obstacles,
    /// Roads and updated route costs.
    Roads,
    /// River targets, searches, painting and deltas.
    Rivers,
    /// All generation stages completed; output may begin.
    Complete,
}
const STAGES: usize = Stage::Complete as usize + 1;

/// Fixed-size replay diagnostics; snapshots require no heap allocation.
#[derive(Clone, Debug)]
pub struct GenerationReport {
    seed: u32,
    request: Request,
    stage: Stage,
    pub(crate) rng: RetailRng,
    checkpoints: [Option<RngCheckpoint>; STAGES],
    template: Option<usize>,
}
impl GenerationReport {
    fn new(request: Request, seed: u32) -> Self {
        Self {
            seed,
            request,
            stage: Stage::Assets,
            rng: RetailRng::new(seed),
            checkpoints: [None; STAGES],
            template: None,
        }
    }
    fn completed(&mut self, stage: Stage, rng: &RetailRng) {
        self.rng = rng.clone();
        self.checkpoints[stage as usize] = Some(rng.checkpoint());
    }
    fn fail(mut self, fault: impl Into<GenerationFault>, rng: &RetailRng) -> GenerationFailure {
        self.rng = rng.clone();
        GenerationFailure {
            fault: fault.into(),
            report: Box::new(self),
        }
    }
    /// Original seed, before any constructor draws.
    #[must_use]
    pub const fn seed(&self) -> u32 {
        self.seed
    }
    /// Admitted request, retaining both the original and repaired records.
    #[must_use]
    pub const fn request(&self) -> &Request {
        &self.request
    }
    /// Current stage, or Complete after success.
    #[must_use]
    pub const fn stage(&self) -> Stage {
        self.stage
    }
    /// Exact current RNG state, including draws made before a failure.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng.checkpoint()
    }
    /// RNG after a successfully completed stage; absent for unfinished stages.
    #[must_use]
    pub const fn checkpoint(&self, stage: Stage) -> Option<RngCheckpoint> {
        self.checkpoints[stage as usize]
    }
    /// Selected candidate ordinal, once selection succeeds.
    #[must_use]
    pub const fn template_index(&self) -> Option<usize> {
        self.template
    }
}

/// Generation error together with the effective request and exact failure RNG.
#[derive(Debug)]
pub struct GenerationFailure {
    /// Typed source of the failure.
    pub fault: GenerationFault,
    /// Owned diagnostics survive reuse of the workspace. Boxing is error-only.
    pub report: Box<GenerationReport>,
}
impl fmt::Display for GenerationFailure {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{:?} at RNG draw {}: {}",
            self.report.stage,
            self.report.rng.draws(),
            self.fault
        )
    }
}
impl Error for GenerationFailure {
    fn source(&self) -> Option<&(dyn Error + 'static)> {
        Some(&self.fault)
    }
}

/// Failures from parsed-resource admission and generation algorithms.
#[derive(Debug)]
pub enum GenerationFault {
    /// A catalog uses generation rules not yet supported by this pipeline.
    UnsupportedRuleset(crate::rules::Ruleset),
    /// Rules were prepared under the other behavior mode.
    BehaviorMismatch,
    /// Prototype preparation failed.
    Catalog(CatalogError),
    /// Hotfix lacks a required prototype relationship.
    RequiredPrototypes(RequiredPrototypeError),
    /// Template preparation failed.
    Template(TemplateError),
    /// No template or an unusable selected template.
    Selection(SelectionError),
    /// Zone placement failed.
    Layout(LayoutError),
    /// Boundary generation failed.
    Boundary(BoundaryError),
    /// Terrain painting failed.
    Terrain(TerrainError),
    /// Object/cell access or native geometry failed.
    Placement(PlacementError),
    /// Town placement failed.
    Town(TownError),
    /// Connection or junction generation failed.
    Connection(ConnectionError),
    /// Mine placement failed.
    Mine(MineError),
    /// Eager treasure definition construction failed.
    Treasure(TreasureError),
    /// Treasure placement or completion failed.
    TreasureGeneration(TreasureGenerationError),
}
macro_rules! fault_conversions {
    ($($variant:ident($ty:ty)),* $(,)?) => {
        $(impl From<$ty> for GenerationFault { fn from(error: $ty) -> Self { Self::$variant(error) } })*
        impl fmt::Display for GenerationFault {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                match self { Self::BehaviorMismatch => f.write_str("placement rules use another behavior mode"),
                    Self::UnsupportedRuleset(rules) => write!(f, "generation does not yet support {rules:?} catalogs"),
                    $(Self::$variant(error) => error.fmt(f)),* }
            }
        }
    };
}
fault_conversions! { Catalog(CatalogError), RequiredPrototypes(RequiredPrototypeError), Template(TemplateError), Selection(SelectionError),
Layout(LayoutError), Boundary(BoundaryError), Terrain(TerrainError), Placement(PlacementError),
Town(TownError), Connection(ConnectionError), Mine(MineError), Treasure(TreasureError),
TreasureGeneration(TreasureGenerationError) }
impl Error for GenerationFault {}

/// Request-dependent immutable inputs kept alive while a generated map borrows
/// them. Repeating generation starts from the same post-constructor RNG state.
pub struct PreparedGeneration<'a> {
    assets: Assets<'a>,
    prototypes: PrototypeCatalog<'a>,
    templates: Vec<TemplateCandidate<'a>>,
    water: Water,
    report: GenerationReport,
}
impl<'a> Assets<'a> {
    /// Filter prototypes, resolve water, and admit templates in constructor order.
    /// No template choice or map placement happens during preparation.
    ///
    /// # Errors
    /// Reports incompatible rule mode or prototype/template admission failure,
    /// retaining constructor RNG draws in the failure report.
    pub fn prepare(
        self,
        request: Request,
        seed: u32,
    ) -> Result<PreparedGeneration<'a>, GenerationFailure> {
        let mut rng = RetailRng::new(seed);
        let mut report = GenerationReport::new(request, seed);
        for rules in [self.creatures.ruleset(), self.artifacts.ruleset()] {
            if rules != crate::rules::Ruleset::Complete {
                return Err(report.fail(GenerationFault::UnsupportedRuleset(rules), &rng));
            }
        }
        if self.placement.behavior().is_hotfix() != report.request.behavior().is_hotfix() {
            return Err(report.fail(GenerationFault::BehaviorMismatch, &rng));
        }
        let prototypes = self
            .prototypes
            .prepare(
                self.placement,
                report.request.version(),
                report.request.behavior(),
            )
            .map_err(|error| report.clone().fail(error, &rng))?;
        report.completed(Stage::Assets, &rng);
        report.stage = Stage::Templates;
        let water = resolve_water(report.request.water(), &mut rng);
        let templates = self
            .templates
            .prepare(&report.request, water)
            .map_err(|error| report.clone().fail(error, &rng))?;
        report.completed(Stage::Templates, &rng);
        Ok(PreparedGeneration {
            assets: self,
            prototypes,
            templates,
            water,
            report,
        })
    }
}
impl PreparedGeneration<'_> {
    /// Constructor-stage report, before template choice and placement.
    #[must_use]
    pub const fn report(&self) -> &GenerationReport {
        &self.report
    }
}

/// Mutable per-run storage. Drop the borrowed map before reusing this workspace.
/// Separate workspaces permit independent maps to use shared immutable assets.
#[derive(Default)]
pub struct GenerationWorkspace {
    layout: LayoutWorkspace,
    boundaries: BoundaryWorkspace,
    terrain: TerrainWorkspace,
    placement: PlacementWorkspace,
    objects: ObjectArena,
    treasures: TreasureWorkspace,
    group: TreasureGroupWorkspace,
    obstacles: ObstacleWorkspace,
    offers: Vec<SelectedTreasure>,
    nested_groups: Vec<Box<TreasureGroupWorkspace>>,
}
/// Complete generation, borrowing its buffers and immutable source context.
pub struct GeneratedMap<'a> {
    generation: TreasureGeneration<'a, 'a, 'a, 'a, 'a, 'a, 'a>,
    objects: &'a ObjectArena,
    report: GenerationReport,
}
impl GeneratedMap<'_> {
    /// Terrain, flags, zone state and ordered world-object identities.
    #[must_use]
    pub fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.generation.ready().map()
    }
    /// Geometry and payload records, including modeled retained references.
    #[must_use]
    pub const fn objects(&self) -> &ObjectArena {
        self.objects
    }
    /// Definition and reward context needed for serialization.
    #[must_use]
    pub const fn treasures(&self) -> &TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
        &self.generation
    }
    /// Final RNG and all completed generation checkpoints.
    #[must_use]
    pub const fn report(&self) -> &GenerationReport {
        &self.report
    }
}

impl GenerationWorkspace {
    /// Run the entire native generation pipeline; no file IO, Wine or process
    /// state is involved. A failed run may be followed by a fresh generation.
    ///
    /// # Errors
    /// Reports a typed stage fault with the effective request and exact RNG state.
    #[expect(
        clippy::too_many_lines,
        reason = "one source-ordered orchestration sequence keeps stage boundaries and RNG ownership visible"
    )]
    pub fn generate<'run>(
        &'run mut self,
        prepared: &'run PreparedGeneration<'_>,
    ) -> Result<GeneratedMap<'run>, GenerationFailure> {
        let mut report = prepared.report.clone();
        let mut rng = prepared.report.rng.clone();
        self.group.reset_for_generation();
        for group in &mut self.nested_groups {
            group.reset_for_generation();
        }
        self.offers.clear();
        self.objects.reset();
        let assets = prepared.assets;
        let catalog = &prepared.prototypes;
        macro_rules! stage {
            ($stage:ident, $expression:expr) => {{
                report.stage = Stage::$stage;
                report.checkpoints[Stage::$stage as usize] = None;
                let result = $expression.map_err(|error| report.clone().fail(error, &rng))?;
                report.completed(Stage::$stage, &rng);
                result
            }};
        }
        // No-template skips definitions, but still checks required prototypes
        // after constructor work and before generate's empty-template return.
        let definitions = if prepared.templates.is_empty() {
            None
        } else {
            Some(stage!(
                Definitions,
                self.treasures.prepare(catalog, assets.creatures)
            ))
        };
        stage!(
            Admission,
            catalog.require_generation(prepared.report.request.levels(), assets.artifacts)
        );
        let Some(definitions) = definitions else {
            report.stage = Stage::Selection;
            return Err(report.fail(SelectionError::NoTemplates, &rng));
        };
        let selected = stage!(
            Selection,
            SelectedTemplate::select(&prepared.templates, &prepared.report.request, &mut rng)
        );
        report.template = Some(selected.source_index());
        let layout = stage!(
            Layout,
            self.layout.generate(
                &selected,
                &prepared.report.request,
                prepared.water,
                &mut rng
            )
        );
        let boundaries = stage!(Boundaries, self.boundaries.generate(layout, &mut rng));
        let coverage = stage!(Coverage, boundaries.prepare_terrain(&mut rng));
        let terrain = stage!(Terrain, self.terrain.paint(coverage, &mut rng));
        let map = stage!(Terrain, self.placement.begin(terrain));
        let towns = stage!(Towns, map.place_towns(&mut self.objects, catalog, &mut rng));
        let borders = stage!(Borders, towns.reserve_connection_borders(&mut rng));
        let islands = stage!(Islands, borders.place_water_islands(&mut rng));
        let paths = stage!(
            Paths,
            islands.build_connection_paths(&mut self.objects, catalog, &mut rng)
        );
        let repaired = stage!(WaterBorders, paths.repair_water_borders(&mut rng));
        let connecting = stage!(
            DirectConnections,
            repaired.begin_connections(&self.objects, catalog)
        );
        let direct = stage!(
            DirectConnections,
            connecting.connect_direct_zones(&mut self.objects, catalog, assets.creatures, &mut rng)
        );
        let portals = stage!(
            Portals,
            direct.connect_remaining_zones(&mut self.objects, catalog, assets.creatures, &mut rng)
        );
        let junctions = stage!(Junctions, portals.prepare_junctions(&mut rng));
        let mines = stage!(
            Mines,
            junctions.place_mines(&mut self.objects, catalog, assets.creatures, &mut rng)
        );
        let paths = stage!(
            TreasurePaths,
            mines.prepare_treasure_paths(&mut self.objects, catalog, &mut rng)
        );
        let ready = stage!(Treasures, paths.begin_treasures(definitions));
        let mut generation = stage!(
            Treasures,
            ready.begin_generation(&self.objects, assets.spells, assets.artifacts)
        );
        generation.exchange_scratch(&mut self.offers, &mut self.nested_groups);
        let treasures =
            generation.place_all_treasures(&mut self.group, &mut self.objects, &mut rng);
        generation.exchange_scratch(&mut self.offers, &mut self.nested_groups);
        stage!(Treasures, treasures);
        let map = generation.map_mut();
        stage!(Underground, map.decorate_underground(&mut rng));
        stage!(Coasts, map.mark_coastal_tiles());
        stage!(
            Obstacles,
            map.decorate_obstacles(
                &mut self.obstacles,
                catalog,
                assets.placement,
                &mut self.objects,
                &mut rng
            )
        );
        stage!(Roads, map.create_roads(catalog, &self.objects, &mut rng));
        stage!(
            Rivers,
            map.create_rivers(catalog, &mut self.objects, &mut rng)
        );
        report.stage = Stage::Complete;
        report.completed(Stage::Complete, &rng);
        Ok(GeneratedMap {
            generation,
            objects: &self.objects,
            report,
        })
    }
}
