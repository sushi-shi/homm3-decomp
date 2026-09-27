#ifndef HOMM3_PLATFORM_FILE_TIME_H
#define HOMM3_PLATFORM_FILE_TIME_H

#include "platform.h"
#if defined(HOMM3_TARGET_MAC)
#include <DateTimeUtils.h>
#include <Files.h>
typedef DateTimeRec FileTime;

// Mac retail 0:0x277b60..0x2781ec: state, fork handle and FSSpec.
// This platform adapter has no Windows game-class counterpart.
// open's directory argument is a native FSSpec: 0x277c34/0x277c38
// read vRefNum at +2 and parID at +4 before calling FSMakeFSSpec.
// Selection's native directory getter (0x175674) returns one of three
// runtime-populated pointer cells at 1:0x553500/0x553504/0x553510.
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
