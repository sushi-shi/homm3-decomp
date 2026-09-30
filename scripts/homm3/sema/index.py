"""homm3.sema.index - the Model, indexed for address queries.

sema reads identity from ONE place: `homm3.model.resolve()`. Every name,
unit, kind, size, space, channel and alias in a sema view comes from a
Binding - never from a re-join of the label tables, never from a generated
name file. This module adds only the lookups: address -> binding (exact or
containing), name -> address (mangled, `CClass::Member`, bare member), and
the short readable spelling of a mangled name.
"""

from __future__ import annotations

import bisect
import re
from functools import lru_cache

from homm3.model import Binding, Model, resolve

#: channels whose claim is a reconstruction under src/ (the matched frontier)
SRC_CHANNELS = ("src", "src_compgen", "src_dyninit", "src_data_compgen")

_IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*$")
_LOCAL_SCOPE = re.compile(r"\?[0-9]+\?\??(.*)$")
_SPECIAL = {
    "0": "{cls}::{cls}", "1": "{cls}::~{cls}",
    "_7": "{cls}::`vftable'", "_8": "{cls}::`vbtable'",
    "_G": "{cls}::`vector deleting dtor'",
    "_E": "{cls}::`scalar deleting dtor'",
    "_D": "{cls}::`vbase destructor'",
    "_R": "{cls}::`RTTI'", "_C": "`string'",
}


def split_mangled(name: str) -> tuple[str | None, list[str]]:
    """(special_code, name tokens innermost-first) for an MSVC mangling.

    Tokens are the qualified-name chain as spelled: `?Open@CMirrorFile@@...`
    -> (None, ['Open', 'CMirrorFile']). A function-local static's scope
    tokens start with `?` and are left in place for the caller to cut.
    Returns (None, []) for a name that is not a C++ mangling."""
    if not name.startswith("?"):
        return (None, [])
    body, special = name[1:], None
    if body.startswith("?"):
        body = body[1:]
        if body.startswith("_") and len(body) > 1:
            special, body = "_" + body[1], body[2:]
            if special == "_R" and body[:1].isdigit():
                body = body[1:]
        elif body:
            special, body = body[0], body[1:]
    end = body.find("@@")
    tokens = (body[:end] if end >= 0 else body).split("@")
    return (special, [t for t in tokens if t != ""])


def short_name(name: str) -> str:
    """The readable spelling of a mangled symbol: `CImage::RenderFrame`,
    `CMirrorFile::~CMirrorFile`, a function-local static's own identifier
    (`clip`), or the plain C name. Unmanglable names pass through."""
    if not name:
        return ""
    bare = name[1:] if name.startswith("_?") else name
    if not bare.startswith("?"):
        return bare.lstrip("_").split("@")[0] or bare
    special, tokens = split_mangled(bare)
    if not tokens:
        return bare
    if special:
        cls = tokens[0]
        return _SPECIAL.get(special, "{cls}::operator").format(cls=cls)
    member, scope = tokens[0], []
    for t in tokens[1:]:
        if not _IDENT.match(t):
            break                      # `?1?` etc: a local scope, not a class
        scope.append(t)
    if not scope and len(tokens) > 1:
        # a FUNCTION-LOCAL static: `_?clip@?1??RenderFrame@CImage@@...` - the
        # `?<depth>?` scope token carries the enclosing function's name. Two
        # methods of one class routinely spell the same local name (CImage has
        # a `clip` in RenderFrame AND in RenderFrameClipped), so the enclosing
        # function is part of the identity, not decoration.
        m = _LOCAL_SCOPE.match(tokens[1])
        if m and m.group(1):
            return f"{m.group(1)}::{member}"
    return "::".join(list(reversed(scope)) + [member])


class Index:
    """Address <-> binding lookups over one resolved Model."""

    def __init__(self, model: Model | None = None):
        self.model = model or resolve()
        self.functions = sorted(self.model.functions, key=lambda b: b.rva)
        self.data = sorted(self.model.data, key=lambda b: b.rva)
        self.fstarts = [b.rva for b in self.functions]
        self.dstarts = [b.rva for b in self.data]
        self._fmap = {b.rva: b for b in self.functions}
        self._dmap = {b.rva: b for b in self.data}
        self._byname: dict[str, set[int]] | None = None

    # --- address -> binding -------------------------------------------------

    def func(self, rva: int) -> Binding | None:
        """The function binding STARTING at `rva`."""
        return self._fmap.get(rva)

    def datum(self, rva: int) -> Binding | None:
        """The data binding STARTING at `rva`."""
        return self._dmap.get(rva)

    def at(self, rva: int) -> Binding | None:
        """The binding starting exactly at `rva`, either space."""
        return self._fmap.get(rva) or self._dmap.get(rva)

    def owner(self, rva: int) -> Binding | None:
        """The function whose extent CONTAINS `rva` (size-bounded: a site past
        a body's end belongs to no function, never to the previous one)."""
        i = bisect.bisect_right(self.fstarts, rva) - 1
        if i < 0:
            return None
        b = self.functions[i]
        return b if rva < b.rva + b.size else None

    def data_owner(self, rva: int) -> Binding | None:
        """The datum whose extent CONTAINS `rva`."""
        i = bisect.bisect_right(self.dstarts, rva) - 1
        if i < 0:
            return None
        b = self.data[i]
        return b if rva < b.rva + b.size else None

    def covering(self, rva: int) -> Binding | None:
        """The binding covering `rva` in either space."""
        return self.owner(rva) or self.data_owner(rva)

    def preceding_func(self, rva: int) -> Binding | None:
        """The nearest admitted function row at or before `rva` (whose extent
        need NOT contain it - that is what owner() answers)."""
        i = bisect.bisect_right(self.fstarts, rva) - 1
        return self.functions[i] if i >= 0 else None

    def next_start(self, rva: int) -> int | None:
        """The next admitted function start after `rva` (the gap's far edge)."""
        i = bisect.bisect_right(self.fstarts, rva)
        return self.fstarts[i] if i < len(self.fstarts) else None

    # --- naming -------------------------------------------------------------

    def is_claimed(self, b: Binding | None) -> bool:
        return bool(b is not None and b.channel)

    def is_src(self, b: Binding | None) -> bool:
        """Reconstructed under src/ - the matched side of the frontier."""
        return bool(b is not None and b.channel in SRC_CHANNELS)

    def display(self, b: Binding | None, rva: int | None = None) -> str:
        """A binding's readable name, or an honest placeholder."""
        if b is None:
            return f"(no admitted row @0x{rva or 0:08x})"
        if b.name:
            return short_name(b.name)
        what = {"thunk": "linker thunk", "eh": "EH funclet", "pad": "padding"}.get(
            b.kind, "func" if b.space == "text" else b.kind or "datum")
        return f"(unclaimed {what} @0x{b.rva:06x})"

    def label(self, rva: int) -> str:
        """`0x00153810 CImage::RenderFrameClipped [cimage]` for an exact start,
        or the containing binding with a +offset."""
        b = self.at(rva)
        off = 0
        if b is None:
            b = self.covering(rva)
            off = rva - b.rva if b else 0
        unit = f" [{b.unit}]" if b is not None and b.unit else ""
        plus = f"+0x{off:x}" if off else ""
        return f"0x{rva:08x} {self.display(b, rva)}{plus}{unit}"

    def ref_label(self, rva: int) -> str:
        """The operand spelling for a referenced address: `clip+0x64`,
        `CImage::Render`, the body behind a linker thunk, or a bare address
        when no admitted row owns it."""
        b = self.at(rva)
        if b is not None and (b.name or b.kind != "thunk"):
            return self.display(b, rva)
        from homm3.sema.image import retail
        body = retail().jmp_target(rva)
        if body is not None and body != rva:
            inner = self.at(body)
            if inner is not None:
                return f"{self.display(inner, body)} (via thunk 0x{rva:06x})"
        if b is not None:
            return self.display(b, rva)
        b = self.covering(rva)
        if b is None:
            return f"0x{rva:08x}"
        return f"{self.display(b, rva)}+0x{rva - b.rva:x}"

    # --- name -> address ----------------------------------------------------

    @property
    def byname(self) -> dict[str, set[int]]:
        """{spelling: {rva}} - exact mangled names, alias claims, the short
        `CClass::Member` form and the bare member name all resolve."""
        if self._byname is None:
            out: dict[str, set[int]] = {}

            def add(key: str, rva: int):
                if key:
                    out.setdefault(key, set()).add(rva)

            for b in self.functions + self.data:
                for nm in [b.name] + [a.name for a in b.aliases]:
                    if not nm:
                        continue
                    add(nm, b.rva)
                    s = short_name(nm)
                    add(s, b.rva)
                    if "::" in s:
                        add(s.rsplit("::", 1)[1], b.rva)
            self._byname = out
        return self._byname

    def resolve_name(self, token: str) -> list[int]:
        """Candidate rvas for a hex address or a name spelling."""
        try:
            return [int(token, 16)]
        except ValueError:
            pass
        return sorted(self.byname.get(token, ()))

    # --- unit views ---------------------------------------------------------

    def units(self) -> dict[str, list[Binding]]:
        """{unit: [binding]} over every claimed row (functions and data)."""
        out: dict[str, list[Binding]] = {}
        for b in self.functions + self.data:
            if b.unit:
                out.setdefault(b.unit, []).append(b)
        for rows in out.values():
            rows.sort(key=lambda b: b.rva)
        return out


@lru_cache(maxsize=1)
def index() -> Index:
    """The process-wide Index (one Model resolve per process)."""
    return Index()
