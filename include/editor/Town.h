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

// The 41 building slots of a town's state, its events and the town pages'
// trees (Loki's Town.h).
enum TBuilding {
    eBuildingNone = -1,
    eBuildingTownHall = 0,
    eBuildingCityHall = 1,
    eBuildingCapitol = 2,
    eBuildingFort = 3,
    eBuildingCitadel = 4,
    eBuildingCastle = 5,
    eBuildingTavern = 6,
    eBuildingBlacksmith = 7,
    eBuildingMarketplace = 8,
    eBuildingResourceSilo = 9,
    eBuildingArtifactMerchant = 10,
    eBuildingMageGuild1 = 11,
    eBuildingMageGuild2 = 12,
    eBuildingMageGuild3 = 13,
    eBuildingMageGuild4 = 14,
    eBuildingMageGuild5 = 15,
    eBuildingShipyard = 16,
    eBuildingGrail = 17,
    eBuildingSpecial1 = 18,
    eBuildingSpecial2 = 19,
    eBuildingSpecial3 = 20,
    eBuildingSpecial4 = 21,
    eBuildingDwelling1 = 22,
    eBuildingUpgradedDwelling1 = 23,
    eBuildingHorde1 = 24,
    eBuildingDwelling2 = 25,
    eBuildingUpgradedDwelling2 = 26,
    eBuildingHorde2 = 27,
    eBuildingDwelling3 = 28,
    eBuildingUpgradedDwelling3 = 29,
    eBuildingHorde3 = 30,
    eBuildingDwelling4 = 31,
    eBuildingUpgradedDwelling4 = 32,
    eBuildingHorde4 = 33,
    eBuildingDwelling5 = 34,
    eBuildingUpgradedDwelling5 = 35,
    eBuildingHorde5 = 36,
    eBuildingDwelling6 = 37,
    eBuildingUpgradedDwelling6 = 38,
    eBuildingDwelling7 = 39,
    eBuildingUpgradedDwelling7 = 40,
    kNumBuildings = 41
};

class TTown : public TLinkableObject, public TPlayableObject {
public:
    enum { s_kNumBuildings = kNumBuildings, s_kNumGeneratorTypes = 7 };

    // A town type's creature generator (its enumerators are not recovered).
    enum TGeneratorType {
    };

    // One building's state: built, and disabled.
    class TBuildingState {
    public:
        TBuildingState() : _m_bBuilt(false), _m_bDisabled(false) {}

        bool getBBuilt() const { return _m_bBuilt; }
        bool getBDisabled() const { return _m_bDisabled; }
        void setBBuilt(bool bBuilt) { _m_bBuilt = bBuilt; }
        void setBDisabled(bool bDisabled) { _m_bDisabled = bDisabled; }

        bool operator==(const TBuildingState& other) const
        {
            return _m_bBuilt == other._m_bBuilt && _m_bDisabled == other._m_bDisabled;
        }

    private:
        bool _m_bBuilt : 1;
        bool _m_bDisabled : 1;
    };

    // A building's names in a town type, and the building it hangs under
    // in the town pages' trees (Loki's TBuildingTraits).
    struct TBuildingTraits {
        bool isDisallowed() const { return m_pName == NULL; }

        const char* m_pName;
        const char* m_pDescription;
        TBuilding m_building;
    };

    // The creatures one of a town type's generators makes.
    class TGeneratorTraits {
    public:
        virtual const char* getBaseCreatureName() const = 0;
        virtual const char* getUpgradeCreatureName() const = 0;
    };

    // A town type's name, buildings and generators (12 bytes; the player
    // page reads the name, h3maped 0x47347f).
    struct TTypeTraits {
        const char* m_pName;
        const TBuildingTraits (&m_akBuildingTraits)[s_kNumBuildings];
        const TGeneratorTraits* const (&m_apGeneratorTraits)[s_kNumGeneratorTypes];

        // Whether the town type's mage guild has the 0-based level
        // (h3maped 0x4cb3c9).
        bool hasMageGuildLevel(unsigned int level) const;
    };

    // h3maped 0x5a50d0: points at the rows (one per town type).
    static const TTypeTraits* s_akTypeTraits;

    // A town's timed event: the map's, plus the buildings it builds and
    // the creatures it adds to each generator (0x70 bytes; the town's
    // writer steps by 0x70, 0x4c2cf5).
    // A bonus to each generator's weekly growth, up to s_kMax. /OPT:ICF
    // folds get and set onto TResourceQuantities' identical accessors.
    class TGeneratorBonuses {
    public:
        enum { s_kMax = 9999 };

        TGeneratorBonuses() : _m_bonuses(0) {}

        unsigned int get(TGeneratorType type) const;
        void set(TGeneratorType type, unsigned int newBonus);

        bool operator==(const TGeneratorBonuses& other) const { return _m_bonuses == other._m_bonuses; }
        bool operator!=(const TGeneratorBonuses& other) const { return !(*this == other); }

    private:
        TArray<unsigned int, s_kNumGeneratorTypes> _m_bonuses;
    };

    class TTimedEvent : public ::TTimedEvent {
    public:
        // h3maped 0x4c22c1 and 0x4c323f.
        TTimedEvent();
        TTimedEvent& operator=(const TTimedEvent& other);

        void setBuildMask(const std::bitset<s_kNumBuildings>& newMask) { _m_buildMask = newMask; }
        void setGeneratorBonuses(const TGeneratorBonuses& newBonuses) { _m_generatorBonuses = newBonuses; }
        const std::bitset<s_kNumBuildings>& getBuildMask() const { return _m_buildMask; }
        const TGeneratorBonuses& getGeneratorBonuses() const { return _m_generatorBonuses; }

        bool operator==(const TTimedEvent& other) const
        {
            return static_cast<const ::TTimedEvent&>(*this) == other && _m_buildMask == other._m_buildMask
                   && _m_generatorBonuses == other._m_generatorBonuses;
        }
        bool operator!=(const TTimedEvent& other) const { return !(*this == other); }

    private:
        std::bitset<s_kNumBuildings> _m_buildMask;
        TGeneratorBonuses _m_generatorBonuses;
    };

    virtual TLinkableObject* getPContainedObject();
    virtual const TLinkableObject* getPContainedObject() const;

    virtual void write(TRawOStream* pOStream, int version) const;

    // The faction: the object type's subtype for a town (0x4c2a24).
    TTownType getTownType() const;
    const TTypeTraits& getTownTypeTraits() const { return s_akTypeTraits[getTownType()]; }
    bool getBCustomName() const { return _m_bCustomName; }
    void setBCustomName(bool bCustomName) { _m_bCustomName = bCustomName; }
    const std::string& getName() const { return _m_name; }
    // h3maped 0x4c238d.
    void setName(const std::string& newName);
    // A random town's alignment: the player whose town type it takes, or
    // none for its owner's or a random one (set by 0x4c24e2).
    TPlayer getAlignment() const { return _m_alignment; }
    void setAlignment(TPlayer newAlignment);
    const TArmy& getGarrison() const { return _m_garrison; }
    bool getBCustomGarrison() const { return _m_bCustomGarrison; }
    bool getBCustomBuildings() const { return _m_bCustomBuildings; }
    void setBCustomBuildings(bool bCustomBuildings) { _m_bCustomBuildings = bCustomBuildings; }
    const TArray<TBuildingState, s_kNumBuildings>& getBuildingStates() const { return _m_aBuildingState; }
    // h3maped 0x4c23d0.
    void setBuildingStates(const TArray<TBuildingState, s_kNumBuildings>& newBuildingStates);
    bool getBGroupedFormation() const { return _m_bGroupedFormation; }
    void setBCustomGarrison(bool bCustomGarrison) { _m_bCustomGarrison = bCustomGarrison; }
    void setBGroupedFormation(bool bGroupedFormation) { _m_bGroupedFormation = bGroupedFormation; }
    // h3maped 0x4c23a4.
    void setGarrison(const TArmy& newGarrison);
    const std::bitset<kNumSpells>& getObligatorySpellsMask() const { return _m_obligatorySpellsMask; }
    const std::bitset<kNumSpells>& getDisabledSpellsMask() const { return _m_disabledSpellsMask; }
    // h3maped 0x4c23f3 and 0x4c2407.
    void setObligatorySpellsMask(const std::bitset<kNumSpells>& newMask);
    void setDisabledSpellsMask(const std::bitset<kNumSpells>& newMask);
    const std::vector<TTimedEvent>& getTimedEvents() const { return _m_events; }
    // h3maped 0x4c241b.
    void setTimedEvents(const std::vector<TTimedEvent>& newTimedEvents);
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
    TPlayer _m_alignment;
};

#endif  /* HOMM3_EDITOR_TOWN_H */
