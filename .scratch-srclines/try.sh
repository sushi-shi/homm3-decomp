#!/usr/bin/env bash
cd /home/sheep/Projects/homm3/wt-cold-windows
out=$(nix develop .#build --command bash -c 'homm3 build --fast 2>&1')
if ! grep -q 'functions exact' <<<"$out"; then
  echo "$1: BUILD FAIL"; tail -25 <<<"$out"; exit 1
fi
printf '%-42s ' "$1"
grep -o '[0-9]*/[0-9]* functions exact ([0-9.]*%)' <<<"$out" | tr '\n' ' '
python3 .scratch-srclines/sc.py "$2" | head -3
