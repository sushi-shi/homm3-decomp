// Shared declarations for source-owned ResourceManager bodies compiled for Mac.
// The 0x1d8 archive-slot stride and +4 LODFile member are visible in the
// eight-slot initializer at mac:0:0x154d04..0x154f20 and bitmap lookup at
// mac:0:0x1546a0. LODEntry's size is read at +0x14 in that lookup.
#ifndef HOMM3_MAC_RESOURCEMANAGER_H
#define HOMM3_MAC_RESOURCEMANAGER_H


// CodeWarrior stores the MSL string as one handle word. This declaration
// supplies its type to the source-owned archive pathname helper. The path
// object itself is an uninitialized file-static definition in the owning TU;
// the runtime string implementation remains external to this comparison.
namespace std {
class string {
    struct rep {
        unsigned long m_length;
        char m_beforeData[8];
        char* m_data;
    }* m_handle;
public:
    string();
    string(const string&);
    ~string();
    const char* c_str() const { return m_handle->m_data; }
};
string operator+(const string&, const char*);

// The retained open body constructs a three-word int vector at frame+0x88:
// capacity, size and data. Its base constructor at 0:0x152b94 writes capacity;
// the derived constructor initializes size/data, and reserve at 0:0x154954
// reads those same offsets. The cleanup path reads size and data directly.
template <class T> class __vector_deleter {
protected:
    unsigned long m_capacity;
    unsigned long m_size;
    T* m_data;
public:
    __vector_deleter(unsigned long initialCapacity);
};
template <class T> class vector : private __vector_deleter<T> {
public:
    vector() : __vector_deleter<T>(0) {
        this->m_size = 0;
        this->m_data = 0;
    }
    ~vector();
    void reserve(unsigned long capacity);
    unsigned long size() const { return this->m_size; }
    T& back() { return this->m_data[this->m_size - 1]; }
    void pop_back();
};
}
struct LODEntry {
    char m_name[16];
    int m_offset;
    int m_size;
};

class LODFile {
    unsigned char m_macStorage[0x1d4];
public:
    unsigned char pointAt(const char* name);
    // The Mac LOD stream adapter at 0:0x151ef4 calls this member with
    // (destination, byte count), then returns that count on success.
    int read(void* dest, int numBytes);
    LODEntry* getItemIndex(const char* name);
    // Mac archive opener at 0:0x152210 passes the pathname and zero flags to
    // the retained LODFile::open body at 0:0x11ba00.
    int open(const char* pathname, int flags);
    void clear();
};

struct TResourceLODSlot {
    const char* m_archiveName;
    LODFile m_file;
};

struct TResourceArchiveList {
    int m_count;
    int* m_indices;
};

struct TResourceArchiveContext {
    TResourceArchiveList m_sprites;
    TResourceArchiveList m_bitmaps;
    TResourceArchiveList m_sounds;
};

extern int* g_videoGameState;
extern TResourceLODSlot g_resourceLodSlots[];
extern TResourceArchiveContext g_resourceArchiveContexts[4];

class font;

// The retained sprite loader copies the 0x310-byte DEF header, advances
// through 0x10-byte sequence records, and reads 0x10/0x20-byte frame headers.
// Its allocation calls prove CSprite 0x38, CSpriteFrame 0x48, TPalette16
// 0x21c and TPalette24 0x31c. The palette pointers sit at sprite +0x20/+0x24.
enum EResourceType {
    RESOURCE_TYPE_SPRITE = 64,
    RESOURCE_TYPE_CREATURE = 66,
    RESOURCE_TYPE_ADVENTURE_OBJECT = 67,
    RESOURCE_TYPE_HERO = 68,
    RESOURCE_TYPE_TILESET = 69,
    RESOURCE_TYPE_POINTER = 70,
    RESOURCE_TYPE_INTERFACE = 71,
    RESOURCE_TYPE_COMBAT_HERO = 73
};
enum TEncodingMethod {
    eEncodeRaw = 0,
    eEncodeGeneralRLE = 1,
    eEncodeTilesetRLE = 2,
    eEncodeAdvObjRLE = 3
};
class resource {
    unsigned char m_afterVptr[0x18];
public:
    virtual ~resource();
};

// The retained palette-data helper at 0:0x153434 reads 0x18 and 0x400 bytes
// through virtual slot +0xc, then allocates 0x31c bytes for TPalette24.
struct TRGBA {
    unsigned char m_red;
    unsigned char m_green;
    unsigned char m_blue;
    unsigned char m_alpha;
};

class TAbstractFile {
public:
    virtual ~TAbstractFile();
    virtual int read(void* data, int size) = 0;
    virtual int write(const void* data, int size) = 0;
};

class TPalette24 : public resource {
    unsigned char m_palette[0x300];
public:
    TPalette24(const unsigned char* data);
    TPalette24(const TPalette24* copy);
    TPalette24(const TRGBA* rgba);
    virtual ~TPalette24();
    void adjustHSV(float hue, float hueAdjust, float saturationAdjust,
                   float valueAdjust);
};

class TPalette16 : public resource {
    unsigned char m_palette[0x200];
public:
    TPalette16(const TPalette16* copy);
    TPalette16(const TPalette24& palette, int rbits, int rshift,
               int gbits, int gshift, int bbits, int bshift);
    virtual ~TPalette16();
};

class CSpriteFrame : public resource {
    unsigned char m_frameStorage[0x2c];
public:
    CSpriteFrame(const char* name, int width, int height,
                 unsigned char* data, int dataSize, TEncodingMethod encoding);
    CSpriteFrame(const char* name, int width, int height,
                 unsigned char* data, int dataSize, TEncodingMethod encoding,
                 int croppedWidth, int croppedHeight, int croppedX,
                 int croppedY);
    virtual ~CSpriteFrame();
};

class CSprite : public resource {
    unsigned char m_sequencePointer[4];
public:
    TPalette16* m_p;
    TPalette24* m_p24;
private:
    unsigned char m_tail[0x10];
public:
    CSprite(const char* name, int type, int width, int height);
    virtual ~CSprite();
    void allocateSeq(int sequenceNumber, int numFrames);
    int addFrame(int sequenceNumber, CSpriteFrame* frame);
};

struct SpriteDefHeader {
    EResourceType m_type;
    int m_width;
    int m_height;
    int m_numSequences;
    unsigned char m_palette[768];
};
struct TSpriteDataHeader {
    int m_sequenceNumber;
    int m_numFrames;
    char* m_frameNames;
    int* m_frameOffsets;
};
struct TCompactSpriteFrameHeader {
    int m_dataSize;
    int m_encoding;
    int m_width;
    int m_height;
};
struct TCroppedSpriteFrameHeader {
    int m_dataSize;
    TEncodingMethod m_encoding;
    int m_width;
    int m_height;
    int m_croppedWidth;
    int m_croppedHeight;
    int m_croppedX;
    int m_croppedY;
};

extern "C" void* memcpy(void* destination, const void* source,
                         unsigned long size);
inline void addPal16(CSprite* sprite, const TPalette16* palette);
inline void addPal24(CSprite* sprite, const TPalette24* palette);
extern int g_firstMaskBits, g_firstMaskShift;
extern int g_greenMaskBits, g_greenMaskShift;
extern int g_lastMaskBits, g_lastMaskShift;

extern unsigned char g_graphicsSaturated;

namespace ResourceManager {
// The retained open body throws and catches this named enum. Its exception
// type name is stored at Mac data 1+0x4cbc9 and in Windows RTTI at 0x00683008.
enum t_open_errors { openErrorGeneric = 0, openErrorRequiredArchive = 1 };
bool open(bool openSprites, bool openBitmaps, int* errorCode);
int getBitmapResourceSize(const char* name);
LODFile* pointToBitmapResource(const char* name);
font* loadFont(const char* name);
// Mac loadFont passes (name, adapter address, LODEntry size) at 0:0x153918.
font* loadFontData(const char* name, TAbstractFile* stream, int fileSize);
TPalette24* getPalette24(const char* name);
TPalette24* loadPalette24Data(const char* name, TAbstractFile* stream);
CSprite* getSprite(const char* name);
resource* getFromCache(const char* name);
void addToCache(resource* value);
LODFile* pointToSpriteResource(const char* name);
}

#endif
