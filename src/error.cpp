// error.cpp - the runtime-error constructors, original file Error.cpp.
// The map editor links it as its own object between EraseToolkit.cpp and
// Event.cpp (h3maped 0x418be8..0x418c73: its .CRT$XCU initializer, then
// TDebugBreak() and TRuntimeError(const char*)), and Loki's port names it
// (object 10, `_GLOBAL_.N.Error.cpp`). The game links the same object after
// dxplay: TRuntimeError(const char*) at 0x49a0c0, then its initializer
// 0x49a1c0, .CRT$XCU slot 334, before the next object's slot 335 at 0x49a1e0.
// Objnames' 0x41b500 expands the derived allocation-error initialization
// around a call here; gzinflatebuf retains and calls 0x4d6b80.
#include "va.h"

#include "exceptions.h"

DATA(0x0063de60) const char TAllocationFailure::_s_kMessage[] =
    "Allocation failure.";

// The game's retained empty-base calls at 0x41b62a and 0x514dbd reach a body
// folded with philAI::philAI at 0x524360; the editor keeps its own copy.
MAC_ADDRESS(0x2207b8, 0x4)
TDebugBreak::TDebugBreak()
{
}

// The body is the base list. RTTI proves the empty TDebugBreak base;
// its default constructor is declared in exceptions.h.
VA(0x0049a0c0, 0xF9)
MAC_ADDRESS(0x2207bc, 0x94)
TRuntimeError::TRuntimeError(const char* text)
    : std::runtime_error(text)
{
}
