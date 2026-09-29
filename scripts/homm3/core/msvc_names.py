"""Shared compiler-name normalization for Gruntz's model on VC6 inputs.

VC6 keeps Clang's array Q spelling and emits file statics with their COFF
name (without VC5's extra $S wrapper). Volatile ordinals in emitted local
statics still normalize for joins. Source declarations own identities.
"""

from __future__ import annotations

import hashlib
import re

#: clang's top-level-array storage class, at the mangled storage-class digit.
ARRAY_STORAGE = re.compile(r"@@([0-9])Q")
#: cl's per-object CodeView counter on a TU-local datum. Every occurrence is
#: volatile, not just the trailing one (a function-local static's guard byte is
#: spelled `?$S<n>@?<scope>??<fn>@4EA$S<n>`). A name that is NOTHING BUT `$S<n>`
#: is not this: that is the rva-keyed spelling `discriminate` produces.
STATIC_ORDINAL = re.compile(r"(?<=.)\$S[0-9]+(?=@|$)")
#: cl's lexical-scope number in a function-local static's mangled name. MSVC
#: spells 1..10 as the digits `0`..`9` and larger values in hex as `A..P@`.
LOCAL_STATIC_SCOPE = re.compile(r"@\?(?:[0-9]|[A-P]+@)\?\?")
#: the scope spelling both sides agree on - clang's, for the one scope we model.
CANONICAL_SCOPE = "@?1??"
# VC5 encodes a source-file anonymous namespace as ?%<absolute cpp path><nonce>@.
# Only a namespace declared in a repository .cpp can be identified from this
# spelling alone. A header path does NOT identify its instantiating TU.
ANONYMOUS_NAMESPACE = re.compile(r"\?%([^@]+?\.(?:cpp|cxx|cc))([0-9]+)@")


def anonymous_namespaces(name: str) -> str:
    """Stable TU identity for source-file anonymous namespaces, never headers.

    Keep the source path below src/, discarding only the checkout prefix and
    compiler nonce. Standard MSVC anonymous-namespace syntax carries the hash.
    This is a comparison/claim spelling; the linker consumes the untouched obj.
    """
    def replace(match: re.Match) -> str:
        path = re.sub(r"/+", "/", match[1].replace("\\", "/"))
        before, separator, relative = path.rpartition("/src/")
        if not separator:
            return match[0]
        identity = "src/" + relative
        digest = hashlib.sha256(identity.encode("utf-8")).hexdigest()[:16]
        return "?A0x" + digest + "@"
    return ANONYMOUS_NAMESPACE.sub(replace, name)


def decorate(name: str) -> str:
    """The i386 COFF global prefix LLVM applies to a name it did not mangle."""
    return name if name.startswith("?") else "_" + name


def mask(name: str) -> str:
    """`name` with every volatile cl ordinal reduced to its canonical form."""
    name = re.sub(r"\$RVA[0-9a-f]+$", "", name)
    if re.fullmatch(r'_?\$S[0-9]+', name):
        return name
    return LOCAL_STATIC_SCOPE.sub(
        CANONICAL_SCOPE, STATIC_ORDINAL.sub("$S", anonymous_namespaces(name)))


def func(name: str, *, decorated: bool = False) -> str:
    """cl 5.0's spelling for a clang-proposed FUNCTION name."""
    out = ARRAY_STORAGE.sub(r"@@\1P", name)
    return mask(out if decorated else decorate(out))


#: a data reference (`A` type code after the variable/static-member digit):
#: Clang spells its storage class `B` where cl 6 emits `A`
#: (`const T (&g_x)[N]` is `...$$CBUT@@A` in every VC6 object).
REFERENCE_STORAGE = re.compile(r"(@@[0-9]A.*)B$")


def data(name: str, *, internal: bool, decorated: bool = False) -> str:
    """VC6 DATA spelling from a Clang declaration; linkage needs no wrapper."""
    out = name if decorated else decorate(name)
    out = REFERENCE_STORAGE.sub(r"\1A", out)
    if internal and out.startswith("?") and LOCAL_STATIC_SCOPE.search(out):
        out = "_" + out
    return mask(out)


def discriminate(name: str, rva: int, *, internal: bool = False) -> str | None:
    """`name` respelled for one rva, or None when the family has no room.

    Only the `$S` family carries a discriminator: the ordinal slot cl fills
    with a per-object counter takes the retail rva instead, so the spelling is
    unique image-wide and `mask` still folds it onto the shared family.
    """
    if internal:
        return f"{name}$RVA{rva:x}"
    return f"{name}{rva}" if name.endswith("$S") else None
