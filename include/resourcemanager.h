#ifndef HOMM3_RESOURCEMANAGER_H
#define HOMM3_RESOURCEMANAGER_H

#include "resource.h"
#include "csprite.h"

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

extern int* g_videoGameState;
// Claimed by resourcemanager.obj; the adventure-map phisher-price command
// toggles it before selecting the palette transform.
extern unsigned char g_graphicsSaturated;  // retail 0x69e5b0

namespace ResourceManager {
// Retail exception type name is ResourceManager::t_open_errors on Windows
// and Mac; the archive opener catches this shared domain in open().
enum t_open_errors { openErrorGeneric = 0, openErrorRequiredArchive = 1 };

void remapGraphics();
void saturateGraphics();
// Complete adds an error-code output to Dreamcast's two-boolean form. The
// sole retail caller passes an int*, and the catch handler stores through it.
bool open(bool openSprites, bool openBitmaps, int* errorCode);
void close();
void expunge();
unsigned char report(const char* filename);
void setPath(const char* path);
void setPixelFormat(unsigned long redMask, unsigned long greenMask,
                    unsigned long blueMask);             // 0x55a6b0
CSprite* getSprite(const char* name);
font* getFont(const char* name);
// Dreamcast and retail oldmain load the same Players.pal through the 24-bit
// sibling immediately after the two TPalette16 loads (retail 0x55b470).
TPalette24* getPalette24(const char* name);
sample* getSample(const char* name);
// Retail body 0x55a800 (bitmapBorder::SetImage's loader).
Bitmap816* getBitmap816(const char* name);
Bitmap16Bit* getBitmap16(const char* name);
void getBackdrop(const char* resName, Bitmap16Bit* destBmap);
TTextResource* getText(const char* name);
TSpreadsheetResource* getSpreadsheet(const char* name);
void addToCache(resource* value);
// Dreamcast resourcemanager.cpp:2377; expanded by Complete's cache getters.
resource* getFromCache(const char* name);

// Existing disposal wrappers expand across TUs in the selection destructor:
// Windows 0x583bb8..0x583c35 and Mac 0x17b5b0..0x17b6bc retain only the
// member virtual calls. Their bodies must be visible at those source calls.
inline void dispose(resource* value) { value->dispose(); }
void dispose(sample* value);
inline void dispose(CSprite* value) { value->dispose(); }
void delSprFromCache();

LODFile* pointToSpriteResource(const char* name);
LODFile* pointToBitmapResource(const char* name);

int readFromBitmapResource(LODFile* resource, void* data, int numBytes);
// Retail 0x55d070 walks the active context's bitmap LOD list until
// getItemIndex finds the named entry, then returns that entry's +0x14 size.
int getBitmapResourceSize(const char* name);

resource* getFromCache(const char* name);

}

// Role-based name for Complete's common missing-resource reporter at
// retail 0x559510. Its fastcall ABI is proven by all thirteen getter call
// sites and its body is reconstructed; no PC symbol source survives, so the
// source name remains explicitly provisional. Use ordinary C++ linkage:
// the bootstrap extern "C" suppressed VC6's exception cleanup in both
// reporters; removing it restores the retail fs:[0] registration and unwind
// states without changing their proven fastcall argument ABI.
void __fastcall reportMissingTypedResource(const char* caller,
                                          int resourceType,
                                          const char* resourceName);

// Complete's sprite-family counterpart. GetSprite's two
// retail call sites prove the same fastcall surface; the PC symbol name is
// provisional because this helper has no Dreamcast identity.
void __fastcall reportMissingSpriteResource(const char* caller,
                                           int resourceType,
                                           const char* resourceName);

#endif  /* HOMM3_RESOURCEMANAGER_H */
