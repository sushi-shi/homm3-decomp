#ifndef HOMM3_U2DVERS_H
#define HOMM3_U2DVERS_H

#include <windows.h>
#include <string>
#include "va.h"

// PROVEN retail layout: both ctor and dtor access only the allocation
// pointer at +0; callers allocate four bytes for the object.
class TFileVersionInfo {
public:
    char* m_data;
    TFileVersionInfo(const char* filename);
    ~TFileVersionInfo();
    // DC's source-visible wrapper. Complete expands it at the selection
    // window call site into the ProductVersion GetVersionInfo call.
    // Both DC publics return native bool (QBA_N / ABA_N), despite their
    // lowered unsigned-char debug records.
    bool getProductVersion(std::string* productVersion) const
    {
        return getVersionInfo("ProductVersion", productVersion);
    }

private:
    bool getVersionInfo(const char* name, std::string* buffer) const;
};
SIZE(TFileVersionInfo, 4);

#endif  /* HOMM3_U2DVERS_H */
