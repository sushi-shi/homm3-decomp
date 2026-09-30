#ifndef HOMM3_DC_PRECOMPILEDHEADERS_H
#define HOMM3_DC_PRECOMPILEDHEADERS_H

#include "va.h"

#include "platform.h"

// E:\gamedcs\DC_precompiledheaders.h:33
template<class T>
DC_ADDRESS(0x020d04, 0x28)
inline const T& cppMax(const T& left, const T& right)
{
    return left < right ? right : left;
}

// The selectors return operand references; includes.h provides the by-value
// integer wrappers used by callers that need copies.
// E:\gamedcs\DC_precompiledheaders.h:41
template<class T>
DC_ADDRESS(0x003b88, 0xe)
DC_ADDRESS(0x04d044, 0x38)
inline const T& cppMin(const T& left, const T& right)
{
    return right < left ? right : left;
}

#endif  /* HOMM3_DC_PRECOMPILEDHEADERS_H */
