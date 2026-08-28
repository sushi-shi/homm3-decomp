//! Optional build of the in-tree C++ DEF renderers for parity tests.

use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    println!("cargo:rerun-if-env-changed=CXX");
    println!("cargo:rerun-if-env-changed=HOMM3_SOURCE_DIR");
    if env::var_os("CARGO_FEATURE_CXX_PARITY").is_none() {
        return;
    }

    let manifest = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    let source_root =
        env::var_os("HOMM3_SOURCE_DIR").map_or_else(|| manifest.join("../.."), PathBuf::from);
    let output = PathBuf::from(env::var_os("OUT_DIR").unwrap());
    let compiler = env::var_os("CXX").unwrap_or_else(|| "clang++".into());
    let object = output.join("cspriteframe-oracle.o");
    let archive = output.join("libcspriteframe_oracle.a");
    let shim = manifest.join("cxx/oracle.cpp");
    let implementation = source_root.join("src/cspriteframe.cpp");

    println!("cargo:rerun-if-changed={}", shim.display());
    println!("cargo:rerun-if-changed={}", implementation.display());
    println!(
        "cargo:rerun-if-changed={}",
        source_root.join("include/cspriteframe.h").display()
    );

    run(
        Command::new(&compiler)
            .arg("-std=c++17")
            .arg("-O2")
            .arg("-U__clang__")
            .arg("-D__declspec(x)=")
            .arg("-D__cdecl=")
            .arg("-I")
            .arg(source_root.join("include"))
            .arg("-I")
            .arg(&source_root)
            .arg("-c")
            .arg(&shim)
            .arg("-o")
            .arg(&object),
        "compile C++ renderer oracle",
    );

    let archiver = env::var_os("AR").unwrap_or_else(|| "ar".into());
    run(
        Command::new(archiver).arg("crs").arg(&archive).arg(&object),
        "archive C++ renderer oracle",
    );
    println!("cargo:rustc-link-search=native={}", output.display());
    println!("cargo:rustc-link-lib=static=cspriteframe_oracle");
    println!("cargo:rustc-link-lib=dylib=stdc++");
}

fn run(command: &mut Command, description: &str) {
    let status = command
        .status()
        .unwrap_or_else(|error| panic!("failed to {description}: {error}"));
    assert!(status.success(), "failed to {description}: {status}");
}
