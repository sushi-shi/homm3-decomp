// resourcemanager.h - prototypes of resourcemanager.cpp (compiland resourcemanager.obj)
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

// --- ResourceManager ---
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:158, dc 0x1213a0) void ResourceManager::RemapGraphics();
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:222, dc 0x121524) void ResourceManager::SaturateGraphics();
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:298, dc 0x12173c) unsigned char ResourceManager::Open(unsigned char open_sprites, unsigned char open_bitmaps);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:338, dc 0x121880) void ResourceManager::Close();
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:357, dc 0x12189c) void ResourceManager::SetPath(const char* path);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:374, dc 0x1218c4) void ResourceManager::SetPixelFormat(unsigned long red_mask, unsigned long green_mask, unsigned long blue_mask);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:438, dc 0x121a2c) TGenericResource* ResourceManager::GetResource(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:729, dc 0x121ac8) Bitmap816* ResourceManager::GetBitmap816(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:906, dc 0x121c5c) Bitmap16Bit* ResourceManager::GetBitmap16(const char* name, unsigned char ignore_cache);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:1027, dc 0x121d90) TPalette16* ResourceManager::GetPalette(const char* name, unsigned char ignore_cache);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:1133, dc 0x121ec8) TPalette24* ResourceManager::GetPalette24(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:1221, dc 0x121fac) font* ResourceManager::GetFont(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:1356, dc 0x12208c) TTextResource* ResourceManager::GetText(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:1566, dc 0x1221fc) unsigned char ResourceManager::GetSoundFile(char* localName, void** data, SoundHeaderStruct** snd, int* size);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:1888, dc 0x122320) CSprite* ResourceManager::GetSprite(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2080, dc 0x122434) void ResourceManager::GetBackdrop(const char* resName, Bitmap16Bit* destBmap);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2110, dc 0x122484) unsigned char ResourceManager::PointToSpriteResource(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2115, dc 0x12249c) int ResourceManager::ReadFromSpriteResource(void* data, int numBytes);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2120, dc 0x1224b4) unsigned char ResourceManager::PointToBitmapResource(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2125, dc 0x1224cc) int ResourceManager::ReadFromBitmapResource(void* data, int numBytes);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2130, dc 0x1224e4) int ResourceManager::GetBitmapResourceSize(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2141, dc 0x122530) void ResourceManager::Dispose(resource* kill);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2196, dc 0x1225c0) void ResourceManager::Dispose(sample* sam);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2204, dc 0x1225dc) void ResourceManager::Dispose(CSprite* kill);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2280, dc 0x1226d4) void ResourceManager::del_Spr_from_Cache();
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2359, dc 0x1228ac) void ResourceManager::Expunge();
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2377, dc 0x122928) resource* ResourceManager::GetFromCache(const char* name);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:2404, dc 0x1229f8) unsigned char ResourceManager::Report(const char* filename);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:121, dc 0x122bd0) void ResourceManager::TCacheMapKey::TCacheMapKey(const char* n);
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:126, dc 0x122bf8) unsigned char ResourceManager::TCacheMapKey::operator<(const ResourceManager::TCacheMapKey* y);

// --- std ---
// CODEVIEW(E:\gamedcs\resourcemanager.cpp:136, dc 0x122c14) void std::map<ResourceManager::TCacheMapKey,resource *,std::less<ResourceManager::T();

#endif  /* HOMM3_RESOURCEMANAGER_H */
