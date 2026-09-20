# Victor resource oracle

The executable comparison links the four recovered Victor
objects using their normal VC6 profiles and calls them beside the pinned retail
image. No retail Victor function or global can satisfy an unresolved candidate
symbol. Only CRT allocation and exception handling remain retail services;
Win32 APIs come from explicit import libraries. The host reuses the RMG oracle's
verified WinMain bootstrap and disposable COFF relocation bridges.

From the VC6/Wine build shell, set `HOMM3_DIR` and `PYTHONPATH` to this checkout:

```sh
homm3 victor corpus --game-dir '/path/to/game' --out build/victor-inputs
homm3 victor compare --corpus build/victor-inputs --out build/victor-comparison
homm3 victor compare --corpus build/victor-inputs --out build/victor-dib --allocation dib
python -m unittest homm3.victor.test_corpus homm3.victor.test_protocol
```

Both output directories must be new. `compare --limit N` selects the first N
distinct inputs for diagnosis. `--timeout` bounds each process (600 seconds by
default). The installation is read-only. Extracted copyrighted assets and all
execution artifacts remain under ignored `build/`.

The installation examined on 2026-09-20 contains 7,215 bitmap occurrences in
LOD archives: 6,971 indexed and 244 packed 24-bit images, with 4,915 distinct
payloads. None is a standard PCX file. These archive resources use Heroes III's
12-byte bitmap wrapper, which Victor does not parse. The corpus encodes their
pixels and palettes losslessly into version-5 PCX inputs. RGB archive pixels
become PCX R/G/B planes; indexed pixels retain all 256 palette colors. Original
archive/member identities, payload hashes, input hashes and the explicit
`lossless-transcode` designation are retained for every occurrence, including
overridden archive entries. Original standard PCX files, if present in another
installation, are retained directly.

Each implementation runs the corpus twice in fresh processes. Within a run,
each image follows `pcxinfo → allocimage → loadpcx → flipimage → freeimage`.
Outputs include all status codes, PCX metadata, image descriptor scalars,
bitmap headers, palettes, and complete pixel buffers before and after loading
and flipping. Destination pixels begin filled with `0xa5`, including row
padding; this is a controlled input. Cleanup must clear the descriptor.
Pointer values are represented by existence and ownership relationships rather
than allocator addresses. Short per-run filenames accommodate `OpenFile`.

The strict output parser rejects missing, truncated, trailing or inconsistent
records. Comparison requires repeatability on both sides, identical output
streams, successful imports and artwork preservation against original archive
pixels and palettes. Provenance records compiler-object hashes, driver hashes,
source hashes, input identities and vendor-library hashes. The candidate DLL
is named `rmg-driver.dll` solely to reuse the existing fixed bootstrap.

The allocation mode is an explicit input: `global` uses ordinary global memory,
and `dib` uses a Win32 DIB section with the real GDI creation and palette APIs
bound identically on both sides. This exercises indexed/true-color PCX and
full-image in-place flips. The installed corpus does not exercise monochrome/four-bit PCX; supplementary
probes cover those modes as described below. It does not exercise clipped regions,
separate flip destinations, malformed-file paths or allocation failures.
DEF sprites, fonts, video, sound and other resource formats are not Victor PCX
inputs. A passing corpus comparison is execution evidence for this boundary,
not a byte match or a universal equivalence proof.

Both final full-corpus campaigns pass: `global` and `dib` each tested 4,915
distinct inputs in four fresh processes, for 39,320 imports covering all 7,215
installed bitmap occurrences. There are no differing outputs, execution errors,
repeatability failures or artwork-check failures. Reports and complete outputs
are retained in `build/victor-global-final/` and `build/victor-dib-all/`.

A supplementary generated corpus exercises monochrome, four-plane and packed
four-bit PCX at widths 1, 7, 8, 9, 31 and 64 (seven rows each). Its 18 inputs
exercise the remaining decode modes and the bit-range helpers through real
Victor calls. These are explicitly synthetic coverage probes, not installed
game resources. Both allocation modes match across all four processes (144
imports); artifacts are under `build/victor-low-depth-global/` and
`build/victor-low-depth-dib/`. Unlike archive images, these native-PCX probes have
no independent archive-artwork expectation.
