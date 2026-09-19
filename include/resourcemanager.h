// resourcemanager.h - resourcemanager.cpp (compiland resourcemanager.obj)
#ifndef HOMM3_RESOURCEMANAGER_H
#define HOMM3_RESOURCEMANAGER_H

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
void remapGraphics();
void saturateGraphics();
// Complete adds an error-code output to Dreamcast's two-boolean form. The
// sole retail caller passes an int*, and the catch handler stores through it.
bool open(bool openSprites, bool openBitmaps, int* errorCode);
void close();
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

void dispose(resource* value);
void dispose(CSprite* value);
void delSprFromCache();

LODFile* pointToSpriteResource(const char* name);
LODFile* pointToBitmapResource(const char* name);

int readFromBitmapResource(LODFile* resource, void* data, int numBytes);
// Retail 0x55d070 walks the active context's bitmap LOD list until
// getItemIndex finds the named entry, then returns that entry's +0x14 size.
int getBitmapResourceSize(const char* name);

resource* getFromCache(const char* name);

}

// Bootstrap name for Complete's large common missing-resource reporter at
// retail 0x559510. Its fastcall ABI is proven by all thirteen getter call
// sites and its body is reconstructed; no PC symbol source survives, so the
// linkage name remains explicitly provisional.
extern "C" void __fastcall game_null_159510(const char* caller,
                                             int resourceType,
                                             const char* resourceName);

// Complete's sprite-family counterpart to game_null_159510. GetSprite's two
// retail call sites prove the same fastcall surface; the PC symbol name is
// provisional because this helper has no Dreamcast identity.
extern "C" void __fastcall game_sprite_1599e0(const char* caller,
                                               int resourceType,
                                               const char* resourceName);

#endif  /* HOMM3_RESOURCEMANAGER_H */
