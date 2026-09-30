#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
runtime="$root/build/cove-runtime"
if [[ ! -f "$runtime/Heroes3.exe" || ! -f "$runtime/Data/cove.lod" ]]; then
    echo "Build and package Cove first; see extensions/cove/README.md." >&2
    exit 1
fi
export WINEPREFIX="${COVE_WINEPREFIX:-$root/build/cove-runtime-wine}"
export WINEDEBUG="${WINEDEBUG:--all}"
wine reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d x11 /f >/dev/null
wine reg add 'HKLM\Software\New World Computing\Heroes of Might and Magic® III\1.0' \
    /v 'First Time' /t REG_DWORD /d 0 /f /reg:32 >/dev/null
cd -- "$runtime"
exec wine explorer /desktop=Cove,1024x768 Heroes3.exe /i0 /s0
