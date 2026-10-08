// exceptions.h - the runtime-error family shared by throw sites.
// Loki h3maped (GCC 2.95.2, object 10 `Error.cpp`) defines TDebugBreak's
// constructor and the four TRuntimeError constructors out of line; the
// allocation failure, the destructors and copy constructors are inline
// (linkonce, first emitted by object 2). RTTI (__tf13TRuntimeError,
// __tf18TAllocationFailure) and the -4 destructor thunks prove the base
// order: the empty TDebugBreak at +0, runtime_error at +4. The header's
// original file name is not proven; Error.h is the likely one.
#ifndef HOMM3_EXCEPTIONS_H
#define HOMM3_EXCEPTIONS_H

#include <stdexcept>
#include <string>

class TDebugBreak {
public:
    TDebugBreak();
};

class TRuntimeError : public TDebugBreak, public std::runtime_error {
public:
    TRuntimeError(const char* text);
    TRuntimeError(const std::string& text);
    // The file/line forms prefix the message with formatDebugMessage's
    // "File:"/"Line:" block; throw sites pass __FILE__ and __LINE__.
    TRuntimeError(const char* file, unsigned line, const char* text);
    TRuntimeError(const char* file, unsigned line, const std::string& text);
};

class TAllocationFailure : public TRuntimeError {
public:
    TAllocationFailure(const char* file, unsigned line)
        : TRuntimeError(file, line, _s_kMessage) {}

    static const char _s_kMessage[];  // "Allocation failure." (Error.cpp)
};

#endif  /* HOMM3_EXCEPTIONS_H */
