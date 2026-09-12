// gzfile.h - the Complete zlib-backed TAbstractFile implementation.
// The module spelling is inferred from the retained class family between
// the retail initializers at 0x4d6c30 and 0x4d6dc0.
#ifndef HOMM3_GZFILE_H
#define HOMM3_GZFILE_H

#include <exception>
#include "armygrp.h"  // TAbstractFile

// Complete wraps the gz handle in an eight-byte polymorphic stream:
// 0x4d6c50 installs the base vptr 0x63dac0, calls gzopen at 0x4d6c84,
// stores its result at +4 and installs 0x63e74c. The virtual table holds
// deleting cleanup / read / write at 0x4d6cd0 / 0x4d6d80 / 0x4d6da0.
// game::SaveGame constructs it at 0x4bf0c8 and closes it at 0x4bf0e4.
// DC game.cpp:3734/3749 instead calls gzopen/gzclose directly around its
// void* outfile (dc 0xa9c34/0xa9cea); there is no TGzFile type or method
// in the pinned CodeView records. Keep that distinction from zlib itself,
// which exists in both builds.
//
// The handle is a member initializer: the retained constructor stores the
// base vptr before gzopen and its own vptr afterward. A body assignment
// reversed that observed boundary in the prior 92.76% control.
class TGzFile : public TAbstractFile {
public:
    // Retail RTTI at 0x677d48 and its two-entry catchable-type array prove
    // this empty std::exception-derived tag. game::SaveGame catches it by
    // value when opening the output stream fails.
    class TOpenFailure : public std::exception {
    };

    TGzFile(const char* path, const char* mode);
    ~TGzFile();
    // Before normalization (function): TGzFile::Read.
    virtual int read(void* data, int size);
    // Before normalization (function): TGzFile::Write.
    virtual int write(const void* data, int size);

    // Before normalization: file.
    void* m_file;  // +0x04, the gzFile handle gzopen returned
};

#endif  /* HOMM3_GZFILE_H */
