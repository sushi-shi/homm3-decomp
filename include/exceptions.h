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
class TDebugBreak : public std::runtime_error {
public:
    TDebugBreak(const char* text);
};

class TRuntimeError : public TDebugBreak {
public:
    TRuntimeError(const char* text);  // 0x49a0c0
};

class TAllocationFailure : public TRuntimeError {
public:
    TAllocationFailure();  // 0x4d6b80, in gzinflatebuf.obj
};

#endif  /* HOMM3_EXCEPTIONS_H */
