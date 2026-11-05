#!/bin/sh
# Starts the HoMM2-vs-HoMM3 mashup battle on the desktop (battle log:
# build/mashup-runtime/mashup.log; the recorder's: mashup-record.log).
#   mashup/run.sh            watch a battle
#   mashup/run.sh --record   watch it and record the same battle (same seed)
#                            to build/mashup-videos/ from a hidden display
# MASHUP_SEED=<n> sets the battle's dice, MASHUP_H2_ARMY=<n> the HoMM2 army
# multiplier (default 3), MASHUP_SPELL_POWER=<n> and MASHUP_KNOWLEDGE=<n>
# both heroes' spell power and knowledge (defaults 50 and 30),
# MASHUP_MIN_MORALE=<n> / MASHUP_MIN_LUCK=<n> every stack's floor (default 2),
# MASHUP_MATCHUP=castle|necropolis|dungeon who fights (default castle). Build first: Heroes3.exe
# (homm3 link), h2core.dll and Data/h2mash.{lod,snd} in build/mashup-runtime.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
cd "$root"
# Always the project's own Wine: a second Wine version on the same prefix
# fails with "wine client error: version mismatch".
if [ -z "${MASHUP_IN_SHELL:-}" ]; then
    MASHUP_IN_SHELL=1 exec nix develop .#build --command "$0" "$@"
fi
: "${MASHUP_SEED:=$(date +%s)}"
export MASHUP_SEED
if [ "${1:-}" = "--record" ]; then
    mkdir -p build/mashup-videos
    video="build/mashup-videos/${MASHUP_MATCHUP:-castle}-$MASHUP_SEED.mp4"
    env -u WAYLAND_DISPLAY MASHUP_LOG=mashup-record.log python3 mashup/tools/headless.py \
        --gameargs "/MA /i0" --record "$video" wait:1200 >build/mashup-record.log 2>&1 &
    echo "recording seed $MASHUP_SEED to $video"
fi
cd build/mashup-runtime
# The desktop has its own prefix; the hidden recorder uses build/mashup-wine.
export WINEPREFIX="$root/build/mashup-wine-desktop"
export WINEDEBUG=-all
wine Heroes3.exe /M /i0
wait
