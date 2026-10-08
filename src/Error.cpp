// Error.cpp - Loki h3maped object 10 (named by its anonymous namespace,
// `_GLOBAL_.N.Error.cpp`): the out-of-line exception constructors.
#include "exceptions.h"

#include <strstream.h>

namespace {

inline std::string formatDebugMessage(const char* file, unsigned line, const char* text)
{
    ostrstream stream;
    stream << text << "\n\nFile: \"" << file << "\"\nLine: " << line;
    return std::string(stream.str());
}

}  // namespace

const char TAllocationFailure::_s_kMessage[] = "Allocation failure.";

TDebugBreak::TDebugBreak()
{
}

TRuntimeError::TRuntimeError(const char* text)
    : std::runtime_error(text)
{
}

TRuntimeError::TRuntimeError(const std::string& text)
    : std::runtime_error(text)
{
}

TRuntimeError::TRuntimeError(const char* file, unsigned line, const char* text)
    : std::runtime_error(formatDebugMessage(file, line, text))
{
}

TRuntimeError::TRuntimeError(const char* file, unsigned line, const std::string& text)
    : std::runtime_error(formatDebugMessage(file, line, text.c_str()))
{
}
