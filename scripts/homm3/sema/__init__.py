"""homm3.sema - read-only semantic navigation over the retail image.

One process per invocation, every tool answering from one lazily-loaded
Context (image, symbol db, relocs, universe classes, objdiff report,
call index) - no per-tool subprocesses, no re-parsing per query.

Tools (dispatch in __main__.py): `xref` (caller tree / callees),
`diff` (base-vs-target block diff), `disasm` (either side of one
function), `rva` (address dossier), `strings` (literal evidence).

rc convention: 0 = answered, 1 = answered-NO (differs), 2 = error.
Every invocation appends one line to build/homm3_sema.log.
"""
