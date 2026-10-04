//! Streaming adapter for `python -m homm3.rmg.corpus check`.
//! One replay per input line; each response is an ASCII header, followed by the
//! exact uncompressed map bytes on success. Assets and workspaces are reused.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    generation::{Assets, GenerationFailure, GenerationWorkspace},
    output::{OutputError, OutputWorkspace},
    placement_rules::PlacementRules,
    prototype::PrototypeSource,
    raw,
    request::Request,
    template::TemplateSource,
    traits::{ArtifactCatalog, CreatureCatalog, SpellCatalog},
};
use homm3_rmg_cli::{replay::Replay, resources::Installation};
use std::{
    error::Error,
    io::{self, BufRead, Write},
    path::PathBuf,
};

type Result<T> = std::result::Result<T, Box<dyn Error>>;

fn write_request(out: &mut impl Write, request: raw::TRandomMapRequest) -> io::Result<()> {
    for byte in request.m_isHumanSeat {
        write!(out, "{byte:02x}")?;
    }
    for value in request.m_townChoices.into_iter().chain([
        request.m_width,
        request.m_height,
        request.m_levels,
        request.m_humanPlayerCount,
        request.m_humanTeamCount,
        request.m_computerPlayerCount,
        request.m_computerTeamCount,
        i32::from_ne_bytes(request.m_waterContent.to_ne_bytes()),
        request.m_monsterStrength,
        i32::from_ne_bytes(request.m_mapVersion.to_ne_bytes()),
    ]) {
        for byte in value.to_le_bytes() {
            write!(out, "{byte:02x}")?;
        }
    }
    Ok(())
}

fn write_failure(
    destination: &mut impl Write,
    error: &(dyn Error + 'static),
    output_request: Option<raw::TRandomMapRequest>,
) -> io::Result<()> {
    if let Some(failure) = error.downcast_ref::<GenerationFailure>() {
        write!(
            destination,
            "error {} {:?} ",
            failure.report.rng().state,
            failure.report.stage()
        )?;
        write_request(destination, failure.report.request().repaired_record())?;
        write!(destination, " ")?;
    } else if let Some(failure) = error.downcast_ref::<OutputError>() {
        write!(
            destination,
            "error {} Output{:?} ",
            failure.rng.state, failure.stage
        )?;
        if let Some(request) = output_request {
            write_request(destination, request)?;
        } else {
            write!(destination, "none")?;
        }
        write!(destination, " ")?;
    } else {
        write!(destination, "error none InputOrValidation none ")?;
    }
    writeln!(
        destination,
        "{}",
        error.to_string().replace(['\n', '\r'], " ")
    )?;
    Ok(())
}

fn main() -> Result<()> {
    let data = PathBuf::from(
        std::env::args_os()
            .nth(1)
            .ok_or("DATA directory required")?,
    );
    let mut installation = Installation::open(&data)?;
    let mut scratch = Vec::new();
    installation.text("rand_trn.txt", &mut scratch)?;
    let hotfix_rules = PlacementRules::parse(&scratch, Behavior::Hotfix)?;
    let retail_rules = PlacementRules::parse(&scratch, Behavior::Retail(RetailProfile::default()))?;
    installation.text("crtraits.txt", &mut scratch)?;
    let creatures = CreatureCatalog::parse(&scratch)?;
    installation.text("sptraits.txt", &mut scratch)?;
    let spells = SpellCatalog::parse(&scratch)?;
    installation.text("artraits.txt", &mut scratch)?;
    let artifacts = ArtifactCatalog::parse(&scratch)?;
    let mut template_bytes = Vec::new();
    installation.text("rmg.txt", &mut template_bytes)?;
    let templates = TemplateSource::parse(&template_bytes)?;
    let mut prototype_bytes = Vec::new();
    installation.text("objects.txt", &mut prototype_bytes)?;
    let prototypes = PrototypeSource::parse(&prototype_bytes, |name| installation.mask(name))?;
    drop(installation);
    drop(scratch);
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
    let mut generation = GenerationWorkspace::default();
    let mut output = OutputWorkspace::default();
    let mut bytes = Vec::new();
    let mut line = String::new();
    let mut input = io::stdin().lock();
    let mut destination = io::BufWriter::new(io::stdout().lock());
    loop {
        line.clear();
        if input.read_line(&mut line)? == 0 {
            break;
        }
        let mut output_request = None;
        let result = (|| -> Result<u32> {
            let replay = Replay::parse(&line)?;
            let request = Request::parse(replay.request, replay.behavior)?;
            let assets = if replay.behavior.is_hotfix() {
                hotfix
            } else {
                retail
            };
            let prepared = assets.prepare(request, replay.seed)?;
            output_request = Some(prepared.report().request().repaired_record());
            let map = generation.generate(&prepared)?;
            output_request = Some(map.report().request().repaired_record());
            bytes.clear();
            Ok(output.write(&map, &mut bytes)?.rng.state)
        })();
        match result {
            Ok(rng) => {
                write!(destination, "ok {rng} {} ", bytes.len())?;
                write_request(
                    &mut destination,
                    output_request.ok_or("missing effective request")?,
                )?;
                writeln!(destination)?;
                destination.write_all(&bytes)?;
            }
            Err(error) => write_failure(&mut destination, error.as_ref(), output_request)?,
        }
        destination.flush()?;
    }
    Ok(())
}
