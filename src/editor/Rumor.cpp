// Rumor.cpp - a map rumor (h3maped 0x4b4631..0x4b4bae, name inferred; Loki
// keeps TRumor in GameMap.cpp): its name and text, the map text's entry and
// the map file's record. A text keeps at most 300 characters. The release
// drops Loki's asserts.
#include "editor/stdafx.h"

#include <ctype.h>
#include <algorithm>
#include <functional>

#include "va.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"

namespace {

inline bool isAllSpace(const string& text);

}  // namespace

VA(0x004b4631, 0x2c)
void TRumor::setNameAndText(const string& newName, const string& newText)
{
    _m_name = newName;
    _m_text = newText;
}

VA(0x004b465d, 0x2ff)
void TRumor::importText(istream* pIStream, EGameVersion version)
{
    string line;
    getline(*pIStream, line);
    if (line != string(kNameStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    replace(line.begin(), line.end(), '\t', ' ');
    string name = line;
    getline(*pIStream, line);
    if (line != string(kTextStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    replace(line.begin(), line.end(), '\t', '\n');
    if (line.size() > s_kMaxTextLen)
        line.erase(s_kMaxTextLen);
    string text = line;
    if (isAllSpace(name) || isAllSpace(text))
        throw TImportTextFailure();
    setNameAndText(name, text);
}

namespace {

VA(0x004b495c, 0x49)  // after its first user
inline bool isAllSpace(const string& text)
{
    return find_if(text.begin(), text.end(), not1(ptr_fun(isspace))) == text.end();
}

}  // namespace

VA_COMPGEN(0x004b49a5, 0x18, IMPLICIT_COPY_CTOR, TImportTextFailure)

VA(0x004b49bd, 0x113)
void TRumor::exportText(ostream* pOStream, EGameVersion version) const
{
    *pOStream << kNameStr << ':' << '\n' << _m_name << '\n';
    string text = _m_text;
    replace(text.begin(), text.end(), '\n', '\t');
    *pOStream << kTextStr << ':' << '\n' << text << '\n';
}

VA(0x004b4ad0, 0xbd)
TRawIStream& operator>>(TRawIStream& stream, TRumor& rumor)
{
    string name;
    string text;
    stream >> name >> text;
    if (text.size() > TRumor::s_kMaxTextLen)
        text.erase(TRumor::s_kMaxTextLen);
    rumor.setNameAndText(name, text);
    return stream;
}

VA(0x004b4b8d, 0x21)
TRawOStream& operator<<(TRawOStream& stream, const TRumor& rumor)
{
    stream << rumor.getName() << rumor.getText();
    return stream;
}
