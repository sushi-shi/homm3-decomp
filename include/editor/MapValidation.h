// MapValidation.h - the map validation function (MapValidation.cpp; Loki
// h3maped object 77). The header's name is not recorded.
//
// TMapValidationFunc collects one line per problem it finds in a map: no
// players or towns, no name or description, unreachable objects, objects
// owned by absent players, landlocked shipyards, unpaired monoliths and
// gates, unobtainable seer's hut quests, and victory and loss conditions
// that cannot be met. It visits the map's conditions as both visitors
// (the victory visitor's vtable first, the loss visitor's at +4; RTTI
// TMapValidationFunc). The map is at +8, the notes at +0xc and their count
// at +0x10: Windows keeps the notes in a CString, each ending in "\r\n".
// The accumulate-resource, defeat-monster and time-expires visits keep the
// visitors' empty defaults (h3maped vtables 0x53d9fc, 0x53d9e8). The hero
// text takes any basic hero, a placeholder too, and reads the map's hero
// prototypes, so it is a member; the creature count has no body of its
// own (every visit expands it).
#ifndef HOMM3_EDITOR_MAPVALIDATION_H
#define HOMM3_EDITOR_MAPVALIDATION_H

#include "artifact_type.h"
#include "creaturetype.h"
#include "Point.h"
#include "editor/Army.h"
#include "editor/Player.h"
#include "editor/VictoryCondition.h"

class TBasicHero;
class TGameMap;
class TSeersHut;
class TTown;

class TMapValidationFunc : private TVictoryCondition::TVisitor, public TLossCondition::TVisitor {
public:
    TMapValidationFunc(const TGameMap& gameMap);

    CString operator()();

    virtual void visit(const TVCAquireArtifact& vc);
    virtual void visit(const TVCAccumulateCreature& vc);
    virtual void visit(const TVCUpgradeTown& vc);
    virtual void visit(const TVCBuildHolyGrailStruct& vc);
    virtual void visit(const TVCDefeatHero& vc);
    virtual void visit(const TVCCaptureTown& vc);
    virtual void visit(const TVCFlagAllCreatureGenerators& vc);
    virtual void visit(const TVCFlagAllMines& vc);
    virtual void visit(const TVCTransportArtifact& vc);
    virtual void visit(const TLCLoseTown& lc);
    virtual void visit(const TLCLoseHero& lc);

private:
    static unsigned int _countCreatures(TCreatureType creatureType, const TArmy& army)
    {
        unsigned int count = 0;
        for (unsigned int i = 0; i < army.size(); ++i)
            if (army[i].getCreatureType() == creatureType)
                count += army[i].getQuantity();
        return count;
    }
    static CString _createTownStr(const TTown& town, const TTilePoint& loc, bool bSecondLayer);
    CString _createHeroStr(const TBasicHero& hero, const TTilePoint& loc, bool bSecondLayer) const;
    static CString _createTeamStr(unsigned int teamNum);

    void _addNote(const CString& note);
    bool _isArtifactOnMap(TArtifact whichArtifact, const TSeersHut* pExcludedHut = 0) const;
    bool _playerMayBeHuman(TPlayer whichPlayer) const;
    void _checkForUnreachableObjects();

    const TGameMap& _m_map;
    CString _m_notes;
    unsigned int _m_numNotes;
};

#endif  /* HOMM3_EDITOR_MAPVALIDATION_H */
