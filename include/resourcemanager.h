#ifndef HOMM3_RESOURCEMANAGER_H
#define HOMM3_RESOURCEMANAGER_H

#include "va.h"

#include "resource.h"
#include "csprite.h"
#include "sample.h"

class CSprite;
class font;
class resource;
class sample;
class TPalette24;
class LODFile;

// Dreamcast: ?GetSprite@ResourceManager@@YAPAVCSprite@@PBD@Z and
// ?GetFont@ResourceManager@@YAPAVfont@@PBD@Z - the namespace-level
// resource acquisition (retail bodies 0x55c7b0 / 0x55bb00, fastcall
// under /Gr; called by the button ctors).
class Bitmap816;
class Bitmap16Bit;
class TPalette16;
class TSpreadsheetResource;
class TTextResource;

extern int& g_videoGameState;
namespace ResourceManager {
// Claimed by resourcemanager.obj; the adventure-map phisher-price command
// toggles it before selecting the palette transform. Loki h3maped exports
// it as ResourceManager::SaturatedGraphicsEasterEgg (1 byte, 0x8420728).
extern unsigned char SaturatedGraphicsEasterEgg;  // retail 0x69e5b0
// Retail exception type name is ResourceManager::t_open_errors on Windows
// and Mac; the archive opener catches this shared domain in open().
enum t_open_errors { openErrorGeneric = 0, openErrorRequiredArchive = 1 };

void RemapGraphics();
void SaturateGraphics();
// Complete adds an error-code output to Dreamcast's two-boolean form. The
// sole retail caller passes an int*, and the catch handler stores through it.
bool Open(bool openSprites, bool openBitmaps, int* errorCode);
void Close();
void Expunge();
unsigned char Report(const char* filename);
void SetPath(const char* path);
void SetPixelFormat(unsigned long redMask, unsigned long greenMask,
                    unsigned long blueMask);             // 0x55a6b0
CSprite* GetSprite(const char* name);
font* GetFont(const char* name);
// Dreamcast and retail oldmain load the same Players.pal through the 24-bit
// sibling immediately after the two TPalette16 loads (retail 0x55b470).
TPalette24* GetPalette24(const char* name);
sample* GetSample(const char* name);
// Retail body 0x55a800 (bitmapBorder::SetImage's loader).
Bitmap816* GetBitmap816(const char* name);
Bitmap16Bit* GetBitmap16(const char* name);
void GetBackdrop(const char* resName, Bitmap16Bit* destBmap);
TTextResource* GetText(const char* name);
TSpreadsheetResource* GetSpreadsheet(const char* name);
void AddToCache(resource* value);
// Dreamcast resourcemanager.cpp:2377; expanded by Complete's cache getters.
resource* GetFromCache(const char* name);

// Existing disposal wrappers are inlined across TUs (the selection
// destructor keeps only the member virtual calls), so their bodies are
// visible here.
DC_ADDRESS(0x122530, 0x90)
inline void Dispose(resource* value) { value->dispose(); }

// Original: ResourceManager::Dispose(sample*), DC resourcemanager.cpp:2196.
// DC releases a ds_engine cache entry; Complete's sample owns its sound
// data and inherits reference-counted resource disposal. Inlined in
// castSpell (spells.cpp), the null guard and virtual call. Header
// visibility and inline linkage are inferred for Complete, not a DC-proven
// explicit qualifier (all three DC overload flags are zero).
DC_ADDRESS(0x1225c0, 0x1c)
inline void Dispose(sample* value)
{
    if (value)
        value->dispose();
}

DC_ADDRESS(0x1225dc, 0xf6)
inline void Dispose(CSprite* value) { value->dispose(); }

// Older DC resourcemanager.cpp:2280 had the cache sweep.
// Complete has no cache-sweep work in the selection teardown; the original
// call stays, with the empty helper visible across translation units.
DC_ADDRESS(0x1226d4, 0x1d6)
inline void del_Spr_from_Cache() {}

LODFile* PointToSpriteResource(const char* name);
LODFile* PointToBitmapResource(const char* name);

int ReadFromBitmapResource(LODFile* resource, void* data, int numBytes);
// Retail 0x55d070 walks the active context's bitmap LOD list until
// getItemIndex finds the named entry, then returns that entry's +0x14 size.
int GetBitmapResourceSize(const char* name);

resource* GetFromCache(const char* name);

}

// Role-based name for Complete's common missing-resource reporter at
// retail 0x559510. No explicit calling convention: the game's thirteen
// getter call sites pass registers (/Gr), the editor's push the arguments
// (h3maped 0x4dc3d6). Its body is reconstructed; no PC symbol source
// survives, so the source name remains explicitly provisional. It has
// ordinary C++ linkage: extern "C" would suppress VC6's exception cleanup
// in both reporters, where retail has the fs:[0] registration and unwind
// states.
void reportMissingTypedResource(const char* caller,
                                int resourceType,
                                const char* resourceName);

// Complete's sprite-family counterpart, with the same calling convention;
// the PC symbol name is provisional because this helper has no Dreamcast
// identity.
void reportMissingSpriteResource(const char* caller,
                                 int resourceType,
                                 const char* resourceName);

#endif  /* HOMM3_RESOURCEMANAGER_H */
