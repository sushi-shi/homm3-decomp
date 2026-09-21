// winfile.h - E:\gamedcs\winfile.h (File / CFindFile)
// Layouts and prototypes are Dreamcast CodeView evidence (LF_CLASS
// File 0x10c7, size 268; LF_CLASS CFindFile 0x10e4, size 564 with the
// WCE _WIN32_FIND_DATAW member), cross-proven on retail by the
// winfile.obj bodies 0x5ffb20..0x5ffdc2 and the File vtable 0x643d20:
// its 11 slots land on the claimed bodies in exactly the DC
// vfptr-offset order (Close=+0 .. GetLength=+40).
#ifndef HOMM3_WINFILE_H
#define HOMM3_WINFILE_H

#include "va.h"

#include <windows.h>

// DC enum 0x10be. The values are the FILE_ATTRIBUTE_* constants;
// faError = -1 backs the GetAttribute failure return.
enum FileAttribute {
    faArchive = 32,
    faNormal = 128,
    faHidden = 2,
    faReadOnly = 1,
    faSystem = 4,
    faTemporary = 256,
    faDirectory = 16,
    faHiddenReadOnly = 3,
    faSystemReadOnly = 5,
    faHiddenSystem = 6,
    faError = -1,
};

// DC enum 0x10c2. The values are GENERIC_READ / GENERIC_WRITE, passed
// straight through as CreateFileA's dwDesiredAccess (byte-proven by
// the retail Open body: `cmp eax, 0x80000000` against modeRead).
enum FileMode {
    modeRead = 0x80000000,
    modeWrite = 0x40000000,
    modeReadWrite = 0xC0000000,
};

// File - a thin kernel32 file wrapper with virtual I/O methods.
// Layout is DC fieldlist 0x10c8, byte-proven on retail: the ctor
// 0x5ffb20 stores the vtable at +0, zeroes m_hFile at +4 and the
// `open` byte at +0x108; every body addresses the handle at [this+4].
// sLastError/open/UpdateError/GetLastError are dead on retail - no
// surviving body touches +8..+0x107 (see winfile.cpp's absence notes).
class File {
public:
    // Dreamcast m_hFile: void* (HANDLE), retail +4. The zero-handle
    // guards in winfile.cpp reuse EAX without an integer alias.
    void* m_file;

    File();
    ~File();

    // Defined inline in winfile.cpp: no retail body, it survives only
    // inlined into Delete and Open as `_access(sFilename, 0) == 0`.
    static unsigned char exists(const char* filename);
    static unsigned char deleteFile(const char* filename);
    unsigned char rename(char* oldName, char* newName);                      // dc 0x198508, no retail body
    unsigned char setAttribute(char* filename, FileAttribute fileAttribute);  // dc 0x198544, no retail body
    FileAttribute getAttribute(char* filename);

    virtual unsigned char close();
    virtual unsigned char open(const char* filename, FileMode mode);
    virtual unsigned char isOpen();
    virtual unsigned long read(void* data, unsigned long dBytes);
    virtual unsigned long write(void* data, unsigned long dBytes);
    virtual unsigned long seek(unsigned long dBytesToSeek, unsigned long dStart);
    virtual unsigned long seekBegin();
    virtual unsigned long seekEnd();
    virtual unsigned long seekCur(int seekAmt);
    virtual unsigned long getPosition();
    virtual unsigned long getLength();

    char* getLastError();  // dc 0x1984d0, no retail body

protected:
    char m_lastError[256];
    unsigned char m_open;

    void updateError(char* errorText);  // dc 0x1984d8, no retail body

private:
    // E:\gamedcs\winfile.h:90-92 - the one header-defined method (the
    // DC winfile.obj contributor segment 0x198864-0x19886f is
    // attributed to winfile.h). No retail body; the retail ctor
    // writes the members directly.
    void init()
    {
        m_file = NULL;
        m_open = FALSE;
    }
};
SIZE(File, 268);

// CFindFile - FindFirstFileA wrapper. ENTIRELY ABSENT from retail:
// the only FindFirstFileA/FindNextFileA/FindClose call sites in the
// image are the CRT _find* functions (0x218113/0x2181df/0x218297).
// DC layout (fieldlist 0x10e5): m_searchHandle at +0, m_findData at
// +4 - the WCE build's _WIN32_FIND_DATAW (size 560, class total 564);
// a Win32 build would carry WIN32_FIND_DATAA (+4..+0x143). No SIZE
// assert: there is no retail stride evidence for either variant.
class CFindFile {
public:
    CFindFile();                              // dc 0x198748
    ~CFindFile();                             // dc 0x198768
    unsigned char findFile(const char* fileName);  // dc 0x198778
    unsigned char findNext();                 // dc 0x19877c
    void close();                             // dc 0x198798
    unsigned long getLength();                // dc 0x1987bc
    char* getFilename();                      // dc 0x1987d4
    unsigned char isReadOnly();               // dc 0x1987dc
    unsigned char isSystem();                 // dc 0x1987ec
    unsigned char isNormal();                 // dc 0x1987fc
    unsigned char isDots();                   // dc 0x198810
    unsigned char isDirectory();              // dc 0x198814
    unsigned char isHidden();                 // dc 0x198830
    unsigned char isArchived();               // dc 0x198840

protected:
    void* m_searchHandle;
    WIN32_FIND_DATAA m_findData;
};

#endif  /* HOMM3_WINFILE_H */
