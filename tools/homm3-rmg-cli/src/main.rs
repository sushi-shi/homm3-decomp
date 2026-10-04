//! Native RMG command-line adapter: installation assets, replay and gzip output.
use clap::{Args, Parser, Subcommand, ValueEnum};
use flate2::{Compression, GzBuilder};
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    generation::{Assets, GenerationWorkspace},
    output::OutputWorkspace,
    placement_rules::PlacementRules,
    prototype::PrototypeSource,
    raw,
    request::{self, Levels, MapSize, Request},
    template::TemplateSource,
    traits::{ArtifactCatalog, CreatureCatalog, SpellCatalog},
};
use homm3_rmg_cli::{replay::Replay, resources::Installation};
use std::{
    error::Error,
    fs::{self, File, OpenOptions},
    io::{self, BufWriter, Write},
    path::{Path, PathBuf},
    process::ExitCode,
};

type Result<T> = std::result::Result<T, Box<dyn Error>>;
#[derive(Parser)]
#[command(name = "homm3-rmg", about = "Native Heroes III random-map generator")]
struct Cli {
    #[command(subcommand)]
    command: Command,
}
#[derive(Subcommand)]
enum Command {
    /// Generate a map from an explicit seed and runtime behavior.
    Generate(Generate),
    /// Repeat the raw request, seed and behavior stored in a replay report.
    Replay {
        #[command(flatten)]
        files: Files,
        #[arg(long)]
        input: PathBuf,
    },
}
#[derive(Args)]
struct Files {
    /// Complete installation Data directory (loose files and LOD archives).
    #[arg(long)]
    data: PathBuf,
    /// Destination compressed .h3m file, replaced only after a successful write.
    #[arg(long, short)]
    output: PathBuf,
    /// Replay and diagnostics path; defaults to OUTPUT with extension .rmg-replay.
    #[arg(long)]
    report: Option<PathBuf>,
}
#[derive(Clone, Copy, ValueEnum)]
enum Mode {
    Retail,
    Hotfix,
}
#[derive(Clone, Copy, ValueEnum)]
enum Format {
    Roe,
    Ab,
    Sod,
}
#[derive(Clone, Copy, ValueEnum)]
enum Water {
    None,
    Normal,
    Islands,
    Random,
}
#[derive(Args)]
struct Generate {
    #[command(flatten)]
    files: Files,
    /// Required to make behavior changes explicit.
    #[arg(long, value_enum)]
    mode: Mode,
    #[arg(long)]
    seed: u32,
    /// Side length in tiles: 36, 72, 108, or 144.
    #[arg(long, default_value_t = 36)]
    size: i32,
    #[arg(long, default_value_t = 1)]
    levels: i32,
    #[arg(long, value_enum, default_value = "sod")]
    format: Format,
    #[arg(long, default_value_t = 2)]
    humans: i32,
    #[arg(long, default_value_t = 0)]
    computers: i32,
    /// Zero means each generated human gets their own team.
    #[arg(long, default_value_t = 0)]
    human_teams: i32,
    #[arg(long, default_value_t = 0)]
    computer_teams: i32,
    /// Zero-based fixed-human player colors, separated by commas.
    #[arg(long, value_delimiter = ',')]
    human_seats: Vec<usize>,
    /// Exactly eight faction IDs (0..8) or -1 for random, separated by commas.
    #[arg(long, value_delimiter = ',', allow_hyphen_values = true)]
    towns: Vec<i32>,
    #[arg(long, value_enum, default_value = "random")]
    water: Water,
    /// Lobby strength: -1 weak, 0 normal, 1 strong.
    #[arg(long, default_value_t = 0, allow_hyphen_values = true)]
    monsters: i32,
    /// Retail compatibility input; recorded verbatim for replay.
    #[arg(long, default_value_t = 0)]
    stack_word: u32,
    #[arg(long, default_value_t = 0)]
    heap_byte: u8,
    #[arg(long)]
    water_guards_match_alignment: bool,
    /// Omit to report a typed fault on an uninitialized retail cursor read.
    #[arg(long, allow_hyphen_values = true)]
    initial_key_tent_color: Option<i32>,
}
impl Generate {
    fn input(&self) -> Result<Replay> {
        let behavior = match self.mode {
            Mode::Hotfix => {
                if self.stack_word != 0
                    || self.heap_byte != 0
                    || self.water_guards_match_alignment
                    || self.initial_key_tent_color.is_some()
                {
                    return Err(input_error("retail residue options require --mode retail"));
                }
                Behavior::Hotfix
            }
            Mode::Retail => Behavior::Retail(RetailProfile {
                stack_word: self.stack_word,
                heap_byte: self.heap_byte,
                water_guards_match_alignment: self.water_guards_match_alignment,
                initial_key_tent_color: self.initial_key_tent_color,
            }),
        };
        let mut r = request::default_record(MapSize::Small, Levels::Surface);
        r.m_width = self.size;
        r.m_height = self.size;
        r.m_levels = self.levels;
        r.m_humanPlayerCount = self.humans;
        r.m_computerPlayerCount = self.computers;
        r.m_humanTeamCount = self.human_teams;
        r.m_computerTeamCount = self.computer_teams;
        r.m_mapVersion = match self.format {
            Format::Roe => raw::RMG_MAP_RESTORATION_OF_ERATHIA,
            Format::Ab => raw::RMG_MAP_ARMAGEDDONS_BLADE,
            Format::Sod => raw::RMG_MAP_SHADOW_OF_DEATH,
        };
        r.m_waterContent = match self.water {
            Water::None => raw::RMG_WATER_NONE,
            Water::Normal => raw::RMG_WATER_NORMAL,
            Water::Islands => raw::RMG_WATER_ISLANDS,
            Water::Random => raw::RMG_WATER_RANDOM,
        };
        r.m_monsterStrength = self.monsters;
        for &seat in &self.human_seats {
            *r.m_isHumanSeat
                .get_mut(seat)
                .ok_or_else(|| input_error("human seats must be color indexes 0..7"))? = 1;
        }
        if !self.towns.is_empty() {
            r.m_townChoices = self
                .towns
                .as_slice()
                .try_into()
                .map_err(|_| input_error("--towns requires exactly eight faction IDs"))?;
        }
        Ok(Replay {
            seed: self.seed,
            behavior,
            request: r,
        })
    }
}
fn input_error(message: &str) -> Box<dyn Error> {
    io::Error::new(io::ErrorKind::InvalidInput, message).into()
}
fn main() -> ExitCode {
    let cli = Cli::parse();
    let result = match cli.command {
        Command::Generate(args) => args.input().and_then(|input| run(&args.files, &input)),
        Command::Replay { files, input } => fs::read_to_string(input)
            .map_err(Into::into)
            .and_then(|text| Replay::parse(&text).map_err(Into::into))
            .and_then(|replay| run(&files, &replay)),
    };
    match result {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            eprintln!("homm3-rmg: {error}");
            ExitCode::FAILURE
        }
    }
}
fn run(files: &Files, input: &Replay) -> Result<()> {
    let report_path = files
        .report
        .clone()
        .unwrap_or_else(|| files.output.with_extension("rmg-replay"));
    let report_name = destination_name(&report_path)?;
    let output_name = destination_name(&files.output)?;
    let resolved_output = fs::canonicalize(&files.output).ok();
    let resolved_report = fs::canonicalize(&report_path).ok();
    // Also reject output -> report symlinks: replacing that report pathname
    // would change what the old output resolves to even if generation fails.
    if report_name == output_name
        || resolved_output
            .as_ref()
            .is_some_and(|p| p == &report_name || Some(p) == resolved_report.as_ref())
    {
        return Err(input_error(
            "map output and replay report must be different paths",
        ));
    }
    // Replace the report pathname through a fresh inode. A pre-existing symlink
    // or hardlink to the map must never truncate the map while writing diagnostics.
    let (report_temporary, file) = Temporary::create(&report_path)?;
    let mut report = BufWriter::new(file);
    input.write(&mut report)?;
    writeln!(report, "\n[diagnostics]\nassets {}", files.data.display())?;
    report.flush()?; // Inputs survive even a resource or generation failure.
    report_temporary.publish(&report_path)?;
    let result = generate(files, input, &mut report);
    if let Err(error) = &result {
        writeln!(report, "failure {error}")?;
    }
    report.flush()?;
    result
}
fn generate(files: &Files, input: &Replay, report: &mut impl Write) -> Result<()> {
    let request = Request::parse(input.request, input.behavior)?;
    writeln!(report, "effective request {:?}", request.repaired_record())?;
    let mut installation = Installation::open(&files.data)?;
    // Only names/templates borrow resource bytes. Trait parsers own compact
    // tables, allowing their shared byte scratch to be recycled immediately.
    let mut scratch = Vec::new();
    installation.text("rand_trn.txt", &mut scratch)?;
    let placement = PlacementRules::parse(&scratch, input.behavior)?;
    installation.text("crtraits.txt", &mut scratch)?;
    let creatures = CreatureCatalog::parse(&scratch)?;
    installation.text("sptraits.txt", &mut scratch)?;
    let spells = SpellCatalog::parse(&scratch)?;
    installation.text("artraits.txt", &mut scratch)?;
    let artifacts = ArtifactCatalog::parse(&scratch)?;
    drop(scratch);
    let mut template_bytes = Vec::new();
    installation.text("rmg.txt", &mut template_bytes)?;
    let templates = TemplateSource::parse(&template_bytes)?;
    let mut prototype_bytes = Vec::new();
    installation.text("objects.txt", &mut prototype_bytes)?;
    let prototypes = PrototypeSource::parse(&prototype_bytes, |name| installation.mask(name))?;
    drop(installation);
    let assets = Assets {
        templates: &templates,
        prototypes: &prototypes,
        placement: &placement,
        creatures: &creatures,
        spells: &spells,
        artifacts: &artifacts,
    };
    let prepared = match assets.prepare(request, input.seed) {
        Ok(p) => p,
        Err(e) => {
            writeln!(report, "generation {:#?}", e.report)?;
            return Err(e.into());
        }
    };
    let mut workspace = GenerationWorkspace::default();
    let map = match workspace.generate(&prepared) {
        Ok(map) => map,
        Err(e) => {
            writeln!(report, "generation {:#?}", e.report)?;
            return Err(e.into());
        }
    };
    writeln!(report, "generation {:#?}", map.report())?;
    // Same-directory temporary file keeps a failed write from replacing output.
    let (temporary, file) = Temporary::create(&files.output)?;
    let destination = BufWriter::new(file);
    let mut compressed = GzBuilder::new()
        .mtime(0)
        .operating_system(0)
        .write(destination, Compression::new(6));
    let output = match OutputWorkspace::default().write(&map, &mut compressed) {
        Ok(output) => output,
        Err(e) => {
            writeln!(report, "output {e:?}")?;
            return Err(e.into());
        }
    };
    writeln!(report, "serialization {output:?}")?;
    report.flush()?;
    let mut destination = compressed.finish()?;
    destination.flush()?;
    drop(destination);
    temporary.publish(&files.output)?;
    writeln!(report, "output {output:?}\nmap {}", files.output.display())?;
    eprintln!(
        "wrote {} (RNG draws {})",
        files.output.display(),
        output.rng.draws
    );
    Ok(())
}
// Canonicalize parent aliases while retaining the final pathname: both output
// and report are replaced by rename, which does not follow an existing symlink.
fn destination_name(path: &Path) -> io::Result<PathBuf> {
    let name = path
        .file_name()
        .ok_or_else(|| io::Error::new(io::ErrorKind::InvalidInput, "output requires a filename"))?;
    let parent = path
        .parent()
        .filter(|p| !p.as_os_str().is_empty())
        .unwrap_or_else(|| Path::new("."));
    Ok(fs::canonicalize(parent)?.join(name))
}
struct Temporary(Option<PathBuf>);
impl Temporary {
    fn publish(mut self, destination: &Path) -> io::Result<()> {
        fs::rename(self.0.as_ref().expect("unpublished temporary"), destination)?;
        self.0 = None; // The old pathname may now be reused by another output.
        Ok(())
    }
    fn create(output: &Path) -> io::Result<(Self, File)> {
        for ordinal in 0..100 {
            let mut path = output.as_os_str().to_owned();
            path.push(format!(".{}.{}.tmp", std::process::id(), ordinal));
            let path = PathBuf::from(path);
            match OpenOptions::new().write(true).create_new(true).open(&path) {
                Ok(file) => return Ok((Self(Some(path)), file)),
                Err(e) if e.kind() == io::ErrorKind::AlreadyExists => {}
                Err(e) => return Err(e),
            }
        }
        Err(io::Error::new(
            io::ErrorKind::AlreadyExists,
            "could not reserve temporary output",
        ))
    }
}
impl Drop for Temporary {
    fn drop(&mut self) {
        if let Some(path) = &self.0 {
            let _ = fs::remove_file(path);
        }
    }
}
