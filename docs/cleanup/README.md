# Source cleanup review

This review applies the H1/H2 cleanup approach to HoMM3 Complete. Windows
retail code, native source evidence and the repository's MAX/HIST accounting
remain authoritative. Readability changes do not establish a recovered
original spelling unless native symbols prove it.

## Precedent

The H1/H2 histories were reviewed on 2026-11-02. Relevant examples are H1
`b803c968` (constant-domain decisions), `0d6fd211` and `3f404239` (proven
Boolean contracts), `1dd0fbb4` (local names), and H2 `5f94b2206` (zero open
literal findings), `d7ab0cdb9` (one remote-command domain), `13905afab`
(typed arrays), `046170ce0` and `ef1b504f0` (declaration ownership),
`86ed7e76a` (Boolean contracts), and `96352ca0c` (assignments outside
conditions). These commits live in their respective repositories.

The H2 constants brief distinguishes semantic reuse from coincident values:
reuse an existing name only when its domain and use agree; otherwise retain
separate domains or document why a number is appropriate. Its zero-open
literal result is a completed H2 review, not an assertion about H3.

## Dedicated lanes

| Lane | Scope | Record |
| --- | --- | --- |
| Enums and constants | Canonical domain ownership, duplicate enums, named constants and literal dispositions | [Enums and constants](enums-constants.md) |
| Control flow | Supported structured replacements for gotos and assignments in conditions | [Control flow](control-flow.md) |
| Boolean contracts | Native return/parameter evidence; preserve field width and ABI | [Boolean types](boolean-types.md) |
| Naming and ownership | Proven names, understood local roles and canonical declarations | [Naming and ownership](naming-ownership.md) |
| Editors | Editor enum reuse, structured flow and source hygiene | [Editors](editors.md) |

Each record distinguishes adopted changes, retained source, rejected probes
and pending work. It identifies owning functions/files, evidence, validation
commands and measured results. A reviewed item need not be removed. Temporary
search manifests and diagnostic output belong in ignored `build/` directories.

All five lanes use separate `/tmp/homm3-cleanup-*-20261102` worktrees and
build directories on `/dev/nvme0n1p1` (ext4). Integration uses
`/tmp/homm3-cleanup-integration-20261102`. The base is `eb0115861`.
The shared Git repository and read-only seed inputs remain in the original
checkout. Existing unrelated worktrees are not modified.

## Integrated results

The five worker commits are integrated into the SSD review branch. The enum
lane removes a duplicate artifact-class domain and names creature-bank extents;
the editor lane replaces 17 tile-direction literals. Control-flow cleanup
moves four assignments out of conditions. All tested goto removals remain
rejected: the last candidate was game-exact but lowered the map-editor
comparison, so integration restored its shared exit. The Boolean lane
restores 16 native predicate declarations, preserving original spellings and
unproven byte storage. Naming cleanup clarifies video state and centralizes
four shared declarations. Rejected probes and remaining leads are recorded in
the five lane documents rather than claimed complete.

The combined targeted VC6 comparison preserves all current-source MAX values.
The approved PR changes add seven exact game functions; #191 retains the
existing direction-selection function and excludes its unsupported identity
ledger. #195 moves the existing packed-bit writer to one shared header and
uses it in player, hero and RMG serialization. The retained return contract
and checked-bit behavior are documented in [the PR review](pr-review.md).

All 28 open PRs have a recorded disposition. Independent compiler checks cover
19 acceptable or corrected candidates; rejected source models and older feature
drafts have explicitly limited review scopes. The remaining PRs stay for the
user's review. Full landing gates and publication are recorded there separately.

## Existing H3 work

The existing [reconstruction debt](../todos/reconstruction_debt.md),
[goto audit](../reconstruction/goto-audit.md),
[union/pragma audit](../reconstruction/union-pragma-audit.md), and
[compiler warnings](../tooling/compiler-warnings.md) remain the detailed
records for earlier investigations. Their counts describe their recorded
checkpoints and must not be presented as fresh whole-tree totals.

Two existing cleanup branches were inspected read-only. `cleanup/strict-enums`
at `388baae83` contains enum auditing and domain changes absent from the base;
`cleanup/naming-pass` at `e039de8eb` contains native member spelling and
declaration cleanup. Both worktrees contain uncommitted work. This review
does not treat those working trees as integrated or overwrite their changes.

## Broader review queue

This queue records scope beyond the first concrete lane changes. An item is
complete only when its review record provides evidence and validation.

| Area | Review rule |
| --- | --- |
| Enum and constant coverage | Record canonical/reuse/retain decisions by semantic domain; keep table data, arithmetic identities and unrelated equal values distinct. |
| Gotos and conditions | Recover natural helper boundaries, scopes and loops; preserve deliberate assignment side effects and cleanup lifetimes. |
| Boolean storage | Use decorated interfaces and all relevant stores/callers; a byte value or a few 0/1 writes alone is insufficient. |
| Names | Native original spellings win; inferred names describe the demonstrated role and retain lookup evidence. |
| Header ownership | Keep one canonical declaration at its owner and verify consumers; avoid cosmetic header churn. |
| Allocation and lifetime | Review element type, count, construction and matching release together; preserve retail behavior and ABI. |
| Casts, unions and pointer views | Resolve actual type/ownership evidence rather than replacing one cast spelling with another. |
| Switches and expressions | Document intentional fallthrough, precedence and side effects; do not silently fix a retail bug. |
| Compiler pragmas and shared helpers | Preserve recovered canonical helper calls and proven inline qualifiers; do not add steering pragmas or dummy operations. |
| Bounds, varargs and unused state | Record demonstrated behavior, allocation bounds and unresolved evidence; do not invent semantics from placeholder names. |
| Comments and documentation | Preserve useful evidence and retained-case reasons; distinguish current results from historical measurements. |

For each adopted game change, use the targeted VC6 build. Refresh owning
targets after compared symbol changes. Preserve earned MAX/HIST and explain
source-owned score changes; incidental CUR movement is not a reason to add
compiler-steering source. Editor changes use their correct image profile.
Bank fresh measured scores and integrate the lanes through the repository's
landing workflow.
