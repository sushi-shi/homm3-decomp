"""Source-owned Dreamcast procedure claims, independent of retail addresses."""
from dataclasses import dataclass
import re

from homm3.retail_labels.source import mask_lexical_noise


@dataclass(frozen=True)
class Claim:
    start: int
    end: int
    offset: int
    size: int
    va: int | None = None


_ANNOTATION = re.compile(r'\b(VA|MAC_ADDRESS|DC_ADDRESS)\s*\(\s*'
                         r'(0[xX][0-9a-fA-F]+)\s*,\s*'
                         r'(0[xX][0-9a-fA-F]+|[0-9]+)\s*\)')


def claims(text: str) -> list[Claim]:
    """Read literal claims; comments and strings never establish a pairing.

    Adjacent annotations may be on separate lines and in either order. A
    declarator ends a group, so one function cannot borrow another's VA.
    """
    masked = mask_lexical_noise(text)
    groups = []
    for match in _ANNOTATION.finditer(masked):
        if not groups or masked[groups[-1][-1].end():match.start()].strip():
            groups.append([])
        groups[-1].append(match)
    result = []
    for group in groups:
        vas = [int(m[2], 16) for m in group if m[1] == 'VA']
        va = vas[0] if len(vas) == 1 else None
        for m in group:
            if m[1] == 'DC_ADDRESS':
                result.append(Claim(m.start(), m.end(), int(m[2], 16), int(m[3], 0), va))
    return result
