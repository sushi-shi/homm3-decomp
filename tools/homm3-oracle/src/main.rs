//! Corpus census and inspection commands for Heroes III resources.

use std::collections::BTreeMap;
use std::error::Error;
use std::fs;
use std::io::{self, Read};
use std::path::{Path, PathBuf};
use std::process::ExitCode;

use clap::{Parser, Subcommand, ValueEnum};
use flate2::bufread::GzDecoder as MemberGzDecoder;
use flate2::read::{GzDecoder, ZlibDecoder};
use homm3_archive::{gzip::Member as GzipMember, SoundArchive, VideoArchive};
use homm3_def::{Dialect, Encoding, Frame, Run, Sprite};
use homm3_lod::{Archive, Entry, Payload};
use homm3_map::{
    campaign::{Campaign, Version as CampaignVersion},
    MapBody, MapHeader, ObjectTable, Terrain, Version as MapVersion, WorldPrefix,
};
use homm3_resource::{iff::Xmidi, Bitmap, BitmapKind, Font, Mask, Palette, Spreadsheet, Text};
use homm3_save::{Kind as SaveKind, SaveGame};

type Result<T> = std::result::Result<T, Box<dyn Error>>;

#[derive(Debug, Parser)]
#[command(version, about)]
struct Cli {
    /// Retail LOD archive to search. Repeat to supply an overlay order.
    #[arg(long, global = true, value_name = "PATH")]
    lod: Vec<PathBuf>,

    /// Retail SND archive to validate. Repeat for base and expansion files.
    #[arg(long, global = true, value_name = "PATH")]
    snd: Vec<PathBuf>,

    /// Retail VID archive to validate. Repeat for base and expansion files.
    #[arg(long, global = true, value_name = "PATH")]
    vid: Vec<PathBuf>,

    /// Scenario map file or directory to validate. Repeat for more roots.
    #[arg(long, global = true, value_name = "PATH")]
    map: Vec<PathBuf>,

    /// Saved game file or directory to validate. Repeat for more roots.
    #[arg(long, global = true, value_name = "PATH")]
    save: Vec<PathBuf>,

    /// Explicit DEF pressing dialect; retail remains the strict default.
    #[arg(long, global = true, value_enum, default_value_t = DefDialect::Retail)]
    def_dialect: DefDialect,

    #[command(subcommand)]
    command: Command,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, ValueEnum)]
enum DefDialect {
    /// English GOG Complete 4.0 / retail layout.
    #[default]
    Retail,
    /// Apply the admitted interleaved-header layout to two known DEF members.
    #[value(alias("steam"))]
    KnownInterleaved,
}

#[derive(Debug, Subcommand)]
enum Command {
    /// List directory members without inflating them.
    List {
        /// Only show members whose names end in this ASCII extension.
        #[arg(long, value_name = "EXTENSION")]
        extension: Option<String>,
    },
    /// Parse and validate every DEF in the supplied archives.
    Census {
        /// Show up to this many representative members per encoding.
        #[arg(long, default_value_t = 3)]
        examples: usize,
    },
    /// Parse every SND and VID index and validate every member extent.
    Containers,
    /// Validate every engine-owned payload and external-handoff envelope in LOD.
    Resources,
    /// Inflate and validate every H3M/TUT scenario header under --map paths.
    Maps,
    /// Inflate and validate every GM1..GM8, TGM, and CGM save under --save paths.
    Saves,
    /// Print packet runs for one frame.
    Tokens {
        /// LOD member name, matched case-insensitively.
        member: String,
        /// Group table index.
        #[arg(long, default_value_t = 0)]
        group: usize,
        /// Frame index within the group.
        #[arg(long, default_value_t = 0)]
        frame: usize,
    },
    /// Decode one frame to an RGBA PNG for visual inspection.
    Dump {
        /// LOD member name, matched case-insensitively.
        member: String,
        /// Output PNG path.
        output: PathBuf,
        /// Group table index.
        #[arg(long, default_value_t = 0)]
        group: usize,
        /// Frame index within the group.
        #[arg(long, default_value_t = 0)]
        frame: usize,
    },
    /// Inflate and write one LOD member exactly as the game receives it.
    Extract {
        /// LOD member name, matched case-insensitively.
        member: String,
        /// Output path.
        output: PathBuf,
    },
}

#[derive(Default)]
struct Census {
    archives: usize,
    members: usize,
    def_members: usize,
    groups: usize,
    frames: usize,
    packed_members: usize,
    def_types: BTreeMap<u32, usize>,
    encodings: BTreeMap<Encoding, usize>,
    examples: BTreeMap<Encoding, Vec<String>>,
    failures: Vec<String>,
}

#[derive(Default)]
struct ContainerCensus {
    sound_archives: usize,
    sound_members: usize,
    video_archives: usize,
    video_members: usize,
    payload_magics: BTreeMap<String, usize>,
    video_extensions: BTreeMap<String, usize>,
    failures: Vec<String>,
}

#[derive(Default)]
struct ResourceCensus {
    archives: usize,
    members: usize,
    extensions: BTreeMap<String, usize>,
    indexed_bitmaps: usize,
    packed24_bitmaps: usize,
    bitmap_pixels: usize,
    palettes: usize,
    fonts: usize,
    font_glyph_bytes: usize,
    masks: usize,
    text_files: usize,
    text_rows: usize,
    spreadsheet_cells: usize,
    xmidi_files: usize,
    xmidi_tracks: usize,
    force_feedback_blobs: usize,
    external_handoff_bytes: usize,
    campaigns: usize,
    campaign_scenarios: usize,
    campaign_maps: usize,
    campaign_versions: BTreeMap<CampaignVersion, usize>,
    campaign_map_versions: BTreeMap<MapVersion, usize>,
    failures: Vec<String>,
}

struct InflatedGzipMember {
    compressed_len: usize,
    bytes: Vec<u8>,
}

struct InflatedCampaign {
    header: Vec<u8>,
    maps: Vec<InflatedGzipMember>,
}

#[derive(Default)]
struct MapCensus {
    files: usize,
    compressed_bytes: usize,
    inflated_bytes: usize,
    header_bytes: usize,
    world_bytes: usize,
    world_prefix_bytes: usize,
    map_stream_bytes: usize,
    terrain_cells: usize,
    terrain_bytes: usize,
    object_stream_bytes: usize,
    object_types: usize,
    placed_objects: usize,
    placed_object_bytes: usize,
    timed_events: usize,
    timed_event_bytes: usize,
    object_classes: BTreeMap<u32, usize>,
    versions: BTreeMap<MapVersion, usize>,
    dimensions: BTreeMap<i32, usize>,
    failures: Vec<String>,
}

#[derive(Default)]
struct SaveCensus {
    files: usize,
    game_saves: usize,
    campaign_saves: usize,
    compressed_bytes: usize,
    inflated_bytes: usize,
    cells: usize,
    cell_object_references: usize,
    object_types: usize,
    objects: usize,
    timed_events: usize,
    town_events: usize,
    towns: usize,
    heroes: usize,
    campaign_heroes: usize,
    recorded_events: usize,
    versions: BTreeMap<i32, usize>,
    map_versions: BTreeMap<i32, usize>,
    map_dimensions: BTreeMap<i32, usize>,
    failures: Vec<String>,
}

fn main() -> ExitCode {
    match run(Cli::parse()) {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            eprintln!("error: {error}");
            ExitCode::FAILURE
        }
    }
}

fn run(cli: Cli) -> Result<()> {
    match cli.command {
        Command::List { extension } => {
            require_lod(&cli.lod)?;
            list(&cli.lod, extension.as_deref())
        }
        Command::Census { examples } => {
            require_lod(&cli.lod)?;
            census(&cli.lod, examples, cli.def_dialect)
        }
        Command::Containers => container_census(&cli.snd, &cli.vid),
        Command::Resources => {
            require_lod(&cli.lod)?;
            resource_census(&cli.lod)
        }
        Command::Maps => map_census(&cli.map),
        Command::Saves => save_census(&cli.save),
        Command::Tokens {
            member,
            group,
            frame,
        } => {
            require_lod(&cli.lod)?;
            with_frame(
                &cli.lod,
                &member,
                group,
                frame,
                cli.def_dialect,
                print_tokens,
            )
        }
        Command::Dump {
            member,
            output,
            group,
            frame,
        } => {
            require_lod(&cli.lod)?;
            with_frame(
                &cli.lod,
                &member,
                group,
                frame,
                cli.def_dialect,
                |sprite, frame| dump_png(sprite, frame, &output),
            )
        }
        Command::Extract { member, output } => {
            require_lod(&cli.lod)?;
            extract_member(&cli.lod, &member, &output)
        }
    }
}

fn save_census(roots: &[PathBuf]) -> Result<()> {
    if roots.is_empty() {
        return Err(invalid_data("at least one --save PATH is required"));
    }
    let mut paths = Vec::new();
    for root in roots {
        collect_save_paths(root, &mut paths)?;
    }
    paths.sort();
    paths.dedup();
    if paths.is_empty() {
        return Err(invalid_data(
            "no .GM1 through .GM8, .TGM, or .CGM files found under --save paths",
        ));
    }

    let mut result = SaveCensus::default();
    for path in paths {
        if let Err(error) = census_save_file(&path, &mut result) {
            result.failures.push(format!("{}: {error}", path.display()));
        }
    }
    report_save_census(&result)
}

fn collect_save_paths(path: &Path, output: &mut Vec<PathBuf>) -> Result<()> {
    if path.is_dir() {
        for entry in fs::read_dir(path)? {
            collect_save_paths(&entry?.path(), output)?;
        }
    } else if path.is_file() && is_save_path(path) {
        output.push(path.to_owned());
    }
    Ok(())
}

fn is_save_path(path: &Path) -> bool {
    path.extension()
        .and_then(|extension| extension.to_str())
        .is_some_and(|extension| {
            let extension = extension.to_ascii_lowercase();
            matches!(
                extension.as_str(),
                "tgm" | "cgm" | "gm1" | "gm2" | "gm3" | "gm4" | "gm5" | "gm6" | "gm7" | "gm8"
            )
        })
}

fn census_save_file(path: &Path, result: &mut SaveCensus) -> Result<()> {
    let compressed = fs::read(path)?;
    let envelope = GzipMember::parse(&compressed)?;
    let mut decoder = GzDecoder::new(compressed.as_slice());
    let mut inflated = Vec::new();
    decoder.read_to_end(&mut inflated)?;
    let inflated_size = u32::try_from(inflated.len())
        .map_err(|_| invalid_data("inflated save exceeds the gzip 32-bit size domain"))?;
    if inflated_size != envelope.uncompressed_size() {
        return Err(invalid_data(format!(
            "gzip trailer size {} differs from inflated size {inflated_size}",
            envelope.uncompressed_size()
        )));
    }

    let save = SaveGame::parse(&inflated)?;
    result.files += 1;
    match save.kind {
        SaveKind::Game => result.game_saves += 1,
        SaveKind::Campaign => result.campaign_saves += 1,
    }
    result.compressed_bytes += compressed.len();
    result.inflated_bytes += inflated.len();
    result.cells += save.counts.cells;
    result.cell_object_references += save.counts.cell_object_references;
    result.object_types += save.counts.object_types;
    result.objects += save.counts.objects;
    result.timed_events += save.counts.timed_events;
    result.town_events += save.counts.town_events;
    result.towns += save.counts.towns;
    result.heroes += save.counts.heroes;
    result.campaign_heroes += save.counts.campaign_heroes;
    result.recorded_events += save.counts.recorded_events;
    *result.versions.entry(save.version).or_default() += 1;
    *result.map_versions.entry(save.map_version).or_default() += 1;
    *result.map_dimensions.entry(save.map_size).or_default() += 1;
    Ok(())
}

fn report_save_census(result: &SaveCensus) -> Result<()> {
    println!("save files:             {}", result.files);
    println!("ordinary saves:         {}", result.game_saves);
    println!("campaign saves:         {}", result.campaign_saves);
    println!("compressed bytes:       {}", result.compressed_bytes);
    println!("inflated bytes:         {}", result.inflated_bytes);
    println!("map cells:              {}", result.cells);
    println!("cell object references: {}", result.cell_object_references);
    println!("object templates:       {}", result.object_types);
    println!("placed objects:         {}", result.objects);
    println!("timed events:           {}", result.timed_events);
    println!("town events:            {}", result.town_events);
    println!("towns:                  {}", result.towns);
    println!("hero records:           {}", result.heroes);
    println!("campaign heroes:        {}", result.campaign_heroes);
    println!("recorded events:        {}", result.recorded_events);
    println!("save versions:");
    for (version, count) in &result.versions {
        println!("  {version}: {count}");
    }
    println!("saved map versions:");
    for (version, count) in &result.map_versions {
        println!("  {version}: {count}");
    }
    println!("saved map dimensions:");
    for (dimension, count) in &result.map_dimensions {
        println!("  {dimension}: {count}");
    }
    if !result.failures.is_empty() {
        println!("failures:");
        for failure in &result.failures {
            println!("  {failure}");
        }
        return Err(invalid_data(format!(
            "{} saved game(s) failed validation",
            result.failures.len()
        )));
    }
    Ok(())
}

fn require_lod(paths: &[PathBuf]) -> Result<()> {
    if paths.is_empty() {
        Err(invalid_data("at least one --lod PATH is required"))
    } else {
        Ok(())
    }
}

fn map_census(roots: &[PathBuf]) -> Result<()> {
    if roots.is_empty() {
        return Err(invalid_data("at least one --map PATH is required"));
    }
    let mut paths = Vec::new();
    for root in roots {
        collect_map_paths(root, &mut paths)?;
    }
    paths.sort();
    paths.dedup();
    if paths.is_empty() {
        return Err(invalid_data(
            "no .h3m or .tut files found under --map paths",
        ));
    }

    let mut result = MapCensus::default();
    for path in paths {
        if let Err(error) = census_map_file(&path, &mut result) {
            result.failures.push(format!("{}: {error}", path.display()));
        }
    }
    report_map_census(&result)
}

fn collect_map_paths(path: &Path, output: &mut Vec<PathBuf>) -> Result<()> {
    if path.is_dir() {
        for entry in fs::read_dir(path)? {
            collect_map_paths(&entry?.path(), output)?;
        }
    } else if path.is_file() && is_map_path(path) {
        output.push(path.to_owned());
    }
    Ok(())
}

fn is_map_path(path: &Path) -> bool {
    path.extension()
        .and_then(|extension| extension.to_str())
        .is_some_and(|extension| {
            extension.eq_ignore_ascii_case("h3m") || extension.eq_ignore_ascii_case("tut")
        })
}

fn census_map_file(path: &Path, result: &mut MapCensus) -> Result<()> {
    let compressed = fs::read(path)?;
    let envelope = GzipMember::parse(&compressed)?;
    let mut decoder = GzDecoder::new(compressed.as_slice());
    let mut inflated = Vec::new();
    decoder.read_to_end(&mut inflated)?;
    let inflated_size = u32::try_from(inflated.len())
        .map_err(|_| invalid_data("inflated map exceeds the gzip 32-bit size domain"))?;
    if inflated_size != envelope.uncompressed_size() {
        return Err(invalid_data(format!(
            "gzip trailer size {} differs from inflated size {inflated_size}",
            envelope.uncompressed_size()
        )));
    }
    let header = MapHeader::parse(&inflated)?;
    let prefix = WorldPrefix::parse(header.world(), header.version)?;
    let terrain = Terrain::parse(prefix.map(), header.size, header.two_layers)?;
    let objects = ObjectTable::parse(terrain.objects())?;
    let body = MapBody::parse(objects, header.version)?;
    if !body.trailing().is_empty() {
        return Err(invalid_data(format!(
            "map body has {} trailing inflated bytes",
            body.trailing().len()
        )));
    }

    result.files += 1;
    result.compressed_bytes += compressed.len();
    result.inflated_bytes += inflated.len();
    result.header_bytes += header.header_len();
    result.world_bytes += header.world().len();
    result.world_prefix_bytes += prefix.consumed();
    result.map_stream_bytes += prefix.map().len();
    result.terrain_cells += terrain.len();
    result.terrain_bytes += terrain.len() * 7;
    result.object_stream_bytes += terrain.objects().len();
    result.object_types += usize::try_from(objects.type_count())?;
    result.placed_objects += usize::try_from(objects.object_count())?;
    result.placed_object_bytes += body.object_bytes().len();
    result.timed_events += usize::try_from(body.timed_events().len())?;
    result.timed_event_bytes += body.event_bytes().len();
    for object in body.objects() {
        *result
            .object_classes
            .entry(object.object_class)
            .or_default() += 1;
    }
    *result.versions.entry(header.version).or_default() += 1;
    *result.dimensions.entry(header.size).or_default() += 1;
    Ok(())
}

fn report_map_census(result: &MapCensus) -> Result<()> {
    println!("scenario files:        {}", result.files);
    println!("compressed bytes:      {}", result.compressed_bytes);
    println!("inflated bytes:        {}", result.inflated_bytes);
    println!("header bytes:          {}", result.header_bytes);
    println!("world bytes:           {}", result.world_bytes);
    println!("world-prefix bytes:    {}", result.world_prefix_bytes);
    println!("terrain/object bytes:  {}", result.map_stream_bytes);
    println!("terrain cells:         {}", result.terrain_cells);
    println!("terrain bytes:         {}", result.terrain_bytes);
    println!("object/event bytes:    {}", result.object_stream_bytes);
    println!("object templates:      {}", result.object_types);
    println!("placed objects:        {}", result.placed_objects);
    println!("placed-object bytes:   {}", result.placed_object_bytes);
    println!("timed events:          {}", result.timed_events);
    println!("timed-event bytes:     {}", result.timed_event_bytes);
    println!("placed object classes:");
    for (object_class, count) in &result.object_classes {
        println!("  {object_class:>3}: {count}");
    }
    println!("map versions:");
    for (version, count) in &result.versions {
        println!("  {version:?}: {count}");
    }
    println!("map dimensions:");
    for (dimension, count) in &result.dimensions {
        println!("  {dimension}: {count}");
    }
    if !result.failures.is_empty() {
        println!("failures:");
        for failure in &result.failures {
            println!("  {failure}");
        }
        return Err(invalid_data(format!(
            "{} scenario map(s) failed validation",
            result.failures.len()
        )));
    }
    Ok(())
}

fn load_archive(path: &Path) -> Result<Vec<u8>> {
    fs::read(path).map_err(|error| invalid_data(format!("cannot read {}: {error}", path.display())))
}

fn list(paths: &[PathBuf], extension: Option<&str>) -> Result<()> {
    for path in paths {
        let image = load_archive(path)?;
        let archive = Archive::parse(&image)
            .map_err(|error| invalid_data(format!("{}: {error}", path.display())))?;
        println!(
            "{}: version={} entries={}",
            path.display(),
            archive.version(),
            archive.len()
        );
        for entry in archive
            .entries()
            .filter(|entry| extension.is_none_or(|wanted| entry.has_extension(wanted)))
        {
            println!(
                "  {:16} {:>10} bytes  {}",
                display_name(entry),
                entry.size,
                if entry.is_compressed() {
                    "zlib"
                } else {
                    "stored"
                }
            );
        }
    }
    Ok(())
}

fn container_census(sound_paths: &[PathBuf], video_paths: &[PathBuf]) -> Result<()> {
    if sound_paths.is_empty() && video_paths.is_empty() {
        return Err(invalid_data(
            "at least one --snd PATH or --vid PATH is required",
        ));
    }

    let mut result = ContainerCensus::default();
    for path in sound_paths {
        result.sound_archives += 1;
        let image = match load_archive(path) {
            Ok(image) => image,
            Err(error) => {
                result.failures.push(error.to_string());
                continue;
            }
        };
        let archive = match SoundArchive::parse(&image) {
            Ok(archive) => archive,
            Err(error) => {
                result.failures.push(format!("{}: {error}", path.display()));
                continue;
            }
        };
        result.sound_members += archive.len();
        for entry in archive.entries() {
            match archive.payload(entry) {
                Ok(bytes) => increment_magic(&mut result.payload_magics, bytes),
                Err(error) => result.failures.push(format!(
                    "{}:{}: {error}",
                    path.display(),
                    display_bytes(entry.name)
                )),
            }
        }
    }

    for path in video_paths {
        result.video_archives += 1;
        let image = match load_archive(path) {
            Ok(image) => image,
            Err(error) => {
                result.failures.push(error.to_string());
                continue;
            }
        };
        let archive = match VideoArchive::parse(&image) {
            Ok(archive) => archive,
            Err(error) => {
                result.failures.push(format!("{}: {error}", path.display()));
                continue;
            }
        };
        result.video_members += archive.len();
        for entry in archive.entries() {
            *result
                .video_extensions
                .entry(extension(entry.name))
                .or_default() += 1;
            match archive.payload(entry) {
                Ok(bytes) => increment_magic(&mut result.payload_magics, bytes),
                Err(error) => result.failures.push(format!(
                    "{}:{}: {error}",
                    path.display(),
                    display_bytes(entry.name)
                )),
            }
        }
    }

    println!("SND archives:          {}", result.sound_archives);
    println!("SND members:           {}", result.sound_members);
    println!("VID archives:          {}", result.video_archives);
    println!("VID members:           {}", result.video_members);
    println!("VID extensions:");
    for (extension, count) in &result.video_extensions {
        println!("  {extension}: {count}");
    }
    println!("payload magics:");
    for (magic, count) in &result.payload_magics {
        println!("  {magic}: {count}");
    }
    if !result.failures.is_empty() {
        println!("failures:");
        for failure in &result.failures {
            println!("  {failure}");
        }
        return Err(invalid_data(format!(
            "{} container item(s) failed validation",
            result.failures.len()
        )));
    }
    Ok(())
}

fn resource_census(paths: &[PathBuf]) -> Result<()> {
    let mut result = ResourceCensus::default();
    for path in paths {
        result.archives += 1;
        let image = match load_archive(path) {
            Ok(image) => image,
            Err(error) => {
                result.failures.push(error.to_string());
                continue;
            }
        };
        let archive = match Archive::parse(&image) {
            Ok(archive) => archive,
            Err(error) => {
                result.failures.push(format!("{}: {error}", path.display()));
                continue;
            }
        };
        result.members += archive.len();
        for entry in archive.entries() {
            let member_extension = extension(entry.name);
            *result
                .extensions
                .entry(member_extension.clone())
                .or_default() += 1;
            if !matches!(
                member_extension.as_str(),
                ".pcx" | ".pal" | ".fnt" | ".msk" | ".txt" | ".xmi" | ".ifr" | ".h3c"
            ) {
                continue;
            }
            let label = format!("{}:{}", path.display(), display_name(entry));
            let bytes = match member_bytes(&archive, entry) {
                Ok(bytes) => bytes,
                Err(error) => {
                    result.failures.push(format!("{label}: {error}"));
                    continue;
                }
            };
            let validation: std::result::Result<(), String> = match member_extension.as_str() {
                ".pcx" => census_bitmap(&bytes, &mut result).map_err(|error| error.to_string()),
                ".pal" => Palette::parse(&bytes)
                    .map(|_| result.palettes += 1)
                    .map_err(|error| error.to_string()),
                ".fnt" => Font::parse(&bytes)
                    .map(|font| {
                        result.fonts += 1;
                        result.font_glyph_bytes += font.data().len();
                    })
                    .map_err(|error| error.to_string()),
                ".msk" => Mask::parse(&bytes)
                    .map(|_| result.masks += 1)
                    .map_err(|error| error.to_string()),
                ".txt" => census_text(&bytes, &mut result).map_err(|error| error.to_string()),
                ".xmi" => Xmidi::parse(&bytes)
                    .map(|xmidi| {
                        result.xmidi_files += 1;
                        result.xmidi_tracks += usize::from(xmidi.track_count());
                        result.external_handoff_bytes += bytes.len();
                    })
                    .map_err(|error| error.to_string()),
                ".ifr" if bytes.starts_with(b"ifpr") => {
                    result.force_feedback_blobs += 1;
                    result.external_handoff_bytes += bytes.len();
                    Ok(())
                }
                ".ifr" => Err("force-feedback payload does not begin with ifpr".to_owned()),
                ".h3c" => census_campaign(&bytes, &mut result).map_err(|error| error.to_string()),
                _ => unreachable!(),
            };
            if let Err(error) = validation {
                result.failures.push(format!("{label}: {error}"));
            }
        }
    }

    report_resource_census(&result)
}

fn report_resource_census(result: &ResourceCensus) -> Result<()> {
    println!("LOD archives:          {}", result.archives);
    println!("LOD members:           {}", result.members);
    println!("member extensions:");
    for (extension, count) in &result.extensions {
        println!("  {extension}: {count}");
    }
    println!("indexed bitmaps:       {}", result.indexed_bitmaps);
    println!("packed-24 bitmaps:     {}", result.packed24_bitmaps);
    println!("bitmap pixels:         {}", result.bitmap_pixels);
    println!("palettes:              {}", result.palettes);
    println!("fonts:                 {}", result.fonts);
    println!("font glyph bytes:      {}", result.font_glyph_bytes);
    println!("masks:                 {}", result.masks);
    println!("text files:            {}", result.text_files);
    println!("text rows:             {}", result.text_rows);
    println!("spreadsheet cells:     {}", result.spreadsheet_cells);
    println!("XMIDI files:           {}", result.xmidi_files);
    println!("XMIDI tracks:          {}", result.xmidi_tracks);
    println!("force-feedback blobs:  {}", result.force_feedback_blobs);
    println!("external handoff bytes:{}", result.external_handoff_bytes);
    println!("campaigns:             {}", result.campaigns);
    println!("campaign scenarios:    {}", result.campaign_scenarios);
    println!("campaign maps:         {}", result.campaign_maps);
    println!("campaign versions:");
    for (version, count) in &result.campaign_versions {
        println!("  {version:?}: {count}");
    }
    println!("campaign map versions:");
    for (version, count) in &result.campaign_map_versions {
        println!("  {version:?}: {count}");
    }
    if !result.failures.is_empty() {
        println!("failures:");
        for failure in &result.failures {
            println!("  {failure}");
        }
        return Err(invalid_data(format!(
            "{} LOD resource(s) failed validation",
            result.failures.len()
        )));
    }
    Ok(())
}

fn census_bitmap(
    bytes: &[u8],
    result: &mut ResourceCensus,
) -> std::result::Result<(), homm3_resource::Error> {
    let bitmap = Bitmap::parse(bytes)?;
    match bitmap.kind() {
        BitmapKind::Indexed8 => result.indexed_bitmaps += 1,
        BitmapKind::Packed24 => result.packed24_bitmaps += 1,
    }
    result.bitmap_pixels += bitmap.width() * bitmap.height();
    Ok(())
}

fn census_text(
    bytes: &[u8],
    result: &mut ResourceCensus,
) -> std::result::Result<(), homm3_resource::Error> {
    let text = Text::parse(bytes)?;
    let spreadsheet = Spreadsheet::parse(bytes)?;
    result.text_files += 1;
    result.text_rows += text.len();
    result.spreadsheet_cells += spreadsheet
        .rows()
        .map(homm3_resource::SpreadsheetRow::len)
        .sum::<usize>();
    Ok(())
}

fn census_campaign(bytes: &[u8], result: &mut ResourceCensus) -> Result<()> {
    let envelope = inflate_campaign(bytes)?;
    let campaign = Campaign::parse(&envelope.header)?;
    if !campaign.trailing().is_empty() {
        return Err(invalid_data(format!(
            "campaign header has {} trailing inflated bytes",
            campaign.trailing().len()
        )));
    }
    if envelope.maps.len() != campaign.embedded_map_count() {
        return Err(invalid_data(format!(
            "campaign declares {} embedded maps but has {} gzip map members",
            campaign.embedded_map_count(),
            envelope.maps.len()
        )));
    }

    for (member, scenario) in envelope
        .maps
        .iter()
        .zip(campaign.iter().filter(|scenario| scenario.is_not_void()))
    {
        let declared_size = usize::try_from(scenario.packed_map_size)
            .map_err(|_| invalid_data("campaign packed map size exceeds the host"))?;
        if declared_size != member.compressed_len {
            return Err(invalid_data(format!(
                "campaign map {} declares {declared_size} compressed bytes, member has {}",
                display_bytes(scenario.map_name.bytes()),
                member.compressed_len
            )));
        }
        let map = MapHeader::parse(&member.bytes)?;
        let prefix = WorldPrefix::parse(map.world(), map.version)?;
        let terrain = Terrain::parse(prefix.map(), map.size, map.two_layers)?;
        let objects = ObjectTable::parse(terrain.objects())?;
        let body = MapBody::parse(objects, map.version)?;
        if !body.trailing().is_empty() {
            return Err(invalid_data(format!(
                "campaign map {} has {} trailing inflated bytes",
                display_bytes(scenario.map_name.bytes()),
                body.trailing().len()
            )));
        }
        *result.campaign_map_versions.entry(map.version).or_default() += 1;
    }

    result.campaigns += 1;
    result.campaign_scenarios += campaign.scenario_count();
    result.campaign_maps += campaign.embedded_map_count();
    *result
        .campaign_versions
        .entry(campaign.version)
        .or_default() += 1;
    Ok(())
}

fn inflate_campaign(bytes: &[u8]) -> Result<InflatedCampaign> {
    if bytes.starts_with(&[0x1f, 0x8b]) {
        let mut members = inflate_gzip_members(bytes)?;
        if members.is_empty() {
            return Err(invalid_data("campaign has no gzip header member"));
        }
        let header = members.remove(0).bytes;
        return Ok(InflatedCampaign {
            header,
            maps: members,
        });
    }

    // Restoration of Erathia stores the campaign header directly and starts
    // the concatenated gzip map members immediately after its last scenario.
    let campaign = Campaign::parse(bytes)?;
    let header_len = bytes
        .len()
        .checked_sub(campaign.trailing().len())
        .ok_or_else(|| invalid_data("campaign header extent underflows"))?;
    let header = copy_bytes(&bytes[..header_len], "campaign header")?;
    let maps = inflate_gzip_members(campaign.trailing())?;
    Ok(InflatedCampaign { header, maps })
}

fn inflate_gzip_members(mut input: &[u8]) -> Result<Vec<InflatedGzipMember>> {
    let mut members = Vec::new();
    while !input.is_empty() {
        let current = input;
        let mut decoder = MemberGzDecoder::new(current);
        let mut bytes = Vec::new();
        decoder.read_to_end(&mut bytes)?;
        let remaining = decoder.into_inner();
        let compressed_len = current.len().saturating_sub(remaining.len());
        if compressed_len == 0 {
            return Err(invalid_data("gzip member decoder made no progress"));
        }
        let envelope = GzipMember::parse(&current[..compressed_len])?;
        let inflated_size = u32::try_from(bytes.len())
            .map_err(|_| invalid_data("inflated gzip member exceeds 32-bit size domain"))?;
        if inflated_size != envelope.uncompressed_size() {
            return Err(invalid_data(format!(
                "gzip member trailer size {} differs from inflated size {inflated_size}",
                envelope.uncompressed_size()
            )));
        }
        members.push(InflatedGzipMember {
            compressed_len,
            bytes,
        });
        input = remaining;
    }
    Ok(members)
}

fn increment_magic(counts: &mut BTreeMap<String, usize>, bytes: &[u8]) {
    let prefix = bytes.get(..4).unwrap_or(bytes);
    let magic = prefix
        .iter()
        .map(|&byte| {
            if byte.is_ascii_graphic() {
                char::from(byte)
            } else {
                '.'
            }
        })
        .collect();
    *counts.entry(magic).or_default() += 1;
}

fn extension(name: &[u8]) -> String {
    name.iter().rposition(|&byte| byte == b'.').map_or_else(
        || "(none)".to_owned(),
        |at| display_bytes(&name[at..]).to_ascii_lowercase(),
    )
}

fn display_bytes(bytes: &[u8]) -> String {
    core::str::from_utf8(bytes).map_or_else(|_| format!("{bytes:02x?}"), str::to_owned)
}

fn census(paths: &[PathBuf], example_limit: usize, dialect: DefDialect) -> Result<()> {
    let mut result = Census::default();
    for path in paths {
        result.archives += 1;
        let image = match load_archive(path) {
            Ok(image) => image,
            Err(error) => {
                result.failures.push(error.to_string());
                continue;
            }
        };
        let archive = match Archive::parse(&image) {
            Ok(archive) => archive,
            Err(error) => {
                result.failures.push(format!("{}: {error}", path.display()));
                continue;
            }
        };
        result.members += archive.len();
        for entry in archive.entries() {
            if entry.is_compressed() {
                result.packed_members += 1;
            }
            if !entry.has_extension(".def") {
                continue;
            }
            result.def_members += 1;
            let label = format!("{}:{}", path.display(), display_name(entry));
            let bytes = match member_bytes(&archive, entry) {
                Ok(bytes) => bytes,
                Err(error) => {
                    result.failures.push(format!("{label}: {error}"));
                    continue;
                }
            };
            let sprite = match Sprite::parse_with_dialect(&bytes, dialect_for(entry, dialect)) {
                Ok(sprite) => sprite,
                Err(error) => {
                    result.failures.push(format!("{label}: {error}"));
                    continue;
                }
            };
            *result.def_types.entry(sprite.header().kind.0).or_default() += 1;
            result.groups += sprite.group_count();
            for (group_index, group) in sprite.groups().enumerate() {
                for frame_index in 0..group.len() {
                    result.frames += 1;
                    let frame = match group.frame(frame_index) {
                        Ok(frame) => frame,
                        Err(error) => {
                            result.failures.push(format!(
                                "{label} group {group_index} frame {frame_index}: {error}"
                            ));
                            continue;
                        }
                    };
                    let encoding = frame.encoding();
                    *result.encodings.entry(encoding).or_default() += 1;
                    let examples = result.examples.entry(encoding).or_default();
                    if examples.len() < example_limit {
                        examples.push(format!(
                            "{} g{} f{} {}x{} crop={}x{}+{},{}",
                            display_name(entry),
                            group_index,
                            frame_index,
                            frame.width(),
                            frame.height(),
                            frame.cropped_width(),
                            frame.cropped_height(),
                            frame.cropped_x(),
                            frame.cropped_y()
                        ));
                    }
                    if let Err(error) = frame.validate() {
                        result.failures.push(format!(
                            "{label} group {group_index} frame {frame_index}: {error}"
                        ));
                    }
                }
            }
        }
    }

    print_census(&result);
    if result.failures.is_empty() {
        Ok(())
    } else {
        Err(invalid_data(format!(
            "{} corpus item(s) failed validation",
            result.failures.len()
        )))
    }
}

fn print_census(census: &Census) {
    println!("archives:             {}", census.archives);
    println!("directory members:    {}", census.members);
    println!("zlib members:         {}", census.packed_members);
    println!("DEF members:          {}", census.def_members);
    println!("DEF groups:           {}", census.groups);
    println!("DEF frames:           {}", census.frames);
    println!("DEF types:");
    for (kind, count) in &census.def_types {
        println!("  {kind:>3}: {count}");
    }
    println!("encodings:");
    for (encoding, count) in &census.encodings {
        println!("  {}: {count}", encoding_name(*encoding));
        if let Some(examples) = census.examples.get(encoding) {
            for example in examples {
                println!("    {example}");
            }
        }
    }
    if !census.failures.is_empty() {
        println!("failures:");
        for failure in &census.failures {
            println!("  {failure}");
        }
    }
}

fn with_frame(
    paths: &[PathBuf],
    member: &str,
    group_index: usize,
    frame_index: usize,
    dialect: DefDialect,
    operation: impl FnOnce(Sprite<'_>, Frame<'_>) -> Result<()>,
) -> Result<()> {
    for path in paths {
        let image = load_archive(path)?;
        let archive = Archive::parse(&image)
            .map_err(|error| invalid_data(format!("{}: {error}", path.display())))?;
        let Some(entry) = archive.find(member) else {
            continue;
        };
        let bytes = member_bytes(&archive, entry)?;
        let sprite = Sprite::parse_with_dialect(&bytes, dialect_for(entry, dialect))?;
        let group = sprite.group(group_index)?;
        let frame = group.frame(frame_index)?;
        return operation(sprite, frame);
    }
    Err(invalid_data(format!(
        "member {member:?} was not found in the supplied LOD archives"
    )))
}

fn dialect_for(entry: Entry<'_>, selected: DefDialect) -> Dialect {
    if selected == DefDialect::KnownInterleaved
        && (entry.matches("SGTWMTA.DEF") || entry.matches("SGTWMTB.DEF"))
    {
        Dialect::InterleavedCompactFrames
    } else {
        Dialect::Retail
    }
}

fn extract_member(paths: &[PathBuf], member: &str, output: &Path) -> Result<()> {
    for path in paths {
        let image = load_archive(path)?;
        let archive = Archive::parse(&image)
            .map_err(|error| invalid_data(format!("{}: {error}", path.display())))?;
        let Some(entry) = archive.find(member) else {
            continue;
        };
        let bytes = member_bytes(&archive, entry)?;
        fs::write(output, bytes)?;
        println!("wrote {}", output.display());
        return Ok(());
    }
    Err(invalid_data(format!(
        "member {member:?} was not found in the supplied LOD archives"
    )))
}

fn print_tokens(sprite: Sprite<'_>, frame: Frame<'_>) -> Result<()> {
    println!(
        "type={} name={} encoding={} full={}x{} crop={}x{}+{},{} payload={}",
        sprite.header().kind.0,
        frame.name_str().unwrap_or("<non-UTF-8>"),
        encoding_name(frame.encoding()),
        frame.width(),
        frame.height(),
        frame.cropped_width(),
        frame.cropped_height(),
        frame.cropped_x(),
        frame.cropped_y(),
        frame.data_size()
    );
    for row in 0..frame.cropped_height() {
        print!("row {row:>4}:");
        let mut decoded = 0usize;
        for run in frame.runs(row)? {
            match run? {
                Run::Fill { color, len } => {
                    print!(" fill({color:#04x} x {len})");
                    decoded += len;
                }
                Run::Literal(bytes) => {
                    print!(" literal({}):", bytes.len());
                    for byte in bytes {
                        print!(" {byte:02x}");
                    }
                    decoded += bytes.len();
                }
            }
        }
        println!(" [{decoded} pixels]");
    }
    Ok(())
}

fn dump_png(sprite: Sprite<'_>, frame: Frame<'_>, output: &Path) -> Result<()> {
    let width = frame.width();
    let height = frame.height();
    let pixel_count = width
        .checked_mul(height)
        .ok_or_else(|| invalid_data("frame dimensions overflow"))?;
    let byte_count = pixel_count
        .checked_mul(4)
        .ok_or_else(|| invalid_data("RGBA dimensions overflow"))?;
    let mut rgba = zeroed_bytes(byte_count, "RGBA output")?;
    let crop_pixels = frame.pixel_len()?;
    let mut indices = zeroed_bytes(crop_pixels, "decoded frame")?;
    frame.decode_into(&mut indices)?;
    let palette = sprite.palette();

    for crop_y in 0..frame.cropped_height() {
        let y = i64::from(frame.cropped_y())
            + i64::try_from(crop_y).map_err(|_| invalid_data("crop y is too large"))?;
        if !(0..i64::try_from(height)?).contains(&y) {
            continue;
        }
        for crop_x in 0..frame.cropped_width() {
            let x = i64::from(frame.cropped_x())
                + i64::try_from(crop_x).map_err(|_| invalid_data("crop x is too large"))?;
            if !(0..i64::try_from(width)?).contains(&x) {
                continue;
            }
            let index = indices[crop_y * frame.cropped_width() + crop_x];
            let rgb = palette.rgb(index);
            let pixel = (usize::try_from(y)? * width + usize::try_from(x)?) * 4;
            rgba[pixel..pixel + 3].copy_from_slice(&rgb);
            rgba[pixel + 3] = 255;
        }
    }

    let file = fs::File::create(output)?;
    let mut encoder = png::Encoder::new(file, u32::try_from(width)?, u32::try_from(height)?);
    encoder.set_color(png::ColorType::Rgba);
    encoder.set_depth(png::BitDepth::Eight);
    let mut writer = encoder.write_header()?;
    writer.write_image_data(&rgba)?;
    println!("wrote {}", output.display());
    Ok(())
}

fn member_bytes(archive: &Archive<'_>, entry: Entry<'_>) -> Result<Vec<u8>> {
    match archive.payload(entry)? {
        Payload::Stored(bytes) => copy_bytes(bytes, "stored member"),
        Payload::Compressed {
            stream,
            unpacked_size,
        } => {
            let limit = u64::try_from(unpacked_size)?
                .checked_add(1)
                .ok_or_else(|| invalid_data("unpacked member size overflows"))?;
            let mut decoder = ZlibDecoder::new(stream).take(limit);
            let mut output = Vec::new();
            output.try_reserve_exact(unpacked_size).map_err(|error| {
                invalid_data(format!(
                    "cannot allocate {unpacked_size} bytes for zlib output: {error}"
                ))
            })?;
            decoder.read_to_end(&mut output)?;
            if output.len() != unpacked_size {
                return Err(invalid_data(format!(
                    "zlib output length {}, directory advertises {unpacked_size}",
                    output.len()
                )));
            }
            Ok(output)
        }
    }
}

fn copy_bytes(bytes: &[u8], what: &str) -> Result<Vec<u8>> {
    let mut output = Vec::new();
    output.try_reserve_exact(bytes.len()).map_err(|error| {
        invalid_data(format!(
            "cannot allocate {} bytes for {what}: {error}",
            bytes.len()
        ))
    })?;
    output.extend_from_slice(bytes);
    Ok(output)
}

fn zeroed_bytes(len: usize, what: &str) -> Result<Vec<u8>> {
    let mut output = Vec::new();
    output.try_reserve_exact(len).map_err(|error| {
        invalid_data(format!("cannot allocate {len} bytes for {what}: {error}"))
    })?;
    output.resize(len, 0);
    Ok(output)
}

fn display_name(entry: Entry<'_>) -> String {
    entry
        .name_str()
        .map_or_else(|| format!("{:02x?}", entry.name), str::to_owned)
}

const fn encoding_name(encoding: Encoding) -> &'static str {
    match encoding {
        Encoding::Raw => "raw",
        Encoding::GeneralRle => "general-rle",
        Encoding::TilesetRle => "tileset-rle",
        Encoding::AdventureRle => "adventure-rle",
    }
}

fn invalid_data(message: impl Into<String>) -> Box<dyn Error> {
    Box::new(io::Error::new(io::ErrorKind::InvalidData, message.into()))
}

#[cfg(test)]
mod tests {
    use std::io::Write as _;
    use std::time::{SystemTime, UNIX_EPOCH};

    use clap::CommandFactory as _;
    use flate2::write::{GzEncoder, ZlibEncoder};
    use flate2::Compression;

    use super::*;

    #[test]
    fn command_definition_is_valid() {
        Cli::command().debug_assert();
    }

    #[test]
    fn compressed_census_and_png_cover_the_std_boundary() {
        let member = def_member();
        let archive_image = lod_image(&member, member.len());
        let archive = Archive::parse(&archive_image).unwrap();
        assert_eq!(
            member_bytes(&archive, archive.entry(0).unwrap()).unwrap(),
            member
        );

        let lod_path = temporary_path("lod");
        let png_path = temporary_path("png");
        fs::write(&lod_path, &archive_image).unwrap();
        census(std::slice::from_ref(&lod_path), 1, DefDialect::Retail).unwrap();

        let bytes = member_bytes(&archive, archive.entry(0).unwrap()).unwrap();
        let sprite = Sprite::parse(&bytes).unwrap();
        let frame = sprite.group(0).unwrap().frame(0).unwrap();
        dump_png(sprite, frame, &png_path).unwrap();
        assert!(fs::read(&png_path)
            .unwrap()
            .starts_with(b"\x89PNG\r\n\x1a\n"));

        fs::remove_file(lod_path).unwrap();
        fs::remove_file(png_path).unwrap();
    }

    #[test]
    fn zlib_output_must_match_the_directory_size() {
        let member = def_member();
        let archive_image = lod_image(&member, member.len() + 1);
        let archive = Archive::parse(&archive_image).unwrap();
        let error = member_bytes(&archive, archive.entry(0).unwrap()).unwrap_err();
        assert!(error.to_string().contains("output length"));
    }

    #[test]
    fn known_interleaved_dialect_is_an_explicit_two_member_manifest() {
        let entry = |name: &'static [u8]| Entry {
            name,
            offset: 0,
            size: 0,
            attributes: 0,
            compressed_size: 0,
        };
        assert_eq!(
            dialect_for(entry(b"sgtwmta.def"), DefDialect::KnownInterleaved),
            Dialect::InterleavedCompactFrames
        );
        assert_eq!(
            dialect_for(entry(b"SGTWMTB.DEF"), DefDialect::KnownInterleaved),
            Dialect::InterleavedCompactFrames
        );
        assert_eq!(
            dialect_for(entry(b"SGTWMTA.DEF"), DefDialect::Retail),
            Dialect::Retail
        );
        assert_eq!(
            dialect_for(entry(b"ANYTHING.DEF"), DefDialect::KnownInterleaved),
            Dialect::Retail
        );
    }

    #[test]
    fn campaign_envelopes_cover_direct_and_gzipped_headers() {
        let map = gzip_member(b"compressed map payload");
        let header = roe_campaign_header(map.len());

        let mut direct = header.clone();
        direct.extend_from_slice(&map);
        let direct_envelope = inflate_campaign(&direct).unwrap();
        assert_eq!(direct_envelope.header, header);
        assert_eq!(direct_envelope.maps.len(), 1);
        assert_eq!(direct_envelope.maps[0].bytes, b"compressed map payload");
        assert_eq!(direct_envelope.maps[0].compressed_len, map.len());

        let mut compressed = gzip_member(&header);
        compressed.extend_from_slice(&map);
        let compressed_envelope = inflate_campaign(&compressed).unwrap();
        assert_eq!(compressed_envelope.header, header);
        assert_eq!(compressed_envelope.maps.len(), 1);
        assert_eq!(compressed_envelope.maps[0].bytes, b"compressed map payload");
        assert_eq!(compressed_envelope.maps[0].compressed_len, map.len());
    }

    #[test]
    fn save_extensions_cover_every_retail_slot() {
        for extension in [
            "gm1", "GM2", "gm3", "gm4", "gm5", "gm6", "gm7", "gm8", "tgm", "CGM",
        ] {
            assert!(is_save_path(Path::new(&format!("slot.{extension}"))));
        }
        assert!(!is_save_path(Path::new("slot.gm9")));
        assert!(!is_save_path(Path::new("savegames.txt")));
    }

    fn temporary_path(extension: &str) -> PathBuf {
        let nonce = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        std::env::temp_dir().join(format!(
            "homm3-oracle-{}-{nonce}.{extension}",
            std::process::id()
        ))
    }

    fn gzip_member(bytes: &[u8]) -> Vec<u8> {
        let mut encoder = GzEncoder::new(Vec::new(), Compression::default());
        encoder.write_all(bytes).unwrap();
        encoder.finish().unwrap()
    }

    fn roe_campaign_header(packed_map_size: usize) -> Vec<u8> {
        let mut bytes = Vec::new();
        bytes.extend_from_slice(&(CampaignVersion::LegacyRestoration as u32).to_le_bytes());
        bytes.push(1); // retail region map 1 has three regions
        push_test_string(&mut bytes, b"Campaign");
        push_test_string(&mut bytes, b"Description");
        push_test_scenario(
            &mut bytes,
            b"ONE.H3M",
            u32::try_from(packed_map_size).unwrap(),
        );
        push_test_scenario(&mut bytes, b"", 0);
        push_test_scenario(&mut bytes, b"", 0);
        bytes
    }

    fn push_test_scenario(bytes: &mut Vec<u8>, name: &[u8], packed_map_size: u32) {
        push_test_string(bytes, name);
        bytes.extend_from_slice(&packed_map_size.to_le_bytes());
        bytes.push(0); // prerequisites
    }

    fn push_test_string(bytes: &mut Vec<u8>, value: &[u8]) {
        bytes.extend_from_slice(&i32::try_from(value.len()).unwrap().to_le_bytes());
        bytes.extend_from_slice(value);
    }

    fn lod_image(member: &[u8], advertised_size: usize) -> Vec<u8> {
        let mut encoder = ZlibEncoder::new(Vec::new(), Compression::default());
        encoder.write_all(member).unwrap();
        let compressed = encoder.finish().unwrap();
        let payload_at = homm3_lod::HEADER_SIZE + homm3_lod::ENTRY_SIZE;
        let mut image = vec![0u8; payload_at];
        image[..4].copy_from_slice(b"LOD\0");
        image[8..12].copy_from_slice(&1u32.to_le_bytes());
        image[homm3_lod::HEADER_SIZE..homm3_lod::HEADER_SIZE + 9].copy_from_slice(b"ONE.DEF\0\0");
        image[homm3_lod::HEADER_SIZE + 16..homm3_lod::HEADER_SIZE + 20]
            .copy_from_slice(&u32::try_from(payload_at).unwrap().to_le_bytes());
        image[homm3_lod::HEADER_SIZE + 20..homm3_lod::HEADER_SIZE + 24]
            .copy_from_slice(&u32::try_from(advertised_size).unwrap().to_le_bytes());
        image[homm3_lod::HEADER_SIZE + 28..homm3_lod::HEADER_SIZE + 32]
            .copy_from_slice(&u32::try_from(compressed.len()).unwrap().to_le_bytes());
        image.extend(compressed);
        image
    }

    fn def_member() -> Vec<u8> {
        let frame_at = homm3_def::DEF_HEADER_SIZE + 16 + 13 + 4;
        let mut member = vec![0u8; frame_at + homm3_def::FRAME_HEADER_SIZE + 1];
        member[0..4].copy_from_slice(&homm3_def::DefType::SPRITE.0.to_le_bytes());
        member[4..8].copy_from_slice(&1u32.to_le_bytes());
        member[8..12].copy_from_slice(&1u32.to_le_bytes());
        member[12..16].copy_from_slice(&1u32.to_le_bytes());
        member[16 + 3..16 + 6].copy_from_slice(&[12, 34, 56]);
        let group = homm3_def::DEF_HEADER_SIZE;
        member[group + 4..group + 8].copy_from_slice(&1u32.to_le_bytes());
        member[group + 16..group + 19].copy_from_slice(b"ONE");
        member[group + 29..group + 33]
            .copy_from_slice(&u32::try_from(frame_at).unwrap().to_le_bytes());
        member[frame_at..frame_at + 4].copy_from_slice(&1u32.to_le_bytes());
        member[frame_at + 8..frame_at + 12].copy_from_slice(&1u32.to_le_bytes());
        member[frame_at + 12..frame_at + 16].copy_from_slice(&1u32.to_le_bytes());
        member[frame_at + 16..frame_at + 20].copy_from_slice(&1u32.to_le_bytes());
        member[frame_at + 20..frame_at + 24].copy_from_slice(&1u32.to_le_bytes());
        member[frame_at + homm3_def::FRAME_HEADER_SIZE] = 1;
        member
    }
}
