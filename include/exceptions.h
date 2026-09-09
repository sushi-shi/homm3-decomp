// exceptions.h - the Complete runtime-error family shared by throw sites.
// Retail ThrowInfo 0x6486c0/0x6486d0 names TAllocationFailure,
// TRuntimeError and TDebugBreak through descriptors 0x660430/0x660498/
// 0x660478. These types have no procedure or class record in pinned DC
// CodeView. Their shared-header placement is a Windows reconstruction;
// RTTI proves type identity and layout, not an original filename.
#ifndef HOMM3_EXCEPTIONS_H
#define HOMM3_EXCEPTIONS_H

#include <va.h>
#include <stdexcept>

// The catchable-type entries describe multiple inheritance, not a linear
// chain: TRuntimeError is 32 bytes, its std::runtime_error base is 28 bytes
// at +0, and the empty one-byte TDebugBreak base is at +0x1d. Allocation
// failure adds no fields. Copy constructors 0x41b7b0/0x41b920 corroborate
// the +0x1d byte before copying the exception and string subobjects.
// Vtables 0x63aba8/0x63abb4 retain the library's three virtual slots.
class TDebugBreak {
public:
    // The default error construction at 0x514dbd calls the three-byte
    // empty-constructor representative 0x524360 on the base at +0x1d.
    // That folded address also represents philAI's proven constructor;
    // it does not independently identify this base's original source file.
    // TRuntimeError(const char*) at 0x49a0c0 elides this same empty base
    // initialization. No evidence supports a separate message overload.
    // Shared-header visibility is reconstructed from the elided and retained
    // calls; the folded body is already represented by philAI's VA claim.
    TDebugBreak() {}
};

class TRuntimeError : public TDebugBreak, public std::runtime_error {
public:
    // The object-table failure at 0x514dba constructs the empty base,
    // default-constructs a string at 0x514dcc, passes it to the retained
    // runtime_error constructor at 0x514dde, then installs 0x63abb4 and
    // throws with the TRuntimeError descriptor. Keep this initialization
    // sequence shared with objnames' corresponding default-error path.
    // Expansion proves visibility there, not an original inline keyword.
    TRuntimeError() : std::runtime_error(std::string()) {}
    TRuntimeError(const char* text);  // retained at 0x49a0c0
};

class TAllocationFailure : public TRuntimeError {
public:
    // Retail 0x4d6b80 forwards the allocation-failure literal to 0x49a0c0
    // at 0x4d6b88, then installs 0x63aba8. The objnames throw at 0x41b6ed
    // expands this same derived initialization, while gzinflatebuf keeps
    // the retained body. The canonical header body serves both uses.
    VA(0x004d6b80, 0x17)  // anchor-callee 0x49a0c0 + anchor-vtable 0x63aba8, retail-only
    TAllocationFailure() : TRuntimeError("Allocation failure.") {}
};

#endif  /* HOMM3_EXCEPTIONS_H */
