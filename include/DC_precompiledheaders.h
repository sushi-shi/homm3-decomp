#ifndef HOMM3_DC_PRECOMPILEDHEADERS_H
#define HOMM3_DC_PRECOMPILEDHEADERS_H

#include <windows.h>

#include "inline/cpp_max.inl"

// The selectors return operand references; includes.h provides the by-value
// integer wrappers used by callers that need copies.
// E:\gamedcs\DC_precompiledheaders.h:41, dc 0x3b88
template<class T>
inline const T& cppMin(const T& left, const T& right)
{
    return right < left ? right : left;
}

#endif  /* HOMM3_DC_PRECOMPILEDHEADERS_H */
