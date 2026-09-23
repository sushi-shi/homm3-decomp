"""Disposable parser-state variants, adapted from Gruntz's TU-state generator.

These declarations exist only in candidate translation units. They vary the
kind, order and number of C1XX symbols rather than merely adding headers.
"""
from __future__ import annotations

import random
from dataclasses import dataclass


FAMILIES = (
    "forest", "typedef", "typedef-count", "enum", "struct", "class",
    "packed", "member", "extern", "static-data", "prototype", "function",
    "mixed",
)
SCALARS = ("char", "unsigned char", "short", "unsigned short", "int", "unsigned long")
VALUES = (-32768, -1, 0, 1, 2, 7, 31, 255, 256, 1024, 32767, 65535)
CONVENTIONS = ("__cdecl", "__fastcall", "__stdcall")


@dataclass(frozen=True)
class StateVariant:
    trial: int
    family: str
    tag: str
    body: str
    permutation: tuple[str, ...] = ()


def declaration_forest(rng: random.Random, ident: str, width: int) -> tuple[str, tuple[str, ...]]:
    """Shuffle several declaration kinds whose C1XX handle costs differ."""
    atoms: list[tuple[str, str]] = []
    for index in range(width):
        scalar = rng.choice(SCALARS)
        name = f"{ident}_TYPE_{index}"
        shape = rng.randrange(5)
        declarations = (
            f"typedef {scalar} {name};\n",
            f"typedef {scalar} *{name};\n",
            f"typedef {scalar} {name}[{2 + index % 7}];\n",
            f"typedef {scalar} (__cdecl *{name})(int, unsigned long);\n",
            f"typedef const {scalar} *{name};\n",
        )
        atoms.append((f"typedef:{shape}:{index}", declarations[shape]))
    for index in range(width):
        name = f"{ident}_CLASS_{index}"
        scalar = rng.choice(SCALARS)
        value = rng.choice((1, 2, 3, 7, 15, 31))
        shape = rng.randrange(8)
        declarations = (
            f"class {name} {{ public: {scalar} m_value; int read(int); }};\n",
            f"class {name} {{ private: {scalar} m_value; public: int identity(int x) "
            f"{{ return x; }} protected: unsigned long m_state; }};\n",
            f"class {name} {{ public: typedef {scalar} TValue; "
            f"enum EKind {{ ZERO = 0, LIMIT = {value} }}; TValue m_values[{2 + index % 4}]; }};\n",
            f"class {name} {{ public: static {scalar} s_value; "
            f"static int staticMethod(int); int method(unsigned long) const; }};\n",
            f"class {name} {{ public: virtual int first(int); "
            f"virtual unsigned long second(unsigned long); }};\n",
            f"class {name} {{ public: int overload(int); int overload(unsigned long); "
            f"int overload(const char*); }};\n",
            f"class {name} {{ private: unsigned int m_low : {1 + index % 7}; "
            f"unsigned int m_high : {1 + (index + 3) % 7}; "
            f"public: int bits() const; }};\n",
            f"#pragma pack(push, {(1, 2, 4, 8)[index % 4]})\n"
            f"class {name} {{ public: char m_tag; {scalar} m_value; "
            f"int packed(int x) {{ return x ^ {value}; }} }};\n#pragma pack(pop)\n",
        )
        atoms.append((f"class:{shape}:{index}", declarations[shape]))
    for index in range(width):
        name = f"{ident}_PROTOTYPE_{index}"
        scalar = rng.choice(SCALARS)
        convention = rng.choice(CONVENTIONS)
        shape = rng.randrange(4)
        declarations = (
            f"{scalar} {convention} {name}({scalar});\n",
            f"int {convention} {name}(int, unsigned long);\n",
            f"{scalar}* {convention} {name}({scalar}*, unsigned int);\n",
            f"void {convention} {name}(const {scalar}*, const {scalar}*);\n",
        )
        atoms.append((f"prototype:{shape}:{index}", declarations[shape]))
    for index in range(width):
        name = f"{ident}_FUNCTION_{index}"
        convention = rng.choice(CONVENTIONS)
        value = rng.choice((1, 2, 3, 7, 15, 31, 63, 127))
        shape = rng.randrange(4)
        declarations = (
            f"static int {convention} {name}(int x) {{ return x; }}\n",
            f"static int {convention} {name}(int x) {{ return x ^ {value}; }}\n",
            f"static unsigned long {convention} {name}(unsigned long a, unsigned long b) "
            f"{{ return (a + b) ^ {value}UL; }}\n",
            f"static int {convention} {name}(int a, int b) "
            f"{{ return a < b ? a + {value} : b - {value}; }}\n",
        )
        atoms.append((f"function:{shape}:{index}", declarations[shape]))
    rng.shuffle(atoms)
    return "".join(body for _, body in atoms), tuple(label for label, _ in atoms)


def make_variants(count: int, families: tuple[str, ...], seed: int,
                  max_declarations: int = 64) -> tuple[StateVariant, ...]:
    if count < 1 or max_declarations < 10:
        raise ValueError("trials must be positive and max declarations at least 10")
    unknown = sorted(set(families) - set(FAMILIES))
    if not families or unknown:
        raise ValueError(f"unknown or empty state families: {', '.join(unknown)}")
    rng = random.Random(seed)
    variants = []
    for trial in range(1, count + 1):
        family = families[(trial - 1) % len(families)]
        tag = f"{seed:08x}-{trial:04d}-{rng.getrandbits(32):08x}"
        ident = f"HOMM3_TU_PROBE_{tag.replace('-', '_').upper()}"
        width = 10 + (trial - 1) % (max_declarations - 9)
        forest, permutation = declaration_forest(rng, ident, width)
        repeat = 1 + rng.randrange(4)
        aliases = "".join(f"typedef {rng.choice(SCALARS)} {ident}_ALIAS_{i};\n"
                          for i in range(repeat))
        typedef_count = "".join(
            f"typedef int HOMM3_TU_COUNT_TYPEDEF_{i:04d};\n"
            for i in range(1, trial + 1))
        values = [rng.choice(VALUES) for _ in range(1 + rng.randrange(8))]
        rng.shuffle(values)
        enum_decl = (f"enum {ident}_ENUM {{\n" + ",\n".join(
            f"    {ident}_VALUE_{i} = {value}" for i, value in enumerate(values)) + "\n};\n")
        members = "".join(
            f"    {rng.choice(SCALARS)} m_probe_{i}"
            f"{'[' + str(1 + rng.randrange(4)) + ']' if rng.randrange(3) == 0 else ''};\n"
            for i in range(1 + rng.randrange(6)))
        struct_decl = f"struct {ident}_STRUCT {{\n{members}}};\n"
        class_decl = f"class {ident}_CLASS {{\npublic:\n{members}}};\n"
        packed = (f"#pragma pack(push, {rng.choice((1, 2, 4, 8))})\n"
                  f"struct {ident}_PACKED {{ char m_tag; int m_value; }};\n#pragma pack(pop)\n")
        methods = "".join(f"    int method{i}(int);\n" for i in range(1 + rng.randrange(3)))
        member = (f"class {ident}_MEMBERS {{\npublic:\n{methods}"
                  f"    int identity(int x) {{ return x; }}\n}};\n")
        extern = "".join(f"extern {rng.choice(SCALARS)} {ident}_EXTERN_{i};\n"
                         for i in range(repeat))
        static_data = "".join(
            f"static {rng.choice(SCALARS)} {ident}_STATIC_{i} = {rng.choice(VALUES)};\n"
            for i in range(repeat))
        prototype = "".join(f"int __fastcall {ident}_PROTOTYPE_{i}(int, int);\n"
                            for i in range(repeat))
        function = (f"static int {ident}_FUNCTION_A(int x) {{ return x ^ 7; }}\n"
                    f"static unsigned long {ident}_FUNCTION_B(unsigned long a, unsigned long b) "
                    f"{{ return a + b; }}\n")
        parts = (forest, aliases, enum_decl, struct_decl, class_decl, packed,
                 member, extern, static_data, prototype, function)
        bodies = dict(zip(("forest", "typedef", "enum", "struct", "class",
                           "packed", "member", "extern", "static-data",
                           "prototype", "function"), parts))
        bodies["typedef-count"] = typedef_count
        bodies["mixed"] = "".join(parts)
        variants.append(StateVariant(trial, family, tag, bodies[family],
                                     permutation if family in ("forest", "mixed") else ()))
    return tuple(variants)
