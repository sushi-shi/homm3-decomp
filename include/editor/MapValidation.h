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
// visitors' empty defaults (h3maped vtables 0x53d9fc, 0x53d9e8).
#ifndef HOMM3_EDITOR_MAPVALIDATION_H
#define HOMM3_EDITOR_MAPVALIDATION_H

#include "artifact_type.h"
#include "Point.h"
#include "editor/Player.h"
#include "editor/VictoryCondition.h"

class TGameMap;
class THero;
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
    static CString _createTownStr(const TTown& town, const TTilePoint& loc, bool bSecondLayer);
    static CString _createHeroStr(const THero& hero, const TTilePoint& loc, bool bSecondLayer);
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
