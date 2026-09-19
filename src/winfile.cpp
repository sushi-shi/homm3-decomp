// 14 retail functions in link order (of 35 DC procs).

// Retail span: the window->wingraph gap. window.cpp's text ends with
// its cinit funclet at 0x5ffb00; winfile.obj's code is exactly
// 0x5ffb20..0x5ffdc2 - 14 File bodies in DC source order, every start
// 16-aligned - followed by winfile's own cinit funclet at 0x5ffdd0
// (byte-identical to window's 0x5ffb00: shared guard byte 0x6abaa0,
// atexit thunk 0x404df0 = a bare `ret`; a header static included by
// both TUs, owner unidentified, left unclaimed like the iconwdgt/
// initialize cinit tails). The next TU starts at 0x5ffdf0 (a
// Bitmap16Bit static-ctor funclet + the SetPlayerPaletteColors pair,
// DC wingraph.cpp:72/:79, then the DirectDraw head).

// Absent from retail (documented, not forced): GetLastError,
// UpdateError, Rename, SetAttribute, GetAttribute (unreferenced -
// nothing in the image touches sLastError/open), Exists (survives
// only inlined into Delete/Open as _access(s,0)==0; the `inline`
// definition below reproduces the absence under the non-/Gy profile),
// File::Init (winfile.h:90 header inline, dc 0x198864), and ALL
// FOURTEEN CFindFile methods (dc 0x198748..0x198863) - the only
// FindFirstFileA/FindNextFileA/FindClose callers in the image are the
// CRT _find* trio at 0x618113/0x6181df/0x618297.

#include <va.h>
#include <io.h>
#include "winfile.h"

VA(0x005ffb20, 0x14)  // dc 0x198434
File::File()
{
    m_file = NULL;
    m_open = FALSE;
}

VA(0x005ffb40, 0x15)  // dc 0x198468
File::~File()
{
    if (m_file)
        CloseHandle(m_file);
}

VA(0x005ffb60, 0xB)  // dc 0x19849c
unsigned char File::isOpen()
{
    return m_file != NULL;
}

VA(0x005ffb70, 0x20)  // dc 0x1984a8
unsigned char File::close()
{
    if (!m_file)
        return FALSE;

    CloseHandle(m_file);
    m_file = NULL;
    return TRUE;
}

// E:\gamedcs\winfile.cpp:70 - no retail body; inlined into Delete and
// Open below (the one-pass inliner needs the body before its callers).
inline unsigned char File::exists(const char* filename)
{
    return _access(filename, 0) == 0;
}

// Original: File::Delete; winfile.cpp:77, dc 0x1984f0.
// The semantic suffix avoids the C++ keyword delete after case normalization.
VA(0x005ffb90, 0x24)  // dc 0x1984f0
unsigned char File::deleteFile(const char* filename)
{
    unsigned char deleted;

    if (!exists(filename))
        return FALSE;

    deleted = DeleteFileA(filename) != 0;
    return deleted;
}

VA(0x005ffbc0, 0x84)  // dc 0x198568
unsigned char File::open(const char* filename, FileMode mode)
{
    if (m_file) {
        CloseHandle(m_file);
        m_file = NULL;
    }

    if (exists(filename)) {
        m_file = CreateFileA(filename, mode, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    } else {
        if (mode == modeRead)
            return FALSE;
        m_file = CreateFileA(filename, mode, FILE_SHARE_READ, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    }

    if (m_file == INVALID_HANDLE_VALUE) {
        m_file = NULL;
        return FALSE;
    }
    return TRUE;
}

VA(0x005ffc50, 0x30)  // dc 0x1985b8
unsigned long File::write(void* data, unsigned long dBytes)
{
    unsigned long dBytesWritten;

    if (!m_file)
        return 0;

    return WriteFile(m_file, data, dBytes, &dBytesWritten, NULL) ? dBytesWritten : 0;
}

VA(0x005ffc80, 0x30)  // dc 0x1985e4
unsigned long File::read(void* data, unsigned long dBytes)
{
    unsigned long dBytesRead;

    if (!m_file)
        return 0;

    return ReadFile(m_file, data, dBytes, &dBytesRead, NULL) ? dBytesRead : 0;
}

VA(0x005ffcb0, 0x16)  // dc 0x198610
unsigned long File::seekEnd()
{
    if (!m_file)
        return 0;

    return SetFilePointer(m_file, 0, NULL, FILE_END);
}

VA(0x005ffcd0, 0x16)  // dc 0x198630
unsigned long File::seekBegin()
{
    if (!m_file)
        return 0;

    return SetFilePointer(m_file, 0, NULL, FILE_BEGIN);
}

VA(0x005ffcf0, 0x21)  // dc 0x198650
unsigned long File::seekCur(int seekAmt)
{
    if (!m_file)
        return 0;

    return SetFilePointer(m_file, seekAmt, NULL, FILE_CURRENT);
}

VA(0x005ffd20, 0x4E)  // dc 0x198670
unsigned long File::seek(unsigned long dBytesToSeek, unsigned long dStart)
{
    if (!m_file)
        return 0;

    if (dStart == 0)
        return SetFilePointer(m_file, dBytesToSeek, NULL, FILE_BEGIN);

    SetFilePointer(m_file, dStart, NULL, FILE_BEGIN);
    return SetFilePointer(m_file, dBytesToSeek, NULL, FILE_CURRENT);
}

VA(0x005ffd70, 0x16)  // dc 0x1986e4
unsigned long File::getPosition()
{
    if (!m_file)
        return 0;

    return SetFilePointer(m_file, 0, NULL, FILE_CURRENT);
}

VA(0x005ffd90, 0x32)  // dc 0x198704
unsigned long File::getLength()
{
    unsigned long dPosition;
    unsigned long dLength;

    if (!m_file)
        return 0;

    dPosition = getPosition();
    dLength = seekEnd();
    seek(dPosition, 0);
    return dLength;
}
