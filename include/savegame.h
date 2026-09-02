// savegame.h - the save-file plumbing game::SaveGame (0x4beea0) needs.
//
// NEITHER DECLARATION HAS A PROVEN OWNER COMPILAND, which is why they
// are here rather than in an existing header: TGzFile's three bodies sit
// in the gametypewindow..hero link-order bracket with four candidate
// units and no Dreamcast row, and the save-slot number below has
// eighteen readers spread across the image with none of them modelled.
// This narrow domain header now serves the two reconstructed consumers,
// game.cpp and kb.cpp, without widening game.h's include-set wall. Move a
// declaration to its real owner header once that owner is proven.
#ifndef HOMM3_SAVEGAME_H
#define HOMM3_SAVEGAME_H

#include <exception>

#include "armygrp.h"  // TAbstractFile

// The zlib-backed save stream. RETAIL-PROVEN, three bodies:
//   0x4d6c50 (118 B) the constructor - stores TAbstractFile's vftable,
//     calls `@gzopen@8(path, mode)` (fastcall, so path in ecx and mode
//     in edx), parks the handle at +4, swaps in TGzFile's own vftable
//     and throws a std::exception when gzopen returns null;
//   0x4d6d60 (25 B) the destructor - `@gzclose@4([this+4])` and the
//     vftable back to the base's, i.e. the ordinary derived-dtor shape;
//   0x4d6cd0 / 0x4d6d80 / 0x4d6da0 the three virtual slots.
// SIZE 8 IS BYTE-PROVEN twice over: the constructor writes only +0 and
// +4, and game::SaveGame's frame puts the object at ebp-0x1c with the
// next local at ebp-0x14.
//
// DECLARED, NOT DEFINED - the bodies belong to whichever unit the
// bracket resolves to, and a consumer only needs the layout and the two
// entry points. The virtual overrides are declared so the class is
// concrete enough to instantiate; nothing here emits a vftable, because
// no constructor body in this TU ever initialises a vptr.
class TGzFile : public TAbstractFile {
public:
    // Retail RTTI at 0x677d48 and its two-entry catchable-type array prove
    // this empty std::exception-derived tag. game::SaveGame catches it by
    // value when opening the output stream fails.
    class TOpenFailure : public std::exception {
    };

    TGzFile(const char* path, const char* mode);
    ~TGzFile();
    virtual int Read(void* data, int size);
    virtual int Write(const void* data, int size);

    void* file;  // +0x04, the gzFile handle gzopen returned
};

// Retail .bss 0x699274. game::SaveGame formats it into "%s.GM%d" for an
// ordinary (non-campaign, non-tutorial) save, which is what makes the
// familiar .GM1 / .GM2 extensions - so it is a save-slot or player-count
// selector. NAME UNATTESTED, address-ordinal placeholder in the
// gUnnamed69ccc4 style; eighteen .text sites read it and none of them is
// modelled yet, so nothing constrains the role further.
extern int gUnnamed699274;

#endif  /* HOMM3_SAVEGAME_H */
