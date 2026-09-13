// u2dvers.h - prototypes of u2dvers.cpp (compiland u2dvers.obj)
#ifndef HOMM3_U2DVERS_H
#define HOMM3_U2DVERS_H

// version.dll's three entry points come from the SDK (<winver.h>, reached
// through <windows.h>), which declares them APIENTRY with no DECLSPEC_IMPORT
// - so they are called through the executable's own stdcall thunks, exactly
// as the retail bytes do. This header used to carry narrow-typed private
// redeclarations that CLASHED with the SDK's wherever a consumer had already
// seen <windows.h>, and a per-TU #ifndef hid the clash; the SDK prototypes
// are the real ones and the call sites below take their `LPSTR` / `LPVOID*`
// spellings.
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
    unsigned char getProductVersion(std::string* productVersion)
    {
        return getVersionInfo("ProductVersion", productVersion);
    }

private:
    unsigned char getVersionInfo(const char* name, std::string* buffer);
};
SIZE(TFileVersionInfo, 4);

#endif  /* HOMM3_U2DVERS_H */
