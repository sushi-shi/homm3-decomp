// StringUtil.h - the dialogs' blank-text test. Loki's port declares
// _isspace(string) in its GTK bridge; the Windows dialogs pass their DDX
// CStrings by reference (h3maped 0x455d02, the first function of the
// HeroPrototypePropsGeneralPage.cpp span). The header's name is not
// recorded.
#ifndef HOMM3_EDITOR_STRINGUTIL_H
#define HOMM3_EDITOR_STRINGUTIL_H

// True when text is empty or holds only white space.
bool _isspace(const CString& text);

#endif  /* HOMM3_EDITOR_STRINGUTIL_H */
