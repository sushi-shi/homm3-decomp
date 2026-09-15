// u2dvers.cpp - E:\gamedcs\u2dvers.cpp (compiland u2dvers.obj)
// 3 functions in link order.
#include <va.h>
#include "u2dvers.h"

VA(0x005eeda0, 0x4C)  // dc 0x18e3b0
FileVersionInfo::FileVersionInfo(const char* filename)
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

VA(0x005eedf0, 0xE)  // dc 0x18e3b4
FileVersionInfo::~FileVersionInfo()
{
    if (m_data)
        delete[] m_data;
}

VA(0x005eee00, 0x265)  // dc 0x18e3b8
unsigned char FileVersionInfo::getVersionInfo(const char* name, std::string* buffer) const
{
    unsigned char found = 0;
    if (m_data) {
        std::string subBlock;
        subBlock = DATA_COMPGEN(0x00643b24, versionInfo040904B0,
            "\\StringFileInfo\\040904B0\\");
        subBlock += name;

        // VerQueryValue hands back a pointer into the version block, so
        // the SDK types it `LPVOID *`; the queried value is the character
        // buffer the caller copies out.
        void* value;
        unsigned int length;
        found = static_cast<unsigned char>(
            VerQueryValueA(m_data, const_cast<char*>(subBlock.c_str()),
                &value, &length) != 0);
        if (!found) {
            subBlock = DATA_COMPGEN(0x00643b40, versionInfo040904e4,
                "\\StringFileInfo\\040904e4\\");
            subBlock += name;
            found = static_cast<unsigned char>(
                VerQueryValueA(m_data, const_cast<char*>(subBlock.c_str()),
                    &value, &length) != 0);
        }

        if (found)
            buffer->assign(static_cast<const char*>(value), length);
    }
    return found;
}
