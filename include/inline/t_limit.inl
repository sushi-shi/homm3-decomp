// E:\gamedcs\includes.h:124, dc 0x20d2c. CodeView types all three
// parameters and the return as const references. The retained retail body and
// ordinary limit expansions use maximum < value for the upper clamp. Retail
// expands this helper through the by-value limit wrapper in the adventure and
// small-window TUs.
template <class T>
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
