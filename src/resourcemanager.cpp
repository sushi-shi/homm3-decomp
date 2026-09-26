#include "va.h"

#include <map>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <utility>
#include "platform.h"
#if defined(_MSC_VER)
#include <yvals.h>
#endif

#include "resourcemanager.h"

#include "abstractfile.h"
#include "bitmap16.h"
#include "bitmap24.h"
#include "bitmap816.h"
#include "csprite.h"
#include "cspriteframe.h"
#include "font.h"
#include "lodfile.h"
#include "ownership.h"
#include "palette.h"
#include "resource.h"
#include "resourcemanager_archive.h"
#include "resourcemanager_sprite_headers.h"
#include "sample.h"
#include "smackmgr.h"
#include "textresource.h"

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
    VA(0x00559140, 0x5) MAC_ADDRESS(0x151f38, 0x8)  // two adapter vtables + exact body, retail-only
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
TPalette16* loadPaletteData(const char* name, TAbstractFile* stream);
TPalette24* getPalette24(const char* name);
TPalette24* loadPalette24Data(const char* name, TAbstractFile* stream);
font* loadFont(const char* name);
font* loadFontData(const char* name, TAbstractFile* stream, int fileSize);
TTextResource* loadText(const char* name);
TTextResource* loadTextData(const char* name, TAbstractFile* stream,
                            int fileSize);
TSpreadsheetResource* loadSpreadsheet(const char* name);
TSpreadsheetResource* loadSpreadsheetData(const char* name,
                                          TAbstractFile* stream,
                                          int fileSize);

}

DATA(0x0069e528)
ResourceManager::TCacheMap g_resourceCache;

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

VA(0x00559110, 0x21) MAC_ADDRESS(0x151ef4, 0x44)  // LOD adapter vtable slot 1
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

VA(0x005594b0, 0x40) MAC_ADDRESS(0x1521d0, 0x40)  // dc 0x122984
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
// Only this TU uses the path. File-static linkage makes CodeWarrior address
// the same-TU object directly through TOC 1+0x5494, as at Mac 0:0x15221c.
DATA(0x0069e4f0) static std::string g_resourcePath;
// Complete-only common diagnostic, reconstructed from the identical seven-part
// messages in 0x559510, 0x5599e0 and both 0x55c3c0 error paths. Retail evaluates
// typeName.c_str() before constructing the stream; an ordinary shared helper
// recovers that argument boundary and the stream's short lifetime. VC6 expands
// this body naturally: no source inline keyword or inline-depth controls.
// Both typed reporters are byte-exact with ordinary string assignments. Pasting
// this body into those callers gives 78.49/72.05 and the wrong stream expansion.
// The original helper name and exact source location are not available.
static void reportMissingResource(const char* caller, const char* typeName,
                                  const char* resourceName)
{
    std::ostringstream message;
    message
        << DATA_COMPGEN(0x00682f18, sampleErrorPrefix,
                        "ResourceManager::")
        << caller
        << DATA_COMPGEN(0x00682f2c, missingResourcePrefix,
                        " could not find the \"")
        << typeName
        << DATA_COMPGEN(0x00682f44, missingResourceMiddle, "\" resource \"")
        << resourceName
        << DATA_COMPGEN(0x00682f54, missingResourceSuffix, "\".");
    MessageBoxA(
        GetForegroundWindow(), message.str().c_str(),
        DATA_COMPGEN(0x00682f08, resourceManagerCaption,
                     "ResourceManager"),
        0);
}

// Complete's common missing-resource reporter has no Dreamcast identity, but
// its thirteen retail callers prove the fastcall surface. The dense 0..96
// dispatch maps the admitted resource values to names and renders every gap
// as `0x` plus a hexadecimal value before building the diagnostic shown by
// the callers.
// 100%: ordinary string assignments plus reportMissingResource recover all
// retail calls and lifetimes without inline-depth pins. The former bootstrap
// C linkage suppressed EH cleanup; keep the ordinary C++ declaration.
VA(0x00559510, 0x4C1)  // caller ABI + retail type-name jump table/message graph
void __fastcall reportMissingTypedResource(const char* caller,
                                          int resourceType,
                                          const char* resourceName)
{
    std::string typeName;
    switch (resourceType) {
    case RESOURCE_TYPE_NONE:
        typeName = DATA_COMPGEN(0x00682fc8, nullResourceType, "null");
        break;
    case RESOURCE_TYPE_DATA:
        typeName = DATA_COMPGEN(0x00682fc0, dataResourceType, "data");
        break;
    case RESOURCE_TYPE_TEXT:
        typeName = DATA_COMPGEN(0x00682fb8, textResourceType, "text");
        break;
    case RESOURCE_TYPE_BITMAP:
        typeName = DATA_COMPGEN(0x00682fb0, bitmap8ResourceType, "bitmap8");
        break;
    case RESOURCE_TYPE_BITMAP24:
        typeName = DATA_COMPGEN(0x00682fa4, bitmap24ResourceType, "bitmap24");
        break;
    case RESOURCE_TYPE_BITMAP16:
        typeName = DATA_COMPGEN(0x00682f98, bitmap16ResourceType, "bitmap16");
        break;
    case RESOURCE_TYPE_BITMAP565:
        typeName = DATA_COMPGEN(0x00682f8c, bitmap565ResourceType, "bitmap565");
        break;
    case RESOURCE_TYPE_BITMAP555:
        typeName = DATA_COMPGEN(0x00682f80, bitmap555ResourceType, "bitmap555");
        break;
    case RESOURCE_TYPE_BITMAP1555:
        typeName = DATA_COMPGEN(0x00682f74, bitmap1555ResourceType,
                                     "bitmap1555");
        break;
    case RESOURCE_TYPE_MIDI:
        typeName = DATA_COMPGEN(0x00682f6c, midiResourceType, "midi");
        break;
    case RESOURCE_TYPE_FONT:
        typeName = DATA_COMPGEN(0x00682f64, fontResourceType, "font");
        break;
    case RESOURCE_TYPE_PALETTE:
        typeName = DATA_COMPGEN(0x00682f5c, paletteResourceType, "palette");
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

    reportMissingResource(caller, typeName.c_str(), resourceName);
}

// Complete splits sprite-family diagnostics from the common resource
// reporter above. GetSprite's two calls prove the fastcall ABI and the retail
// jump table proves the complete 64..79 type-name mapping. Like the common
// reporter, this is 100% with ordinary assignments and reportMissingResource;
// no artificial message scope or inline-depth control is needed.
VA(0x005599e0, 0x448)  // anchor-caller + retail type-name jump table
void __fastcall reportMissingSpriteResource(const char* caller,
                                           int resourceType,
                                           const char* resourceName)
{
    std::string typeName;
    switch (resourceType) {
    case RESOURCE_TYPE_SPRITE:
        typeName = DATA_COMPGEN(0x0067555c, spriteResourceType, "sprite");
        break;
    case RESOURCE_TYPE_SPRITE_DEFINITION:
        typeName = DATA_COMPGEN(
            0x00682ff4, spriteDefinitionResourceType, "spritedef");
        break;
    case RESOURCE_TYPE_CREATURE:
        typeName = DATA_COMPGEN(0x00675550, creatureResourceType, "creature");
        break;
    case RESOURCE_TYPE_ADVENTURE_OBJECT:
        typeName = DATA_COMPGEN(
            0x00675548, adventureObjectResourceType, "advobj");
        break;
    case RESOURCE_TYPE_HERO:
        typeName = DATA_COMPGEN(0x00675540, heroResourceType, "hero");
        break;
    case RESOURCE_TYPE_TILESET:
        typeName = DATA_COMPGEN(0x00675538, tilesetResourceType, "tileset");
        break;
    case RESOURCE_TYPE_POINTER:
        typeName = DATA_COMPGEN(0x00675530, pointerResourceType, "pointer");
        break;
    case RESOURCE_TYPE_INTERFACE:
        typeName = DATA_COMPGEN(0x00675524, interfaceResourceType, "interface");
        break;
    case RESOURCE_TYPE_SPRITE_FRAME:
        typeName = DATA_COMPGEN(0x00682fe4, spriteFrameResourceType,
                                     "sprite frame");
        break;
    case RESOURCE_TYPE_COMBAT_HERO:
        typeName = DATA_COMPGEN(0x00682fd8, combatHeroResourceType,
                                     "combat hero");
        break;
    case RESOURCE_TYPE_ADVENTURE_MASK:
        typeName = DATA_COMPGEN(0x00682fd0, adventureMaskResourceType,
                                     "advmask");
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

    reportMissingResource(caller, typeName.c_str(), resourceName);
}

VA(0x00559e30, 0x1E5) MAC_ADDRESS(0x1523fc, 0x1f4)
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

VA(0x0055a020, 0x221) MAC_ADDRESS(0x152700, 0x224)
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
            sprite->getPalette24().adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
            sprite->resetPalette();
            break;
        }

        case RESOURCE_TYPE_BITMAP: {
            Bitmap816* bitmap = static_cast<Bitmap816*>(value);
            bitmap->getPalette24().adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
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

// Mac 0:0x152210..0x152280 retains this ordinary per-archive opener. Its
// caller at 0:0x152924 calls it for both sprite and bitmap archive lists.
// The helper constructs the archive pathname, opens its LODFile, destroys the
// pathname, and returns whether the open succeeded.
MAC_ADDRESS(0x152210, 0x70)
static bool openArchiveResource(int archiveIndex)
{
    const char* archiveName = g_resourceLodSlots[archiveIndex].m_archiveName;
    LODFile* file = &g_resourceLodSlots[archiveIndex].m_file;
    bool opened = file->open((g_resourcePath + archiveName).c_str(), 0) == 0;
    return opened;
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

// The Mac release retains openArchiveResource at 0:0x152210 and calls it from
// both archive lists; Complete auto-inlines the same ordinary helper. The
// direct archive-name and file expressions preserve Complete's separate field
// relocations. Saving the result of opening a temporary pathname before its
// destructor recovers the returned-string c_str load and matches all 0x2f1
// Windows bytes: 36/36 CFG blocks, sixteen branches and nineteen call sites.
// A shared slot reference/pointer is byte-flat at 97.41%; a named pathname
// reaches 99.91%; returning the direct temporary comparison without a saved
// result drops to 90.02% and adds three blocks. The Mac caller keeps thirteen
// ordered calls but is 516 bytes versus retail's 528 at this TU's measured
// -O3 profile, with a different vector frame/cleanup layout. File-static
// g_resourcePath now emits its retained direct TOC address. A shared slot
// expression removes the Mac helper's extra index multiplication, but loses
// Complete's exact inlined lowering; both field expressions remain. Explicit
// else arms emit the two Mac post-throw
// branches and preserve Windows exactness. Throwing the named constant in the
// archive-index-one arm matches Mac's literal store and also preserves Windows
// exactness. The Mac frame gap remains.
// The flags arrive in ECX/EDX; ret 4 removes the added error-code pointer.
VA(0x0055a250, 0x2F1) MAC_ADDRESS(0x152924, 0x210)  // sole retail caller + two flags/error output, dc 0x12173c
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
                    bool opened = openArchiveResource(archiveIndex);
                    if (!opened) {
                        if (archiveIndex == 1)
                            throw openErrorRequiredArchive;
                        else
                            throw openErrorGeneric;
                    }
                    ++archive;
                } while (--remaining);
            }

            if (openBitmaps) {
                int remaining = context->m_bitmaps.m_count;
                int* archive = context->m_bitmaps.m_indices;
                do {
                    int archiveIndex = *archive;
                    bool opened = openArchiveResource(archiveIndex);
                    if (!opened) {
                        if (archiveIndex == 0)
                            throw openErrorRequiredArchive;
                        else
                            throw openErrorGeneric;
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
    } catch (t_open_errors error) {
        if (errorCode)
            *errorCode = error;
        return false;
    }

    return true;
}

VA(0x0055a550, 0x67) MAC_ADDRESS(0x152b9c, 0x60)
void ResourceManager::close()
{
    expunge();

    for (int i = 0; i < 8; ++i)
        g_resourceLodSlots[i].m_file.clear();
}

VA(0x0055a5c0, 0xE2) MAC_ADDRESS(0x152bfc, 0x3c)
void ResourceManager::setPath(const char* path)
{
#if defined(HOMM3_TARGET_MAC)
    // Mac 0x152bfc assigns the path unchanged; MSL has no _fullpath.
    g_resourcePath = path;
#else
    char fullpath[_MAX_PATH + 1];
    _fullpath(fullpath, path, _MAX_PATH);
    g_resourcePath = fullpath;
#endif
}

VA(0x0055a6b0, 0xEF) MAC_ADDRESS(0x152c38, 0x16c)
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

// Mac 0:0x152df8..0x152fc8 calls the retained pointToBitmapResource helper
// for name and default.pcx. The same two ordinary source calls auto-inline in
// Complete. Dreamcast names the anonymous bmpHeader local but cannot fix its
// scope; retail keeps its stack home live across the loose FILE branch. Its
// function-scope declaration prevents that branch's cache insertion pair from
// reusing the header slot and matches all 0x41f retail bytes, 34 CFG blocks,
// and 32 ordered calls. Moving result before the cache lookup was byte-flat.
VA(0x0055a800, 0x41F) MAC_ADDRESS(0x152df8, 0x1d0)  // bitmapBorder::SetImage loader; dc 0x121ac8
Bitmap816* ResourceManager::getBitmap816(const char* name)
{
    Bitmap816* cached = static_cast<Bitmap816*>(getFromCache(name));
    if (cached)
        return cached;

    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");

    struct {
        int m_dataSize;
        int m_width;
        int m_height;
    } bmpHeader;
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
        LODFile* lodFile = pointToBitmapResource(name);

        if (!lodFile) {
            reportMissingTypedResource(
                DATA_COMPGEN(0x00683030, getBitmap816ErrorContext,
                             "GetBitmap816"),
                RESOURCE_TYPE_BITMAP, name);

            const char* fallbackName = DATA_COMPGEN(
                0x0064108c, defaultBitmap816Name, "default.pcx");
            lodFile = pointToBitmapResource(fallbackName);

            if (!lodFile) {
                reportMissingTypedResource(
                    DATA_COMPGEN(0x00683030, getBitmap816ErrorContext,
                                 "GetBitmap816"),
                    RESOURCE_TYPE_BITMAP, fallbackName);
                return 0;
            }
        }

        {
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

VA(0x0055ac40, 0x388) MAC_ADDRESS(0x152fc8, 0x1b4)
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
        LODFile* lodFile = pointToBitmapResource(name);

        if (!lodFile) {
            reportMissingTypedResource(
                DATA_COMPGEN(0x00683040, loadBitmap16ErrorContext,
                             "GetBitmap16"),
                RESOURCE_TYPE_BITMAP16, name);

            const char* fallbackName = DATA_COMPGEN(
                0x006410a8, defaultBitmap24Name, "dfault24.pcx");
            lodFile = pointToBitmapResource(fallbackName);

            if (!lodFile) {
                reportMissingTypedResource(
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
VA(0x0055afd0, 0x8A) MAC_ADDRESS(0x153204, 0x54)
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

// Mac 0:0x153258 retains the reader immediately before loadPalette. It owns
// the two stream reads, palette temporary, saturation and conversion; Complete
// expands the same work in both its loose-file and archive paths. The helper
// name is inferred from the neighboring retained loadPalette24Data operation.
MAC_ADDRESS(0x153258, 0x104)
TPalette16* ResourceManager::loadPaletteData(const char* name,
                                             TAbstractFile* stream)
{
    char header[24];
    TRGBA paletteData[256];
    stream->read(header, sizeof(header));
    stream->read(paletteData, sizeof(paletteData));
    TPalette24 palette24(paletteData);
    if (g_graphicsSaturated)
        palette24.adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
    return new TPalette16(name, palette24,
        g_firstMaskBits, g_firstMaskShift,
        g_greenMaskBits, g_greenMaskShift,
        g_lastMaskBits, g_lastMaskShift);
}

VA(0x0055b060, 0x377) MAC_ADDRESS(0x15335c, 0x84)  // public GetPalette callee + retail conversion tuple
TPalette16* ResourceManager::loadPalette(const char* name)
{
    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");

    if (file) {
        try {
            t_stdio_file_adapter stream(file);
            TAbstractFile* streamInterface = &stream;
            TPalette16* result = loadPaletteData(name, streamInterface);

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    }

    LODFile* lodFile = pointToBitmapResource(name);

    if (!lodFile) {
        reportMissingTypedResource(
            DATA_COMPGEN(0x0068304c, loadPaletteErrorContext,
                         "GetPalette"),
            RESOURCE_TYPE_PALETTE, name);

        const char* fallbackName = DATA_COMPGEN(
            0x006410b8, defaultPalette16Name, "default.pal");
        lodFile = pointToBitmapResource(fallbackName);

        if (!lodFile) {
            reportMissingTypedResource(
                DATA_COMPGEN(0x0068304c, loadPaletteErrorContext,
                             "GetPalette"),
                RESOURCE_TYPE_PALETTE, fallbackName);
            return 0;
        }
    }

    t_lod_file_adapter stream(lodFile);
    TAbstractFile* streamInterface = &stream;
    return loadPaletteData(name, streamInterface);
}

// Like GetBitmap16, Complete always consults the cache and removes the
// Dreamcast ignore_cache argument; the retained body ends with plain ret.
VA(0x0055b3e0, 0x8A) MAC_ADDRESS(0x1533e0, 0x54)  // dc public GetPalette + retail getter family, dc 0x121d90
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

// The retained Mac helper at 0:0x153434 owns the header/RGBA buffers, reads
// both from the common stream, constructs a palette and applies saturation.
// Its caller passes (name, stream); the first argument is unused in this body.
MAC_ADDRESS(0x153434, 0xa8)
TPalette24* ResourceManager::loadPalette24Data(const char* name,
                                               TAbstractFile* stream)
{
    char header[24];
    TRGBA rgba[256];
    stream->read(header, sizeof(header));
    stream->read(rgba, sizeof(rgba));

    TPalette24* result = new TPalette24(rgba);
    if (g_graphicsSaturated)
        result->adjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
    return result;
}

// Mac 0:0x1534dc..0x153560 is this loader's archive-only port. Its caller at
// 0x10fdf0 passes Players.pal; it calls pointToBitmapResource for the requested
// name and default.pal, then calls the retained loadPalette24Data operation at
// 0x153434 through an 8-byte LOD stream adapter. Complete also opens loose
// FILE resources and reports missing resources. Two source calls to the
// ordinary pointToBitmapResource and loadPalette24Data helpers auto-inline in
// VC6, matching all 0x2d1 retail bytes and its 22 ordered call sites. The
// latter helper owns the header/RGBA locals recorded in Dreamcast's older
// direct-reader function; its recovered lifetime closes the former 8-byte
// frame-coloring residual.
VA(0x0055b470, 0x2D1) MAC_ADDRESS(0x1534dc, 0x84)  // dc/hd public identity + retail palette-file shape, dc 0x121ec8
TPalette24* ResourceManager::getPalette24(const char* name)
{
    FILE* file = fopen((g_resourcePath + name).c_str(), "rb");
    if (file) {
        try {
            t_stdio_file_adapter stream(file);
            TAbstractFile* streamInterface = &stream;
            TPalette24* result = loadPalette24Data(name, streamInterface);

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    }

    LODFile* lodFile = pointToBitmapResource(name);

    if (!lodFile) {
        reportMissingTypedResource(
            DATA_COMPGEN(0x0068304c, loadPaletteErrorContext, "GetPalette"),
            RESOURCE_TYPE_PALETTE, name);

        const char* fallbackName =
            DATA_COMPGEN(0x006410c4, defaultPaletteName, "default.pal");
        lodFile = pointToBitmapResource(fallbackName);

        if (!lodFile) {
            reportMissingTypedResource(
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
    return loadPalette24Data(name, streamInterface);
}

// Dreamcast's older GetFont body records a TFontSpec local and calls the
// reference-taking font::SetPalette. Mac loadFontData at 0:0x153560 likewise
// passes the fetched palette directly at 0x153700, with no copy temporary.
// Passing *palette matches the declared reference interface, removes VC6's
// implicit TPalette16 pointer-conversion temporary and closes this body.
VA(0x0055b750, 0x17A) MAC_ADDRESS(0x153560, 0x260)
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
            result.get()->setPalette(*palette);
        }
        catch (...) {
            palette->dispose();
            throw;
        }
        palette->dispose();
    }

    return result.release();
}

// Mac 0:0x1538a8..0x153944 is this named loader's archive-only port: the
// font getter calls it at 0x15396c, and it calls pointToBitmapResource for
// name/default.fnt before getItemIndex(name) and Mac loadFontData at 0x153560.
// The latter reads through the same 8-byte LOD stream adapter and performs
// endian conversion of the font records. Complete also opens loose FILE
// resources and uses its separate Windows loadFontData body at 0x55b750. Two
// ordinary source calls to pointToBitmapResource auto-inline in VC6 and close
// this 0x229-byte Windows body exactly, preserving all 18 ordered calls.
VA(0x0055b8d0, 0x229) MAC_ADDRESS(0x1538a8, 0x9c)
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

    LODFile* lodFile = pointToBitmapResource(name);

    if (!lodFile) {
        reportMissingTypedResource(
            DATA_COMPGEN(0x00683058, loadFontErrorContext, "GetFont"),
            RESOURCE_TYPE_FONT, name);

        const char* fallbackName =
            DATA_COMPGEN(0x006410d0, defaultFontName, "default.fnt");
        lodFile = pointToBitmapResource(fallbackName);

        if (!lodFile) {
            reportMissingTypedResource(
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

VA(0x0055bb00, 0x8A) MAC_ADDRESS(0x153944, 0x54)
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

// Mac 0:0x153998 retains this reader directly before loadText. The size,
// stream and name arguments feed one allocation, virtual read and constructor.
// Complete expands the same body in its file and archive branches.
MAC_ADDRESS(0x153998, 0xb4)
TTextResource* ResourceManager::loadTextData(const char* name,
                                             TAbstractFile* stream,
                                             int fileSize)
{
    std::auto_ptr<char> data(new char[fileSize]);
    stream->read(data.get(), fileSize);
    return new TTextResource(name, fileSize, data.get());
}

VA(0x0055bb90, 0x240) MAC_ADDRESS(0x153a4c, 0x8c)
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
                TAbstractFile* streamInterface = &stream;
                result = loadTextData(name, streamInterface, fileSize);
            }

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    }

    LODFile* lodFile = pointToBitmapResource(name);

    if (!lodFile) {
        reportMissingTypedResource(
            DATA_COMPGEN(0x00683060, loadTextErrorContext, "GetText"),
            RESOURCE_TYPE_TEXT, name);
        return 0;
    }

    int fileSize = lodFile->getItemIndex(name)->m_size;
    t_lod_file_adapter stream(lodFile);
    TAbstractFile* streamInterface = &stream;
    return loadTextData(name, streamInterface, fileSize);
}

VA(0x0055bdd0, 0x8A) MAC_ADDRESS(0x153ad8, 0x54)
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

// Mac 0:0x153b2c is the corresponding retained spreadsheet reader. Its
// neighboring caller is loadSpreadsheet at 0:0x153be0.
MAC_ADDRESS(0x153b2c, 0xb4)
TSpreadsheetResource* ResourceManager::loadSpreadsheetData(
    const char* name, TAbstractFile* stream, int fileSize)
{
    std::auto_ptr<char> data(new char[fileSize]);
    stream->read(data.get(), fileSize);
    return new TSpreadsheetResource(name, fileSize, data.get());
}

VA(0x0055be60, 0x240) MAC_ADDRESS(0x153be0, 0x8c)
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
                TAbstractFile* streamInterface = &stream;
                result = loadSpreadsheetData(name, streamInterface, fileSize);
            }

            fclose(file);
            return result;
        }
        catch (...) {
            fclose(file);
            throw;
        }
    }

    LODFile* lodFile = pointToBitmapResource(name);

    if (!lodFile) {
        reportMissingTypedResource(
            DATA_COMPGEN(0x00683068, loadSpreadsheetErrorContext,
                         "GetSpreadsheet"),
            RESOURCE_TYPE_TEXT, name);
        return 0;
    }

    int fileSize = lodFile->getItemIndex(name)->m_size;
    t_lod_file_adapter stream(lodFile);
    TAbstractFile* streamInterface = &stream;
    return loadSpreadsheetData(name, streamInterface, fileSize);
}

VA(0x0055c0a0, 0x8A) MAC_ADDRESS(0x153c6c, 0x54)  // dc 0x122164
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
MAC_ADDRESS(0x154748, 0x7c)
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
MAC_ADDRESS(0x152194, 0x3c)
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

namespace ResourceManager {
bool getSoundFile(const char* localName, std::auto_ptr<char>& data, int* size);
}

DATA(0x0069e500)
TSoundHeaderDescriptor g_soundHeaderDescriptors[3];

// ECX/EDX carry name/auto_ptr and ret 4 removes the size output. The direct
// Win32 file read replaces Dreamcast's separate data/header outputs.
VA(0x0055c130, 0x28F) MAC_ADDRESS(0x153cc0, 0x144)  // dc GetSoundFile + caller/record layout, dc 0x1221fc
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

// The shared diagnostic restores all 25 retail blocks (23 exact) and leaves
// only the two message-stream vbase-destructor closures over-expanded:
// 98.2289 MAX, with the former pasted-body implementation's 100 retained in
// HIST. The reproduced VC6 trace has caller cb=445 (budget floor 1000);
// the closures cost 43 and receive 96/62 inside reportMissingResource.
// Do not paste the helper back or add an inline-depth control to suppress them.
static void reportMissingSample(const char* name)
{
    reportMissingResource(
        DATA_COMPGEN(0x00683078, getSampleErrorContext, "GetSample"),
        DATA_COMPGEN(0x00683084, sampleResourceKind, "sfx"), name);
}
VA(0x0055c3c0, 0x356) MAC_ADDRESS(0x153e04, 0x108)  // GetSample callee + GetSoundFile/default.wav graph
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
        reportMissingSample(name);
        const char* fallbackName = DATA_COMPGEN(
            0x006410dc, defaultSampleName, "default.wav");
        if (!getSoundFile(fallbackName, data, &size)) {
            reportMissingSample(fallbackName);
            return 0;
        }
    }

    return new sample(name, data.get(), size, 0, 127, 1);
}

VA(0x0055c720, 0x8A) MAC_ADDRESS(0x153f78, 0x54)  // dc 0x1222e0
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
// Both retail targets expand this body in getSprite; Mac -O3 calls it unless
// qualified inline, while VC6 is byte-flat. The keyword is a source-model
// inference: Dreamcast's standalone body does not settle the declaration.
// Its expansion retains deletion of the old palette and construction from
// the new value.
// Complete's palette copy interface takes a pointer, where DC takes a ref.
inline void addPal16(CSprite* sprite, const TPalette16* pal)
{
    if (sprite->m_p)
        delete sprite->m_p;
    sprite->m_p = new TPalette16(pal);
}

// Original: addPal24; csprite.cpp:986, dc 0x73bac.
inline void addPal24(CSprite* sprite, const TPalette24* pal)
{
    if (sprite->m_p24)
        delete sprite->m_p24;
    sprite->m_p24 = new TPalette24(pal);
}

// Dreamcast GetSprite (dc 0x122320) proves GetFromCache, SpriteDefHeader
// Sdef, the archive load and AddToCache. Complete adds DEF sequence/frame
// parsing and a cache lookup for each frame name. Mac 0:0x153fcc..0x1545e4
// calls pointToSpriteResource twice and expands addPal16/24 in place. The
// ordinary Mac -O3 probe emitted two separate 136-byte helpers; explicit
// inline expands them and admits the paired byte diff. Windows has all 46
// calls in retail order, but the DEF loop has a different register and
// local-slot allocation. The Mac target also uses an eight-byte handle
// adapter for DEF buffers (TempNewHandle/NewHandle via 0:0x26ab14); the
// Win32 source uses raw new[]/delete[]. A canonical Mac adapter remains to
// be recovered.
// A guarded post-test frame loop reaches 88.7694% with the current canonical
// helpers, compared with 88.7468% for an entry-tested for loop and 88.69% for
// a while spelling. All retain an extra entry jump and 73 versus 72 CFG blocks.
// Moving memcpy before definitionPosition
// or changing definitionPosition to an advancing cursor gives 86.39%; Mac
// instruction scheduling alone does not establish that C++ statement order.
// Splitting the pointer
// assignment/increment across memcpy gives 86.88% and 74/72 CFG blocks;
// retail x86 itself advances before the copy. The first sequence-header walk
// now names the +0x10 pointer cursor visible in both x86 and Mac; VC6 lowers
// it byte-identically to the indexed reference at 88.74677%.
// The retail second loop advances a sequence-record cursor by 0x10, but
// spelling it as a separate C++ pointer regresses Windows 88.7694% to
// 87.6710% and Mac 10.3846% to 10.3205%; the indexed reference is retained.
// The earlier flattened-cache model reached 88.8564% in HIST, but lost the
// proven shared cache-helper structure and remains only a diagnostic lead.
VA(0x0055c7b0, 0x743) MAC_ADDRESS(0x153fcc, 0x618)  // anchor-caller/body records, dc 0x122320; wall
CSprite* ResourceManager::getSprite(const char* name)
{
    CSprite* cached = static_cast<CSprite*>(getFromCache(name));
    if (cached)
        return cached;

    LODFile* lodFile = pointToSpriteResource(name);

    if (!lodFile) {
#ifdef _WIN32
        // Complete reports failed archive lookups; Mac retries without a reporter.
        reportMissingSpriteResource(
            DATA_COMPGEN(0x00683088, getSpriteErrorContext, "GetSprite"),
            RESOURCE_TYPE_SPRITE, name);
#endif

        lodFile = pointToSpriteResource(name);

        if (!lodFile) {
#ifdef _WIN32
            reportMissingSpriteResource(
                DATA_COMPGEN(0x00683088, getSpriteErrorContext, "GetSprite"),
                RESOURCE_TYPE_SPRITE, name);
#endif
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
    TSpriteDataHeader* sequence = sequences;
    for (sequenceIndex = 0;
         sequenceIndex < sdef.m_numSequences;
         ++sequenceIndex, ++sequence) {
        memcpy(sequence, definitionPosition, sizeof(*sequence));
        definitionPosition += sizeof(*sequence);

        sequence->m_frameNames = new char[sequence->m_numFrames * 13];
        memcpy(sequence->m_frameNames, definitionPosition,
               sequence->m_numFrames * 13);
        definitionPosition += sequence->m_numFrames * 13;

        sequence->m_frameOffsets = new int[sequence->m_numFrames];
        memcpy(sequence->m_frameOffsets, definitionPosition,
               sequence->m_numFrames * sizeof(int));
        definitionPosition += sequence->m_numFrames * sizeof(int);
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
            ++frameIndex;
            frameNameOffset += 13;
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

VA(0x0055cf00, 0x4B) MAC_ADDRESS(0x1545e4, 0x7c)
void ResourceManager::getBackdrop(const char* resName, Bitmap16Bit* destBmap)
{
    Bitmap816* source = getBitmap816(resName);
    if (source) {
        source->draw(0, 0, source->getWidth(), source->getHeight(),
                     destBmap, 0, 0, false);
        source->dispose();
    } else {
        reportMissingTypedResource(
            DATA_COMPGEN(0x00683094, getBackdropErrorContext, "GetBackdrop"),
            RESOURCE_TYPE_BITMAP, resName);
    }
}

VA(0x0055cf50, 0x83) MAC_ADDRESS(0x1522ec, 0x88)
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

VA(0x0055cfe0, 0x83) MAC_ADDRESS(0x152374, 0x88)  // bitmap-field twin of PointToSpriteResource
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
// Twenty-one context/list and loop spellings remained at 96.7742%. The shared
// single-call source loop also compiles to Complete's two lookup call sites
// at that same score, while Mac 0:0x1546a0 keeps one call in the loop. A Mac
// list reference adds an address instruction; a named context, named game
// state, array-of-three view, for-initializer lifetime, and -O2/-O3 are
// byte-flat in the Mac probe. A break/shared return moves the size load
// behind the loop.
// The direct list expression and simple loop are retained.
VA(0x0055d070, 0x5C) MAC_ADDRESS(0x1546a0, 0x88)  // retail archive-list walk + dc/hd name corroboration
int ResourceManager::getBitmapResourceSize(const char* name)
{
    int* archive =
        g_resourceArchiveContexts[*g_videoGameState].m_bitmaps.m_indices;
    for (;;) {
        LODEntry* entry = g_resourceLodSlots[*archive].m_file.getItemIndex(name);
        if (entry)
            return entry->m_size;
        ++archive;
    }
}

VA(0x0055d0d0, 0x11) MAC_ADDRESS(0x154728, 0x20)  // caller-family merge + explicit LOD receiver/ret 4, dc 0x1224cc
int ResourceManager::readFromBitmapResource(LODFile* resource, void* data,
                                             int numBytes)
{
    return resource->read(data, numBytes);
}

VA(0x0055d0f0, 0xA1) MAC_ADDRESS(0x1547c4, 0x78)  // resource vslot 1 + cache-key/lower-bound proof
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
VA(0x0055d1a0, 0x118) MAC_ADDRESS(0x15483c, 0x118)
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
