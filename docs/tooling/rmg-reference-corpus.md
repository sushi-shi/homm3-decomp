# Local RMG reference corpus

The corpus records C++ outcomes for 100,000 deterministic requests in each of
retail and hotfix modes. Generated references stay under ignored `build/`;
only the capture/checking tools belong in Git. Rust never supplies reference
values. Capture uses one worker; independent Rust checks can run while it works.

Each reference records the uncompressed H3M SHA-256 and byte length, final RNG
state, effective request, return code and x87 control words. Crashes, timeouts
and rejections remain distinct outcomes. These are hash tables, not retained
map files: a mismatch can be detected without C++, but locating its first byte
requires recapturing that case with the frozen oracle.

`manifest.json` fingerprints installation assets (including loose overrides),
the pinned retail image, frozen harness binaries, libraries and hotfix objects.
`cases.json` retains original requests, seeds and controlled stack/heap inputs.
`references.sqlite3` stores the indexed inputs and results. Retail replay uses
the recorded harness policy: heap-derived water-town permissions, alignment
matching when heap fill is nonzero, and the signed stack word as the initial
key-tent cursor. This policy is specific to these captures.

Run the following commands in the repository's Nix build environment with
`PYTHONPATH=scripts`. Choose an absolute corpus path so different worktrees can
share it. In this development session it is located at
`/home/sheep/Projects/homm3/homm3-decomp/build/rmg-reference`.

```sh
export RMG_CORPUS=/absolute/path/to/build/rmg-reference
export RMG_DATA=/absolute/path/to/game/Data

python -m homm3.rmg.corpus --out "$RMG_CORPUS" status
```

To create a corpus from the existing native captures:

```sh
python -m homm3.rmg.corpus --out "$RMG_CORPUS" init \
  --retail-corpus /path/to/sampled-100000 \
  --hotfix-oracle /path/to/frozen/hotfix \
  --hotfix-captures /path/to/hotfix-campaign \
  --data "$RMG_DATA"
```

The optional hotfix import checks archived job bytes and native result records;
it does not trust a previous Rust comparison's pass/fail verdict. Retail imports
select the native side's hash only when it is tied to the recorded native state.

To capture or resume missing hotfix entries, set `WINEPREFIX` to the prepared
oracle prefix, then run one background worker. This example restricts it to
CPU 0 and lowers its scheduling priority:

```sh
nohup nice -n 10 taskset -c 0 python -m homm3.rmg.corpus \
  --out "$RMG_CORPUS" capture --mode hotfix --data "$RMG_DATA" \
  > "$RMG_CORPUS/capture.log" 2>&1 < /dev/null &
```

A writer lock prevents duplicate capture workers. Each committed batch survives
interruption; restarting captures only missing entries. Faulting jobs are
recorded, and jobs not reached after a crash are retried. Binaries are verified
before capture. `progress.json`, `capture.log` and `run-*.json` record progress
and execution provenance. A complete table includes fault outcomes; it does not
mean every request generated a map successfully.

Build the streaming Rust adapter once after code changes, then check either
mode without Wine. Assets and workspaces are reused between requests:

```sh
cargo build --release --manifest-path tools/Cargo.toml \
  -p homm3-rmg-cli --example corpus_replay

python -m homm3.rmg.corpus --out "$RMG_CORPUS" check \
  --mode hotfix --data "$RMG_DATA" \
  --runner tools/target/release/examples/corpus_replay \
  --report build/hotfix-comparison.jsonl
```

Adjust the runner path if `CARGO_TARGET_DIR` is set. Reports must be new paths;
the checker never overwrites prior evidence. `--limit`, `--start` and repeated
`--name` arguments select a subset. It stops at the first unexplained mismatch
unless `--keep-going` is supplied. Reports record individual results, the
runner's hash and the corpus manifest's hash. Installation changes fail before
comparison, including new loose-file overrides.

Byte hash/length, effective request and RNG must all match for `equal`.
The specific hotfix missing-town rejection also requires matching RNG and
request. Known out-of-array coast reads and unassigned-player-zone writes in
retail are reported as separate typed faults, never as equal maps. Native
crashes and timeouts are recorded separately and skipped during Rust checking.
