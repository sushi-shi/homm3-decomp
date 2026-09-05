// exceptions.h - the game's own std::runtime_error family.
//
// NAMES ARE RETAIL RTTI, NOT GUESSES. Every `__CxxThrowException` record
// that carries one of these spells the whole chain out: 0x6486c0 reads
// `.?AVTAllocationFailure@@` over `.?AVTRuntimeError@@` over
// `.?AVTDebugBreak@@` over `.?AVruntime_error@std@@` over
// `.?AVexception@@`, and 0x6487a8 is the bare `.?AVruntime_error@std@@`
// that std::runtime_error::_Doraise (0x41bc10) rethrows through. That is
// also what identifies two long-unowned bodies: 0x41ba90 (354 B) is
// `std::runtime_error::runtime_error(const string&)`'s COMDAT, and
// 0x49a0c0 (249 B) - carried in dxplay.cpp's next-lane note as
// "substantial, unidentified" - is TRuntimeError's `const char*`
// constructor, which strlens its argument into the base's string.
//
// DECLARED, NOT DEFINED: the two constructors live in compilands this
// header does not own, and the retail call at every throw site is exactly
// what a definition-less declaration emits. Nothing here needs a body.
#ifndef HOMM3_EXCEPTIONS_H
#define HOMM3_EXCEPTIONS_H

#include <stdexcept>

// The three-slot vftables retail keeps for this family (0x63aba8 and its
// neighbour at 0x63abb4) are byte-identical copies of Dinkumware's
// {deleting dtor, what, _Doraise}, so no level below adds a virtual.
//
// LAYOUT CORRECTED 2026-09-05 from the throw records' own CatchableType
// arrays, which publish sizeof and the this-displacement of every base.
// 0x6486d0 (TRuntimeError) lists, in order:
//     TRuntimeError        size 32  mdisp 0
//     TDebugBreak          size  1  mdisp 29
//     runtime_error        size 28  mdisp 0
//     exception            size 12  mdisp 0
// and 0x6486c0 (TAllocationFailure) prepends a size-32 mdisp-0 row for
// itself. So TDebugBreak is an EMPTY class carried as a SEPARATE BASE at
// +0x1d, not a link in a single chain under runtime_error: a chain would
// give it size 28 and displacement 0. The 29 is what an empty base gets
// when MSVC defers it past the 28-byte runtime_error subobject, and the
// two copy constructors in the advmgr..advspells gap corroborate it
// directly - 0x41b7b0 and 0x41b920 both open by copying the byte at
// `[src+0x1d]` into `[dst+0x1d]` before forwarding to
// `exception::exception(const exception&)` and the string at +0xc.
class TDebugBreak {
public:
    // 0x524360, two bytes: `mov eax,ecx / ret`. The image-name table's
    // loader constructs the base with no argument at all before running
    // the runtime_error base, so the family carries a default constructor
    // beside the message-carrying one.
    TDebugBreak();
    TDebugBreak(const char* text);
};

class TRuntimeError : public TDebugBreak, public std::runtime_error {
public:
    // INLINE, and the throw at 0x514dba is what proves it: retail expands
    // the whole constructor at that site - TDebugBreak's out-of-line
    // 0x524360 for the empty base at +0x1d, a DEFAULT-constructed
    // std::string handed to runtime_error's out-of-line 0x41ba90, then the
    // 0x63abb4 vftable store - where the message-carrying form below is a
    // single call. Base order is declaration order, and retail runs
    // TDebugBreak first.
    TRuntimeError() : std::runtime_error(std::string()) {}
    TRuntimeError(const char* text);  // 0x49a0c0
};

class TAllocationFailure : public TRuntimeError {
public:
    // INLINE, and the two throw sites prove it from opposite sides:
    // objnames.obj's InitializeAdventureObjectNames (0x41b500) EXPANDS the
    // whole body at its throw - the literal, the out-of-line
    // TRuntimeError(const char*) at 0x49a0c0, then the 0x63aba8 vftable
    // store - while gzinflatebuf.obj keeps the 0x4d6b80 COMDAT and calls
    // it at all three of its throws. Same source, two /Ob2 verdicts; a
    // definition confined to gzinflatebuf.cpp can only produce the second.
    TAllocationFailure() : TRuntimeError("Allocation failure.") {}
};

#endif  /* HOMM3_EXCEPTIONS_H */
