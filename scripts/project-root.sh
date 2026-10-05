#!/bin/sh
# Resolve the live checkout before a shell/CLI creates generated artifacts.
# Filesystem markers work in ordinary checkouts, worktrees and source archives.
#
#   project-root.sh [DIR]   the checkout containing DIR (default $PWD)
#   project-root.sh --select
#       the `homm3` wrapper's choice (homm3.core.root applies the same rule):
#       the checkout containing $PWD wins over an inherited HOMM3_DIR that
#       names another checkout, with one warning on stderr. HOMM3_DIR_FORCE=1
#       keeps HOMM3_DIR; outside any checkout HOMM3_DIR is used.
homm3_root() {
    homm3_search_dir=$(CDPATH= cd -- "$1" 2>/dev/null && pwd -P) || return 1
    while :; do
        if [ -f "$homm3_search_dir/flake.nix" ] &&
           [ -f "$homm3_search_dir/config/project.toml" ] &&
           [ -f "$homm3_search_dir/config/units.toml" ] &&
           [ -d "$homm3_search_dir/scripts/homm3" ]; then
            printf '%s\n' "$homm3_search_dir"
            return 0
        fi
        [ "$homm3_search_dir" = / ] && return 1
        homm3_search_dir=${homm3_search_dir%/*}
        [ -n "$homm3_search_dir" ] || homm3_search_dir=/
    done
}

homm3_no_root() {
    printf '%s\n' 'homm3: no project root above the requested directory; run inside the checkout or set HOMM3_DIR for the CLI.' >&2
    exit 1
}

if [ "${1-}" != --select ]; then
    homm3_root "${1:-$PWD}" || homm3_no_root
    exit 0
fi

homm3_here=$(homm3_root "$PWD") || homm3_here=
if [ -z "${HOMM3_DIR-}" ]; then
    [ -n "$homm3_here" ] || homm3_no_root
    printf '%s\n' "$homm3_here"
    exit 0
fi
if [ -z "$homm3_here" ] || [ "${HOMM3_DIR_FORCE-}" = 1 ]; then
    homm3_root "$HOMM3_DIR" || homm3_no_root
    exit 0
fi
homm3_named=$(homm3_root "$HOMM3_DIR") || homm3_named=
if [ "$homm3_named" != "$homm3_here" ]; then
    printf 'homm3: HOMM3_DIR=%s is a different checkout than the current directory'"'"'s; using %s (set HOMM3_DIR_FORCE=1 to keep HOMM3_DIR)\n' \
        "$HOMM3_DIR" "$homm3_here" >&2
fi
printf '%s\n' "$homm3_here"
