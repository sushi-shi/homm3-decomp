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
class Bitmap24Bit;
class TPalette16;
class TSpreadsheetResource;
class TTextResource;

struct SoundHeaderStruct;

// Loki h3maped object 37 (ResourceManager.cpp, RoE source): the resource
// cache and LOD access as namespace-level functions under their original
// spellings. Return types are provisional until each body is matched.
namespace ResourceManager {

void RemapGraphics();
void SaturateGraphics();
bool Open(bool openSprites, bool openBitmaps);
void Close();
// The port's stand-in for the Windows CRT _fullpath.
char* _fullpath(char* absPath, const char* relPath, int maxLength);
void SetPath(const char* path);
void SetPixelFormat(unsigned long redMask, unsigned long greenMask, unsigned long blueMask);
resource* GetResource(const char* name);
Bitmap816* GetBitmap816(const char* name);
Bitmap24Bit* GetBitmap24(const char* name);
Bitmap16Bit* GetBitmap16(const char* name, bool unknown);
TPalette16* GetPalette(const char* name, bool unknown);
TPalette24* GetPalette24(const char* name);
font* GetFont(const char* name);
TTextResource* GetText(const char* name);
TSpreadsheetResource* GetSpreadsheet(const char* name);
int GetSoundFile(char* name, void*& data, SoundHeaderStruct*& header, int& size);
sample* GetSample(const char* name);
CSprite* GetSprite(const char* name);
void GetBackdrop(const char* name, Bitmap16Bit* destBmap);
void GetBackdrop24(const char* name, Bitmap16Bit* destBmap);
LODFile* PointToSpriteResource(const char* name);
int ReadFromSpriteResource(void* data, int numBytes);
LODFile* PointToBitmapResource(const char* name);
int ReadFromBitmapResource(void* data, int numBytes);
int GetBitmapResourceSize(const char* name);
void Dispose(resource* value);
void Dispose(CSprite* value);
void Expunge();
void Report(const char* filename);

}

#endif  /* HOMM3_RESOURCEMANAGER_H */
