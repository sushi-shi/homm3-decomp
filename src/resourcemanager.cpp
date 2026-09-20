#include <va.h>
#include <yvals.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <utility>
#include "resource.h"
#include "resourcemanager.h"
#include "resourcemanager_archive.h"
#include "abstractfile.h"
#include "resourcemanager_sprite_headers.h"
#include "textresource.h"
#include "lodfile.h"
#include "bitmap16.h"
#include "bitmap24.h"
#include "bitmap816.h"
#include "csprite.h"
#include "cspriteframe.h"
#include "font.h"
#include "ownership.h"
#include "palette.h"
#include <memory>
#include <stdlib.h>
#include <string>
#include <windows.h>
#include <sstream>
#include "sample.h"
#include "smackmgr.h"

class LODFile;

namespace ResourceManager {

// Complete's resource readers adapt either an ordinary FILE or a selected
// LODFile to the common three-slot stream ABI. The HD names are admitted only
// after retail proves both layouts (vptr + one pointer), vtable slots and read
// behavior at these addresses. These implementation types are used only by
// this resource-loading module; they have no shared header interface.
// Both Write methods fold with CHeroWindowEx::OnWidgetDeselect at 0x559140.
class t_stdio_file_adapter : public TAbstractFile {
public:
    explicit t_stdio_file_adapter(FILE* value) : m_file(value) {}

    virtual int read(void* data, int size);
    VA(0x00559140, 0x5)  // two adapter vtables + exact body, retail-only
    virtual int write(const void*, int) { return 0; }

    FILE* m_file;
};

class t_lod_file_adapter : public TAbstractFile {
public:
    explicit t_lod_file_adapter(LODFile* value) : m_lodFile(value) {}

    virtual int read(void* data, int size);
    virtual int write(const void*, int) { return 0; }

    LODFile* m_lodFile;
};

// Dreamcast resourcemanager.cpp:121/126 proves the class, constructor and
// ordinary const operator< (original field: name). Retail 0x55ac20 copies
// twelve bytes and terminates byte 12; 0x55ebd0 compares these keys with
// _stricmp. Complete's map node has its key at +0xc and resource* at +0x1c.
class TCacheMapKey {
public:
    char m_name[13];
    TCacheMapKey(const char* name);
    bool operator<(const TCacheMapKey& other) const;
};

typedef std::map<TCacheMapKey, resource*> TCacheMap;
SIZE(TCacheMapKey, 13);
SIZE(TCacheMap, 16);

Bitmap16Bit* loadBitmap16(const char* name);
TPalette16* loadPalette(const char* name);
TPalette24* getPalette24(const char* name);
font* loadFont(const char* name);
font* loadFontData(const char* name, TAbstractFile* stream, int fileSize);
TTextResource* loadText(const char* name);
TSpreadsheetResource* loadSpreadsheet(const char* name);

}

DATA(0x0069e528)
ResourceManager::TCacheMap g_resourceCache;

#if 0 // @carcass - unlocated/unreconstructed Dreamcast roster rows

// E:\gamedcs\resourcemanager.cpp:158
DC_ONLY(0x1213a0, 0x182)
void ResourceManager::remapGraphics()
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:222
DC_ONLY(0x121524, 0x216)
void ResourceManager::saturateGraphics()
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:298
DC_ONLY(0x12173c, 0x144)
unsigned char ResourceManager::Open(unsigned char open_sprites, unsigned char open_bitmaps)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:338
DC_ONLY(0x121880, 0x1C)
void ResourceManager::Close()
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:357
DC_ONLY(0x12189c, 0x26)
void ResourceManager::setPath(const char* path)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:374
DC_ONLY(0x1218c4, 0x168)
void ResourceManager::setPixelFormat(unsigned long red_mask, unsigned long green_mask, unsigned long blue_mask)
{
    // @stub
}

// Retired generic DATA resource family. All 23 Complete resource ctor
// callers and the admitted resource vtables belong to the concrete typed
// resource classes; see genericresource.cpp and the exact dc_only.tsv rows.
// E:\gamedcs\resourcemanager.cpp:438
DC_ONLY(0x121a2c, 0x9C)
TGenericResource* ResourceManager::GetResource(const char* name)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:729
DC_ONLY(0x121ac8, 0x194)
Bitmap816* ResourceManager::getBitmap816(const char* name)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:906
DC_ONLY(0x121c5c, 0x134)
Bitmap16Bit* ResourceManager::getBitmap16(const char* name, unsigned char ignore_cache)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:1027
DC_ONLY(0x121d90, 0x138)
TPalette16* ResourceManager::getPalette(const char* name, unsigned char ignore_cache)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:1133
DC_ONLY(0x121ec8, 0xE4)
TPalette24* ResourceManager::getPalette24(const char* name)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:1221
DC_ONLY(0x121fac, 0xE0)
font* ResourceManager::getFont(const char* name)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:1356
DC_ONLY(0x12208c, 0xD8)
TTextResource* ResourceManager::getText(const char* name)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:1461
#endif

// ResourceManager's retail archive pool is eight interleaved 0x190-byte
// slots. Open proves the leading dword is the archive pathname and every
// resource lookup independently proves the LODFile subobject at +4.
struct TResourceLODSlot {
    const char* m_archiveName;
    LODFile m_file;

    TResourceLODSlot(const char* name);
};
SIZE(TResourceLODSlot, 0x190);

VA(0x005590f0, 0x1D)  // stdio adapter vtable slot 1
int ResourceManager::t_stdio_file_adapter::read(void* data, int size)
{
    return fread(data, 1, size, m_file);
}

VA(0x00559110, 0x21)  // LOD adapter vtable slot 1
int ResourceManager::t_lod_file_adapter::read(void* data, int size)
{
    return m_lodFile->read(data, size) ? 0 : size;
}

VA(0x005591e0, 0x1A)
TResourceLODSlot::TResourceLODSlot(const char* name)
    : m_archiveName(name)
{
}

VA_COMPGEN(0x00559440, 0x6E, IMPLICIT_DTOR, map)

VA(0x005594b0, 0x40)  // dc 0x122984
void ResourceManager::addToCache(resource* value)
{
    g_resourceCache.insert(std::make_pair(value->getName(), value));
    value->addRef();
}

// The ostringstream used by both missing-resource reporters makes VC6 retain
// this Dinkumware vbase-destructor closure. The base object's ??_D public and
// both retail calls independently identify it; all 20 emitted bytes agree.
#if 0  // @carcass -- compiler/library COMDAT emitted by ostringstream
VA(0x005594f0, 0x14)  // anchor-caller + emitted COFF public, retail-only
void basic_ostringstream::`vbase destructor'();
#endif

DATA(0x00694d60) unsigned int Bitmap16Bit::s_greenMask;
DATA(0x00694d64) unsigned int Bitmap16Bit::s_blueMask;
DATA(0x00694d68) unsigned int Bitmap16Bit::s_redMask;
DATA(0x0069cc60) unsigned int TPalette16::s_greenMask;
DATA(0x0069cc64) unsigned int TPalette16::s_redMask;
DATA(0x0069cc68) unsigned int TPalette16::s_blueMask;
DATA(0x0069e598) unsigned long g_spriteMaskFirst;
DATA(0x0069e59c) unsigned long g_spriteMaskGreen;
// Toggled by the retail adventure-map command that dispatches
// SaturateGraphics/RemapGraphics; every resource loader consults the byte.
DATA(0x0069d858) unsigned long g_spriteMaskLast;
DATA(0x0069e5b0) unsigned char g_graphicsSaturated;
DATA(0x0069d868) int g_firstMaskShift;
DATA(0x0069d860) int g_firstMaskBits;
DATA(0x0069d864) int g_greenMaskShift;
DATA(0x0069d854) int g_greenMaskBits;
DATA(0x0069d85c) int g_lastMaskShift;
DATA(0x0069e5a0) int g_lastMaskBits;
DATA(0x0069e4f0) std::string g_resourcePath;

// Complete's common missing-resource reporter has no Dreamcast identity, but
// its thirteen retail callers prove the fastcall surface. The dense 0..96
// dispatch maps the admitted resource values to names and renders every gap
// as `0x` plus a hexadecimal value before building the diagnostic shown by
// the callers.
// WALL (89.3910%): all twelve named cases, the hexadecimal default, seven-part
// message and MessageBox call are closed, and all 29 retail blocks now agree
// in flow (23 exact, six size-only). Retail calls assign at the first eight
// cases and expands four late sites. Hoisting strlen before a pinned two-arg
// assign in the first four cases preserves those calls while reproducing the
// late expansion budget. The threshold ratchet was N=0 77.3910, N=1 82.2494,
// N=2 flat, N=3 83.5393 (negative control), and N=4 89.3910. The residual six
// size-only blocks are the default/message stream's frame coloring: an added
// message scope regresses to 87.0742, and predict-inline leaves only one
// ios_base destructor and one string::_Tidy over-inlined.
VA(0x00559510, 0x4C1)  // caller ABI + retail type-name jump table/message graph
void __fastcall game_null_159510(const char* caller,
                                 int resourceType,
                                 const char* resourceName)
{
    std::string typeName;
    switch (resourceType) {
    case RESOURCE_TYPE_NONE: {
        const char* value =
            DATA_COMPGEN(0x00682fc8, nullResourceType, "null");
        std::string::size_type length = strlen(value);
// INLINE BOUNDARY: game_null_159510 -> basic_string::assign(ptr,len).
// Retail keeps the first eight assign calls and expands four late cases;
// hoisting strlen makes this pin govern assign alone. Flattening all four
// restores the 77.3910 baseline; the N=3 control is 83.5393 vs N=4 89.3910.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_DATA: {
        const char* value =
            DATA_COMPGEN(0x00682fc0, dataResourceType, "data");
        std::string::size_type length = strlen(value);
        // Same caller/callee boundary and N=3/N=4 ratchet as above.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_TEXT: {
        const char* value =
            DATA_COMPGEN(0x00682fb8, textResourceType, "text");
        std::string::size_type length = strlen(value);
        // Same caller/callee boundary and N=3/N=4 ratchet as above.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_BITMAP: {
        const char* value =
            DATA_COMPGEN(0x00682fb0, bitmap8ResourceType, "bitmap8");
        std::string::size_type length = strlen(value);
        // Same caller/callee boundary and N=3/N=4 ratchet as above.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_BITMAP24:
        typeName.assign(
            DATA_COMPGEN(0x00682fa4, bitmap24ResourceType, "bitmap24"));
        break;
    case RESOURCE_TYPE_BITMAP16:
        typeName.assign(
            DATA_COMPGEN(0x00682f98, bitmap16ResourceType, "bitmap16"));
        break;
    case RESOURCE_TYPE_BITMAP565:
        typeName.assign(
            DATA_COMPGEN(0x00682f8c, bitmap565ResourceType, "bitmap565"));
        break;
    case RESOURCE_TYPE_BITMAP555:
        typeName.assign(
            DATA_COMPGEN(0x00682f80, bitmap555ResourceType, "bitmap555"));
        break;
    case RESOURCE_TYPE_BITMAP1555:
        typeName.assign(DATA_COMPGEN(0x00682f74, bitmap1555ResourceType,
                                     "bitmap1555"));
        break;
    case RESOURCE_TYPE_MIDI:
        typeName.assign(
            DATA_COMPGEN(0x00682f6c, midiResourceType, "midi"));
        break;
    case RESOURCE_TYPE_FONT:
        typeName.assign(
            DATA_COMPGEN(0x00682f64, fontResourceType, "font"));
        break;
    case RESOURCE_TYPE_PALETTE:
        typeName.assign(
            DATA_COMPGEN(0x00682f5c, paletteResourceType, "palette"));
        break;
    default: {
        std::ostringstream numericType;
        numericType
            << DATA_COMPGEN(0x00682f58, resourceTypeHexPrefix, "0x")
            << std::hex << resourceType;
        typeName = numericType.str();
        break;
    }
    }

#pragma inline_depth(0)
    std::ostringstream message;
#pragma inline_depth()
    message
        << DATA_COMPGEN(0x00682f18, sampleErrorPrefix,
                        "ResourceManager::")
        << caller
        << DATA_COMPGEN(0x00682f2c, missingResourcePrefix,
                        " could not find the \"")
        << typeName.c_str()
        << DATA_COMPGEN(0x00682f44, missingResourceMiddle, "\" resource \"")
        << resourceName
        << DATA_COMPGEN(0x00682f54, missingResourceSuffix, "\".");
    MessageBoxA(
        GetForegroundWindow(), message.str().c_str(),
        DATA_COMPGEN(0x00682f08, resourceManagerCaption,
                     "ResourceManager"),
        0);
#pragma inline_depth(0)
}
#pragma inline_depth()

// Complete splits sprite-family diagnostics from the common resource
// reporter above. GetSprite's two calls prove the fastcall ABI and the retail
// jump table proves the complete 64..79 type-name mapping. WALL (86.1324%):
// all eleven named cases, the hexadecimal default, seven-part message and
// MessageBox call agree, and all 28 retail blocks now agree in flow (22 exact,
// six size-only). Retail calls assign for the early cases and expands four
// late sites. The explicit strlen plus assign-only pin ratchets N=0 70.5441,
// N=1 flat, N=2 76.7853, N=3 flat, N=4 78.4735 (negative control), and N=5
// 86.1324 -> 86.9559: the message stream is BLOCK-SCOPED. Retail's frame is
// 0xac against our 0x130, and the 0x84 surplus is exactly one ostringstream -
// retail overlays the default arm's `numericType` onto `message`, which VC6
// will only do once `message` has a scope of its own. The frame is now
// retail's to the byte. MEASURED AND REJECTED: the identical scope in
// game_null_159510 above COSTS 2.62 (89.3910 -> 86.7726) even though it makes
// that frame exact too - it adds five early-return destructor blocks there,
// so this is a per-function verdict, not a rule. Predict-inline leaves only
// one ios_base destructor and one string::_Tidy over-inlined; the remaining
// six blocks are size-only stream frame coloring rather than a missing
// semantic branch.
VA(0x005599e0, 0x448)  // anchor-caller + retail type-name jump table; wall
void __fastcall game_sprite_1599e0(const char* caller,
                                   int resourceType,
                                   const char* resourceName)
{
    std::string typeName;
    switch (resourceType) {
    case RESOURCE_TYPE_SPRITE: {
        const char* value =
            DATA_COMPGEN(0x0067555c, spriteResourceType, "sprite");
        std::string::size_type length = strlen(value);
// INLINE BOUNDARY: game_sprite_1599e0 -> basic_string::assign(ptr,len).
// Retail keeps the early assign calls and expands four late cases; hoisting
// strlen makes this pin govern assign alone. Flattening all five restores the
// 70.5441 baseline; the N=4 control is 78.4735 vs N=5 86.1324.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_SPRITE_DEFINITION: {
        const char* value = DATA_COMPGEN(
            0x00682ff4, spriteDefinitionResourceType, "spritedef");
        std::string::size_type length = strlen(value);
        // Same caller/callee boundary and N=4/N=5 ratchet as above.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_CREATURE: {
        const char* value =
            DATA_COMPGEN(0x00675550, creatureResourceType, "creature");
        std::string::size_type length = strlen(value);
        // Same caller/callee boundary and N=4/N=5 ratchet as above.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_ADVENTURE_OBJECT: {
        const char* value = DATA_COMPGEN(
            0x00675548, adventureObjectResourceType, "advobj");
        std::string::size_type length = strlen(value);
        // Same caller/callee boundary and N=4/N=5 ratchet as above.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_HERO: {
        const char* value =
            DATA_COMPGEN(0x00675540, heroResourceType, "hero");
        std::string::size_type length = strlen(value);
        // Same caller/callee boundary and N=4/N=5 ratchet as above.
#pragma inline_depth(0)
        typeName.assign(value, length);
#pragma inline_depth()
        break;
    }
    case RESOURCE_TYPE_TILESET:
        typeName.assign(
            DATA_COMPGEN(0x00675538, tilesetResourceType, "tileset"));
        break;
    case RESOURCE_TYPE_POINTER:
        typeName.assign(
            DATA_COMPGEN(0x00675530, pointerResourceType, "pointer"));
        break;
    case RESOURCE_TYPE_INTERFACE:
        typeName.assign(
            DATA_COMPGEN(0x00675524, interfaceResourceType, "interface"));
        break;
    case RESOURCE_TYPE_SPRITE_FRAME:
        typeName.assign(DATA_COMPGEN(0x00682fe4, spriteFrameResourceType,
                                     "sprite frame"));
        break;
    case RESOURCE_TYPE_COMBAT_HERO:
        typeName.assign(DATA_COMPGEN(0x00682fd8, combatHeroResourceType,
                                     "combat hero"));
        break;
    case RESOURCE_TYPE_ADVENTURE_MASK:
        typeName.assign(DATA_COMPGEN(0x00682fd0, adventureMaskResourceType,
                                     "advmask"));
        break;
    default: {
        std::ostringstream numericType;
        numericType
            << DATA_COMPGEN(0x00682f58, resourceTypeHexPrefix, "0x")
            << std::hex << resourceType;
        typeName = numericType.str();
        break;
    }
    }

    {
#pragma inline_depth(0)
    std::ostringstream message;
#pragma inline_depth()
    message
        << DATA_COMPGEN(0x00682f18, sampleErrorPrefix,
                        "ResourceManager::")
        << caller
        << DATA_COMPGEN(0x00682f2c, missingResourcePrefix,
                        " could not find the \"")
        << typeName.c_str()
        << DATA_COMPGEN(0x00682f44, missingResourceMiddle, "\" resource \"")
        << resourceName
        << DATA_COMPGEN(0x00682f54, missingResourceSuffix, "\".");
    MessageBoxA(
        GetForegroundWindow(), message.str().c_str(),
        DATA_COMPGEN(0x00682f08, resourceManagerCaption,
                     "ResourceManager"),
        0);
    }
#pragma inline_depth(0)
}
#pragma inline_depth()

VA(0x00559e30, 0x1E5)
void ResourceManager::remapGraphics()
{
    for (TCacheMap::iterator position = g_resourceCache.begin();
         position != g_resourceCache.end(); position++) {
        resource* value = position->second;

        switch (value->getResType()) {
        case RESOURCE_TYPE_BITMAP16: {
            std::auto_ptr<Bitmap16Bit> loaded(loadBitmap16(value->getName()));
            if (loaded.get()) {
                loaded->draw(0, 0, loaded->getWidth(), loaded->getHeight(),
                             static_cast<Bitmap16Bit*>(value), 0, 0, false);
            }
            break;
        }

        case RESOURCE_TYPE_CREATURE:
        case RESOURCE_TYPE_ADVENTURE_OBJECT:
        case RESOURCE_TYPE_HERO:
        case RESOURCE_TYPE_TILESET:
        case RESOURCE_TYPE_POINTER:
        case RESOURCE_TYPE_INTERFACE:
        case RESOURCE_TYPE_COMBAT_HERO:
            static_cast<CSprite*>(value)->resetPalette();
            break;

        case RESOURCE_TYPE_BITMAP:
            static_cast<Bitmap816*>(value)->resetPalette();
            break;

        case RESOURCE_TYPE_FONT: {
            std::auto_ptr<TPalette16> palette(loadPalette(
                DATA_COMPGEN(0x0067f780, resourceGamePaletteName,
                             "game.pal")));
            if (palette.get())
                static_cast<font*>(value)->setPalette(*palette);
            break;
        }

        case RESOURCE_TYPE_PALETTE: {
            TPalette16* destination = static_cast<TPalette16*>(value);
            std::auto_ptr<TPalette16> loaded(loadPalette(value->getName()));
            if (loaded.get())
                destination->m_colors = loaded->m_colors;
            break;
        }
        }

    }
}

VA(0x0055a020, 0x221)
void ResourceManager::saturateGraphics()
{
    for (TCacheMap::iterator position = g_resourceCache.begin();
         position != g_resourceCache.end(); position++) {
        resource* value = position->second;

        switch (value->getResType()) {
        case RESOURCE_TYPE_BITMAP16: {
            std::auto_ptr<Bitmap16Bit> loaded(loadBitmap16(value->getName()));
            if (loaded.get()) {
                loaded->draw(0, 0, loaded->getWidth(), loaded->getHeight(),
                             static_cast<Bitmap16Bit*>(value), 0, 0, false);
            }
            break;
        }

        case RESOURCE_TYPE_CREATURE:
        case RESOURCE_TYPE_ADVENTURE_OBJECT:
        case RESOURCE_TYPE_HERO:
        case RESOURCE_TYPE_TILESET:
        case RESOURCE_TYPE_POINTER:
        case RESOURCE_TYPE_INTERFACE:
        case RESOURCE_TYPE_COMBAT_HERO: {
            CSprite* sprite = static_cast<CSprite*>(value);
            sprite->m_p24->adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
            sprite->resetPalette();
            break;
        }

        case RESOURCE_TYPE_BITMAP: {
            Bitmap816* bitmap = static_cast<Bitmap816*>(value);
            bitmap->m_p24.adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
            bitmap->resetPalette();
            break;
        }

        case RESOURCE_TYPE_FONT: {
            std::auto_ptr<TPalette16> palette(loadPalette("game.pal"));
            if (palette.get())
                static_cast<font*>(value)->setPalette(*palette);
            break;
        }

        case RESOURCE_TYPE_PALETTE: {
            TPalette16* destination = static_cast<TPalette16*>(value);
            std::auto_ptr<TPalette16> loaded(loadPalette(value->getName()));
            if (loaded.get())
                destination->m_colors = loaded->m_colors;
            break;
        }
        }

    }
}

// Complete's loader adds an error-code output and two nested handlers around
// the simpler Dreamcast archive walk. Retail reserves an eight-entry cleanup
// vector but never appends to it; preserve that shipped behavior. The inner
// catch-all drains any recorded archives and rethrows, while the outer int
// handler publishes the failing archive class to the caller.

// Dreamcast Open (dc 0x12173c) proves the open_sprites/open_bitmaps parameters,
// `sprite_pathname` then `bitmap_pathname` locals, and each copy/append/c_str/
// LODFile::open statement group. Retail additionally proves that each open
// result is saved before the pathname destructor and tested afterward; the
// scoped pathname plus `opened` spelling recovers that lowering.

// Residual (84.1089%): candidate and retail retain the same 36-block,
// sixteen-branch, three-return, seven-state/two-try structure. The remaining
// split is a positional Dinkumware inliner inversion: retail expands reserve's
// _Ucopy loop but calls _Destroy from pop_back, while this context does the
// inverse. Pinning pop_back is the negative control (80.6210%). Vector and
// pathname expression variants were flat or worse, so this is a bounded
// A8/A9 inliner wall rather than missing archive behavior.
// The flags arrive in ECX/EDX; ret 4 removes the added error-code pointer.
VA(0x0055a250, 0x2F1)  // sole retail caller + two flags/error output, dc 0x12173c
bool ResourceManager::open(bool openSprites, bool openBitmaps, int* errorCode)
{
    try {
        std::vector<int> openedArchives;
        openedArchives.reserve(8);

        try {
            TResourceArchiveContext* context =
                &g_resourceArchiveContexts[*g_videoGameState];

            if (openSprites) {
                int remaining = context->m_sprites.m_count;
                int* archive = context->m_sprites.m_indices;
                do {
                    int archiveIndex = *archive;
                    TResourceLODSlot& slot =
                        g_resourceLodSlots[archiveIndex];
                    LODFile* file = &slot.m_file;
                    bool opened;
                    {
                        std::string spritePathname =
                            g_resourcePath + slot.m_archiveName;
                        opened = file->open(spritePathname.c_str(), 0) == 0;
                    }
                    if (!opened) {
                        if (archiveIndex == 1)
                            throw archiveIndex;
                        throw 0;
                    }
                    ++archive;
                } while (--remaining);
            }

            if (openBitmaps) {
                int remaining = context->m_bitmaps.m_count;
                int* archive = context->m_bitmaps.m_indices;
                do {
                    int archiveIndex = *archive;
                    TResourceLODSlot& slot =
                        g_resourceLodSlots[archiveIndex];
                    LODFile* file = &slot.m_file;
                    bool opened;
                    {
                        std::string bitmapPathname =
                            g_resourcePath + slot.m_archiveName;
                        opened = file->open(bitmapPathname.c_str(), 0) == 0;
                    }
                    if (!opened) {
                        if (archiveIndex == 0)
                            throw 1;
                        throw 0;
                    }
                    ++archive;
                } while (--remaining);
            }

        } catch (...) {
            while (openedArchives.size()) {
                int archiveIndex = openedArchives.back();
                openedArchives.pop_back();
                g_resourceLodSlots[archiveIndex].m_file.clear();
            }
            throw;
        }
    } catch (int error) {
        if (errorCode)
            *errorCode = error;
        return false;
    }

    return true;
}

VA(0x0055a550, 0x67)
void ResourceManager::close()
{
    expunge();

    for (int i = 0; i < 8; ++i)
        g_resourceLodSlots[i].m_file.clear();
}

VA(0x0055a5c0, 0xE2)
void ResourceManager::setPath(const char* path)
{
    char fullpath[_MAX_PATH + 1];
    _fullpath(fullpath, path, _MAX_PATH);
    g_resourcePath = fullpath;
}

VA(0x0055a6b0, 0xEF)
void ResourceManager::setPixelFormat(unsigned long redMask,
                                     unsigned long greenMask,
                                     unsigned long blueMask)
{
    CSprite::setPixelFormat(redMask, greenMask, blueMask);
    Bitmap16Bit::setPixelFormat(redMask, greenMask, blueMask);
    TPalette16::setPixelFormat(redMask, greenMask, blueMask);
    g_spriteMaskFirst = redMask;
    g_spriteMaskGreen = greenMask;
    g_spriteMaskLast = blueMask;

    g_firstMaskShift = 0;
    while (!(redMask & 1) && redMask) {
        redMask >>= 1;
        ++g_firstMaskShift;
    }
    g_firstMaskBits = 0;
    while (redMask) {
        ++g_firstMaskBits;
        redMask >>= 1;
    }

    g_greenMaskShift = 0;
    while (!(greenMask & 1) && greenMask) {
        greenMask >>= 1;
        ++g_greenMaskShift;
    }
    g_greenMaskBits = 0;
    while (greenMask) {
        ++g_greenMaskBits;
        greenMask >>= 1;
    }

    g_lastMaskShift = 0;
    while (!(blueMask & 1) && blueMask) {
        blueMask >>= 1;
        ++g_lastMaskShift;
    }
    g_lastMaskBits = 0;
    while (blueMask) {
        ++g_lastMaskBits;
        blueMask >>= 1;
    }
}

VA_COMPGEN(0x0055a7a0, 0x21, SCALAR_DELETING_DTOR,
           t_stdio_file_adapter)
VA_COMPGEN(0x0055a7d0, 0x21, SCALAR_DELETING_DTOR,
           t_lod_file_adapter)

VA(0x0055a800, 0x41F)  // bitmapBorder::SetImage loader; dc 0x121ac8
Bitmap816* ResourceManager::getBitmap816(const char* name)
{
    Bitmap816* cached = static_cast<Bitmap816*>(getFromCache(name));
    if (cached)
        return cached;

    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");

    Bitmap816* result;
    if (file) {
        fclose(file);
        result = new Bitmap816(
            name, g_resourcePath.c_str(),
            g_firstMaskBits, g_firstMaskShift,
            g_greenMaskBits, g_greenMaskShift,
            g_lastMaskBits, g_lastMaskShift);
        if (result)
            addToCache(result);
        return result;
    }

    {
        TResourceArchiveList& archives =
            g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
        int remaining = archives.m_count;
        int* archive = archives.m_indices;
        LODFile* lodFile = &g_resourceLodSlots[*archive].m_file;

        while (!lodFile->pointAt(name)) {
            ++archive;
            if (!--remaining) {
                lodFile = 0;
                break;
            }
            lodFile = &g_resourceLodSlots[*archive].m_file;
        }

        if (!lodFile) {
            game_null_159510(
                DATA_COMPGEN(0x00683030, getBitmap816ErrorContext,
                             "GetBitmap816"),
                RESOURCE_TYPE_BITMAP, name);

            const char* fallbackName = DATA_COMPGEN(
                0x0064108c, defaultBitmap816Name, "default.pcx");
            TResourceArchiveList& fallbackArchives =
                g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
            int fallbackRemaining = fallbackArchives.m_count;
            int* fallbackArchive = fallbackArchives.m_indices;
            lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;

            while (!lodFile->pointAt(fallbackName)) {
                ++fallbackArchive;
                if (!--fallbackRemaining) {
                    lodFile = 0;
                    break;
                }
                lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;
            }

            if (!lodFile) {
                game_null_159510(
                    DATA_COMPGEN(0x00683030, getBitmap816ErrorContext,
                                 "GetBitmap816"),
                    RESOURCE_TYPE_BITMAP, fallbackName);
                return 0;
            }
        }

        {
            struct {
                int m_dataSize;
                int m_width;
                int m_height;
            } bmpHeader;
            lodFile->read(&bmpHeader, sizeof(bmpHeader));
            TAutoArrayPtr<unsigned char> data(
                new unsigned char[bmpHeader.m_dataSize]);
            lodFile->read(data.get(), bmpHeader.m_dataSize);

            TPalette24 palette24;
            lodFile->read(palette24.m_palette, sizeof(palette24.m_palette));
            if (g_graphicsSaturated)
                palette24.adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);

            TPalette16 palette16(
                palette24,
                g_firstMaskBits, g_firstMaskShift,
                g_greenMaskBits, g_greenMaskShift,
                g_lastMaskBits, g_lastMaskShift);

            result = new Bitmap816(
                name, bmpHeader.m_width, bmpHeader.m_height, data.get(),
                &palette16, bmpHeader.m_dataSize);
            if (result)
                result->setPalette(&palette24);
        }

        if (result)
            addToCache(result);
    }

    return result;
}

VA(0x0055ac20, 0x20)
ResourceManager::TCacheMapKey::TCacheMapKey(const char* value)
{
    strncpy(m_name, value, 12);
    m_name[12] = 0;
}

bool ResourceManager::TCacheMapKey::operator<(const TCacheMapKey& other) const
{
    return _stricmp(m_name, other.m_name) < 0;
}

VA(0x0055ac40, 0x388)
Bitmap16Bit* ResourceManager::loadBitmap16(const char* name)
{
    Bitmap16Bit* result = 0;
    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");

    if (file) {
        fclose(file);

        std::auto_ptr<Bitmap24Bit> source(
            new Bitmap24Bit(name, g_resourcePath.c_str()));
        if (g_graphicsSaturated)
            source->adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);

        result = new Bitmap16Bit(
            name, source->getWidth(), source->getHeight());
        source->draw(0, 0, source->getWidth(), source->getHeight(),
                     result, 0, 0);
        return result;
    } else {
        TResourceArchiveList& archives =
            g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
        int remaining = archives.m_count;
        int* archive = archives.m_indices;
        LODFile* lodFile = &g_resourceLodSlots[*archive].m_file;

        while (!lodFile->pointAt(name)) {
            ++archive;
            if (!--remaining) {
                lodFile = 0;
                break;
            }
            lodFile = &g_resourceLodSlots[*archive].m_file;
        }

        if (!lodFile) {
            game_null_159510(
                DATA_COMPGEN(0x00683040, loadBitmap16ErrorContext,
                             "GetBitmap16"),
                RESOURCE_TYPE_BITMAP16, name);

            const char* fallbackName = DATA_COMPGEN(
                0x006410a8, defaultBitmap24Name, "dfault24.pcx");
            TResourceArchiveList& fallbackArchives =
                g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
            int fallbackRemaining = fallbackArchives.m_count;
            int* fallbackArchive = fallbackArchives.m_indices;
            lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;

            while (!lodFile->pointAt(fallbackName)) {
                ++fallbackArchive;
                if (!--fallbackRemaining) {
                    lodFile = 0;
                    break;
                }
                lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;
            }

            if (!lodFile) {
                game_null_159510(
                    DATA_COMPGEN(0x00683040, loadBitmap16ErrorContext,
                                 "GetBitmap16"),
                    RESOURCE_TYPE_BITMAP16, fallbackName);
                return 0;
            }
        }

        TBitmapResourceHeader header;
        lodFile->read(&header, sizeof(header));
        TAutoArrayPtr<unsigned char> data(
            new unsigned char[header.m_dataSize]);
        lodFile->read(data.get(), header.m_dataSize);

        std::auto_ptr<Bitmap24Bit> source(new Bitmap24Bit(
            name, header.m_width, header.m_height, data.get(), header.m_dataSize));
        if (g_graphicsSaturated)
            source->adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);

        result = new Bitmap16Bit(
            name, source->getWidth(), source->getHeight());
        source->draw(0, 0, source->getWidth(), source->getHeight(),
                     result, 0, 0);
        return result;
    }
}

// This getter family is one retail template: only the load callee and result
// type differ. The scoped lookup locals let C1 reuse their frame slots for
// AddToCache's insertion pair, exactly as in GetSpreadsheet below. Restoring
// that helper's std::pair conversion closes every member of the family.
// Complete removes ignore_cache: ECX supplies the name and the body always
// performs the shared cache lookup before loading, then returns with ret.
VA(0x0055afd0, 0x8A)
Bitmap16Bit* ResourceManager::getBitmap16(const char* name)
{
    Bitmap16Bit* cached = static_cast<Bitmap16Bit*>(getFromCache(name));
    if (cached)
        return cached;

    Bitmap16Bit* loaded = loadBitmap16(name);
    if (loaded)
        addToCache(loaded);
    return loaded;
}

// WALL (95.6920%): both resource paths read the exact 24-byte header (DC type
// 0x289e proves plain char[24]) and
// 256-TRGBA payload, construct the shared-slot TPalette24, apply the optional
// saturation transform, and feed the six retail pixel-mask globals to the
// named TPalette16 constructor.  The ordinary path destroys the temporary
// palette before fclose; making the file/archive arms an explicit if/else
// lets C1 color both branch-local palettes at [ebp-0x364] and recovers
// retail's exact 0x758-byte frame (the non-exclusive source used two slots and
// a 0xa7c frame).

// The unwind map settles the LIFETIME question exactly (EH census
// 2026-09-06). Retail's FuncInfo at 0x6528d0 has maxState 8, one try
// block [0..3] with catchHigh 4, and its eight entries mirror the two
// arms three for three: state 1 / 5 destroy the stream adapter
// ([ebp-0x28] file arm, [ebp-0x1c] archive arm), 2 / 6 the TPalette24 at
// [ebp-0x364], 3 / 7 `operator delete([ebp-0x20])` for the half-built
// TPalette16, and states 0 and 4 carry NO action (the try entry and the
// catch itself). Our transcript is [reg,-1,1,2,3,4,2,6,7,8,6] against
// retail's [reg,1,2,3,1,5,6,7,5] - two surplus regions, and both are the
// SAME one: the leading pathname temporary. Retail brackets it with NO
// state store at all (`call operator+`, c_str INLINED to
// `mov eax,[eax+4]` plus its empty-string branch, `call fopen`, then the
// destructor inlined down to a CALLED `_Tidy(1)`), and writes its first
// state only at fn+0x6b, after the `if (file)` test. We write state=esi
// at +0x41 and state=-1 at +0x5a because the retained inline_depth(0)
// leaves a real `call c_str` inside the temporary's lifetime. So the
// missing shape is "expand c_str AND the temporary's destructor, but
// call _Tidy" - which no placement of the existing pin reaches, and
// which is why the whole tail of both arms is numbered one state high.

// The leading string temporary is the bounded residual shared with LoadFont
// and GetPalette24. Retail inlines c_str and the parent destructor but calls
// _Tidy(true); inline_depth(0), retained below, calls both parents, while no
// pin and function-wide auto_inline(off) expand the full teardown. A named
// scoped string and a const-reference lifetime are worse as well. why-reg v2
// finds the same three first definitions/pseudos but a C1-state ESI/EDI
// processing-order permutation; its only legal declaration-order probe
// worsens the register-visible distance 67 -> 75. The surviving 22-vs-24
// block split and 22-vs-21 call count are therefore inliner/front-end walls,
// not missing resource behavior.
VA(0x0055b060, 0x377)  // public GetPalette callee + retail conversion tuple
TPalette16* ResourceManager::loadPalette(const char* name)
{
    char header[24];
    TRGBA paletteData[256];
#pragma inline_depth(0)
    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");
#pragma inline_depth()

    if (file) {
        try {
            t_stdio_file_adapter stream(file);
            TAbstractFile* streamInterface = &stream;
            streamInterface->read(header, sizeof(header));
            streamInterface->read(paletteData, sizeof(paletteData));

            TPalette16* result;
            {
                TPalette24 palette24(paletteData);
                if (g_graphicsSaturated)
                    palette24.adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);

                result = new TPalette16(
                    name, palette24,
                    g_firstMaskBits, g_firstMaskShift,
                    g_greenMaskBits, g_greenMaskShift,
                    g_lastMaskBits, g_lastMaskShift);
            }

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    } else {
        TResourceArchiveList& archives =
            g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
        int remaining = archives.m_count;
        int* archive = archives.m_indices;
        LODFile* lodFile = &g_resourceLodSlots[*archive].m_file;

        while (!lodFile->pointAt(name)) {
            ++archive;
            if (!--remaining) {
                lodFile = 0;
                break;
            }
            lodFile = &g_resourceLodSlots[*archive].m_file;
        }

        if (!lodFile) {
            game_null_159510(
                DATA_COMPGEN(0x0068304c, loadPaletteErrorContext,
                             "GetPalette"),
                RESOURCE_TYPE_PALETTE, name);

            const char* fallbackName = DATA_COMPGEN(
                0x006410b8, defaultPalette16Name, "default.pal");
            TResourceArchiveList& fallbackArchives =
                g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
            int fallbackRemaining = fallbackArchives.m_count;
            int* fallbackArchive = fallbackArchives.m_indices;
            lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;

            while (!lodFile->pointAt(fallbackName)) {
                ++fallbackArchive;
                if (!--fallbackRemaining) {
                    lodFile = 0;
                    break;
                }
                lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;
            }

            if (!lodFile) {
                game_null_159510(
                    DATA_COMPGEN(0x0068304c, loadPaletteErrorContext,
                                 "GetPalette"),
                    RESOURCE_TYPE_PALETTE, fallbackName);
                return 0;
            }
        }

        t_lod_file_adapter stream(lodFile);
        TAbstractFile* streamInterface = &stream;
        streamInterface->read(header, sizeof(header));
        streamInterface->read(paletteData, sizeof(paletteData));

        TPalette24 palette24(paletteData);
        if (g_graphicsSaturated)
            palette24.adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);

        return new TPalette16(
            name, palette24,
            g_firstMaskBits, g_firstMaskShift,
            g_greenMaskBits, g_greenMaskShift,
            g_lastMaskBits, g_lastMaskShift);
    }
}

// Like GetBitmap16, Complete always consults the cache and removes the
// Dreamcast ignore_cache argument; the retained body ends with plain ret.
VA(0x0055b3e0, 0x8A)  // dc public GetPalette + retail getter family, dc 0x121d90
TPalette16* ResourceManager::getPalette(const char* name)
{
    TPalette16* cached = static_cast<TPalette16*>(getFromCache(name));
    if (cached)
        return cached;

    TPalette16* loaded = loadPalette(name);
    if (loaded)
        addToCache(loaded);
    return loaded;
}

// Residual (99.9273%): Dreamcast's two named locals are restored literally as
// char header[24] (type 0x289e) then TRGBA rgba[256] (type 0x289f), and its
// header-read, rgba-read, direct TPalette24 construction and optional AdjustHSV
// order is preserved in both retail-corroborated Complete paths. The ordinary/
// archive adapters, fallback search, reporter calls, allocation and cleanup
// semantics agree. Candidate and retail each contain 220 body instructions,
// 24 blocks and the same 14-branch sequence; paired calls and data relocations
// agree. OpenResourcePath closes the earlier string-temporary midpoint.

// The remainder is only stack coloring: candidate frame 0x444 versus retail
// 0x43c, because retail overlaps eight bytes of the destroyed path-string slot
// with the later stdio adapter while this C1 state does not. A generated
// 120-form declaration/result/file-lifetime tree is flat at this peak except
// explicit zero initialization (93.10), and eight natural helper bodies leave
// the direct expression best. Renewed tests are also bounded: branch-local
// result declarations, result/file reordering, a shared interface pointer, an
// interface-before-adapter declaration and a named fopen result are byte-flat;
// an explicit ordinary/archive else is 93.1045, direct adapter calls are
// 96.8864, and a named path local is 99.8773 while losing exact LoadFont.
// why-reg's nine legal probes are flat or worse. Treat this as TU/C1 state, not
// permission to remove the Dreamcast-proven local names or statement shape.
// Residual (99.9273%): the frame is 8 bytes too large and every slot below
// the two read buffers is shifted with it. Retail OVERLAYS the block-scoped
// `t_stdio_file_adapter stream` onto the dead `gResourcePath + name` string
// temporary (both at [ebp-0x28]) and lands `result` at [ebp-0x20]; this
// compile gives the adapter its own pair at [ebp-0x24]/[ebp-0x20] and puts
// `result` below it, which pushes header/rgba down by 8. Measured and
// rejected 2026-09-05: `result` declared after `file` 99.93 (byte-flat),
// the adapter hoisted above the `try` 98.73, both together 98.73. The two
// read buffers cannot be block-scoped - the LOD path below reads through
// them too.
VA(0x0055b470, 0x2D1)  // dc/hd public identity + retail palette-file shape, dc 0x121ec8
TPalette24* ResourceManager::getPalette24(const char* name)
{
    TPalette24* result;
    char header[24];
    TRGBA rgba[256];
    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");
    if (file) {
        try {
            t_stdio_file_adapter stream(file);
            TAbstractFile* streamInterface = &stream;
            streamInterface->read(header, sizeof(header));
            streamInterface->read(rgba, sizeof(rgba));

            result = new TPalette24(rgba);
            if (g_graphicsSaturated)
                result->adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    }

    TResourceArchiveList& archives =
        g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
    int remaining = archives.m_count;
    int* archive = archives.m_indices;
    LODFile* lodFile = &g_resourceLodSlots[*archive].m_file;

    while (!lodFile->pointAt(name)) {
        ++archive;
        if (!--remaining) {
            lodFile = 0;
            break;
        }
        lodFile = &g_resourceLodSlots[*archive].m_file;
    }

    if (!lodFile) {
        game_null_159510(
            DATA_COMPGEN(0x0068304c, loadPaletteErrorContext, "GetPalette"),
            RESOURCE_TYPE_PALETTE, name);

        const char* fallbackName =
            DATA_COMPGEN(0x006410c4, defaultPaletteName, "default.pal");
        TResourceArchiveList& fallbackArchives =
            g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
        int fallbackRemaining = fallbackArchives.m_count;
        int* fallbackArchive = fallbackArchives.m_indices;
        lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;

        while (!lodFile->pointAt(fallbackName)) {
            ++fallbackArchive;
            if (!--fallbackRemaining) {
                lodFile = 0;
                break;
            }
            lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;
        }

        if (!lodFile) {
            game_null_159510(
                DATA_COMPGEN(0x0068304c, loadPaletteErrorContext,
                             "GetPalette"),
                RESOURCE_TYPE_PALETTE, fallbackName);
            return 0;
        }
    }

    // Complete routes Dreamcast's two direct LODFile::read calls through the
    // exact t_lod_file_adapter::Read receiver. Retail proves the vtable owner
    // at 0x641128 and its slot-1 target at 0x559110; keeping this adapter is a
    // revision fact, not permission to flatten the older helper operation.
    t_lod_file_adapter stream(lodFile);
    TAbstractFile* streamInterface = &stream;
    streamInterface->read(header, sizeof(header));
    streamInterface->read(rgba, sizeof(rgba));

    result = new TPalette24(rgba);
    if (g_graphicsSaturated)
        result->adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
    return result;
}

VA(0x0055b750, 0x17A)
font* ResourceManager::loadFontData(const char* name, TAbstractFile* stream,
                                    int fileSize)
{
    font::TFontSpec spec;
    stream->read(&spec, sizeof(spec));

    int dataSize = fileSize - sizeof(spec);
    std::auto_ptr<unsigned char> data(new unsigned char[dataSize]);
    stream->read(data.get(), dataSize);

    std::auto_ptr<font> result(
        new font(name, spec, dataSize, data.get()));
    data = std::auto_ptr<unsigned char>(0);

    TPalette16* palette = getPalette("game.pal");
    if (palette) {
        try {
            result.get()->setPalette(palette);
        }
        catch (...) {
            palette->dispose();
            throw;
        }
        palette->dispose();
    }

    return result.release();
}

VA(0x0055b8d0, 0x229)
font* ResourceManager::loadFont(const char* name)
{
    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");

    if (file) {
        try {
            fseek(file, 0, SEEK_END);
            int fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            t_stdio_file_adapter stream(file);
            TAbstractFile* streamInterface = &stream;
            font* result = loadFontData(name, streamInterface, fileSize);

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    }

    LODFile* lodFile = 0;
    TResourceArchiveList& archives =
        g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
    int remaining = archives.m_count;
    int* archive = archives.m_indices;
    lodFile = &g_resourceLodSlots[*archive].m_file;

    while (!lodFile->pointAt(name)) {
        ++archive;
        if (!--remaining) {
            lodFile = 0;
            break;
        }
        lodFile = &g_resourceLodSlots[*archive].m_file;
    }

    if (!lodFile) {
        game_null_159510(
            DATA_COMPGEN(0x00683058, loadFontErrorContext, "GetFont"),
            RESOURCE_TYPE_FONT, name);

        const char* fallbackName =
            DATA_COMPGEN(0x006410d0, defaultFontName, "default.fnt");
        TResourceArchiveList& fallbackArchives =
            g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
        int fallbackRemaining = fallbackArchives.m_count;
        int* fallbackArchive = fallbackArchives.m_indices;
        lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;

        while (!lodFile->pointAt(fallbackName)) {
            ++fallbackArchive;
            if (!--fallbackRemaining) {
                lodFile = 0;
                break;
            }
            lodFile = &g_resourceLodSlots[*fallbackArchive].m_file;
        }

        if (!lodFile) {
            game_null_159510(
                DATA_COMPGEN(0x00683058, loadFontErrorContext, "GetFont"),
                RESOURCE_TYPE_FONT, fallbackName);
            return 0;
        }
    }

    int fileSize = lodFile->getItemIndex(name)->m_size;
    t_lod_file_adapter stream(lodFile);
    TAbstractFile* streamInterface = &stream;
    return loadFontData(name, streamInterface, fileSize);
}

VA(0x0055bb00, 0x8A)
font* ResourceManager::getFont(const char* name)
{
    font* cached = static_cast<font*>(getFromCache(name));
    if (cached)
        return cached;

    font* loaded = loadFont(name);
    if (loaded)
        addToCache(loaded);
    return loaded;
}

VA(0x0055bb90, 0x240)
TTextResource* ResourceManager::loadText(const char* name)
{
    FILE* file = fopen(
        (g_resourcePath + name).c_str(),
        DATA_COMPGEN(0x00677d6c, resourceReadMode, "rb"));

    if (file) {
        try {
            fseek(file, 0, SEEK_END);
            int fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            TTextResource* result;
            {
                t_stdio_file_adapter stream(file);
                std::auto_ptr<char> data(new char[fileSize]);
                TAbstractFile* streamInterface = &stream;
                streamInterface->read(data.get(), fileSize);
                result = new TTextResource(name, fileSize, data.get());
            }

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    }

    TResourceArchiveList& archives =
        g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
    int remaining = archives.m_count;
    int* archive = archives.m_indices;
    LODFile* lodFile = &g_resourceLodSlots[*archive].m_file;

    while (!lodFile->pointAt(name)) {
        ++archive;
        if (!--remaining) {
            lodFile = 0;
            break;
        }
        lodFile = &g_resourceLodSlots[*archive].m_file;
    }

    if (!lodFile) {
        game_null_159510(
            DATA_COMPGEN(0x00683060, loadTextErrorContext, "GetText"),
            RESOURCE_TYPE_TEXT, name);
        return 0;
    }

    int fileSize = lodFile->getItemIndex(name)->m_size;
    t_lod_file_adapter stream(lodFile);
    std::auto_ptr<char> data(new char[fileSize]);
    TAbstractFile* streamInterface = &stream;
    streamInterface->read(data.get(), fileSize);
    return new TTextResource(name, fileSize, data.get());
}

VA(0x0055bdd0, 0x8A)
TTextResource* ResourceManager::getText(const char* name)
{
    TTextResource* cached = static_cast<TTextResource*>(getFromCache(name));
    if (cached)
        return cached;

    TTextResource* loaded = loadText(name);
    if (loaded)
        addToCache(loaded);
    return loaded;
}

VA(0x0055be60, 0x240)
TSpreadsheetResource* ResourceManager::loadSpreadsheet(const char* name)
{
    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");

    if (file) {
        try {
            fseek(file, 0, SEEK_END);
            int fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            TSpreadsheetResource* result;
            {
                t_stdio_file_adapter stream(file);
                std::auto_ptr<char> data(new char[fileSize]);
                TAbstractFile* streamInterface = &stream;
                streamInterface->read(data.get(), fileSize);
                result =
                    new TSpreadsheetResource(name, fileSize, data.get());
            }

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    }

    TResourceArchiveList& archives =
        g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
    int remaining = archives.m_count;
    int* archive = archives.m_indices;
    LODFile* lodFile = &g_resourceLodSlots[*archive].m_file;

    while (!lodFile->pointAt(name)) {
        ++archive;
        if (!--remaining) {
            lodFile = 0;
            break;
        }
        lodFile = &g_resourceLodSlots[*archive].m_file;
    }

    if (!lodFile) {
        game_null_159510(
            DATA_COMPGEN(0x00683068, loadSpreadsheetErrorContext,
                         "GetSpreadsheet"),
            RESOURCE_TYPE_TEXT, name);
        return 0;
    }

    int fileSize = lodFile->getItemIndex(name)->m_size;
    t_lod_file_adapter stream(lodFile);
    std::auto_ptr<char> data(new char[fileSize]);
    TAbstractFile* streamInterface = &stream;
    streamInterface->read(data.get(), fileSize);
    return new TSpreadsheetResource(name, fileSize, data.get());
}

VA(0x0055c0a0, 0x8A)  // dc 0x122164
TSpreadsheetResource* ResourceManager::getSpreadsheet(const char* name)
{
    TSpreadsheetResource* cached = static_cast<TSpreadsheetResource*>(getFromCache(name));
    if (cached)
        return cached;

    TSpreadsheetResource* loaded = loadSpreadsheet(name);
    if (loaded)
        addToCache(loaded);
    return loaded;
}

#if 0 // @carcass - remaining Dreamcast roster rows

// E:\gamedcs\resourcemanager.cpp:1566
DC_ONLY(0x1221fc, 0xE4)
unsigned char ResourceManager::getSoundFile(char* localName, void** data, SoundHeaderStruct** snd, int* size)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:1606
DC_ONLY(0x1222e0, 0x40)
sample* ResourceManager::getSample(const char* name)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:2080
DC_ONLY(0x122434, 0x4E)
void ResourceManager::getBackdrop(const char* resName, Bitmap16Bit* destBmap)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:2110
DC_ONLY(0x122484, 0x16)
unsigned char ResourceManager::pointToSpriteResource(const char* name)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:2115
// DC reads through the single global SpriteResourceFile. Complete removes
// that implicit stream: getSprite (0x55c7b0) selects an archive with
// pointToSpriteResource (0x55cf50), then reads the returned LODFile directly.
// The old two-argument stateful read is retired; see config/dc_only.tsv.
DC_ONLY(0x12249c, 0x18)
int ResourceManager::ReadFromSpriteResource(void* data, int numBytes)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:2120
DC_ONLY(0x1224b4, 0x16)
unsigned char ResourceManager::pointToBitmapResource(const char* name)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:2125
DC_ONLY(0x1224cc, 0x18)
int ResourceManager::readFromBitmapResource(void* data, int numBytes)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:2130
DC_ONLY(0x1224e4, 0x4C)
int ResourceManager::getBitmapResourceSize(const char* name)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\resourcemanager.cpp:2141, dc 0x122530.
// Complete routes disposal through the resource virtual method.
void ResourceManager::dispose(resource* value) { value->dispose(); }

// Original: ResourceManager::Dispose; resourcemanager.cpp:2196, dc 0x1225c0
// DC releases a ds_engine sample-cache entry. Complete's sample owns its
// sound data and inherits reference-counted resource disposal (0x55d0f0).
void ResourceManager::dispose(sample* value)
{
    if (value)
        value->dispose();
}

// E:\gamedcs\resourcemanager.cpp:2204, dc 0x1225dc.
// Complete routes disposal through the resource virtual method.
void ResourceManager::dispose(CSprite* value) { value->dispose(); }

// E:\gamedcs\resourcemanager.cpp:2280, dc 0x1226d4.
// Complete retains no work at the cache-sweep call sites.
void ResourceManager::delSprFromCache()
{
}

// Original: ResourceManager::Expunge; resourcemanager.cpp:2359, dc 0x1228ac
void ResourceManager::expunge()
{
    TCacheMap::iterator position = g_resourceCache.begin();
    while (position != g_resourceCache.end()) {
        resource* value = position->second;
        if (value)
            delete value;
        ++position;
    }

    g_resourceCache.clear();
}

// A cache hit adds a reference before returning the resource.
resource* ResourceManager::getFromCache(const char* name)
{
    TCacheMap::iterator found = g_resourceCache.find(name);
    if (found == g_resourceCache.end())
        return 0;
    resource* value = found->second;
    value->addRef();
    return value;
}

// Original: ResourceManager::Report; resourcemanager.cpp:2404, dc 0x1229f8
// Optimized release hook: the executable body is only return true.
unsigned char ResourceManager::report(const char* filename)
{
    return 1;
}

#if 0  // @carcass

// E:\gamedcs\resourcemanager.cpp:2397
DC_ONLY(0x122984, 0x72)
void ResourceManager::addToCache(resource* r)
{
    // @stub
}













// E:\gamedcs\resourcemanager.cpp:121
DC_ONLY(0x122bd0, 0x28)
void ResourceManager::TCacheMapKey::TCacheMapKey(const char* n)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:126
DC_ONLY(0x122bf8, 0x1C)
unsigned char ResourceManager::TCacheMapKey::operator<(const ResourceManager::TCacheMapKey* y)
{
    // @stub
}

// E:\gamedcs\resourcemanager.cpp:136
DC_ONLY(0x122c14, 0x18)
void std::map<ResourceManager::TCacheMapKey,resource *,std::less<ResourceManager::T()
{
    // @stub
}

#endif

namespace ResourceManager {
bool getSoundFile(const char* localName, std::auto_ptr<char>& data, int* size);
}

DATA(0x0069e500)
TSoundHeaderDescriptor g_soundHeaderDescriptors[3];

// ECX/EDX carry name/auto_ptr and ret 4 removes the size output. The direct
// Win32 file read replaces Dreamcast's separate data/header outputs.
VA(0x0055c130, 0x28F)  // dc GetSoundFile + caller/record layout, dc 0x1221fc
bool ResourceManager::getSoundFile(const char* localName,
                                   std::auto_ptr<char>& data,
                                   int* size)
{
    DWORD bytesRead;
    std::string soundName = localName;
    std::string::size_type extension = soundName.find('.');
    if (extension != std::string::npos)
        soundName.erase(extension);

    TResourceArchiveContext* context =
        &g_resourceArchiveContexts[*g_videoGameState];
    int remaining = context->m_sounds.m_count;
    int* archive = context->m_sounds.m_indices;
    int x;

    do {
        x = 0;
        TSoundHeaderDescriptor& descriptor =
            g_soundHeaderDescriptors[*archive];
        for (; x < *descriptor.m_count; ++x) {
            if (_stricmp((*descriptor.m_sounds)[x].m_filename,
                         soundName.c_str()) == 0) {
                SoundHeaderStruct& header = (*descriptor.m_sounds)[x];
                *size = header.m_size;
                data = std::auto_ptr<char>(new char[*size]);
                SetFilePointer(*descriptor.m_file, header.m_offset, 0,
                               FILE_BEGIN);
                ReadFile(*descriptor.m_file, data.get(), *size, &bytesRead, 0);
                return true;
            }
        }

        ++archive;
    } while (--remaining);

    return false;
}

namespace ResourceManager {
sample* loadSample(const char* name);
}

VA(0x0055c3c0, 0x356)  // GetSample callee + GetSoundFile/default.wav graph
sample* ResourceManager::loadSample(const char* name)
{
    FILE* file = fopen(
        (g_resourcePath + name).c_str(),
        DATA_COMPGEN(0x00677d6c, sampleReadMode, "rb"));

    if (file) {
        sample* result;
        try {
            {
                fseek(file, 0, SEEK_END);
                int size = ftell(file);
                fseek(file, 0, SEEK_SET);
                std::auto_ptr<char> data(new char[size]);
                fread(data.get(), size, 1, file);
                result = new sample(name, data.get(), size, 0, 127, 1);
            }
            fclose(file);
            return result;
        } catch (...) {
            fclose(file);
            throw;
        }
    }

    std::auto_ptr<char> data;
    int size;
    if (!getSoundFile(name, data, &size)) {
        {
            std::ostringstream message;
            message
                << DATA_COMPGEN(0x00682f18, sampleErrorPrefix,
                                "ResourceManager::")
                << DATA_COMPGEN(0x00683078, getSampleErrorContext, "GetSample")
                << DATA_COMPGEN(0x00682f2c, missingResourcePrefix,
                                " could not find the \"")
                << DATA_COMPGEN(0x00683084, sampleResourceKind, "sfx")
                << DATA_COMPGEN(0x00682f44, missingResourceMiddle, "\" resource \"")
                << name
                << DATA_COMPGEN(0x00682f54, missingResourceSuffix, "\".");
            MessageBoxA(
                GetForegroundWindow(), message.str().c_str(),
                DATA_COMPGEN(0x00682f08, resourceManagerCaption,
                             "ResourceManager"),
                0);
        }
        const char* fallbackName = DATA_COMPGEN(
            0x006410dc, defaultSampleName, "default.wav");
        if (!getSoundFile(fallbackName, data, &size)) {
            {
                std::ostringstream message;
                message
                    << DATA_COMPGEN(0x00682f18, sampleErrorPrefix,
                                    "ResourceManager::")
                    << DATA_COMPGEN(0x00683078, getSampleErrorContext, "GetSample")
                    << DATA_COMPGEN(0x00682f2c, missingResourcePrefix,
                                    " could not find the \"")
                    << DATA_COMPGEN(0x00683084, sampleResourceKind, "sfx")
                    << DATA_COMPGEN(0x00682f44, missingResourceMiddle, "\" resource \"")
                    << fallbackName
                    << DATA_COMPGEN(0x00682f54, missingResourceSuffix, "\".");
                MessageBoxA(
                    GetForegroundWindow(), message.str().c_str(),
                    DATA_COMPGEN(0x00682f08, resourceManagerCaption,
                                 "ResourceManager"),
                    0);
            }
            return 0;
        }
    }

    return new sample(name, data.get(), size, 0, 127, 1);
}

VA(0x0055c720, 0x8A)  // dc 0x1222e0
sample* ResourceManager::getSample(const char* name)
{
    sample* cached = static_cast<sample*>(getFromCache(name));
    if (cached)
        return cached;

    sample* loaded = loadSample(name);
    if (loaded)
        addToCache(loaded);
    return loaded;
}

// Original: addPal16; csprite.cpp:978, dc 0x73b64.
// Complete moved DEF parsing from CSprite::SpriteDataReload into getSprite.
// This ordinary attachment helper moves with that operation; its expansion
// retains deletion of the old palette and construction from the new value.
// Complete's palette copy interface takes a pointer, where DC takes a ref.
void addPal16(CSprite* sprite, const TPalette16* pal)
{
    if (sprite->m_p)
        delete sprite->m_p;
    sprite->m_p = new TPalette16(pal);
}

// Original: addPal24; csprite.cpp:986, dc 0x73bac.
void addPal24(CSprite* sprite, const TPalette24* pal)
{
    if (sprite->m_p24)
        delete sprite->m_p24;
    sprite->m_p24 = new TPalette24(pal);
}

// Dreamcast GetSprite (dc 0x122320) proves GetFromCache, SpriteDefHeader
// Sdef, the archive load and AddToCache. Complete adds the DEF sequence/frame
// parsing and a second GetFromCache call for each frame name.
// The prior flattened cache model reached 88.8564% (kept in HIST); its frame
// loop still chose frameIndex instead of retail's persistent zero for ESI.
// Hoisting sequenceNumber, named/default key construction, and declaration,
// register and loop controls did not fix that allocation. With the real map
// and shared cache helpers the current 84.2097% also reflects tree::find
// expanding at the frame lookup and a retained insertion-result pair ctor.
// Sixteen key/pair-construction controls preserve at best that current score;
// named-key locals and explicit-key insertion pairs are worse. Recover the
// nested compiler decisions through these helpers, not copied lookup bodies.
VA(0x0055c7b0, 0x743)  // anchor-caller/body records, dc 0x122320; wall
CSprite* ResourceManager::getSprite(const char* name)
{
    CSprite* cached = static_cast<CSprite*>(getFromCache(name));
    if (cached)
        return cached;

    LODFile* lodFile = pointToSpriteResource(name);

    if (!lodFile) {
        game_sprite_1599e0(
            DATA_COMPGEN(0x00683088, getSpriteErrorContext, "GetSprite"),
            RESOURCE_TYPE_SPRITE, name);

        lodFile = pointToSpriteResource(name);

        if (!lodFile) {
            game_sprite_1599e0(
                DATA_COMPGEN(0x00683088, getSpriteErrorContext, "GetSprite"),
                RESOURCE_TYPE_SPRITE, name);
            return 0;
        }
    }

    LODEntry* entry = lodFile->getItemIndex(name);
    unsigned char* fileData = new unsigned char[entry->m_size];
    lodFile->read(fileData, entry->m_size);

    SpriteDefHeader sdef;
    unsigned char* definitionPosition = fileData + sizeof(sdef);
    memcpy(&sdef, fileData, sizeof(sdef));

    CSprite* sprite = new CSprite(
        name, sdef.m_type, sdef.m_width, sdef.m_height);
    if (!sprite)
        return 0;

    TSpriteDataHeader* sequences =
        new TSpriteDataHeader[sdef.m_numSequences];

    int sequenceIndex;
    for (sequenceIndex = 0;
         sequenceIndex < sdef.m_numSequences;
         ++sequenceIndex) {
        TSpriteDataHeader& sequence = sequences[sequenceIndex];
        memcpy(&sequence, definitionPosition, sizeof(sequence));
        definitionPosition += sizeof(sequence);

        sequence.m_frameNames = new char[sequence.m_numFrames * 13];
        memcpy(sequence.m_frameNames, definitionPosition,
               sequence.m_numFrames * 13);
        definitionPosition += sequence.m_numFrames * 13;

        sequence.m_frameOffsets = new int[sequence.m_numFrames];
        memcpy(sequence.m_frameOffsets, definitionPosition,
               sequence.m_numFrames * sizeof(int));
        definitionPosition += sequence.m_numFrames * sizeof(int);
    }

    for (sequenceIndex = 0;
         sequenceIndex < sdef.m_numSequences;
         ++sequenceIndex) {
        TSpriteDataHeader& sequence = sequences[sequenceIndex];
        sprite->allocateSeq(sequence.m_sequenceNumber, sequence.m_numFrames);

        int frameIndex = 0;
        if (frameIndex < sequence.m_numFrames) {
            int frameNameOffset = 0;
            do {
            TCompactSpriteFrameHeader compactHeader;
            TCroppedSpriteFrameHeader croppedHeader;
            unsigned char* frameData;
            unsigned char* frameSource;
            int frameDataSize;

            if (sdef.m_type == RESOURCE_TYPE_SPRITE ||
                sdef.m_type == RESOURCE_TYPE_CREATURE ||
                sdef.m_type == RESOURCE_TYPE_ADVENTURE_OBJECT ||
                sdef.m_type == RESOURCE_TYPE_HERO ||
                sdef.m_type == RESOURCE_TYPE_INTERFACE ||
                sdef.m_type == RESOURCE_TYPE_TILESET ||
                sdef.m_type == RESOURCE_TYPE_POINTER ||
                sdef.m_type == RESOURCE_TYPE_COMBAT_HERO) {
                unsigned char* source =
                    fileData + sequence.m_frameOffsets[frameIndex];
                memcpy(&croppedHeader, source, sizeof(croppedHeader));
                frameDataSize = croppedHeader.m_dataSize;
                frameData = new unsigned char[frameDataSize];
                frameSource = source + sizeof(croppedHeader);
            } else {
                memcpy(&compactHeader, definitionPosition,
                       sizeof(compactHeader));
                definitionPosition += sizeof(compactHeader);
                frameDataSize = compactHeader.m_dataSize;
                frameData = new unsigned char[frameDataSize];
                frameSource =
                    fileData + sequence.m_frameOffsets[frameIndex];
            }
            memcpy(frameData, frameSource, frameDataSize);

            CSpriteFrame* frame = static_cast<CSpriteFrame*>(getFromCache(
                sequence.m_frameNames + frameNameOffset));

            if (!frame) {
                if (sdef.m_type == RESOURCE_TYPE_SPRITE ||
                    sdef.m_type == RESOURCE_TYPE_CREATURE ||
                    sdef.m_type == RESOURCE_TYPE_ADVENTURE_OBJECT ||
                    sdef.m_type == RESOURCE_TYPE_HERO ||
                    sdef.m_type == RESOURCE_TYPE_INTERFACE ||
                    sdef.m_type == RESOURCE_TYPE_TILESET ||
                    sdef.m_type == RESOURCE_TYPE_POINTER ||
                    sdef.m_type == RESOURCE_TYPE_COMBAT_HERO) {
                    frame = new CSpriteFrame(
                        sequence.m_frameNames + frameNameOffset,
                        croppedHeader.m_width, croppedHeader.m_height,
                        frameData, croppedHeader.m_dataSize,
                        croppedHeader.m_encoding,
                        croppedHeader.m_croppedWidth,
                        croppedHeader.m_croppedHeight,
                        croppedHeader.m_croppedX, croppedHeader.m_croppedY);
                } else {
                    frame = new CSpriteFrame(
                        sequence.m_frameNames + frameNameOffset,
                        compactHeader.m_width, compactHeader.m_height,
                        frameData, compactHeader.m_dataSize,
                        croppedHeader.m_encoding);
                }

                addToCache(frame);
            }

            sprite->addFrame(sequence.m_sequenceNumber, frame);
            delete[] frameData;
            frameNameOffset += 13;
                ++frameIndex;
            } while (frameIndex < sequence.m_numFrames);
        }
    }

    for (sequenceIndex = 0;
         sequenceIndex < sdef.m_numSequences;
         ++sequenceIndex) {
        delete[] sequences[sequenceIndex].m_frameNames;
        delete[] sequences[sequenceIndex].m_frameOffsets;
    }
    delete[] sequences;

    TPalette24 palette24(sdef.m_palette);
    if (g_graphicsSaturated)
        palette24.adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);

    TPalette16 palette16(
        palette24,
        g_firstMaskBits, g_firstMaskShift,
        g_greenMaskBits, g_greenMaskShift,
        g_lastMaskBits, g_lastMaskShift);

    addPal16(sprite, &palette16);
    addPal24(sprite, &palette24);

    delete[] fileData;
    addToCache(sprite);
    return sprite;
}

// The pointee's exact domain name is not yet proven; retail consumers agree
// that this global points at the small active game/resource-context ordinal.
DATA(0x0069923c)
int* g_videoGameState;

DATA(0x0069d870)
TResourceLODSlot g_resourceLodSlots[8] = {
    DATA_COMPGEN(0x00682ef8, resourceBitmapArchiveName, "h3bitmap.lod"),
    DATA_COMPGEN(0x00682ee8, resourceSpriteArchiveName, "h3sprite.lod"),
    DATA_COMPGEN(0x00682ed8, resourceAbBitmapArchiveName, "h3ab_bmp.lod"),
    DATA_COMPGEN(0x00682ec8, resourceAbSpriteArchiveName, "h3ab_spr.lod"),
    DATA_COMPGEN(0x00682eb8, resourcePBitmapArchiveName, "h3pbitma.lod"),
    DATA_COMPGEN(0x00682ea8, resourcePSpriteArchiveName, "h3psprit.lod"),
    DATA_COMPGEN(0x00682e98, resourceAbPBitmapArchiveName, "h3abp_bm.lod"),
    DATA_COMPGEN(0x00682e88, resourceAbPSpriteArchiveName, "h3abp_sp.lod")
};

DATA(0x0069e538)
TResourceArchiveContext g_resourceArchiveContexts[4];

VA(0x0055cf00, 0x4B)
void ResourceManager::getBackdrop(const char* resName, Bitmap16Bit* destBmap)
{
    Bitmap816* source = getBitmap816(resName);
    if (source) {
        source->draw(0, 0, source->getWidth(), source->getHeight(),
                     destBmap, 0, 0, false);
        source->dispose();
    } else {
        game_null_159510(
            DATA_COMPGEN(0x00683094, getBackdropErrorContext, "GetBackdrop"),
            RESOURCE_TYPE_BITMAP, resName);
    }
}

VA(0x0055cf50, 0x83)
LODFile* ResourceManager::pointToSpriteResource(const char* name)
{
    TResourceArchiveList& archives =
        g_resourceArchiveContexts[*g_videoGameState].m_sprites;
    int remaining = archives.m_count;
    int* archive = archives.m_indices;
    LODFile* file = &g_resourceLodSlots[*archive].m_file;

    while (!file->pointAt(name)) {
        ++archive;
        if (!--remaining)
            return 0;
        file = &g_resourceLodSlots[*archive].m_file;
    }

    return file;
}

VA(0x0055cfe0, 0x83)  // bitmap-field twin of PointToSpriteResource
LODFile* ResourceManager::pointToBitmapResource(const char* name)
{
    TResourceArchiveList& archives =
        g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
    int remaining = archives.m_count;
    int* archive = archives.m_indices;
    LODFile* file = &g_resourceLodSlots[*archive].m_file;

    while (!file->pointAt(name)) {
        ++archive;
        if (!--remaining)
            return 0;
        file = &g_resourceLodSlots[*archive].m_file;
    }

    return file;
}

// Dreamcast's direct method (dc 0x1224e4) is one source statement with no
// recorded locals; retail expands it into this three-block Complete archive
// walk. Candidate and retail have the same 30/31-instruction sequence. Retail
// retains a dead `lea` of the selected 24-byte context after loading the
// bitmap list; pointer/reference/accessor spellings remove it, a named context
// reference is byte-flat, and a by-value spelling performs a real 24-byte
// copy. why-reg's model finds no binding divergence and its guided volatile
// probe is worse (5 rather than 3 masked slots), so the residual is a bounded
// C1 dead-address-materialization wall.
// Twenty-one further source candidates combine context/list references,
// pointers, list snapshots, and while/do/for lookup loops. Their maximum is
// still 96.7742%; the canonical archive model and original loop are retained.
VA(0x0055d070, 0x5C)  // retail archive-list walk + dc/hd name corroboration
int ResourceManager::getBitmapResourceSize(const char* name)
{
    TResourceArchiveList& archives =
        g_resourceArchiveContexts[*g_videoGameState].m_bitmaps;
    int* archive = archives.m_indices;
    LODEntry* entry = g_resourceLodSlots[*archive].m_file.getItemIndex(name);
    while (!entry)
        entry = g_resourceLodSlots[*++archive].m_file.getItemIndex(name);

    return entry->m_size;
}

VA(0x0055d0d0, 0x11)  // caller-family merge + explicit LOD receiver/ret 4, dc 0x1224cc
int ResourceManager::readFromBitmapResource(LODFile* resource, void* data,
                                             int numBytes)
{
    return resource->read(data, numBytes);
}

VA(0x0055d0f0, 0xA1)  // resource vslot 1 + cache-key/lower-bound proof
void resource::dispose()
{
    if (this) {
        release();
        if (getReferenceCount() == 0) {
            ResourceManager::TCacheMap::iterator found =
                g_resourceCache.find(getName());
            if (found != g_resourceCache.end()) {
                g_resourceCache.erase(found);
                delete this;
            }
        }
    }
}

// CSprite's retail vtable fixes this Complete-only override as slot 1.
// DC CSprite type 0x17d3 / fields 0x17d4 has no Dispose virtual; its
// ResourceManager disposal functions supply the older frame-walk semantics.
// Retail proves that GetNumSeqs consumes resType and
// that every live frame is released before the base cache-removal path. The
// complete 25-block / 280-byte body is exact, including the map/tree iterator
// return boundary retained inside the inlined base disposal.
VA(0x0055d1a0, 0x118)
void CSprite::dispose()
{
    if (this) {
        release();
        if (getReferenceCount() == 0) {
            int sequenceCount = getNumSeqs(getResType());
            for (int sequence = 0; sequence < sequenceCount; ++sequence) {
                if (isValidSeq(sequence)) {
                    int frameCount = getNumFrames(sequence);
                    for (int frame = 0; frame < frameCount; ++frame) {
                        CSpriteFrame* image = getFrame(sequence, frame);
                        if (image)
                            image->dispose();
                    }
                }
            }
            resource::dispose();
        }
    }
}

// Dreamcast resourcemanager.cpp:2377/2380/2382/2391: one map lookup,
// end test, resource extraction and AddRef. Complete expands this ordinary
// helper in the getters; the nested map/tree decisions are compiler-owned.

VA_COMPGEN(0x0055D2C0, 0xBE, CLASS_CTOR, map)

VA_COMPGEN(0x0055d380, 0x2C, MAP_INSERT, TCacheMapKey)
VA_COMPGEN(0x0055d3b0, 0x56, MAP_FIND, TCacheMapKey)
VA_COMPGEN(0x0055e330, 0x56, TREE_FIND, TCacheMapKey)
// Insert's locked key search calls the node rebalance at 0x55e7e0 and
// predecessor walk at 0x55ec30. Their stock XTREE bodies own all three.
VA_COMPGEN(0x0055dbc0, 0x12D, TREE_INSERT, TCacheMapKey)
VA_COMPGEN(0x0055e7e0, 0x301, TREE_NODE_INSERT, TCacheMapKey)
VA_COMPGEN(0x0055ec30, 0xB3, TREE_CONST_ITERATOR_DEC, TCacheMapKey)

VA_COMPGEN(0x0055d410, 0x117, CLASS_CTOR, basic_ostringstream)
VA_COMPGEN(0x0055d630, 0x1AD, STRINGBUF_OVERFLOW, char)
VA_COMPGEN(0x0055db40, 0x7D, IMPLICIT_DTOR, basic_stringbuf)

// The resource cache's own map teardown helper, and the two remaining
// stringbuf members. All three byte-verified against the emitted COMDATs
// (0.977 / 0.992 / 0.978); `_Erase` keys off the tree's NAMED key type,
// TCacheMapKey, exactly as the iterator-increment claim below does.
VA_COMPGEN(0x0055e760, 0x7E, TREE_ERASE, TCacheMapKey)
VA_COMPGEN(0x0055eaf0, 0xDC, STRINGBUF_INIT, char)

VA_COMPGEN(0x0055E390, 0xA3, TREE_CONST_ITERATOR_INC, TCacheMapKey)

VA_COMPGEN(0x0055e740, 0x17, TREE_LOWER_BOUND, TCacheMapKey)
VA_COMPGEN(0x0055ebd0, 0x5A, TREE_LBOUND, TCacheMapKey)

// COMDAT pairing: basic_ostringstream::_G?$basic_ostringstream, mnemonic agreement 1.000.
VA_COMPGEN(0x0055dae0, 0x30, SCALAR_DELETING_DTOR, basic_ostringstream)

VA_COMPGEN(0x0055db10, 0x21, SCALAR_DELETING_DTOR, basic_stringbuf)

// COMDAT pairing: basic_stringbuf::0?$basic_stringbuf, mnemonic agreement 0.994.
VA_COMPGEN(0x0055e440, 0xFF, CLASS_CTOR, basic_stringbuf)

// COMDAT pairing: basic_ostringstream::1?$basic_ostringstream, mnemonic agreement 0.944.
VA_COMPGEN(0x0055d530, 0xC2, IMPLICIT_DTOR, basic_ostringstream)

VA_COMPGEN(0x0055dcf0, 0x50F, TREE_ERASE_ITERATOR, TCacheMapKey)

// COMDAT pairing: _Tree<TCacheMapKey, resource*>::erase(first, last), 0.960.
VA_COMPGEN(0x0055e200, 0x121, TREE_ERASE_RANGE, TCacheMapKey)

// COMDAT pairing: str on the char instantiation, mnemonic agreement 0.976.
VA_COMPGEN(0x0055e570, 0x1C5, STRINGBUF_STR, char)

// COMDAT pairing: seekoff on the char instantiation, mnemonic agreement 0.945.
VA_COMPGEN(0x0055d8a0, 0x15B, STRINGBUF_SEEKOFF, char)

// COMDAT pairing: seekpos on the char instantiation, mnemonic agreement 0.919.
VA_COMPGEN(0x0055da00, 0xD8, STRINGBUF_SEEKPOS, char)

// COMDAT pairing: pbackfail on the char instantiation, mnemonic agreement 0.968.
VA_COMPGEN(0x0055d7e0, 0x5D, STRINGBUF_PBACKFAIL, char)

// COMDAT pairing: underflow on the char instantiation, mnemonic agreement 0.938.
VA_COMPGEN(0x0055d840, 0x5A, STRINGBUF_UNDERFLOW, char)

VA_COMPGEN(0x0055ecf0, 0x18, PAIR_CTOR, cstr_resource_pair)

// COMDAT pairing: std::operator+(const string&, const char*). The two
// operator+ overloads are one key, but each object emits exactly one of them
// and they differ: seerhut's is the (const&, const&) form already claimed at
// 0x174d10, this one is the (const&, const char*) form, reached from
// ResourceManager::Open, GetBitmap816, LoadBitmap16 and LoadPalette - each
// appending a literal extension to a path.
VA_COMPGEN(0x004b7ec0, 0x114, BASIC_STRING_CONCAT, char)

VA_COMPGEN(0x0048d7d0, 0x28, LOCALE_FACET_INCREF, char)

// COMDAT pairing: basic_ostringstream<char>::str and
// basic_streambuf<char>::setg, agreements 1.000 and 1.000 at exactly equal
// 33-byte extents.
VA_COMPGEN(0x0055d600, 0x21, OSTRINGSTREAM_STR, char)
VA_COMPGEN(0x0055e540, 0x21, STREAMBUF_SETG, char)
