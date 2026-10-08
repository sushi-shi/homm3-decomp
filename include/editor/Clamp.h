// Clamp.h - clamp(lo, value, hi), the editor's range clamp. Its int and
// unsigned instantiations are kept linkonce bodies (first emitted by
// Monster.cpp and MapEditWnd.cpp); TEditTimedEventGeneralPage::OnOK clamps
// the first day with it. The header name is not proven.
#ifndef HOMM3_EDITOR_CLAMP_H
#define HOMM3_EDITOR_CLAMP_H

template <class T>
inline const T& clamp(const T& lo, const T& value, const T& hi)
{
    return value < lo ? lo : (value > hi ? hi : value);
}

#endif  /* HOMM3_EDITOR_CLAMP_H */
