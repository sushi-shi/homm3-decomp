/* va.h - the annotation contract, v2 (port plan P0.2; homm2-decomp
 * vocabulary adopted per the delink lessons, decision log 2026-08-04).
 *
 * Two arms. Under clang (analysis/clangd only) VA and DATA become real
 * annotate attributes so libclang can read them off the AST; under VC6
 * (the matching compiler) every macro expands to nothing - EXCEPT
 * DATA_COMPGEN, which expands to its value argument in BOTH arms so it
 * can wrap an expression in place. Nothing here may perturb codegen.
 *
 * Addresses are ABSOLUTE VAs (image base 0x400000) in source; every
 * generated artifact uses rvas (the scanners subtract the base and fail
 * on addresses below it).
 *
 *   VA(addr, size)                 function definition matched to the
 *                                  pinned retail image at addr/size
 *   VA_COMPGEN(addr, size, kind, owner)
 *                                  compiler-generated function with no
 *                                  source definition to sit on; kind is
 *                                  STATIC_INIT_DISPATCH / STATIC_ATEXIT /
 *                                  STATIC_DTOR / STATIC_CTOR /
 *                                  DEFAULT_CTOR_CLOSURE /
 *                                  VECTOR_DELETING_DTOR / VECTOR_DTOR /
 *                                  VECTOR_* / BITSET_* / TREE_* / MAP_* / STD_* /
 *                                  BASIC_STRING_* / OSTREAM_* /
 *                                  INSERTION_SORT_1 /
 *                                  BITSET_AND_ASSIGN / BITSET_OR /
 *                                  IMPLICIT_COPY_CTOR /
 *                                  IMPLICIT_COPY_ASSIGN / IMPLICIT_DTOR;
 *                                  owner names the
 *                                  global, class, vector element, or
 *                                  specialization token that causes it;
 *                                  direct-symbol kinds only claim a named
 *                                  COFF symbol VC6 already emitted
 *   DATA(addr)                     owning global datum definition, or the
 *                                  canonical extern when its defining TU
 *                                  is not authored; externs identify storage
 *                                  and do not claim an initializer match
 *   DATA_COMPGEN(addr, name, value)
 *                                  anonymous compiler-generated allocation
 *                                  (string literal / float pool entry);
 *                                  name is a stable semantic identifier,
 *                                  never a compiler counter
 *   DATA_COMPGEN_GUARD(addr, name, owner)
 *                                  compiler-emitted static-init guard word
 *   HOMM3_RELEASE_VERIFY(expr)      release-form invariant carrier; the
 *                                  expression is evaluated, like VERIFY, and
 *                                  must be supported by source-shape evidence
 *   MEMSET(dest, value, bytes, i)   counted fill retained for retail codegen;
 *                                  searchable candidate for a future memset
 *   MEMCPY(dest, src, bytes, i)     counted copy retained for retail codegen;
 *                                  searchable candidate for a future memcpy
 *   MEMSET_LOCAL(dest, value, bytes, count, i)
 *                                  counted fill whose index declaration and
 *                                  source bound must remain inside the loop
 *   OVERRIDE                       `override` under clang, nothing under VC6
 *   SIZE(type, bytes)              struct-size assertion (clang arm only)
 */
#ifndef HOMM3_VA_H
#define HOMM3_VA_H

// The source inventory selects VC6 project branches but still needs these
// annotations. HOMM3_SOURCE_OWNERSHIP is set only by that analysis tool.
#if defined(__clang__) || defined(HOMM3_SOURCE_OWNERSHIP)

#define VA(addr, size) __attribute__((annotate("va:" #addr " size:" #size)))
#define VA_COMPGEN(addr, size, kind, owner)
#define DATA(addr) __attribute__((annotate("data:" #addr)))
#define DATA_COMPGEN(addr, name, value) value
#define DATA_COMPGEN_GUARD(addr, name, owner)
#define HOMM3_RELEASE_VERIFY(expression) static_cast<void>(expression)
#define OVERRIDE override
#define SIZE(type, bytes) \
    static_assert(sizeof(type) == (bytes), "sizeof(" #type ") != " #bytes)

#else

#define VA(addr, size)
#define VA_COMPGEN(addr, size, kind, owner)
#define DATA(addr)
#define DATA_COMPGEN(addr, name, value) value
#define DATA_COMPGEN_GUARD(addr, name, owner)
#define HOMM3_RELEASE_VERIFY(expression) static_cast<void>(expression)
#define OVERRIDE

#define SIZE(type, bytes)

#endif

// These intentionally expand to the authored counted loops.  The caller owns
// the index variable so its type, scope and register lifetime remain visible
// to the matching compiler.  The byte-count argument mirrors the CRT calls
// that can replace these markers after exact matching is no longer required.
#define MEMSET(destination, value, byteCount, index)                         \
    for (index = 0;                                                         \
         index < static_cast<int>((byteCount) / sizeof((destination)[0]));  \
         ++index)                                                           \
        (destination)[index] = (value)

#define MEMCPY(destination, source, byteCount, index)                        \
    for (index = 0;                                                         \
         index < static_cast<int>((byteCount) / sizeof((destination)[0]));  \
         ++index)                                                           \
        (destination)[index] = (source)[index]

// Some inlined loops are sensitive to the local declaration and to the exact
// source spelling of a dynamic bound.  byteCount remains the eventual memset
// operand; elementCount reproduces the authored loop until that migration.
#define MEMSET_LOCAL(destination, value, byteCount, elementCount, index)     \
    for (int index = 0; index < elementCount; ++index) {                    \
        (destination)[index] = (value);                                     \
    }

#endif /* HOMM3_VA_H */
