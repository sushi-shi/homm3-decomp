{
  description = "Heroes III matching-decompilation environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";
    rust-overlay = {
      url = "github:oxalica/rust-overlay/6cddd512fa2bf7231f098d3a2f92f6e4cff71e0a";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    vostok-delinker-src = {
      url = "github:srp-survarium/vostok-delinker/1393e24b4804cb357fdac147c68013f0aa5a9d95";
      flake = false;
    };
    objdiff-src = {
      url = "github:encounter/objdiff/v3.7.3";
      flake = false;
    };
  };

  outputs = { nixpkgs, rust-overlay, vostok-delinker-src, objdiff-src, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ rust-overlay.overlays.default ];
      };

      rust = pkgs.rust-bin.nightly.latest.default.override {
        extensions = [ "rust-src" "rustfmt" "clippy" ];
      };
      nightly-rustPlatform = pkgs.makeRustPlatform {
        cargo = rust;
        rustc = rust;
      };

      vostok-delinker = nightly-rustPlatform.buildRustPackage {
        pname = "vostok-delinker";
        version = "0.1.0";
        src = vostok-delinker-src;
        cargoHash = "sha256-ZwFdbqUyh4b0S+fUYKGMN1fWaxRu1zU2ozKpe7CbcYs=";
      };

      objdiffVersion = "3.7.3";
      objdiffUrl = name:
        "https://github.com/encounter/objdiff/releases/download/v${objdiffVersion}/${name}";
      objdiffGuiLibs = with pkgs; [
        libGL
        libxkbcommon
        wayland
        fontconfig
        freetype
        libx11
        libxcursor
        libxi
        libxrandr
        libxcb
      ];

      objdiff-cli = nightly-rustPlatform.buildRustPackage {
        pname = "objdiff-cli";
        version = objdiffVersion;
        src = objdiff-src;
        cargoHash = "sha256-Z9vyUj35nrHuUoOYM54RLCn7CzcQ6k3A6FsDYKCVqVM=";
        cargoBuildFlags = [ "-p" "objdiff-cli" ];
        cargoTestFlags = [ "-p" "objdiff-core" "-p" "objdiff-cli" ];
        cargoInstallFlags = [ "-p" "objdiff-cli" ];
        nativeBuildInputs = [ pkgs.protobuf ];
        OBJDIFF_REGENERATE_PROTO = "1";
      };

      objdiff = pkgs.stdenv.mkDerivation {
        pname = "objdiff";
        version = objdiffVersion;
        src = pkgs.fetchurl {
          url = objdiffUrl "objdiff-linux-x86_64";
          hash = "sha256-1pzhzJUl/BJQP2XS333KIfkx1YYi8ZyRdPMv5MnJGyA=";
        };
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.autoPatchelfHook pkgs.makeWrapper ];
        buildInputs = [ pkgs.stdenv.cc.cc.lib ] ++ objdiffGuiLibs;
        installPhase = ''
          install -Dm755 $src $out/bin/objdiff
          wrapProgram $out/bin/objdiff \
            --prefix LD_LIBRARY_PATH : "${pkgs.lib.makeLibraryPath objdiffGuiLibs}"
        '';
      };

      homm3-cli = pkgs.writeShellScriptBin "homm3" ''
        project_dir="''${HOMM3_DIR:-$PWD}"
        export PYTHONDONTWRITEBYTECODE=1
        export PYTHONPATH="$project_dir/scripts''${PYTHONPATH:+:$PYTHONPATH}"
        exec python3 -m homm3 "$@"
      '';

      analysisPython = pkgs.python3.withPackages (ps: [
        ps.pyghidra
        ps.libclang
        ps.capstone
      ]);

      objdiffShimHook = ''
        if [ -z "''${HOMM3_OBJDIFF_WRAPPED:-}" ] && command -v objdiff >/dev/null 2>&1; then
          _real_objdiff="$(command -v objdiff)"
          _objdiff_bin="$HOMM3_DIR/build/objdiff-shim"
          mkdir -p "$_objdiff_bin"
          printf '#!/bin/sh\nfor arg in "$@"; do\n  case "$arg" in -p|--project-dir) exec "%s" "$@" ;; esac\ndone\nexec "%s" -p "%s/build/objdiff" "$@"\n' \
            "$_real_objdiff" "$_real_objdiff" "$HOMM3_DIR" > "$_objdiff_bin/objdiff"
          chmod +x "$_objdiff_bin/objdiff"
          export PATH="$_objdiff_bin:$PATH"
          export HOMM3_OBJDIFF_WRAPPED=1
        fi
      '';

      ghidraEnvHook = ''
        export GHIDRA_INSTALL_DIR="${pkgs.ghidra}/lib/ghidra"
        export JAVA_HOME="${pkgs.jdk21}/lib/openjdk"
      '';

      commonTools = [
        homm3-cli
        rust
        vostok-delinker
        objdiff
        objdiff-cli
      ] ++ (with pkgs; [
        analysisPython
        ghidra
        jdk21
        ninja
        llvm
        llvmPackages.clang-unwrapped
        ripgrep
        file
        xxd
        jq
        binutils
        p7zip
      ]);

      commonShellHook = ''
        export HOMM3_DIR="$PWD"
        export HOMM3_CLANG="${pkgs.llvmPackages.clang-unwrapped}/bin/clang"
        export PYTHONDONTWRITEBYTECODE=1
        export PYTHONPATH="$HOMM3_DIR/scripts''${PYTHONPATH:+:$PYTHONPATH}"
        ${ghidraEnvHook}
        ${objdiffShimHook}
      '';
    in {
      packages.${system} = {
        inherit vostok-delinker objdiff objdiff-cli;
        default = vostok-delinker;
      };

      devShells.${system} = {
        default = pkgs.mkShell {
          name = "homm3-decomp";
          packages = commonTools;
          shellHook = commonShellHook;
        };

        build = pkgs.mkShell {
          name = "homm3-build";
          packages = commonTools ++ [ pkgs.wineWow64Packages.staging ];
          shellHook = commonShellHook + ''
            export HOMM3_TOOLCHAIN="''${HOMM3_TOOLCHAIN:-$HOMM3_DIR/build/homm3-toolchain-vc6-sp3}"
            export MSVC_DIR="$HOMM3_TOOLCHAIN/msvc"
            export WINEPREFIX="$HOMM3_DIR/build/wineprefix"
            export WINEDEBUG="fixme-all,err-kerberos"
            export WINEDLLOVERRIDES="mscoree,mshtml="
            case "$-" in
              *i*) trap 'wineserver -k >/dev/null 2>&1 || true' EXIT ;;
            esac
          '';
        };
      };
    };
}
