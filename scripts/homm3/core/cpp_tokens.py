"""Lexical C/C++ tokens for comment-only edits and implementation fingerprints."""
from __future__ import annotations

import hashlib
import json
import re
from typing import NamedTuple


class Token(NamedTuple):
    kind: str
    start: int
    end: int
    text: str


LEX = re.compile(
    r'(?P<comment>//[^\n]*|/\*[\s\S]*?\*/)'
    r'|(?P<literal>(?:u8|[LuU])?"(?:\\[\s\S]|[^"\\])*"'
    r"|(?:[LuU])?'(?:\\[\s\S]|[^'\\\n])*')"
    r'|(?P<space>\s+)'
    r'|(?P<word>[A-Za-z_$][\w$]*)'
    r'|(?P<number>(?:\d|\.\d)(?:[eEpP][+-]|[\w.])*)'
    r'|(?P<operator>->\*|\.\.\.|<<=|>>=|::|->|\.\*|\+\+|--|&&|\|\||'
    r'<=|>=|==|!=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|<<|>>|##|[^\s])')


def tokens(source: str):
    # C translation phase 2 joins continued physical lines before comments.
    # Keep physical offsets for edits, including continuations in // comments.
    at = 0
    while at < len(source):
        match = LEX.match(source, at)
        if match is None:
            raise ValueError(f"unrecognized C++ token at {at}")
        end = match.end()
        if match.lastgroup == "comment" and match.group().startswith("//"):
            while end < len(source) and source[end - 1:end] == "\\":
                newline = source.find("\n", end + 1)
                end = len(source) if newline < 0 else newline
        yield Token(match.lastgroup, at, end, source[at:end])
        at = end


def code_tokens(source: str) -> tuple[str, ...]:
    """Ignore comments/spacing without joining identifiers or punctuators.

    Directive newlines remain significant, as does the whitespace separating
    a function-like macro's name and opening parenthesis.
    """
    source = re.sub(r"\\\r?\n", "", source)
    result, line_start, directive = [], True, False
    macro_name_end = None
    define = False
    for token in tokens(source):
        if token.kind in ("comment", "space"):
            if token.kind == "space" and "\n" in token.text:
                if directive:
                    result.append("<directive-end>")
                line_start, directive, define, macro_name_end = True, False, False, None
            continue
        if line_start and token.text == "#":
            directive = True
        elif directive and result and result[-1] == "#" and token.text == "define":
            define = True
        elif define:
            macro_name_end, define = token.end, False
        elif macro_name_end is not None:
            if token.text == "(" and token.start != macro_name_end:
                result.append("<object-like-macro>")
            macro_name_end = None
        result.append(token.text)
        line_start = False
    if directive:
        result.append("<directive-end>")
    return tuple(result)


def fingerprint(source: str) -> str:
    payload = json.dumps(code_tokens(source), ensure_ascii=True, separators=(",", ":"))
    return "tokens1:" + hashlib.sha1(payload.encode()).hexdigest()[:12]
