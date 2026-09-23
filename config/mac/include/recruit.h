// Mac declaration view for source-owned recruitUnit bodies. Bodies and helper
// definitions remain in their canonical source/header locations.
#ifndef HOMM3_MAC_RECRUIT_H
#define HOMM3_MAC_RECRUIT_H

#include "va.h"
#include "message_record.h"

extern "C" int sprintf(char*, const char*, ...);

enum TCreatureType { CREATURE_NONE = -1 };
enum TArtifact { ARTIFACT_NONE = -1 };
enum { MESSAGE_WIDGET = 0x200, RECRUIT_QUANTITY_ID = 0x20e,
       RECRUIT_CREATURE_0_ID = 0x21a, RECRUIT_CREATURE_1_ID = 0x21b,
       RECRUIT_CREATURE_2_ID = 0x21c, RECRUIT_CREATURE_3_ID = 0x21d,
       GENERAL_TEXT_RECRUIT_TITLE = 17, g_creatureTypeLast = 150 };
const unsigned int g_ctaSiegeWeapon = 0x40;

struct TCreatureTypeTraits {
    char m_beforeAttributes[0x10];
    unsigned int m_attributes;
    const char* m_name;
    const char* m_pluralName;
    char m_afterNames[0x74 - 0x1c];
};
extern const TCreatureTypeTraits (&g_creatureTypeTraits)[150];

namespace std {
template <class T> class vector {
    char m_beforeFirst[8];
    T* m_first;
public:
    // Mac getText indexes through a helper returning the vector's iterator
    // slot at +8, then loads the first-element pointer and indexed element.
    T* const* begin() const;
    T& operator[](int index) const { return (*begin())[index]; }
};
}
class TTextResource {
    char m_beforeText[0x1c];
public:
    std::vector<char*> m_text;
#include "mac_shared/textresource_get_text.h"
#include "mac_shared/textresource_index.h"
};
extern const TTextResource* g_generalText;
extern char g_text[];

class hero {
public:
    unsigned char hasArtifact(int artifact) const;
};
class armyGroup;
namespace GameTime { unsigned long get(); }
extern unsigned long g_timers[10];

class playerData {
    char m_beforeResources[0x98];
public:
    long m_resources[7];
    unsigned char isLocalHuman() const;
};
extern playerData* g_currentPlayer;
extern int g_remoteOn;

class widget {
public:
    enum { WIDGET_SET_TEXT = 3, WIDGET_SET_COLOR = 8 };
    // The Mac update body dispatches enable at vtable +0x2c. These ABI
    // slots include platform-specific entries before the canonical method.
    virtual void macSlot0(); virtual void macSlot1();
    virtual void macSlot2(); virtual void macSlot3();
    virtual void macSlot4(); virtual void macSlot5();
    virtual void macSlot6(); virtual void macSlot7();
    virtual void macSlot8(); virtual void macSlot9();
    virtual void macSlot10();
    virtual void enable(unsigned char value);
};
class button : public widget {};
class slider : public widget {
public:
    // The same target dispatches these two at +0x3c and +0x40.
    virtual void macSliderSlot12(); virtual void macSliderSlot13();
    virtual void macSliderSlot14();
    virtual void setResolution(int value);
    virtual void setState(int value);
};

class heroWindow {
    virtual void macWindowSlot0();
    char m_beforeRecruitTail[0x48 - 4];
public:
    int broadcastMessage(message& msg);
};
class recruitUnit;
class TRecruitWindow : public heroWindow {
public:
    recruitUnit* m_recruitInfo;
    button* m_acceptButton;
    button* m_maximumButton;
    slider* m_quantitySlider;
};
extern TRecruitWindow* g_recruitWindow;

class TPalette16 {
    char m_beforeData[0x1c];
public:
    unsigned short m_data[256];
};
extern TPalette16* g_systemPalette;

class baseManagerCore {
    char m_beforeStatus[0x30];
public:
    int m_status;
};
class baseManager : public baseManagerCore {
    virtual int open(int) = 0;
    virtual void close() = 0;
    virtual int main(message&) = 0;
public:
    baseManager();
};
class recruitUnit : public baseManager {
    char m_beforeType[0x48 - 0x38];
public:
    int m_type;
    unsigned char m_viewOnly;
private:
    char m_beforeMonsterType[3];
public:
    TCreatureType m_monsterType;
    short* m_numAvail;
    int m_selectedPosition;
    TCreatureType m_monType1;
    TCreatureType m_monType2;
    TCreatureType m_monType3;
    TCreatureType m_monType4;
    short* m_available[4];
    hero* m_thisHero;
private:
    char m_beforeGoldPerTroop[4];
public:
    long m_goldPerTroop;
    int m_altResource;
    int m_resourcesPerTroop;
    int m_inTownMainScreen;
private:
    char m_beforeCurrArmyGroup[4];
public:
    armyGroup* m_currArmyGroup;
    unsigned char m_currArmyGroupIsTownGarrison;
private:
    char m_beforeMaxAvail[0xac - 0x9d];
public:
    int m_maxAvail;
    long m_totalGold;
    int m_totalResources;
    int m_numberToBuy;
    recruitUnit(armyGroup* newGroup, unsigned char groupIsTownGarrison,
        TCreatureType monType1, short* numMon1,
        TCreatureType monType2, short* numMon2,
        TCreatureType monType3, short* numMon3,
        TCreatureType monType4, short* numMon4);
    recruitUnit(hero* thisHero,
        TCreatureType monType1, short* numMon1,
        TCreatureType monType2, short* numMon2,
        TCreatureType monType3, short* numMon3,
        TCreatureType monType4, short* numMon4);
    virtual int open(int newPriority);
    virtual void close();
    virtual int main(message& msg);
    void updateCost();
    void update(unsigned char newMonster, long slot);
};

template <class T> inline const T& min(const T& left, const T& right)
{
    return left < right ? left : right;
}
TArtifact siegeMonsterToSiegeArtifact(TCreatureType type);
#include "mac_shared/creaturetype_get_army_name.h"

#endif
