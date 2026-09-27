#ifndef HOMM3_PLATFORM_FILE_TIME_H
#define HOMM3_PLATFORM_FILE_TIME_H

#include "platform.h"
#if defined(HOMM3_TARGET_MAC)
#include <DateTimeUtils.h>
#include <Files.h>
typedef DateTimeRec FileTime;

// Mac retail 0:0x277b60..0x2781ec: state, fork handle and FSSpec.
// This platform adapter has no Windows game-class counterpart.
// Native directory handles allocate eight bytes at 0x20f4e8/0x20f550/
// 0x20f584. Their constructor (0x2782ec) clears the state byte; file open
// reads their volume at +2 and directory ID at +4 before FSMakeFSSpec.
struct MacDirectoryAdapter {
    unsigned char m_open;
    short m_volume;
    long m_directoryId;
};

// Selection's native directory getter (0x175674) returns one of three
// runtime-populated adapter pointer cells at 1:0x553500/0x553504/0x553510.
class MacFileAdapter {
public:
    unsigned char m_open;
    long m_file;
    FSSpec m_spec;
    MacFileAdapter();
    ~MacFileAdapter();
    OSErr open(const MacDirectoryAdapter* directory, const char* filename, OSType creator,
               OSType type, bool create);
    int close();
    void getModificationDate(FileTime* date);
};
#else
typedef FILETIME FileTime;
#endif

#endif
