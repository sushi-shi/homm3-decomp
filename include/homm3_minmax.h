#ifndef HOMM3_MINMAX_H
#define HOMM3_MINMAX_H

#include <algorithm>

// E:\gamedcs\includes.h:97,114. The by-value wrappers call the
// reference-returning selectors; their argument copies are observable in
// retail's inlined operand homes (kb's icon clamp and campaign hero limits).
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

inline int min(int left, int right)
{
    return std::_cpp_min(left, right);
}

inline int max(int left, int right)
{
    return std::_cpp_max(left, right);
}

#endif
