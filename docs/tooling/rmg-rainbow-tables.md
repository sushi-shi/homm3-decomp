# RMG rainbow tables

The RMG rainbow tables record C++ outcomes for a fixed, growing list of
deterministic requests in each of retail and hotfix modes. Like a password
rainbow table, they are precomputed native results looked up by input, so a
check runs only the Rust candidate, never retail or Wine. Generated tables stay
under ignored `build/`; only the capture/checking tools belong in Git. Rust
never supplies reference values. Capture uses one worker; independent Rust
checks can run while it works.

Each reference records the uncompressed H3M SHA-256 and byte length, final RNG
state, effective request, return code and x87 control words. Crashes, timeouts
and rejections remain distinct outcomes. These are hash tables, not retained
map files: a mismatch can be detected without C++, but locating its first byte
requires recapturing that case with the frozen oracle.

`manifest.json` fingerprints installation assets (including loose overrides),
the pinned retail image, frozen harness binaries, libraries and hotfix objects.
`cases.json` retains original requests, seeds and controlled stack/heap inputs;
`manifest.json` records its digest, request count and every extension.
`references.sqlite3` stores the indexed inputs and results. Retail replay uses
the recorded harness policy: heap-derived water-town permissions, alignment
matching when heap fill is nonzero, and the signed stack word as the initial
key-tent cursor. This policy is specific to these captures.

Run the following commands in the repository's Nix build environment
(`nix develop .#build`) with `PYTHONPATH=scripts`, or through the installed
wrapper as `homm3 rmg rainbow …`. `python -m homm3.rmg.corpus` remains an
alias with the same arguments. Choose an absolute directory so different
worktrees can share it. In this development session the tables live at
`/home/sheep/Projects/homm3/homm3-decomp/build/rmg-reference` (the directory
keeps its original name).

```sh
export RMG_RAINBOW=/absolute/path/to/build/rmg-reference
export RMG_DATA=/absolute/path/to/game/Data

homm3 rmg rainbow --out "$RMG_RAINBOW" status
```

`status` reports per-mode outcome counts and missing entries, overall and for
each segment: the imported requests and every later extension.

## Creating and extending

To create tables from the existing native captures:

```sh
homm3 rmg rainbow --out "$RMG_RAINBOW" init \
  --retail-corpus /path/to/sampled-100000 \
  --hotfix-oracle /path/to/frozen/hotfix \
  --hotfix-captures /path/to/hotfix-campaign \
  --data "$RMG_DATA"
```

The optional hotfix import checks archived job bytes and native result records;
it does not trust a previous Rust comparison's pass/fail verdict. Retail imports
select the native side's hash only when it is tied to the recorded native state.

The first 100,000 requests (`sample2-000000` to `sample2-099999`) come from the
sampled retail campaign in the [RMG oracle](../oracles/rmg.md#sampled-campaign-result).
`extend` appends more requests from the same distribution and never changes an
existing request or result:

```sh
homm3 rmg rainbow --out "$RMG_RAINBOW" extend --count 100000
```

`sample_case` in `scripts/homm3/rmg/corpus.py` is that campaign's sampler
(`build/rmg-oracle/sample_campaign_100k.py`, master seed `0x524d4732`). Fed the
original seed stream, it reproduces all 100,000 imported requests exactly. Map
size, levels, map version, water and monster strength cycle with the case
index; player and team counts, seats, fixed towns, `stackWord` and `heapByte`
are drawn at random. Extensions continue the names and index cycle. Each new
case draws from its own generator, seeded with the extension master seed
(default `0x524d4733`) and its index. A seed already in the table is redrawn.
One extension by 2N therefore equals two by N. `manifest.json` records each
extension's sampler version, master seed, range, Python version and resulting
`cases.json` digest. The writer lock prevents extending during a capture.
New requests are missing in both modes until `capture` fills them.

## Capturing

To capture or resume missing entries, set `WINEPREFIX` to a prepared oracle
prefix, then run one background worker per table directory. Use a prefix that
no interactive shell shares: leaving a `nix develop .#build` shell runs
`wineserver -k` on its prefix, and a killed batch records a process error.
This example restricts the worker to CPU 0, lowers its priority, and captures
retail before hotfix:

```sh
nohup nice -n 10 taskset -c 0 sh -c '
  python -m homm3.rmg.corpus --out "$RMG_RAINBOW" capture --mode retail --data "$RMG_DATA" &&
  python -m homm3.rmg.corpus --out "$RMG_RAINBOW" capture --mode hotfix --data "$RMG_DATA"' \
  > "$RMG_RAINBOW/capture.log" 2>&1 < /dev/null &
```

A long capture should not run from a worktree that may be removed: run it from
the main checkout, or from a `git archive` snapshot of `scripts/homm3` plus the
project markers (`flake.nix`, `config/project.toml`, `config/units.toml`) kept
beside the tables.

A writer lock prevents duplicate capture workers. Each batch commits atomically;
interrupting the worker (for example with `kill`) loses at most the running
batch, and restarting captures only missing entries. Faulting jobs are
recorded, and jobs not reached after a crash are retried. Binaries are verified
before capture. The frozen retail driver predates batching, so retail captures
one job per Wine process by default; hotfix runs 32. A batch the driver ignored,
or a clean exit without job outputs, stops the capture instead of being stored. `progress.json`, `capture.log` and `run-*.json` record progress
and execution provenance. A complete table includes fault outcomes; it does not
mean every request generated a map successfully.

The database uses SQLite WAL mode and capture writes only between batches, so
`check` and `status` read safely while a capture runs. A running check sees the
snapshot that existed when its query started.

## Checking Rust

Build the streaming Rust adapter once after code changes, then check either
mode without Wine. Assets and workspaces are reused between requests:

```sh
cargo build --release --manifest-path tools/Cargo.toml \
  -p homm3-rmg-cli --example corpus_replay

homm3 rmg rainbow --out "$RMG_RAINBOW" check \
  --mode hotfix --data "$RMG_DATA" \
  --runner tools/target/release/examples/corpus_replay \
  --report build/hotfix-comparison.jsonl
```

Adjust the runner path if `CARGO_TARGET_DIR` is set. Reports must be new paths;
the checker never overwrites prior evidence. By default every stored reference
is checked; `--limit`, `--start` and repeated `--name` arguments select a
subset. It stops at the first unexplained mismatch unless `--keep-going` is
supplied. Reports record individual results, the runner's hash and the
manifest's hash. Installation changes fail before comparison, including new
loose-file overrides.

Byte hash/length, effective request and RNG must all match for `equal`.
The specific hotfix missing-town rejection also requires matching RNG and
request. Known out-of-array coast reads and unassigned-player-zone writes in
retail are reported as separate typed faults, never as equal maps. Native
crashes and timeouts are recorded separately and skipped during Rust checking.
