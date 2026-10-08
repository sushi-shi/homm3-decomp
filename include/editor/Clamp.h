// Clamp.h - clamp(lo, value, hi), the editor's range clamp (Loki
// h3maped). Loki keeps its int and unsigned instantiations as linkonce
// bodies; the Windows dialogs expand it inline, through references to the
// bounds' temporaries (TOptionsDlg's autosave period). The header name is
// not proven.
#ifndef HOMM3_EDITOR_CLAMP_H
#define HOMM3_EDITOR_CLAMP_H

template <class T>
inline const T& clamp(const T& lo, const T& value, const T& hi)
{
    return value < lo ? lo : (value > hi ? hi : value);
}

#endif  /* HOMM3_EDITOR_CLAMP_H */
