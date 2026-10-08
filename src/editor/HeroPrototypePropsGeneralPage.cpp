// HeroPrototypePropsGeneralPage.cpp - the general page of the map
// specifications' hero customization sheet (h3maped 0x455aee..0x456b9c;
// GOG only). Restored so far: the dialogs' blank-text test, the object's
// first function.
#include "editor/stdafx.h"

#include <ctype.h>

#include "va.h"
#include "editor/StringUtil.h"

VA(0x00455d02, 0x2a)
bool _isspace(const CString& text)
{
    unsigned int i = text.GetLength();
    while (i > 0) {
        if (!isspace(text.GetAt(--i)))
            return false;
    }
    return true;
}
