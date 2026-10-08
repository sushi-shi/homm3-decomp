// MapValidation.h - the map validation function (Loki h3maped
// MapValidation.cpp, object 77). The header's name is not recorded; the
// source file's is (its asserts).
//
// TMapValidationFunc collects one line per problem it finds in a map: no
// players or towns, no name or description, unreachable objects, objects
// owned by absent players, landlocked shipyards, unpaired monoliths and
// gates, unobtainable seer's hut quests, and victory and loss conditions
// that cannot be met. It visits the map's conditions as both visitors
// (the victory visitor's vtable first, the loss visitor's at +4); the
// type_info's base list marks the victory visitor private and the loss
// visitor public. The map is at +8 (_m_map, from the asserts), the notes
// at +0xc and their count at +0x18; the notes' and count's names, and
// the parameter names, are not recorded.
#ifndef HOMM3_EDITOR_MAPVALIDATION_H
#define HOMM3_EDITOR_MAPVALIDATION_H

#include <string>

#include "armygrp.h"
#include "artifact_type.h"
#include "editor/Point.h"
#include "editor/Player.h"
#include "editor/VictoryCondition.h"

class TArmy;
class TGameMap;
class THero;
class TSeersHut;
class TTown;

class TMapValidationFunc : private TVictoryCondition::TVisitor, public TLossCondition::TVisitor {
public:
    TMapValidationFunc(const TGameMap& map);

    string operator()();

    virtual void visit(const TVCAquireArtifact& vc);
    virtual void visit(const TVCAccumulateCreature& vc);
    virtual void visit(const TVCAccumulateResource& vc);
    virtual void visit(const TVCUpgradeTown& vc);
    virtual void visit(const TVCBuildHolyGrailStruct& vc);
    virtual void visit(const TVCDefeatHero& vc);
    virtual void visit(const TVCCaptureTown& vc);
    virtual void visit(const TVCDefeatMonster& vc);
    virtual void visit(const TVCFlagAllCreatureGenerators& vc);
    virtual void visit(const TVCFlagAllMines& vc);
    virtual void visit(const TVCTransportArtifact& vc);
    virtual void visit(const TLCLoseTown& lc);
    virtual void visit(const TLCLoseHero& lc);
    virtual void visit(const TLCTimeExpires& lc);

private:
    static unsigned int _countCreatures(TCreatureType creatureType, const TArmy& army);
    static string _createTownStr(const TTown& town, const TPoint<unsigned int>& loc, bool bSecondLayer);
    static string _createHeroStr(const THero& hero, const TPoint<unsigned int>& loc, bool bSecondLayer);
    static string _createTeamStr(unsigned int teamNum);

    void _addNote(char* note);
    void _addNote(const char* note) { _addNote(const_cast<char*>(note)); }
    bool _isArtifactOnMap(TArtifact whichArtifact, const TSeersHut* pExcludedHut = 0) const;
    bool _playerMayBeHuman(TPlayer whichPlayer) const;
    void _checkForUnreachableObjects();

    const TGameMap& _m_map;
    string _m_notes;
    unsigned int _m_numNotes;
};

#endif  /* HOMM3_EDITOR_MAPVALIDATION_H */
