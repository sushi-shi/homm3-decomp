// FormattedString.h - a CString built from a printf format, as the
// dialogs label their per-player and per-slot controls. The variadic
// constructor is a __cdecl member taking its object on the stack; it never
// inlines, so its body is a COMDAT emitted with its first user (h3maped
// 0x402ee7, in the ArmyDlg.cpp span). The class name is not recorded.
#ifndef HOMM3_EDITOR_FORMATTEDSTRING_H
#define HOMM3_EDITOR_FORMATTEDSTRING_H

#include <stdarg.h>

#include "va.h"

class TFormattedString : public CString {
public:
    VA(0x00402ee7, 0x36)
    TFormattedString(LPCTSTR lpszFormat, ...)
    {
        va_list argList;
        va_start(argList, lpszFormat);
        FormatV(lpszFormat, argList);
        va_end(argList);
    }
};

#endif  /* HOMM3_EDITOR_FORMATTEDSTRING_H */
