# Agent Guide

Read `CLAUDE.md` before changing anything. Keep this file durable: no current
assignments, queue snapshots, percentages, or next actions.

## Objective

Byte-matching reconstruction of the pinned target — **English GOG Heroes III Complete
4.0 (engine 3.2)**, `HEROES3.EXE`, 2,732,032 bytes, SHA-256 `057c9d88e720…` — with the
VC6 SP3 toolchain. The project is in the bootstrap phase: porting the Gruntz pipeline
architecture step by step per `docs/gruntz-script-port.md`.

## Hard rules

- **Supervised review.** Nothing is ported, copied, or admitted — from
  `decomp-attempt-1`, Gruntz, NH3API, or anywhere — without the user's explicit
  approval. Survey and propose; never land unrequested material. Record each approved
  decision in the port plan's decision log (§5) within the same change.
- **Retail bytes are the only correctness authority.** The Dreamcast dump and NH3API
  are naming/semantic evidence, never address ground truth. objdiff percentages guide
  work; they are not proof.
- **Vendored sources stay pristine.** No annotation macros or edits under `vendor/`.
- **Game bytes stay out of git.** The executable and media live under gitignored
  `build/` (working copies) and `../orig/` (safekeeping); hash-verify before use.
- One reviewable unit per step: each port-plan step is roughly one commit, landed only
  after it has been read and adapted in session.

## Build

```sh
nix develop .#build
homm3 init | configure | build | link | clean     # see CLAUDE.md for semantics
```

Run `homm3 build` before trusting any comparison output. The delink step is not yet
ported; objdiff targets are placeholders until it lands.

## Git discipline

- Focused commits with short imperative subjects matching the existing history
  ("Add …", "Update …", "Restructure …"). No bodies unless a body earns its place.
- Never revert user or concurrent-agent changes. Stage only what the change declares;
  leave unrelated dirty files alone.
- Do not push without being asked.
