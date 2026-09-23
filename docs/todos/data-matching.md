# Data matching

The [TSV → delinker → objdiff pipeline](../tooling/data-coverage.md) is enabled
in ordinary full builds. Start with `build/gen/data/retail-data.tsv`: every retail
`.rdata`, `.data` and zero-fill byte is either delivered to the delinker or listed
as a gap/conflict. Source annotations own names; generated TSVs join them to VC6
storage. Vendor ownership includes independently anchored Microsoft library data.

Remaining work:

- Resolve unknown and conflicting ranges; establish real table extents rather
  than assigning an entire gap to the preceding symbol.
- Recover missing candidate definitions and independently anchor pointer targets.
- Resolve missing/ambiguous library contributions, COMMON selections and folded
  compiler copies without selecting them solely because their bytes agree.
- Fix initializer, pointer and extent differences shown by objdiff and the static
  pointer/coverage TSVs.
- Enable Vostok `--strict` once every referenced data target has an admitted owner.

The unique retail denominator, delinker delivery and objdiff match percentage are
separate measurements. Zero agreement alone does not prove runtime initialization
or correct shared-storage dimensions and byte/pixel units.
