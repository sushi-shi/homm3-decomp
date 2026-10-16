"""Trim compiler fill without cutting x86 instruction operands."""
from functools import lru_cache


@lru_cache(maxsize=1)
def _decoder():
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    return Cs(CS_ARCH_X86, CS_MODE_32)


def trimmed_end(payload: bytes, start: int, end: int, floor: int = 0) -> int:
    """Return a padding-free endpoint, retaining relocations and instructions.

    A final 90/CC byte may be a jump displacement or an immediate. Decode
    from the symbol start to the apparent fill boundary; if an instruction
    crosses it, retain the whole instruction. Undecodable prefixes provide
    no boundary proof, so leave their extent alone.
    """
    floor = max(start + 1, floor)
    boundary = end
    while boundary > floor and payload[boundary - 1] in (0x90, 0xCC):
        boundary -= 1
    if boundary == end:
        return end
    for address, size, _mnemonic, _operands in _decoder().disasm_lite(
            payload[start:end], start):
        instruction_end = address + size
        if instruction_end >= boundary:
            return max(boundary, instruction_end)
    return end
