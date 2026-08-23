#!/usr/bin/env python3
"""homm3.core.clang - the MSVC-target clang probe (extraction's name oracle).

One job: hand `homm3.retail_labels.source` the LLVM IR of a game TU, in
which `@llvm.global.annotations` pairs each `VA()` annotation string
DIRECTLY with the function's MSVC-mangled symbol. That pairing is what the
lexical scan cannot produce: no positional join, so an inline header
definition can never steal a nearby address, and no lossy demangle key, so
two functions that normalize to the same spelling can never swap names.

clang is a NAME ORACLE, never a code oracle. Nothing here compiles the
matching build - VC6 SP3 under Wine remains the sole verdict on a match
(`homm3.core.cc_wrap`). The IR is read for symbol names and thrown away.

THE FLAG SET reproduces cl's own view of the TU well enough that the two
compilers agree on the mangled name, and nothing more:

    --target=i686-pc-windows-msvc   the Microsoft mangler and i386 ABI
    -fms-compatibility -fms-extensions -fms-compatibility-version=1200
                                    VC6-era language rules
    /Gr                             the build's fastcall default; without it
                                    every free function mangles `@@YA` where
                                    cl writes `@@YI` (measured: 171 of 1397
                                    claims, all of them free functions)
    /EHsc                           the build's /GX; without it every `try`
                                    in the tree is a hard error (7 TUs)

THE INCLUDE MIRROR. VC6 ships its headers UPPERCASE (`BITSET`, `VECTOR`,
`STRING`) and its STL predates the standard it targets, so on a
case-sensitive filesystem clang cannot even open `<bitset>`, and once it
can, seven headers do not parse. `mirror()` generates a symlink farm under
lowercase names and replaces exactly the files that need a conformance fix
(see PATCHES - each entry states what cl accepts and clang does not). The
real toolchain headers are never touched: only cl reads those, and only
clang reads the mirror.

The patches are argued mangling-neutral, but the argument is not what
admits them: `source.py` CONFIRMS every name this module proposes against
the symbol table of cl's own base obj for the same TU. A patch that
perturbed a mangling would drop the confirmation rate, so the corpus-wide
confirmation IS the mirror's negative control, re-run on every extraction.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

from homm3.core import common
from homm3.core.cc_wrap import msvc_dir

MIRROR = common.HOMM3_DIR / "build/gen/msvc-include"
STAMP = MIRROR / ".mirror-stamp"

#: Bumped whenever PATCHES changes, so an existing mirror regenerates.
PATCH_VERSION = 2

TARGET = "i686-pc-windows-msvc"
MSC_VER = "1200"
FLAGS = [f"--target={TARGET}", "-fms-compatibility",
         f"-fms-compatibility-version={MSC_VER}", "-fms-extensions",
         "/EHsc", "/Gr", "/D_WINDOWS", "-Wno-everything"]


def _drop_redundant_traits_default(text: str) -> str:
    """VC6 repeats `= char_traits<_E>` on every REdeclaration of the stream
    templates; <iosfwd> is the primary and keeps it. cl tolerates the
    repeat, clang rejects it (`template parameter redefines default
    argument`). Dropping a repeated default cannot change a mangling: the
    template's own name and its arguments are unchanged."""
    return text.replace("class _Tr = char_traits<_E> >", "class _Tr >")


def _template_prefix_specializations(text: str) -> str:
    """VC6 writes explicit specializations without the `template<>` C++
    requires (`class _CRTIMP ctype<char> : ...`). Adding the prefix is pure
    syntax: the specialization, and so every name mangled through it, is
    the same entity either way."""
    return re.sub(r"^class _CRTIMP (codecvt<wchar_t, char, mbstate_t>"
                  r"|ctype<char>)", r"template<> class _CRTIMP \1",
                  text, flags=re.M)


def _vector_bool(text: str) -> str:
    """Two VC6-isms in <vector>'s vector<bool>:

    * the specialization again lacks `template<>`;
    * `const_iterator(const iterator&)` reads members of the nested
      `iterator`, which at that point is only forward-declared. cl delays
      the inline body until the class is complete; clang parses it in
      place and resolves `iterator` to the injected base-class name
      `std::iterator` instead. The converting constructor is dropped from
      the mirror - no game TU converts a vector<bool> iterator, and a
      constructor's presence does not change any other name's mangling."""
    text = text.replace("\nclass vector<_Bool, _Bool_allocator> {",
                        "\ntemplate<> class vector<_Bool, _Bool_allocator> {")
    return text.replace("\t\tconst_iterator(const iterator& _X)\n"
                        "\t\t\t: _Off(_X._Off), _Ptr(_X._Ptr) {}\n", "")


def _sstream(text: str) -> str:
    """VC6 repeats both default template arguments from <iosfwd> on the
    four string-stream definitions, and finds ``in``/``out`` through their
    dependent stream bases. Clang rejects the repeated defaults and does not
    perform that legacy dependent-base lookup. Removing the defaults and
    qualifying the same ios_base enumerators changes neither entity nor any
    name mangled through it."""
    text = text.replace(
        "\tclass _Tr = char_traits<_E>,\n\tclass _A = allocator<_E> >",
        "\tclass _Tr,\n\tclass _A >")
    text = text.replace("openmode _M = in)",
                        "openmode _M = ios_base::in)")
    text = text.replace("_Sb(_M | in)", "_Sb(_M | ios_base::in)")
    text = text.replace("openmode _M = out)",
                        "openmode _M = ios_base::out)")
    text = text.replace("_Sb(_M | out)", "_Sb(_M | ios_base::out)")
    text = text.replace("openmode _W = in | out)",
                        "openmode _W = ios_base::in | ios_base::out)")
    return text


def _qualify_ios_enumerators(names):
    """`flags() & unitbuf`, `_Bfl == oct`: <ios> gives ios_base both an
    ENUMERATOR and a same-named std:: manipulator function. cl's lookup
    finds the enumerator; clang finds the manipulator and reports an
    invalid operand. Qualifying picks the entity cl already picked."""
    def apply(text: str) -> str:
        for name in names:
            text = text.replace(f"flags() & {name}", f"flags() & ios_base::{name}")
            text = text.replace(f"== {name}", f"== ios_base::{name}")
        return text
    return apply


#: {mirror file name: rewrite}. Every entry states, in its rewrite's
#: docstring, what cl accepts that clang does not.
PATCHES = {
    "utility": _drop_redundant_traits_default,
    "streambuf": _drop_redundant_traits_default,
    "ios": _drop_redundant_traits_default,
    "xlocale": _template_prefix_specializations,
    "vector": _vector_bool,
    "ostream": lambda t: _qualify_ios_enumerators(
        ("unitbuf", "oct", "hex", "dec"))(_drop_redundant_traits_default(t)),
    "istream": lambda t: _qualify_ios_enumerators(
        ("skipws", "oct", "hex", "dec"))(_drop_redundant_traits_default(t)),
    "sstream": _sstream,
}


def clang_bin() -> str | None:
    """The probe binary, or None when clang is absent (outside the nix
    devshell). Extraction degrades to the lexical channel and SAYS SO -
    a missing tool must never look like a TU with no claims."""
    exe = os.environ.get("HOMM3_CLANG") or shutil.which("clang")
    return exe if exe and Path(exe).exists() else None


def mirror() -> Path | None:
    """The generated lowercase, conformance-patched VC6 include tree.

    Regenerated whenever the toolchain root or PATCH_VERSION changes; the
    symlinks cost nothing and the patched copies are 7 small files."""
    root = msvc_dir() / "include"
    if not root.is_dir():
        return None
    want = f"{os.path.realpath(root)}\n{PATCH_VERSION}\n"
    if STAMP.is_file() and STAMP.read_text() == want:
        return MIRROR
    MIRROR.parent.mkdir(parents=True, exist_ok=True)
    tmp = Path(tempfile.mkdtemp(dir=str(MIRROR.parent)))
    for entry in sorted(root.iterdir()):
        (tmp / entry.name.lower()).symlink_to(entry)
    for name, rewrite in PATCHES.items():
        link = tmp / name
        if not link.exists():
            continue
        text = link.resolve().read_text(errors="replace")
        patched = rewrite(text)
        link.unlink()
        link.write_text(patched)
    (tmp / STAMP.name).write_text(want)
    if MIRROR.is_dir():
        shutil.rmtree(MIRROR)
    tmp.replace(MIRROR)
    return MIRROR


def emit_ir(src: Path, extra_flags: list[str] | None = None) -> str | None:
    """Textual LLVM IR for one TU, or None when clang cannot read it.

    None is never silent at the call site: a TU whose IR is missing keeps
    the lexical channel and is REPORTED, because a probe that quietly
    contributed zero names would shrink the denominator of every later
    count."""
    exe, inc = clang_bin(), mirror()
    if exe is None or inc is None:
        return None
    with tempfile.TemporaryDirectory() as td:
        out = Path(td) / "tu.ll"
        cmd = [exe, "--driver-mode=cl", *FLAGS, *(extra_flags or []),
               "-imsvc", str(inc), f"/I{common.HOMM3_DIR / 'include'}",
               "-Xclang", "-emit-llvm", "-o", str(out), "-c", str(src)]
        try:
            subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        except (OSError, subprocess.SubprocessError):
            return None
        if not out.is_file() or not out.stat().st_size:
            return None
        return out.read_text(errors="replace")
