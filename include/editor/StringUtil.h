// StringUtil.h - the dialogs' blank-text test. Loki's port declares
// _isspace(string) in its GTK bridge; the Windows dialogs pass their DDX
// CStrings by reference (h3maped 0x455d02, the first function of the
// HeroPrototypePropsGeneralPage.cpp span). The header's name is not
// recorded.
#ifndef HOMM3_EDITOR_STRINGUTIL_H
#define HOMM3_EDITOR_STRINGUTIL_H

// True when text is empty or holds only white space.
bool _isspace(const CString& text);

// A string's buffer, released when it goes out of scope (MapDoc.cpp's
// file name prompt hands it to the common file dialog).
class TStringBuffer {
public:
    TStringBuffer(CString& string, int minLength) : _m_string(string), _m_pBuffer(string.GetBuffer(minLength)) {}
    ~TStringBuffer() { _m_string.ReleaseBuffer(); }

    operator LPTSTR() const { return _m_pBuffer; }

private:
    CString& _m_string;
    LPTSTR _m_pBuffer;
};

#endif  /* HOMM3_EDITOR_STRINGUTIL_H */
