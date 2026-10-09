// Town.h - a town on the map (Town.cpp; Loki h3maped). The Windows town is
// a linkable object (+0) and a playable one (+0xc); its members follow at
// +0x14 in the order its copy constructor (h3maped 0x4c1a90) copies them:
// Loki's four flags, the name, the garrison, the 41 building states, the
// obligatory and disabled spell masks (Complete writes the first for
// Armageddon's Blade maps and later, 0x4c2c10), the timed events, the
// visiting hero (+0xb4) and Shadow of Death's alignment byte.
//
// Ported so far: the layout and the accessors the map needs.
#ifndef HOMM3_EDITOR_TOWN_H
#define HOMM3_EDITOR_TOWN_H

#include <bitset>
#include <string>
#include <vector>

#include "editor/Army.h"
#include "editor/Array.h"
#include "editor/ObjectSpecializations.h"
#include "editor/TimedEvent.h"
#include "town_type.h"

class THero;

class TTown : public TLinkableObject, public TPlayableObject {
public:
    enum { s_kNumBuildings = 41, s_kNumGeneratorTypes = 7 };

    // One building's state: built, and disabled.
    class TBuildingState {
    public:
        bool getBBuilt() const { return _m_bBuilt; }
        bool getBDisabled() const { return _m_bDisabled; }

    private:
        bool _m_bBuilt : 1;
        bool _m_bDisabled : 1;
    };

    // A building's names in a town type, and the building it hangs under
    // in the town pages' trees (Loki's TBuildingTraits).
    struct TBuildingTraits {
        const char* m_pName;
        const char* m_pDescription;
        int m_building;
    };

    // The creatures one of a town type's generators makes.
    class TGeneratorTraits;

    // A town type's name, buildings and generators (12 bytes; the player
    // page reads the name, h3maped 0x47347f).
    struct TTypeTraits {
        const char* m_pName;
        const TBuildingTraits (&m_akBuildingTraits)[s_kNumBuildings];
        const TGeneratorTraits* const (&m_apGeneratorTraits)[s_kNumGeneratorTypes];
    };

    // h3maped 0x5a50d0: points at the rows (one per town type).
    static const TTypeTraits* s_akTypeTraits;

    // A town's timed event: the map's, plus the buildings it builds and
    // the creatures it adds to each generator (0x70 bytes; the town's
    // writer steps by 0x70, 0x4c2cf5).
    class TTimedEvent : public ::TTimedEvent {
    private:
        std::bitset<s_kNumBuildings> _m_buildMask;
        TArray<unsigned int, s_kNumGeneratorTypes> _m_generatorBonuses;
    };

    virtual TLinkableObject* getPContainedObject();
    virtual const TLinkableObject* getPContainedObject() const;

    // The faction: the object type's subtype for a town (0x4c2a24).
    TTownType getTownType() const;
    const TTypeTraits& getTownTypeTraits() const { return s_akTypeTraits[getTownType()]; }
    const std::string& getName() const { return _m_name; }
    const TArmy& getGarrison() const { return _m_garrison; }
    THero* getPVisitingHero() { return _m_pVisitingHero; }
    // Keeps a clone of the hero, or none (0x4c242d).
    void setVisitingHero(const THero* pHero);
    const THero* getPVisitingHero() const { return _m_pVisitingHero; }

private:
    bool _m_bCustomName : 1;
    bool _m_bCustomGarrison : 1;
    bool _m_bCustomBuildings : 1;
    bool _m_bGroupedFormation : 1;
    std::string _m_name;
    TArmy _m_garrison;
    TArray<TBuildingState, s_kNumBuildings> _m_aBuildingState;
    std::bitset<kNumSpells> _m_obligatorySpellsMask;
    std::bitset<kNumSpells> _m_disabledSpellsMask;
    std::vector<TTimedEvent> _m_events;
    THero* _m_pVisitingHero;
    unsigned char _m_alignment;
};

#endif  /* HOMM3_EDITOR_TOWN_H */
