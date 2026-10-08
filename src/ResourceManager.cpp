// ResourceManager.cpp - Loki h3maped object 37: the resource cache over the
// two LOD archives (h3bitmap.lod, h3sprite.lod) and the typed loaders.
// The loaders keep RoE's unused loose-file path (a FILE* and a flag that
// stay null/false) beside the LOD reads.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <strstream.h>
#include <map>
#include <string>

#include "resourcemanager.h"

#include "bitmap16.h"
#include "bitmap24.h"
#include "bitmap816.h"
#include "csprite.h"
#include "cspriteframe.h"
#include "font.h"
#include "genericresource.h"
#include "lodfile.h"
#include "palette.h"
#include "resourcemanager_sprite_headers.h"
#include "sample.h"
#include "soundheader.h"
#include "textresource.h"

namespace ResourceManager {

// The cache key: a resource name compared without case.
struct TCacheMapKey {
    TCacheMapKey(const char* name)
    {
        strncpy(Name, name, 12);
        Name[12] = 0;
    }
    bool operator<(const TCacheMapKey& other) const
    {
        return strcasecmp(Name, other.Name) < 0;
    }

    char Name[13];
};

// The three ints that open every bitmap entry of h3bitmap.lod.
struct TBitmapHeader {
    int size;
    int width;
    int height;
};

static char BitmapLodName[] = "h3bitmap.lod";
static char SpriteLodName[] = "h3sprite.lod";
static bool BitmapsOpen = false;
static bool SpritesOpen = false;
bool SaturatedGraphicsEasterEgg = false;
static int RemapCount = 0;
static int SaturateCount = 0;

static string Path;
static LODFile BitmapLod;
static LODFile SpriteLod;
static map<TCacheMapKey, resource*> Cache;

int RedBits;
int GreenBits;
int BlueBits;
int RedShift;
int GreenShift;
int BlueShift;
int RedMask;
int GreenMask;
int BlueMask;

static resource* GetFromCache(const char* name);
static void AddToCache(resource* r);
static void ReportResourceNotFound(const char* function, EResourceType type, const char* name);
static void ReportSoundNotFound(const char* function, EResourceType type, const char* name);
static void ReportSpriteNotFound(const char* function, EResourceType type, const char* name);

void RemapGraphics()
{
    for (map<TCacheMapKey, resource*>::iterator it = Cache.begin(); it != Cache.end(); it++) {
        resource* r = it->second;
        ++RemapCount;
        switch (r->get_resType()) {
        case RESOURCE_TYPE_BITMAP16: {
            Bitmap16Bit* bmp = GetBitmap16(r->get_Name(), true);
            if (bmp) {
                bmp->Draw(0, 0, bmp->GetWidth(), bmp->GetHeight(), (Bitmap16Bit*)r, 0, 0, false);
                delete bmp;
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
            ((CSprite*)r)->ResetPalette();
            break;
        case RESOURCE_TYPE_BITMAP:
            ((Bitmap816*)r)->ResetPalette();
            break;
        case RESOURCE_TYPE_FONT: {
            TPalette16* pal = GetPalette("game.pal", true);
            if (pal) {
                ((font*)r)->SetPalette(*pal);
                delete pal;
            }
            break;
        }
        case RESOURCE_TYPE_PALETTE: {
            TPalette16* pal = GetPalette(r->get_Name(), true);
            if (pal) {
                memcpy(((TPalette16*)r)->m_data, pal->m_data, sizeof(pal->m_data));
                delete pal;
            }
            break;
        }
        }
    }
}

void SaturateGraphics()
{
    for (map<TCacheMapKey, resource*>::iterator it = Cache.begin(); it != Cache.end(); it++) {
        resource* r = it->second;
        ++SaturateCount;
        switch (r->get_resType()) {
        case RESOURCE_TYPE_BITMAP16: {
            Bitmap16Bit* bmp = GetBitmap16(r->get_Name(), true);
            if (bmp) {
                bmp->Draw(0, 0, bmp->GetWidth(), bmp->GetHeight(), (Bitmap16Bit*)r, 0, 0, false);
                delete bmp;
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
            ((CSprite*)r)->GetPalette24()->AdjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
            ((CSprite*)r)->ResetPalette();
            break;
        case RESOURCE_TYPE_BITMAP:
            ((Bitmap816*)r)->GetPalette24().AdjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
            ((Bitmap816*)r)->ResetPalette();
            break;
        case RESOURCE_TYPE_FONT: {
            TPalette16* pal = GetPalette("game.pal", true);
            if (pal) {
                ((font*)r)->SetPalette(*pal);
                delete pal;
            }
            break;
        }
        case RESOURCE_TYPE_PALETTE: {
            TPalette16* pal = GetPalette(r->get_Name(), true);
            if (pal) {
                memcpy(((TPalette16*)r)->m_data, pal->m_data, sizeof(pal->m_data));
                delete pal;
            }
            break;
        }
        }
    }
}

bool Open(bool openSprites, bool openBitmaps)
{
    int result;
    if (openBitmaps) {
        string fileName = Path;
        fileName += BitmapLodName;
        result = BitmapLod.open(fileName.c_str(), 1);
        if (result == 0)
            BitmapsOpen = true;
    }
    if (openSprites) {
        string fileName = Path;
        fileName += SpriteLodName;
        result = SpriteLod.open(fileName.c_str(), 1);
        if (result == 0)
            SpritesOpen = true;
    }
    return (SpritesOpen || !openSprites) && (BitmapsOpen || !openBitmaps);
}

void Close()
{
    Expunge();
    SpritesOpen = false;
    BitmapsOpen = false;
}

void _fullpath(char* absPath, const char* relPath, int maxLength)
{
    strncpy(absPath, relPath, maxLength);
}

void SetPath(const char* path)
{
    char fullPath[4096];
    _fullpath(fullPath, path, sizeof(fullPath) - 1);
    Path = fullPath;
}

void SetPixelFormat(unsigned long redMask, unsigned long greenMask, unsigned long blueMask)
{
    CSprite::SetPixelFormat(redMask, greenMask, blueMask);
    Bitmap16Bit::SetPixelFormat(redMask, greenMask, blueMask);
    TPalette16::SetPixelFormat(redMask, greenMask, blueMask);

    RedMask = redMask;
    GreenMask = greenMask;
    BlueMask = blueMask;

    for (RedShift = 0; (redMask & 1) == 0 && redMask != 0; redMask >>= 1)
        RedShift++;
    for (RedBits = 0; redMask != 0; redMask >>= 1)
        RedBits++;
    for (GreenShift = 0; (greenMask & 1) == 0 && greenMask != 0; greenMask >>= 1)
        GreenShift++;
    for (GreenBits = 0; greenMask != 0; greenMask >>= 1)
        GreenBits++;
    for (BlueShift = 0; (blueMask & 1) == 0 && blueMask != 0; blueMask >>= 1)
        BlueShift++;
    for (BlueBits = 0; blueMask != 0; blueMask >>= 1)
        BlueBits++;
}

resource* GetResource(const char* name)
{
    TGenericResource* r = (TGenericResource*)GetFromCache(name);
    if (r)
        return r;

    FILE* fp = NULL;
    bool fromFile = false;
    int size;
    {
        LODEntry* entry = BitmapLod.getItemIndex(name);
        if (!entry) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_DATA, name);
            return NULL;
        }
        if (entry->m_attrib != RESOURCE_TYPE_DATA) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_DATA, name);
            return NULL;
        }
        if (!BitmapLod.pointAt(name)) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_DATA, name);
            return NULL;
        }
        size = entry->m_size;
    }
    char* data = new char[size];
    if (!fromFile)
        BitmapLod.read(data, size);
    else
        fread(data, size, 1, fp);
    r = new TGenericResource(name, size, data);
    delete[] data;
    if (fromFile)
        fclose(fp);
    if (r)
        AddToCache(r);
    return r;
}

Bitmap816* GetBitmap816(const char* name)
{
    const char* defaultName = "default.pcx";
    Bitmap816* bmp = (Bitmap816*)GetFromCache(name);
    if (bmp)
        return bmp;

    bool found = BitmapLod.pointAt(name);
    if (!found) {
        ReportResourceNotFound("GetBitmap8", RESOURCE_TYPE_BITMAP, name);
        found = BitmapLod.pointAt(defaultName);
    }
    if (!found) {
        ReportResourceNotFound("GetBitmap8", RESOURCE_TYPE_BITMAP, name);
        return NULL;
    }

    TBitmapHeader header;
    BitmapLod.read(&header, sizeof(header));
    unsigned char* data = new unsigned char[header.size];
    BitmapLod.read(data, header.size);
    TPalette24 palette24;
    BitmapLod.read(palette24.m_palette, sizeof(palette24.m_palette));
    if (SaturatedGraphicsEasterEgg)
        palette24.AdjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
    TPalette16 palette16(palette24, RedBits, RedShift, GreenBits, GreenShift, BlueBits, BlueShift);
    bmp = new Bitmap816(name, header.width, header.height, data, palette16, header.size);
    bmp->SetPalette(palette24);
    delete[] data;
    if (bmp)
        AddToCache(bmp);
    return bmp;
}

Bitmap24Bit* GetBitmap24(const char* name)
{
    const char* defaultName = "dfault24.pcx";
    Bitmap24Bit* bmp = (Bitmap24Bit*)GetFromCache(name);
    if (bmp)
        return bmp;

    bool found = BitmapLod.pointAt(name);
    if (!found) {
        ReportResourceNotFound("GetBitmap24", RESOURCE_TYPE_BITMAP24, name);
        found = BitmapLod.pointAt(defaultName);
    }
    if (!found) {
        ReportResourceNotFound("GetBitmap24", RESOURCE_TYPE_BITMAP24, name);
        return NULL;
    }

    TBitmapHeader header;
    BitmapLod.read(&header, sizeof(header));
    unsigned char* data = new unsigned char[header.size];
    BitmapLod.read(data, header.size);
    bmp = new Bitmap24Bit(name, header.width, header.height, data, header.size);
    delete[] data;
    if (bmp)
        AddToCache(bmp);
    return bmp;
}

Bitmap16Bit* GetBitmap16(const char* name, bool noCache)
{
    const char* defaultName = "dfault24.pcx";
    Bitmap16Bit* bmp;
    if (!noCache) {
        bmp = (Bitmap16Bit*)GetFromCache(name);
        if (bmp)
            return bmp;
    }

    bool found = BitmapLod.pointAt(name);
    if (!found) {
        ReportResourceNotFound("GetBitmap16", RESOURCE_TYPE_BITMAP24, name);
        found = BitmapLod.pointAt(defaultName);
    }
    if (!found) {
        ReportResourceNotFound("GetBitmap16", RESOURCE_TYPE_BITMAP24, name);
        return NULL;
    }

    TBitmapHeader header;
    BitmapLod.read(&header, sizeof(header));
    unsigned char* data = new unsigned char[header.size];
    BitmapLod.read(data, header.size);
    Bitmap24Bit* bmp24 = new Bitmap24Bit(name, header.width, header.height, data, header.size);
    bmp = new Bitmap16Bit(name, bmp24->GetWidth(), bmp24->GetHeight());
    if (bmp && bmp24) {
        if (SaturatedGraphicsEasterEgg)
            bmp24->AdjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
        bmp24->Draw(0, 0, bmp24->GetWidth(), bmp24->GetHeight(), bmp, 0, 0);
        if (!noCache)
            AddToCache(bmp);
    }
    delete bmp24;
    delete[] data;
    return bmp;
}

TPalette16* GetPalette(const char* name, bool noCache)
{
    const char* defaultName = "default.pal";
    TPalette16* pal = NULL;
    if (!noCache) {
        pal = (TPalette16*)GetFromCache(name);
        if (pal)
            return pal;
    }

    FILE* fp = NULL;
    bool fromFile = false;
    bool found = BitmapLod.pointAt(name);
    if (!found) {
        ReportResourceNotFound("GetPalette", RESOURCE_TYPE_PALETTE, name);
        found = BitmapLod.pointAt(defaultName);
    }
    if (!found) {
        ReportResourceNotFound("GetPalette", RESOURCE_TYPE_PALETTE, name);
        return NULL;
    }

    char header[24];
    if (!fromFile)
        BitmapLod.read(header, sizeof(header));
    else
        fseek(fp, sizeof(header), SEEK_CUR);
    TRGBA colors[256];
    if (!fromFile)
        BitmapLod.read(colors, sizeof(colors));
    else
        fread(colors, sizeof(colors), 1, fp);
    TPalette24 palette24(colors);
    if (SaturatedGraphicsEasterEgg)
        palette24.AdjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
    pal = new TPalette16(name, palette24, RedBits, RedShift, GreenBits, GreenShift, BlueBits, BlueShift);
    if (fromFile)
        fclose(fp);
    if (pal && !noCache)
        AddToCache(pal);
    return pal;
}

TPalette24* GetPalette24(const char* name)
{
    const char* defaultName = "default.pal";
    TPalette24* pal = NULL;
    FILE* fp = NULL;
    bool fromFile = false;
    bool found = BitmapLod.pointAt(name);
    if (!found) {
        ReportResourceNotFound("GetPalette", RESOURCE_TYPE_PALETTE, name);
        found = BitmapLod.pointAt(defaultName);
    }
    if (!found) {
        ReportResourceNotFound("GetPalette", RESOURCE_TYPE_PALETTE, name);
        return NULL;
    }

    char header[24];
    if (!fromFile)
        BitmapLod.read(header, sizeof(header));
    else
        fseek(fp, sizeof(header), SEEK_CUR);
    TRGBA colors[256];
    if (!fromFile)
        BitmapLod.read(colors, sizeof(colors));
    else
        fread(colors, sizeof(colors), 1, fp);
    pal = new TPalette24(colors);
    if (SaturatedGraphicsEasterEgg)
        pal->AdjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
    if (fromFile)
        fclose(fp);
    return pal;
}

font* GetFont(const char* name)
{
    const char* defaultName = "default.fnt";
    font* f = (font*)GetFromCache(name);
    if (f)
        return f;

    FILE* fp = NULL;
    bool fromFile = false;
    int size;
    {
        LODEntry* entry = BitmapLod.getItemIndex(name);
        if (!entry) {
            ReportResourceNotFound("GetFont", RESOURCE_TYPE_FONT, name);
            entry = BitmapLod.getItemIndex(defaultName);
        }
        if (!entry) {
            ReportResourceNotFound("GetFont", RESOURCE_TYPE_FONT, name);
            return NULL;
        }
        BitmapLod.pointAt(entry->m_name);
        size = entry->m_size;
    }
    font::TFontSpec spec;
    if (!fromFile)
        BitmapLod.read(&spec, sizeof(spec));
    else
        fread(&spec, sizeof(spec), 1, fp);
    int dataSize = size - sizeof(spec);
    unsigned char* data = new unsigned char[dataSize];
    if (!data)
        return NULL;
    if (!fromFile) {
        BitmapLod.read(data, dataSize);
    } else {
        int count = fread(data, 1, dataSize, fp);
        if (count != dataSize) {
            int error = ferror(fp);
            error = feof(fp);
        }
    }
    f = new font(name, spec, dataSize, data);
    delete[] data;
    if (fromFile)
        fclose(fp);
    if (f)
        AddToCache(f);
    TPalette16* pal = GetPalette("game.pal", false);
    if (pal) {
        if (f)
            f->SetPalette(*pal);
        Dispose(pal);
    }
    return f;
}

TTextResource* GetText(const char* name)
{
    TTextResource* text = (TTextResource*)GetFromCache(name);
    if (text)
        return text;

    FILE* fp = NULL;
    bool fromFile = false;
    int size;
    {
        LODEntry* entry = BitmapLod.getItemIndex(name);
        if (!entry) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_TEXT, name);
            return NULL;
        }
        if (entry->m_attrib != RESOURCE_TYPE_TEXT) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_TEXT, name);
            return NULL;
        }
        if (!BitmapLod.pointAt(name)) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_TEXT, name);
            return NULL;
        }
        size = entry->m_size;
    }
    char* data = new char[size];
    if (!fromFile)
        BitmapLod.read(data, size);
    else
        fread(data, size, 1, fp);
    text = new TTextResource(name, size, data);
    delete[] data;
    if (fromFile)
        fclose(fp);
    if (text)
        AddToCache(text);
    return text;
}

TSpreadsheetResource* GetSpreadsheet(const char* name)
{
    TSpreadsheetResource* sheet = (TSpreadsheetResource*)GetFromCache(name);
    if (sheet)
        return sheet;

    FILE* fp = NULL;
    bool fromFile = false;
    int size;
    {
        LODEntry* entry = BitmapLod.getItemIndex(name);
        if (!entry) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_TEXT, name);
            return NULL;
        }
        if (entry->m_attrib != RESOURCE_TYPE_TEXT) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_TEXT, name);
            return NULL;
        }
        if (!BitmapLod.pointAt(name)) {
            ReportResourceNotFound("GetResource", RESOURCE_TYPE_TEXT, name);
            return NULL;
        }
        size = entry->m_size;
    }
    char* data = new char[size];
    if (!fromFile)
        BitmapLod.read(data, size);
    else
        fread(data, size, 1, fp);
    sheet = new TSpreadsheetResource(name, size, data);
    delete[] data;
    if (fromFile)
        fclose(fp);
    if (sheet)
        AddToCache(sheet);
    return sheet;
}

bool GetSoundFile(char* name, void*& data, SoundHeaderStruct*& header, int& size)
{
    printf("GetSoundFile() in ResourceManager...Write me!\n");
    exit(0);

    int result;
    int i;
    for (i = 0; i < numSound; i++) {
        if (strcasecmp(SoundHeader[i].m_filename, name) == 0) {
            header = &SoundHeader[i];
            size = header->m_size;
            data = new char[size];
            return true;
        }
    }
    for (i = 0; i < numSoundCD; i++) {
        if (strcasecmp(SoundHeaderCD[i].m_filename, name) == 0) {
            header = &SoundHeaderCD[i];
            size = header->m_size;
            data = new char[size];
            return true;
        }
    }
    return false;
}

sample* GetSample(const char* name)
{
    const char* defaultName = "default.wav";
tryAgain:
    sample* s = (sample*)GetFromCache(name);
    if (s)
        return s;

    FILE* fp = NULL;
    bool fromFile = false;
    int size;
    char* data;
    char baseName[4096];
    SoundHeaderStruct* header;
    strcpy(baseName, name);
    strtok(baseName, ".");
    if (!GetSoundFile(baseName, (void*&)data, header, size)) {
        if (name != defaultName) {
            ReportSoundNotFound("GetSample", RESOURCE_TYPE_SFX, name);
            name = defaultName;
            goto tryAgain;
        }
        if (data)
            delete[] data;
        return NULL;
    }
    s = new sample(name, data, size, 0, 127, 1);
    delete[] data;
    if (s)
        AddToCache(s);
    return s;
}

CSprite* GetSprite(const char* name)
{
    const char* defaultName = "default.def";
    const char* defaultPalette = "default.pal";
    int i;
    LODEntry* entry;
    CSprite* sprite = (CSprite*)GetFromCache(name);
    if (sprite)
        return sprite;

    bool found = SpriteLod.pointAt(name);
    entry = SpriteLod.getItemIndex(name);
    if (!found) {
        ReportSpriteNotFound("GetSprite", RESOURCE_TYPE_SPRITE, name);
        found = SpriteLod.pointAt(defaultName);
        entry = SpriteLod.getItemIndex(defaultName);
    }
    if (!found) {
        ReportSpriteNotFound("GetSprite", RESOURCE_TYPE_SPRITE, name);
        return NULL;
    }

    SpriteDefHeader sdef;
    unsigned char* fileData = new unsigned char[entry->m_size];
    unsigned char* definitionPosition = fileData;
    SpriteLod.read(fileData, entry->m_size);
    memcpy(&sdef, definitionPosition, sizeof(sdef));
    definitionPosition += sizeof(sdef);

    sprite = new CSprite(name, sdef.m_type, sdef.m_width, sdef.m_height);
    if (!sprite)
        return NULL;

    TSpriteDataHeader* sequences = new TSpriteDataHeader[sdef.m_numSequences];
    for (i = 0; i < sdef.m_numSequences; i++) {
        memcpy(&sequences[i], definitionPosition, sizeof(TSpriteDataHeader));
        definitionPosition += sizeof(TSpriteDataHeader);
        sequences[i].m_frameNames = new char[sequences[i].m_numFrames][13];
        memcpy(sequences[i].m_frameNames, definitionPosition, sequences[i].m_numFrames * 13);
        definitionPosition += sequences[i].m_numFrames * 13;
        sequences[i].m_frameOffsets = new int[sequences[i].m_numFrames];
        memcpy(sequences[i].m_frameOffsets, definitionPosition, sequences[i].m_numFrames * sizeof(int));
        definitionPosition += sequences[i].m_numFrames * sizeof(int);
    }

    for (i = 0; i < sdef.m_numSequences; i++) {
        int sequenceNumber = sequences[i].m_sequenceNumber;
        sprite->AllocateSeq(sequenceNumber, sequences[i].m_numFrames);
        for (int frameIndex = 0; frameIndex < sequences[i].m_numFrames; frameIndex++) {
            TCompactSpriteFrameHeader compactHeader;
            TCroppedSpriteFrameHeader croppedHeader;
            unsigned char* frameData;
            int result;
            if (sdef.m_type == RESOURCE_TYPE_SPRITE ||
                sdef.m_type == RESOURCE_TYPE_CREATURE ||
                sdef.m_type == RESOURCE_TYPE_ADVENTURE_OBJECT ||
                sdef.m_type == RESOURCE_TYPE_HERO ||
                sdef.m_type == RESOURCE_TYPE_INTERFACE ||
                sdef.m_type == RESOURCE_TYPE_TILESET ||
                sdef.m_type == RESOURCE_TYPE_POINTER ||
                sdef.m_type == RESOURCE_TYPE_COMBAT_HERO) {
                memcpy(&croppedHeader, fileData + sequences[i].m_frameOffsets[frameIndex],
                       sizeof(croppedHeader));
                frameData = new unsigned char[croppedHeader.m_dataSize];
                memcpy(frameData, fileData + (sequences[i].m_frameOffsets[frameIndex] + sizeof(croppedHeader)),
                       croppedHeader.m_dataSize);
            } else {
                memcpy(&compactHeader, definitionPosition, sizeof(compactHeader));
                definitionPosition += sizeof(compactHeader);
                frameData = new unsigned char[compactHeader.m_dataSize];
                memcpy(frameData, fileData + sequences[i].m_frameOffsets[frameIndex],
                       compactHeader.m_dataSize);
            }

            CSpriteFrame* frame = NULL;
            frame = (CSpriteFrame*)GetFromCache(sequences[i].m_frameNames[frameIndex]);
            if (!frame) {
                if (sdef.m_type == RESOURCE_TYPE_SPRITE ||
                    sdef.m_type == RESOURCE_TYPE_CREATURE ||
                    sdef.m_type == RESOURCE_TYPE_ADVENTURE_OBJECT ||
                    sdef.m_type == RESOURCE_TYPE_HERO ||
                    sdef.m_type == RESOURCE_TYPE_INTERFACE ||
                    sdef.m_type == RESOURCE_TYPE_TILESET ||
                    sdef.m_type == RESOURCE_TYPE_POINTER ||
                    sdef.m_type == RESOURCE_TYPE_COMBAT_HERO)
                    frame = new CSpriteFrame(sequences[i].m_frameNames[frameIndex],
                                             croppedHeader.m_width, croppedHeader.m_height,
                                             frameData, croppedHeader.m_dataSize,
                                             croppedHeader.m_encoding,
                                             croppedHeader.m_croppedWidth,
                                             croppedHeader.m_croppedHeight,
                                             croppedHeader.m_croppedX, croppedHeader.m_croppedY);
                else
                    frame = new CSpriteFrame(sequences[i].m_frameNames[frameIndex],
                                             compactHeader.m_width, compactHeader.m_height,
                                             frameData, compactHeader.m_dataSize,
                                             croppedHeader.m_encoding);
                AddToCache(frame);
            }
            result = sprite->AddFrame(sequenceNumber, frame);
            delete[] frameData;
        }
    }

    for (i = 0; i < sdef.m_numSequences; i++) {
        delete[] sequences[i].m_frameNames;
        delete[] sequences[i].m_frameOffsets;
    }
    delete[] sequences;

    TPalette24 palette24(sdef.m_palette);
    if (SaturatedGraphicsEasterEgg)
        palette24.AdjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
    TPalette16 palette16(palette24, RedBits, RedShift, GreenBits, GreenShift, BlueBits, BlueShift);
    sprite->SetPalette(palette16);
    sprite->SetPalette(palette24);
    delete[] fileData;
    AddToCache(sprite);
    return sprite;
}

void GetBackdrop(const char* name, Bitmap16Bit* destBmap)
{
    Bitmap816* bmp = GetBitmap816(name);
    if (bmp) {
        bmp->Draw(0, 0, bmp->GetWidth(), bmp->GetHeight(), destBmap, 0, 0, false);
        Dispose(bmp);
    } else
        ReportResourceNotFound("GetBackdrop", RESOURCE_TYPE_BITMAP, name);
}

void GetBackdrop24(const char* name, Bitmap16Bit* destBmap)
{
    Bitmap24Bit* bmp = GetBitmap24(name);
    if (bmp) {
        bmp->Draw(0, 0, bmp->GetWidth(), bmp->GetHeight(), destBmap, 0, 0);
        Dispose(bmp);
    } else
        ReportResourceNotFound("GetBackdrop24", RESOURCE_TYPE_BITMAP, name);
}

bool PointToSpriteResource(const char* name)
{
    return SpriteLod.pointAt(name);
}

int ReadFromSpriteResource(void* data, int numBytes)
{
    return SpriteLod.read(data, numBytes);
}

bool PointToBitmapResource(const char* name)
{
    return BitmapLod.pointAt(name);
}

int ReadFromBitmapResource(void* data, int numBytes)
{
    return BitmapLod.read(data, numBytes);
}

int GetBitmapResourceSize(const char* name)
{
    return BitmapLod.getItemIndex(name)->m_size;
}

void Dispose(resource* r)
{
    if (r == NULL)
        return;
    int count = r->Release();
    if (count == 0) {
        map<TCacheMapKey, resource*>::iterator it = Cache.find(TCacheMapKey(r->get_Name()));
        if (it != Cache.end()) {
            Cache.erase(it);
            delete r;
        }
    }
}

void Dispose(CSprite* s)
{
    if (s == NULL)
        return;
    int count = s->Release();
    if (count == 0) {
        int numSeqs = CSprite::GetNumSeqs(s->get_resType());
        for (int seq = 0; seq < numSeqs; seq++) {
            if (s->IsValidSeq(seq)) {
                int numFrames = s->GetNumFrames(seq);
                for (int frame = 0; frame < numFrames; frame++) {
                    resource* f = s->GetFrame(seq, frame);
                    if (f)
                        Dispose(f);
                }
            }
        }
        map<TCacheMapKey, resource*>::iterator it = Cache.find(TCacheMapKey(s->get_Name()));
        if (it != Cache.end()) {
            Cache.erase(it);
            delete s;
        }
    }
}

void Expunge()
{
    for (map<TCacheMapKey, resource*>::iterator it = Cache.begin(); it != Cache.end(); it++)
        delete it->second;
    Cache.clear();
}

static resource* GetFromCache(const char* name)
{
    map<TCacheMapKey, resource*>::iterator it = Cache.find(TCacheMapKey(name));
    if (it == Cache.end())
        return NULL;
    resource* r = it->second;
    r->AddRef();
    return r;
}

static void AddToCache(resource* r)
{
#line 2039
    assert(*r->get_Name() != 0);
    Cache.insert(pair<TCacheMapKey, resource*>(TCacheMapKey(r->get_Name()), r));
    r->AddRef();
}

bool Report(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (fp == NULL)
        return false;
    for (map<TCacheMapKey, resource*>::const_iterator it = Cache.begin(); it != Cache.end(); it++)
        fprintf(fp, "%13s: %13s 0x%08x %d\n", it->first.Name, it->second->get_Name(),
                it->second->get_resType(), it->second->GetReferenceCount());
    fclose(fp);
    return true;
}

static void ReportResourceNotFound(const char* function, EResourceType type, const char* name)
{
    ostrstream message;
    message << "ResourceManager::" << function << " could not find the '";
    switch (type) {
    case RESOURCE_TYPE_NONE: message << "null"; break;
    case RESOURCE_TYPE_DATA: message << "data"; break;
    case RESOURCE_TYPE_TEXT: message << "text"; break;
    case RESOURCE_TYPE_BITMAP: message << "bitmap8"; break;
    case RESOURCE_TYPE_BITMAP24: message << "bitmap24"; break;
    case RESOURCE_TYPE_BITMAP16: message << "bitmap16"; break;
    case RESOURCE_TYPE_BITMAP565: message << "bitmap565"; break;
    case RESOURCE_TYPE_BITMAP555: message << "bitmap555"; break;
    case RESOURCE_TYPE_BITMAP1555: message << "bitmap1555"; break;
    case RESOURCE_TYPE_MIDI: message << "midi"; break;
    case RESOURCE_TYPE_FONT: message << "font"; break;
    case RESOURCE_TYPE_PALETTE: message << "palette"; break;
    default: message << "0x" << hex << type; break;
    }
    message << "' resource '" << name << "' in the file '" << Path << BitmapLodName << "'." << ends;
    printf("\n------------------------------\n%s\n------------------------------\n\n", message.str());
    message.freeze(0);
}

static void ReportSoundNotFound(const char* function, EResourceType type, const char* name)
{
    ostrstream message;
    message << "ResourceManager::" << function << " could not find the '";
    message << "sfx";
    message << "' resource '" << name << ends;
    printf("\n------------------------------\n%s\n------------------------------\n\n", message.str());
    message.freeze(0);
}

static void ReportSpriteNotFound(const char* function, EResourceType type, const char* name)
{
    ostrstream message;
    message << "ResourceManager::" << function << " could not find the '";
    switch (type) {
    case RESOURCE_TYPE_SPRITE: message << "sprite"; break;
    case RESOURCE_TYPE_SPRITE_DEFINITION: message << "spritedef"; break;
    case RESOURCE_TYPE_CREATURE: message << "creature"; break;
    case RESOURCE_TYPE_ADVENTURE_OBJECT: message << "advobj"; break;
    case RESOURCE_TYPE_HERO: message << "hero"; break;
    case RESOURCE_TYPE_TILESET: message << "tileset"; break;
    case RESOURCE_TYPE_POINTER: message << "pointer"; break;
    case RESOURCE_TYPE_INTERFACE: message << "interface"; break;
    case RESOURCE_TYPE_SPRITE_FRAME: message << "sprite frame"; break;
    case RESOURCE_TYPE_COMBAT_HERO: message << "combat hero"; break;
    case RESOURCE_TYPE_ADVENTURE_MASK: message << "advmask"; break;
    default: message << "0x" << hex << type; break;
    }
    message << "' resource '" << name << "' in the file '" << Path << SpriteLodName << "'." << ends;
    printf("\n------------------------------\n%s\n------------------------------\n\n", message.str());
    message.freeze(0);
}

}  // namespace ResourceManager
