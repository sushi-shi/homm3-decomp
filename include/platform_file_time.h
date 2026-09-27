#ifndef HOMM3_PLATFORM_FILE_TIME_H
#define HOMM3_PLATFORM_FILE_TIME_H

#include "platform.h"
#if defined(HOMM3_TARGET_MAC)
#include <DateTimeUtils.h>
#include <Files.h>
typedef DateTimeRec FileTime;

// Mac retail 0:0x277b60..0x2781ec: state, fork handle and FSSpec.
// This platform adapter has no Windows game-class counterpart.
class MacFileAdapter {
public:
    unsigned char m_open;
    long m_file;
    FSSpec m_spec;
    MacFileAdapter();
    ~MacFileAdapter();
    OSErr open(const FSSpec* directory, const char* filename, OSType creator,
               OSType type, bool create);
    int close();
    void getModificationDate(FileTime* date);
};
#else
typedef FILETIME FileTime;
#endif

#endif
