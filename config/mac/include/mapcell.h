// Mac declaration view for the source-owned NewfullMap object-index rebuild.
// Mac 0:0x1270c0..0x127278 reads objectTypes size/data at +4/+8 and the
// 232 per-class vectors at +0xa8, with 0x38-byte object-type records. The
// image-name representation supplies length at +0 and bytes at +0xc to the
// retained MSL string::compare call at 0:0x41c4.
#ifndef HOMM3_MAC_MAPCELL_H
#define HOMM3_MAC_MAPCELL_H

#define HOMM3_TARGET_MAC 1
#include "va.h"
// The MSL allocator temporary is stack-addressable but its inline default
// construction emits no store. The string constructor receives it in r4 at
// readMonsterData+0x1a8, immediately before the pinned 0x46c8 call.
namespace std { template<class T> class allocator { public: allocator() {} }; }
namespace Metrowerks {
namespace details {
template<class Allocator, class Size, int Version> class compressed_pair_imp;
template<class Allocator, class Size>
class compressed_pair_imp<Allocator, Size, 1> : private Allocator {
    Size m_second;
public:
    explicit compressed_pair_imp(Size value) : m_second(value) {}
    Size& second() { return m_second; }
};
}
template<class Allocator, class Size>
class compressed_pair
    : private details::compressed_pair_imp<Allocator, Size, 1> {
    typedef details::compressed_pair_imp<Allocator, Size, 1> base;
public:
    explicit compressed_pair(Size value) : base(value) {}
    Size& second() { return base::second(); }
};
}
namespace std {
class string {
    struct rep {
        unsigned long m_length;
        char m_beforeData[8];
        char* m_data;
    }* m_handle;
public:
    string(const allocator<char>& value = allocator<char>());
    ~string();
    const char* c_str() const { return m_handle->m_data; }
    int compare(unsigned long start, unsigned long count,
                const char* bytes, unsigned long length) const;
    bool operator==(const string& other) const {
        return compare(0, m_handle->m_length,
                       other.m_handle->m_data,
                       other.m_handle->m_length) == 0;
    }
};
// The CodeWarrior MSL non-POD vector uses this deleter/imp/vector chain.
// Its three data words preserve the +0/+4/+8 reads in the pinned helper;
// modeling the hierarchy changes only stack reservation in this body.
template<class T> class __vector_deleter {
protected:
    Metrowerks::compressed_pair<allocator<T>, unsigned long> m_capacity;
    unsigned long m_size;
    T* m_data;
    T*& data() { return m_data; }
    T* const& data() const { return m_data; }
public:
    __vector_deleter() : m_capacity(0), m_size(0), m_data(0) {}
};
template<class T> class __vector_imp : private __vector_deleter<T> {
protected:
    unsigned long size() const { return this->m_size; }
    T& operator[](unsigned long index) { return *(this->data() + index); }
};
template<class T> class vector : private __vector_imp<T> {
public:
    ~vector();
    unsigned long size() const { return __vector_imp<T>::size(); }
    T& operator[](unsigned long index) {
        return __vector_imp<T>::operator[](index);
    }
    void resize(unsigned long size);
    void clear();
    void push_back(const T& value);
};
}

enum { ARTIFACT_NONE = -1, MAP_FORMAT_RESTORATION_OF_ERATHIA = 14 };
enum {
    MONSTER_QTY_UNRESOLVED = 0,
    MONSTER_QTY_RANDOM_1_7 = 1,
    MONSTER_QTY_RANDOM_1_10 = 2,
    MONSTER_QTY_RANDOM_4_10 = 3,
    MONSTER_QTY_FIXED_10 = 4
};
int random(int minimum, int maximum);
void incProgressBar(bool update);

// Mac loadMapObjects calls sprite dispose through vtable +0xc. The
// source-owned resource base declares that virtual slot, which CSprite
// overrides; no resource storage is accessed by this MAPCELL body.
class resource {
public:
    virtual ~resource();
    virtual void dispose();
    virtual unsigned int getSize() const = 0;
};
class CSprite : public resource {
public:
    virtual ~CSprite();
    virtual void dispose();
    virtual unsigned int getSize() const;
};
namespace ResourceManager {
CSprite* getSprite(const char* name);
}

class TAbstractFile {
public:
    virtual ~TAbstractFile();
    virtual int read(void* data, int size) = 0;
    virtual int write(const void* data, int size) = 0;
};
class NewSMapHeader {
public:
    static int readString(TAbstractFile* infile, std::string& value);
};
// Mac read at 0:0x11d5f0 reads the 28-byte resource lane at +4, then
// one-byte flags at +0x20..+0x22 and two-byte timing fields at +0x24/+0x26.
class TTimedEvent {
public:
    std::string m_message;
    int m_resQty[7];
    unsigned char m_playerFlags;
    unsigned char m_applyToHuman;
    unsigned char m_applyToComputer;
    unsigned short m_firstTime;
    unsigned short m_interval;
    int read(TAbstractFile* infile, int saveVersion);
};
struct type_point {
    short m_x : 10;
    short m_y : 10;
    short m_z : 4;
};
// DC names the six wandering-monster lanes. CodeWarrior packs the first
// declared lane at the high end of the PPC word. The declaration order here
// reverses the Windows layout so retail's qty lands at object+2 low 12 bits,
// disposition at object+0 bits 12..16, and custom at object+0 bit 31.
struct MonsterInfo {
    unsigned long m_custom : 1;
    unsigned long m_unused27 : 4;
    unsigned long m_index : 8;
    unsigned long m_dontGrow : 1;
    unsigned long m_neverFlee : 1;
    signed long m_disposition : 5;
    unsigned long m_qty : 12;
};
class CObject {
public:
    union {
        unsigned long m_extraInfo;
        MonsterInfo m_monsterInfo;
    };
    unsigned char m_x;
    unsigned char m_y;
    unsigned char m_z;
    unsigned char m_paddingBeforeTypeId;
    unsigned short m_typeIndex;
    unsigned char m_animationOffset;
    unsigned char m_paddingAfterFrameOffset;
};
class MonsterData {
public:
    std::string m_message;
    int m_resQty[7];
    int m_artifact;
#include "inline/mapcell_monster_data_ctor.inl"
};
class game {
    char m_beforeMapHeader[0x1f0f8];
public:
    struct { int m_version; } m_mapHeader;
    void recordMonsterIdentifier(int identifier, type_point point);
};
extern game* g_game;

class CObjectType {
public:
    std::string m_imageName;
    char m_beforeObjectType[0x2c - sizeof(std::string)];
    int m_objectType;
    int m_extra;
    char m_beforeObjectTypeIndex[2];
    // The Mac store sign-extends the source index before writing at +0x36.
    short m_objectTypeIndex;
};

class NewfullMap {
public:
    std::vector<CObjectType> m_objectTypes;
    std::vector<CObject> m_objects;
    std::vector<CSprite*> m_sprites;
private:
    char m_beforeCustomMonsterList[0x30 - sizeof(std::vector<CObjectType>)
                                     - sizeof(std::vector<CObject>)
                                     - sizeof(std::vector<CSprite*>)];
public:
    std::vector<MonsterData> m_customMonsterList;
private:
    char m_beforeObjectTypeIndex[0xa8 - 0x30 - sizeof(std::vector<MonsterData>)];
public:
    std::vector<CObjectType> m_objectTypeIndex[232];
    void newfullMapFn005042C0();
    int readMonsterData(TAbstractFile* infile, CObject* monsterObject);
    int loadObjectType(TAbstractFile* infile, CObjectType* objectType);
    int loadObject(TAbstractFile* infile, CObject* object);
    int loadMapObjects(TAbstractFile* infile);
};

#endif
