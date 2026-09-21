#!/bin/sh
# Resolve the live checkout before a shell/CLI creates generated artifacts.
# Filesystem markers work in ordinary checkouts, worktrees and source archives.
homm3_search_dir=$(CDPATH= cd -- "${1:-$PWD}" && pwd -P) || exit 1
while :; do
    if [ -f "$homm3_search_dir/flake.nix" ] &&
       [ -f "$homm3_search_dir/config/project.toml" ] &&
       [ -f "$homm3_search_dir/config/units.toml" ] &&
       [ -d "$homm3_search_dir/scripts/homm3" ]; then
        printf '%s\n' "$homm3_search_dir"
        exit 0
    fi
    if [ "$homm3_search_dir" = / ]; then
        printf '%s\n' 'homm3: no project root above the requested directory; run inside the checkout or set HOMM3_DIR for the CLI.' >&2
        exit 1
    fi
    homm3_search_dir=${homm3_search_dir%/*}
    [ -n "$homm3_search_dir" ] || homm3_search_dir=/
done
