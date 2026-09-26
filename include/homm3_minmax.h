// Public min/max compatibility for the pinned VC6 library.
#ifndef HOMM3_MINMAX_H
#define HOMM3_MINMAX_H

#include "includes.h"
#include <algorithm>

// VC6's XUTILITY exposes these standard reference selectors only as
// _cpp_min/_cpp_max. Keep the public std:: names at game call sites;
// the by-value game helpers in includes.h remain separate overloads.
#if defined(_MSC_VER) && _MSC_VER <= 1200
namespace std {
template<class T>
inline const T& min(const T& left, const T& right)
{
    return right < left ? right : left;
}

template<class T>
inline const T& max(const T& left, const T& right)
{
    return left < right ? right : left;
}
}
#endif

#endif
