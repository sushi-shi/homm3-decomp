# NH3API v2.1 IDB - browsable reference corpus

Extracted by `scripts/homm3/carve/idb_extract.py` (one-off) from the IDA
7.6 database released with NH3API v2.1 (asset sha256 `4a39cc47...`, read in
place from `../decomp-attempt-1/build/analysis/ida/H3.exe.idb`, parsed with
its vendored python-idb).

**Addresses are the HD pressing** (`Heroes3 HD.exe`, md5 `e41e00f3...`) -
NOT the pinned retail image. They are address-compatible with our HD Mod
executable (98.3% of NH3API call-macro addresses are call targets there),
so `functions.csv` carries a `retail_rva` column where the hdmap
masked-identity bridge lands; every other address is reference-only.
Names, layouts, enums, and prototypes are external candidates to consult -
an identity still needs retail-byte proof before it is claimed.

| file | contents |
|---|---|
| types.h | all TIL local types as C declarations (the class layouts) |
| structs.h | the structs view with explicit member offsets |
| enums.txt | enum netnodes (member rendering where python-idb allows) |
| functions.csv | functions with prototypes (parameter names!) + bridge |
| names.csv | every name in the database, classified |
| vtables.csv | the `??_7` vtable addresses |
| comments.csv | every stored comment |

Rendering caveats: python-idb draws enum member values with a high-dword
serial artifact (`K_SPELL_COUNT = 0x100000051` - the TRUE value is the low
32 bits, 0x51); struct members appear in declared order without offsets;
the IDA 7.6 `id2` section is unparsed (no known loss for these tables).
