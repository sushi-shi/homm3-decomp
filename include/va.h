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
 *                                  VECTOR_* / BITSET_* / TREE_* / STD_* /
 *                                  IMPLICIT_COPY_CTOR /
 *                                  IMPLICIT_COPY_ASSIGN / IMPLICIT_DTOR;
 *                                  owner names the
 *                                  global, class, vector element, or
 *                                  specialization token that causes it;
 *                                  direct-symbol kinds only claim a named
 *                                  COFF symbol VC6 already emitted
 *   DATA(addr)                     global datum definition (never on a
 *                                  header extern)
 *   DATA_COMPGEN(addr, name, value)
 *                                  anonymous compiler-generated allocation
 *                                  (string literal / float pool entry);
 *                                  name is a stable semantic identifier,
 *                                  never a compiler counter
 *   DATA_COMPGEN_GUARD(addr, name, owner)
 *                                  compiler-emitted static-init guard word
 *   DC_ONLY(off, cb)               evidenced only in the Dreamcast build
 *                                  (CodeView proc at .text offset/cb);
 *                                  makes NO claim about the retail image
 *   HOMM3_RELEASE_VERIFY(expr)      release-form invariant carrier; the
 *                                  expression is evaluated, like VERIFY, and
 *                                  must be supported by source-shape evidence
 *   INLINE_GATE(statement)          mark one source-proven call boundary;
 *                                  VC6 SP3 lacks __pragma, so matching source
 *                                  pairs this marker with adjacent
 *                                  inline_depth pragmas. Never a substitute
 *                                  for fabricated source mass or behavior
 *   OVERRIDE                       `override` under clang, nothing under VC6
 *   SIZE(type, bytes)              struct-size assertion (clang arm only)
 */
#ifndef HOMM3_VA_H
#define HOMM3_VA_H

#ifdef __clang__

#define VA(addr, size) __attribute__((annotate("va:" #addr " size:" #size)))
#define VA_COMPGEN(addr, size, kind, owner)
#define DATA(addr) __attribute__((annotate("data:" #addr)))
#define DATA_COMPGEN(addr, name, value) value
#define DATA_COMPGEN_GUARD(addr, name, owner)
#define DC_ONLY(off, cb)
#define HOMM3_RELEASE_VERIFY(expression) static_cast<void>(expression)
#define INLINE_GATE(statement) statement
#define OVERRIDE override
#define SIZE(type, bytes) \
    static_assert(sizeof(type) == (bytes), "sizeof(" #type ") != " #bytes)

#else

#define VA(addr, size)
#define VA_COMPGEN(addr, size, kind, owner)
#define DATA(addr)
#define DATA_COMPGEN(addr, name, value) value
#define DATA_COMPGEN_GUARD(addr, name, owner)
#define DC_ONLY(off, cb)
#define HOMM3_RELEASE_VERIFY(expression) static_cast<void>(expression)
#define INLINE_GATE(statement) statement
#define OVERRIDE

#define SIZE(type, bytes)

#endif

#endif /* HOMM3_VA_H */
