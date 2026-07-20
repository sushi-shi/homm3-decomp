# Reproduce the HoMM3 Visual C++ 6.0 SP3 toolchain bundle from preserved media.
#
# Usage from the repository root:
#   nix-shell scripts/create-toolchain-release.nix
#
# All original media are pinned from Internet Archive metadata.

{ pkgs ? import <nixpkgs> { config.allowUnfree = true; } }:

let
  vc6-disc1 = pkgs.fetchurl {
    name = "VSE600EUN1.ISO";
    url = "https://archive.org/download/1998-10-01-visual-studio-6.0-enterprise-edition-disc-1/VSE600EUN1.ISO";
    hash = "sha1-vXQvfM6vLjAymCuitFgjWPebOwQ=";
  };
  vc6-sp3 = pkgs.fetchurl {
    name = "TNSB9908.iso";
    url = "https://archive.org/download/ms-technet-tnsb9908/TNSB9908.iso";
    hash = "sha1-v1me0Vhhtp6on/pSYm6yUt8gU74=";
  };
  directx7-sdk = pkgs.fetchurl {
    name = "DirectX-7-SDK.iso";
    url = "https://archive.org/download/cdrom-directx-7-sdk/Dx7.iso";
    hash = "sha1-MQQcWID5tAwbCGp3bmYQpYkgB9k=";
  };
in
pkgs.mkShell {
  packages = [
    pkgs.python3
    pkgs.p7zip
    pkgs.cabextract
    pkgs.gnutar
    pkgs.xz
    pkgs.binutils
  ];

  shellHook = ''
    export VC6_DISC1="${vc6-disc1}"
    export VC6_SP3="${vc6-sp3}"
    export DIRECTX7_SDK_MEDIA="${directx7-sdk}"
    export HOMM3_DIR="$PWD"
    exec python3 ${./create-toolchain-release.py}
  '';
}
