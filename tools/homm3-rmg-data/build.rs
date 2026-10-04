//! Generate the raw data boundary from the game's canonical declarations.
use std::{
    env, fs,
    path::{Path, PathBuf},
    process::{Command, Stdio},
};

fn extract_sources(manifest: &Path, output: &Path) -> PathBuf {
    let template = manifest.join("export.cpp.in");
    let extractor = manifest.join("extract.py");
    for path in [&template, &extractor] {
        println!("cargo:rerun-if-changed={}", path.display());
    }
    for variable in ["HOMM3_PYTHON", "MSVC_DIR", "HOMM3_TOOLCHAIN"] {
        println!("cargo:rerun-if-env-changed={variable}");
    }
    let source = output.join("export.cpp");
    let extracted = Command::new(env::var_os("HOMM3_PYTHON").unwrap_or_else(|| "python3".into()))
        .arg(&extractor)
        .arg(&template)
        .arg(&source)
        .stderr(Stdio::inherit())
        .output()
        .expect("run Clang source extraction (requires the repository Python/Clang environment)");
    assert!(
        extracted.status.success(),
        "extract canonical RMG definitions"
    );
    print!(
        "{}",
        String::from_utf8(extracted.stdout).expect("UTF-8 Cargo dependency directives")
    );
    source
}

fn main() {
    let manifest = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    let include = manifest.join("../../include");
    let wrapper = manifest.join("wrapper.h");
    println!("cargo:rerun-if-changed={}", wrapper.display());

    // C enums stay integers at the untrusted boundary. No rustified enum can
    // accidentally turn an unknown input discriminant into undefined behavior.
    let bindings = bindgen::Builder::default()
        .header(wrapper.to_string_lossy())
        .clang_args(["-x", "c++", "-std=c++14", "-fms-extensions"])
        .clang_arg(format!("-I{}", include.display()))
        .allowlist_type("TRandomMapRequest|TRmgTerrain(Pattern|Transition)Entry|TTownType|ETownTypeCount|TTerrainType|TArtifact|ERmg.*|ERandomMapResult|EMapDimension|ETileDirection|EGameResource|TAdventureObjectType|TAdvObjectNameRow|EObjectMaskFrame|EObjectSlotCategory|TSpellSchool|EArtifactClass|EKeyColor|ESpellCount|TSecondarySkill|EQuestType|TSeerRewardType|EMapFormatVersion")
        .allowlist_var("RMG_.*|RANDOM_MAP_.*|TOWN_.*|ARTIFACT_.*|MAP_DIMENSION_.*|NUM_RESOURCES|SHAPE_.*|ADVENTURE_OBJECT_TRAIT_COUNT")
        .ignore_functions()
        .ignore_methods()
        .with_codegen_config(bindgen::CodegenConfig::TYPES | bindgen::CodegenConfig::VARS)
        .prepend_enum_name(false)
        .derive_debug(true)
        .derive_partialeq(true)
        .derive_eq(true)
        .layout_tests(false)
        .rust_target(bindgen::RustTarget::stable(82, 0).unwrap())
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("generate RMG definitions from the canonical C++ headers");
    bindings
        .write_to_file(PathBuf::from(env::var_os("OUT_DIR").unwrap()).join("bindings.rs"))
        .expect("write generated RMG definitions");

    // Compile and run for HOST, even when Rust itself is being cross-compiled.
    // This exports scalar values, never native struct bytes or pointer values.
    let output = PathBuf::from(env::var_os("OUT_DIR").unwrap());
    for header in [
        "creature_traits.h",
        "spell_traits.h",
        "spellschool.h",
        "spelleffect_type.h",
        "hero_traits.h",
        "hero_class.h",
        "creature_type.h",
        "artifact_data.h",
        "adventure_object_subtype.h",
    ] {
        println!("cargo:rerun-if-changed={}", include.join(header).display());
    }
    let host = env::var("HOST").unwrap();
    let executable = output.join(if host.contains("windows") {
        "export-data.exe"
    } else {
        "export-data"
    });
    let source = extract_sources(&manifest, &output);
    let compiler = cc::Build::new()
        .cpp(true)
        .target(&host)
        .host(&host)
        .cargo_metadata(false)
        .get_compiler();
    let mut compile = compiler.to_command();
    if compiler.is_like_msvc() {
        compile
            .arg("/nologo")
            .arg("/EHsc")
            .arg(format!("/I{}", include.display()))
            .arg(&source)
            .arg(format!("/Fe{}", executable.display()));
    } else {
        compile
            .arg("-std=c++11")
            .arg("-I")
            .arg(&include)
            .arg(&source)
            .arg("-o")
            .arg(&executable);
    }
    assert!(
        compile
            .current_dir(&output)
            .status()
            .expect("run C++ data exporter compiler")
            .success(),
        "compile canonical data exporter"
    );
    let generated = Command::new(executable)
        .output()
        .expect("execute canonical data exporter");
    assert!(generated.status.success(), "canonical data exporter failed");
    fs::write(output.join("tables.rs"), generated.stdout).expect("write source-derived RMG tables");
}
