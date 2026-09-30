// 3 functions in link order.
#include "va.h"

#include "u2dvers.h"

VA(0x005eeda0, 0x4C)
DC_ADDRESS(0x18e3b0, 0x4)
TFileVersionInfo::TFileVersionInfo(const char* filename)
{
    unsigned long ignoredHandle;
    unsigned long size = GetFileVersionInfoSizeA(
        const_cast<char*>(filename), &ignoredHandle);
    if (size > 0) {
        m_data = new char[size];
        if (m_data)
            GetFileVersionInfoA(const_cast<char*>(filename), 0, size, m_data);
    } else {
        m_data = 0;
    }
}

VA(0x005eedf0, 0xE)
DC_ADDRESS(0x18e3b4, 0x4)
TFileVersionInfo::~TFileVersionInfo()
{
    if (m_data)
        delete[] m_data;
}

VA(0x005eee00, 0x265)
DC_ADDRESS(0x18e3b8, 0x4)
bool TFileVersionInfo::getVersionInfo(const char* name, std::string* buffer) const
{
    // The PC implementation is absent from DC's stub. Retail normalizes
    // both WinAPI results before preserving this byte through cleanup.
    // Both language/code-page prefixes are .rdata arrays (0x643b24,
    // 0x643b40), not pooled .data literals.
    DATA(0x00643b24) static const char usEnglishUnicodeBlock[] =
        "\\StringFileInfo\\040904B0\\";
    DATA(0x00643b40) static const char usEnglishAnsiBlock[] =
        "\\StringFileInfo\\040904e4\\";
    bool found = false;
    if (m_data) {
        std::string subBlock;
        subBlock = usEnglishUnicodeBlock;
        subBlock += name;

        // VerQueryValue hands back a pointer into the version block, so
        // the SDK types it `LPVOID *`; the queried value is the character
        // buffer the caller copies out.
        void* value;
        unsigned int length;
        found = VerQueryValueA(m_data, const_cast<char*>(subBlock.c_str()),
                &value, &length) != 0;
        if (!found) {
            subBlock = usEnglishAnsiBlock;
            subBlock += name;
            found = VerQueryValueA(m_data, const_cast<char*>(subBlock.c_str()),
                    &value, &length) != 0;
        }

        if (found)
            buffer->assign(static_cast<const char*>(value), length);
    }
    return found;
}
