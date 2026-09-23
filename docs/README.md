# Documentation

Start with the [README quickstart](../README.md#quickstart) to set up, build and
iterate on a function.

## Matching

- [Source ownership](matching/source-ownership.md) and [source facts](matching/source-facts.md)
- [Dreamcast line tables](matching/dc-line-tables.md)
- [Naming conventions](matching/naming.md)
- [Source hypothesis runner](matching/source-hypotheses.md)
- [VC6 compiler reference](vc6/README.md)

## Formats and oracles

- [Codec module](formats/codec-module.md), [LOD and DEF](formats/lod-def-formats.md), [format matrix](formats/resource-format-matrix.md)
- [RMG oracle](oracles/rmg.md) and [Victor oracle](oracles/victor.md)

## Reconstruction

- [Field layout evidence](reconstruction/field-layouts.md)
- [Game source models](reconstruction/game-source-models.md), [RMG source models](reconstruction/rmg-source-models.md), [historical match recovery](reconstruction/historical-matches.md)
- [Address arithmetic](reconstruction/address-arithmetic-audit.md), [owner pointers](reconstruction/owner-pointer-audit.md), [pointer boundaries](reconstruction/pointer-boundary-repairs.md)
- [Gotos](reconstruction/goto-audit.md), [unions and pragmas](reconstruction/union-pragma-audit.md), [preprocessor](reconstruction/preprocessor-audit.md)

## Tooling and reference

- [Data flow](tooling/data-flow.md), [telemetry](tooling/telemetry.md), [performance measurement](tooling/performance.md), [compiler warnings](tooling/compiler-warnings.md)
- [Complete retail byte accounting](tooling/data-coverage.md)
- [Executable libraries](reference/executable-libraries.md)
- [RMG undefined behavior](reference/rmg-undefined-behavior.md) and [Voronoi provenance](reference/rmg-voronoi-provenance.md)

## Outstanding work

- [Reconstruction debt](todos/reconstruction_debt.md)
- [Unresolved fields](todos/unresolved-data-fields.md) and [data matching](todos/data-matching.md)
- [Save-game oracle](todos/save-game-oracle.md)
- [Tooling](todos/tooling.md)
- [VC6 out-of-line helpers](todos/vc6-budget-free-out-of-line-functions.md)
