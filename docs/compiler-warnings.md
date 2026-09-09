# Compiler warning audit

Generate a new report with the repository's configured Clang and hash-verified
MSVC 6.0 SP3 toolchain:

```sh
nix develop .#build
homm3 warnings
```

The command audits every TU in `config/units.toml`, including vendored zlib.
It creates a new timestamped directory under `build/compiler-warnings/` and
prints the path to `report.md`. Each invocation uses fresh disposable MSVC
objects; an incremental build with nothing to compile cannot produce a false
zero-warning result. The README checklist is manually maintained and is not
updated by this command.

Useful narrower runs:

```sh
homm3 warnings --compiler msvc
homm3 warnings --compiler clang
homm3 warnings --unit army --unit game --jobs 4
homm3 warnings --output /tmp/homm3-warnings
```

A partial run explicitly reports its selected units and the full manifest
denominator. `--timeout` sets the per-TU limit in seconds. All units are
attempted even if another unit fails. Exit status 0 means every selected
compiler invocation completed without errors; 1 means compilation, capture,
timeout, parsing, or input-freshness problems are recorded in the report.
Warnings alone do not make the audit fail.

## What is generated

| File | Contents |
| --- | --- |
| `report.md` | Coverage, compiler versions, warning counts by owner and diagnostic code, selected correctness-related diagnostics, every TU's status and raw-log link |
| `diagnostics.tsv` | Every unique primary warning and error, with location, code, message, emission count and all reporting TUs |
| `diagnostics.json` | The same complete inventory in structured form |
| `jobs.json` | Every invocation's exact command, exit/status, diagnostics and any unparsed diagnostic lines |
| `run.json` | Commit, timestamps, compiler identities, generator/input hashes, freshness result, project/vendor warning controls and source files outside the manifest |
| `logs/*.log` | Complete compiler output, including notes and template-instantiation context |
| `objects/*.obj` | Disposable MSVC `/W4` outputs, separate from matching objects |

The report's selected correctness-related table includes uninitialized values,
missing returns, dangling stack references, format/varargs problems and other
specific warning classes. It is a triage aid. The TSV/JSON files retain all
other categories, including unsafe buffer usage, conversions, unused values,
layout, portability, language extensions and SDK compatibility warnings.

For example, inspect project diagnostics without including SDK warnings:

```sh
jq '.[] | select(.origin == "project-source" or .origin == "project-header")' \
  /path/to/report/diagnostics.json
```

## Compiler coverage

MSVC uses the real per-TU flags from `config/units.toml`, including optimization,
calling convention, runtime and exception settings, with `/W4` added. Its
original SDK headers are used. The normal compilation wrapper discards output
after a successful compile, so this audit captures the compiler directly with
the same include paths instead of counting ordinary build-log lines.

Clang uses the same manifest-derived commands as the editor database: the
configured `HOMM3_CLANG`, `i686-pc-windows-msvc` target, VC6 compatibility,
per-TU defines/calling convention and generated SDK include mirror. The audit
replaces `-Wno-everything` with `-Weverything`, enables `-Wsystem-headers`, removes
diagnostic/error limits and runs `-fsyntax-only`. Project headers are marked as
system includes in the editor setup, so enabling system-header warnings is
necessary to include them in this census. Clang remains a diagnostic tool;
MSVC is the matching verdict.

The two compilers can disagree about legal legacy source. For example, the
existing VC6 `deque` header's dependent-base member lookup can fail in Clang.
Such errors are retained and affected TUs are marked incomplete. An error-free
MSVC build does not make the corresponding Clang diagnostic pass complete.

## Counting and interpretation

A unique diagnostic is the tuple **compiler, normalized file path, line,
column, severity, warning code/option and message**. Identical emissions from
multiple TUs count once, with their occurrence count and TU list preserved.
Different template-instantiation messages remain separate diagnostics. These
are diagnostic counts, not independently confirmed bugs or comparable measures
of compiler quality.

Diagnostics without a source location also include the TU in their identity:
VC6 can reuse generated names such as `$E50` in different objects. They are
reported as unlocated instead of being assigned a guessed source owner.

Ownership distinguishes project sources, project headers, vendored code, SDK
headers, compiler support and driver diagnostics. SDK paths identify the
original MSVC headers and the Clang mirror separately. Notes are not counted
as warnings; full notes and traces remain in each raw log. Invocation failures
and unparsed diagnostic lines never count as successful zero-warning passes.

The census covers the configured build, not every theoretical compilation:
inactive preprocessor branches, uninstantiated templates, headers not reached
by these TUs and source files without an admitted compiler profile remain
outside complete coverage. Existing source/SDK diagnostic controls still apply.
The tracked project/vendor controls are inventoried in `run.json`; SDK controls
are not inventoried.
Compiler warnings do not replace runtime checks or retail evidence, and a
warning-free TU is not proof of correct behavior. In particular, inspect inline
assembly and non-returning helper paths before treating a missing-return warning
as a defect.

The implementation is [compiler_warnings.py](../scripts/homm3/analysis/compiler_warnings.py).
Parser and coverage controls run with:

```sh
PYTHONPATH=scripts python -m unittest homm3.analysis.test_compiler_warnings
```
