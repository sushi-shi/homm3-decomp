// FormattedStdString.h - a std::string built from a printf format, as the
// random dwellings name their levels and towns (Generator.cpp). The
// variadic constructor is a __cdecl member taking its object on the
// stack; it never inlines, so its body is a COMDAT emitted with its first
// user (h3maped 0x43e68e). It formats through _formatV (0x41e597), which
// prints into a 4 KB static buffer and assigns the text; that body opens
// its own object between FlaggablePropsDlg.cpp and GameMap.cpp. The class
// name is not recorded.
#ifndef HOMM3_EDITOR_FORMATTEDSTDSTRING_H
#define HOMM3_EDITOR_FORMATTEDSTDSTRING_H

#include <stdarg.h>
#include <string>

#include "va.h"

class TFormattedStdString : public std::string {
public:
    VA(0x0043e68e, 0x3d)
    TFormattedStdString(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        _formatV(format, args);
        va_end(args);
    }

private:
    void _formatV(const char* format, va_list args);
};

#endif  /* HOMM3_EDITOR_FORMATTEDSTDSTRING_H */
