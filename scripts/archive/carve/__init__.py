"""homm3.carve - bootstrap-scoped carving of function/vtable inventories.

Retirement-scoped scaffolding (see README.md): produces build/carve/functions.tsv
(rva, size; size INCLUDES jump tables) and vtables.tsv (rva, function_count) from
the pinned retail HEROES3.EXE, then the whole package is archived.
"""
