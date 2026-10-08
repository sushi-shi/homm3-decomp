// Town.h - towns (Loki h3maped Town.cpp, object 35).
//
// TTown is a TPlayableObject with custom-part flags (+8), a name (+0xc), a
// garrison (+0x18), the state of its 41 buildings (+0x50), the spells
// its mage guild may not offer (+0x7c), its own timed events (+0x88) and
// the visiting hero (+0x94). Its type traits (TTown::s_akTypeTraits) hold
// each town type's building traits (TBuildingTraits[41]) and its seven
// creature generators' traits (TGeneratorTraits*, an abstract class with
// absolute and indeterminate kinds in Town.cpp). TTown::TTimedEvent adds
// the buildings to build (+0x44) and the generator bonuses (+0x4c) to a
// map timed event. Enumerator names of TBuilding and TGeneratorType, and
// TBuildingTraits' first two fields, are not recovered.
#ifndef HOMM3_EDITOR_TOWN_H
#define HOMM3_EDITOR_TOWN_H

#include <bitset>
#include <string>
#include <vector>

#include "armygrp.h"
#include "town_type.h"
#include "editor/Army.h"
#include "editor/Array.h"
#include "editor/ObjectSpecializations.h"
#include "editor/TimedEvent.h"

class istream;
class ostream;
class TRawIStream;
class TRawOStream;
class THero;

enum TBuilding {
    eBuildingNone = -1,
    kNumBuildings = 41
};


class TTown : public TPlayableObject {
public:
    enum TGeneratorType {
    };

    static const unsigned int s_kNumGeneratorTypes = 7;
    static const int s_kMaxTimedEvents = 50;

    // A building's state in a town: built, or disabled for this town.
    class TBuildingState {
    public:
        TBuildingState() : _m_bBuilt(false), _m_bDisabled(false) {}

        bool getBBuilt() const { return _m_bBuilt; }
        void setBBuilt(bool bBuilt) { _m_bBuilt = bBuilt; }
        bool getBDisabled() const { return _m_bDisabled; }
        void setBDisabled(bool bDisabled) { _m_bDisabled = bDisabled; }

        bool operator==(const TBuildingState& other) const
        {
            return _m_bBuilt == other._m_bBuilt && _m_bDisabled == other._m_bDisabled;
        }
        bool operator!=(const TBuildingState& other) const { return !(*this == other); }

    private:
        bool _m_bBuilt : 1;
        bool _m_bDisabled : 1;
    };

    // The town pages build their building trees from these: a building whose
    // name is unset is not available in this town type, and m_building is
    // the building it hangs under (eBuildingNone for the roots).
    struct TBuildingTraits {
        TBuildingTraits(TBuilding building) : m_pName(0), m_unknown4(0), m_building(building) {}

        bool isDisallowed() const { return m_pName == 0; }

        const char* m_pName;
        unsigned int m_unknown4;
        TBuilding m_building;
    };

    class TGeneratorTraits {
    public:
        TGeneratorTraits() {}
    };

    struct TTypeTraits {
        TTypeTraits(const TBuildingTraits (&aBuildingTraits)[kNumBuildings],
                    const TGeneratorTraits* const (&apGeneratorTraits)[s_kNumGeneratorTypes]);

        bool hasMageGuildLevel(unsigned int level) const;

        const char* m_pName;
        const TBuildingTraits (&m_akBuildingTraits)[kNumBuildings];
        const TGeneratorTraits* const (&m_apGeneratorTraits)[s_kNumGeneratorTypes];
    };

    // A bonus to each generator's weekly growth, up to s_kMax.
    class TGeneratorBonuses {
    public:
        static const int s_kMax = 9999;

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
        const bitset<kNumBuildings>& getBuildMask() const { return _m_buildMask; }
        void setBuildMask(const bitset<kNumBuildings>& newMask) { _m_buildMask = newMask; }
        const TGeneratorBonuses& getGeneratorBonuses() const { return _m_generatorBonuses; }
        void setGeneratorBonuses(const TGeneratorBonuses& newBonuses) { _m_generatorBonuses = newBonuses; }

        bool operator==(const TTimedEvent& other) const
        {
            return static_cast<const ::TTimedEvent&>(*this) == other && _m_buildMask == other._m_buildMask
                   && _m_generatorBonuses == other._m_generatorBonuses;
        }
        bool operator!=(const TTimedEvent& other) const { return !(*this == other); }

    private:
        bitset<kNumBuildings> _m_buildMask;
        TGeneratorBonuses _m_generatorBonuses;
    };

    static const TTypeTraits* s_akTypeTraits;

    static void initialize();

    TTown(const TObjectType& objType, TPlayer owner);
    TTown(const TObjectType& objType, TRawIStream* pIStream, int version);
    TTown(const TTown& other);
    virtual ~TTown();

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual string getTypeName() const;
    virtual bool isCustomized() const;
    virtual bool hasText() const;
    virtual void exportText(ostream* pOStream) const;

    TTownType getTownType() const;
    const TTypeTraits& getTownTypeTraits() const;

    bool getBCustomName() const { return _m_bCustomName; }
    void setBCustomName(bool bCustom) { _m_bCustomName = bCustom; }
    bool getBCustomGarrison() const { return _m_bCustomGarrison; }
    void setBCustomGarrison(bool bCustom) { _m_bCustomGarrison = bCustom; }
    bool getBCustomBuildings() const { return _m_bCustomBuildings; }
    void setBCustomBuildings(bool bCustom) { _m_bCustomBuildings = bCustom; }
    bool getBGroupedFormation() const { return _m_bGroupedFormation; }
    void setBGroupedFormation(bool bGrouped) { _m_bGroupedFormation = bGrouped; }

    const string& getName() const { return _m_name; }
    void setName(const string& newName);
    const TArmy& getGarrison() const { return _m_garrison; }
    void setGarrison(const TArmy& newGarrison) { _m_garrison = newGarrison; }
    const TArray<TBuildingState, kNumBuildings>& getBuildingStates() const { return _m_aBuildingState; }
    void setBuildingStates(const TArray<TBuildingState, kNumBuildings>& aNewBuildingState);
    bool getBIsBuildingDisabled(TBuilding building) const;
    const bitset<kNumSpells>& getDisabledSpellsMask() const { return _m_disabledSpellsMask; }
    void setDisabledSpellsMask(const bitset<kNumSpells>& newMask);
    const vector<TTimedEvent>& getTimedEvents() const { return _m_events; }
    void setTimedEvents(const vector<TTimedEvent>& newTimedEvents);
    const THero* getPVisitingHero() const { return _m_pVisitingHero; }
    THero* getPVisitingHero() { return _m_pVisitingHero; }
    void setVisitingHero(const THero* pHero);

private:
    bool _m_bCustomName : 1;
    bool _m_bCustomGarrison : 1;
    bool _m_bCustomBuildings : 1;
    bool _m_bGroupedFormation : 1;
    string _m_name;
    TArmy _m_garrison;
    TArray<TBuildingState, kNumBuildings> _m_aBuildingState;
    bitset<kNumSpells> _m_disabledSpellsMask;
    vector<TTimedEvent> _m_events;
    THero* _m_pVisitingHero;
};

TRawOStream& operator<<(TRawOStream& stream, const TTown::TGeneratorBonuses& bonuses);
TRawIStream& operator>>(TRawIStream& stream, TTown::TGeneratorBonuses& bonuses);
TRawOStream& operator<<(TRawOStream& stream, const TTown::TTimedEvent& event);
TRawIStream& operator>>(TRawIStream& stream, TTown::TTimedEvent& event);

#endif  /* HOMM3_EDITOR_TOWN_H */
