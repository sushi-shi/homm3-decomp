#ifndef HOMM3_LIMIT_H
#define HOMM3_LIMIT_H

// E:\gamedcs\includes.h:124-134, DC 0x20d2c/0x1ef5c.
// The integer instance is published as ?t_limit@@YAABHABH00@Z.
// Before normalization: t_limit; parameters min, value, max.
// Retail 0x4e6750 has the same three-reference ABI, lower-bound-first
// comparisons and selected-reference return. The by-value limit wrapper
// owns the three copies whose addresses its callers pass to this selector.
// Source-order control: value > maximum preserves the branch semantics
// but changes retail's cmp [maximum],value / jl to cmp value,[maximum] / jg.
// Keep the lower-bound-first if/else scopes shown by DC lines 125-130.
template<class T>
inline const T& tLimit(const T& minimum, const T& value,
                       const T& maximum)
{
    if (value < minimum) {
        return minimum;
    } else if (maximum < value) {
        return maximum;
    } else {
        return value;
    }
}

inline int limit(int minimum, int value, int maximum)
{
    return tLimit(minimum, value, maximum);
}

#endif
