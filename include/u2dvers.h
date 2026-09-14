// u2dvers.h - prototypes of u2dvers.cpp (compiland u2dvers.obj)
#ifndef HOMM3_U2DVERS_H
#define HOMM3_U2DVERS_H

#include <windows.h>
#include <string>
#include "va.h"

// PROVEN retail layout: both ctor and dtor access only the allocation
// pointer at +0; callers allocate four bytes for the object.
// Before normalization (type): TFileVersionInfo.
#ifndef FileVersionInfo
#define FileVersionInfo TFileVersionInfo
#endif
class FileVersionInfo {
public:
    char* m_data;
    FileVersionInfo(const char* filename);
    ~FileVersionInfo();
    // DC's source-visible wrapper. Complete expands it at the selection
    // window call site into the ProductVersion GetVersionInfo call.
    unsigned char getProductVersion(std::string* productVersion) const
    {
        return getVersionInfo("ProductVersion", productVersion);
    }

private:
    unsigned char getVersionInfo(const char* name, std::string* buffer) const;
};
SIZE(FileVersionInfo, 4);

// --- TFileVersionInfo ---

#endif  /* HOMM3_U2DVERS_H */
