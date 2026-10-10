// MapValidation.cpp - the map validation function, the editor's "Validate
// map" report (h3maped 0x47c4f5..0x47ff04; Loki h3maped object 77). Every
// problem becomes one line of text; the victory and loss conditions are
// checked by visiting them. The Windows release formats its notes with
// CString::Format where Loki's port prints into a 512-byte buffer, and
// drops the asserts.
//
// Complete's report adds checks Loki's lacks: too few enabled heroes,
// keymaster's tents, border guards and gates of one colour without the
// other, a hut of the magi without an eye or the reverse, a seer's hut or
// quest guard without a quest, and the artifacts left for the random ones
// (the map's playability test, GameMap.cpp _isPlayable). Its quest walk
// covers every quest location's artifacts, not only a seer's hut's.
#include "editor/stdafx.h"

#include <ctype.h>
#include <algorithm>
#include <functional>

#include "va.h"
#include "adventureobjecttype.h"
#include "artifact.h"
#include "objnames.h"
#include "retailobjecttype.h"
#include "terrain_type.h"
#include "editor/BlackBox.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/MapValidation.h"
#include "editor/Monster.h"
#include "editor/ObjectSpecializations.h"
#include "editor/Quest.h"
#include "editor/QuestLocation.h"
#include "editor/SeersHut.h"
#include "editor/TilePoint.h"
#include "editor/Town.h"

VA(0x0047c6d4, 0x33)
TMapValidationFunc::TMapValidationFunc(const TGameMap& gameMap) : _m_map(gameMap), _m_numNotes(0)
{
}

VA(0x0047c767, 0xe2d)
CString TMapValidationFunc::operator()()
{
    if (_m_map.getNumPlayableSlots() == 0)
        _addNote(kThereAreNoPlayersOnMapStr);
    else if (_m_map.getNumTownsOnMap() == 0)
        _addNote(kThereAreNoTownsOnMapStr);

    unsigned int numAvailableHeroes = (~_m_map.getDisabledHeroes()).count();
    unsigned int numHeroesNeeded = _m_map.getNumTownsOnMap() + _m_map.getNumPlayableSlots() * 10;
    if (numAvailableHeroes < numHeroesNeeded)
        _addNote(TFormattedString(kTooFewAvailableHeroesFmtStr, numAvailableHeroes, numHeroesNeeded));

    const string& name = _m_map.getName();
    if (find_if(name.begin(), name.end(), not1(ptr_fun(isspace))) == name.end())
        _addNote(kMapHasNoNameStr);
    const string& desc = _m_map.getDesc();
    if (find_if(desc.begin(), desc.end(), not1(ptr_fun(isspace))) == desc.end())
        _addNote(kMapHasNoDescStr);

    if (_m_map.isGrailOnMap() && _m_map.getNumObelisksOnMap() == 0)
        _addNote(kGrailPlacedButNoObelisksStr);

    _checkForUnreachableObjects();

    TPlayerMask ownersMask;
    unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
    unsigned int i;
    for (i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TFlaggableObject* pFlaggable = dynamic_cast<const TFlaggableObject*>(layer.getPObject(*iter));
            if (pFlaggable != NULL && pFlaggable->getOwner() != ePlayerNone)
                ownersMask[pFlaggable->getOwner()] = true;
        }
    }
    for (unsigned int player = 0; player < kNumPlayers; ++player) {
        if (ownersMask[player] && !_m_map.isPlayerPresent(TPlayer(player))) {
            CString note;
            note.Format(kPlayerOwnsObjectsButNotPresentFmtStr, akPlayerTraits[player].m_pName);
            _addNote(note);
        }
    }

    for (i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TTown* pTown = dynamic_cast<const TTown*>(layer.getPObject(*iter));
            if (pTown != NULL && pTown->getBuildingStates()[eBuildingShipyard].getBBuilt()) {
                TTilePoint loc = layer.getObjectLoc(*iter) - pTown->getTriggerLoc();
                unsigned int shoreY = loc.y() + 2;
                if (shoreY >= _m_map.getWidth()
                    || ((loc.x() - 1 >= _m_map.getWidth()
                         || layer.getCell(loc.x() - 1, shoreY).getTerrainType() != eTerrainWater)
                        && (loc.x() + 1 >= _m_map.getWidth()
                            || layer.getCell(loc.x() + 1, shoreY).getTerrainType() != eTerrainWater))) {
                    CString townStr;
                    townStr.Format(kObjectAtLocationFmtStr, pTown->getTownTypeTraits().m_pName, loc.x(), loc.y(), i);
                    CString note;
                    note.Format(kTownHasShipyardButIsLandlockedFmtStr, (LPCTSTR)townStr);
                    _addNote(note);
                }
            }
        }
    }

    TArray<unsigned int, 2> aNumGates(0);
    TArray<unsigned int, kNumMonolithTypes> aNumMonoliths(0);
    TArray<unsigned int, kNumOneWayMonolithTypes> aNumEntrances(0);
    TArray<unsigned int, kNumOneWayMonolithTypes> aNumExits(0);
    unsigned int numWhirlpools = 0;
    TArray<unsigned int, kNumKeymasterTentTypes> aNumKeymasterTents(0);
    TArray<unsigned int, kNumKeymasterTentTypes> aNumBorderGuards(0);
    TArray<unsigned int, kNumKeymasterTentTypes> aNumBorderGates(0);
    unsigned int numHutsOfMagi = 0;
    unsigned int numEyesOfMagi = 0;
    for (i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TGameObject& obj = layer.getObject(*iter);
            switch (obj.getType()) {
            case UNDERGROUND_GATE:
                ++aNumGates[i];
                break;
            case LITH_TWOWAY:
                ++aNumMonoliths[obj.getExtra()];
                break;
            case LITH_ONEWAY_ENTRANCE:
                ++aNumEntrances[obj.getExtra()];
                break;
            case LITH_ONEWAY_EXIT:
                ++aNumExits[obj.getExtra()];
                break;
            case WHIRLPOOL:
                ++numWhirlpools;
                break;
            case BORDER_TENT:
                ++aNumKeymasterTents[obj.getExtra()];
                break;
            case BORDER_GUARD:
                ++aNumBorderGuards[obj.getExtra()];
                break;
            case BORDER_GATE:
                ++aNumBorderGates[obj.getExtra()];
                break;
            case HUT_OF_MAGI:
                ++numHutsOfMagi;
                break;
            case EYE_OF_MAGI:
                ++numEyesOfMagi;
                break;
            }
        }
    }
    unsigned int type;
    for (type = 0; type < kNumMonolithTypes; ++type) {
        if (aNumMonoliths[type] == 1) {
            CString note;
            note.Format(kOnlyOneMonolithOnMapFmtStr, akMonolithTypeTraits[type].m_name);
            _addNote(note);
        }
    }
    for (type = 0; type < kNumOneWayMonolithTypes; ++type) {
        const char* typeName = akOneWayMonolithTypeTraits[type].m_name;
        if (aNumEntrances[type] > 0 && aNumExits[type] == 0) {
            CString note;
            note.Format(kMonolithEntranceButNoExitFmtStr, typeName, typeName);
            _addNote(note);
        } else if (aNumEntrances[type] == 0 && aNumExits[type] > 0) {
            CString note;
            note.Format(kMonolithExitButNoEntranceFmtStr, typeName, typeName);
            _addNote(note);
        }
    }
    if (numLayers == 1) {
        if (aNumGates[0] > 0)
            _addNote(kSubterraneanGatesOnOneLayerMapStr);
    } else if (aNumGates[0] > aNumGates[1])
        _addNote(kMoreSubterraneanGatesAboveThanBelowStr);
    else if (aNumGates[0] < aNumGates[1])
        _addNote(kMoreSubterraneanGatesBelowThanAboveStr);
    if (numWhirlpools == 1)
        _addNote(kOneWhirlpoolStr);
    for (type = 0; type < kNumKeymasterTentTypes; ++type) {
        const char* colorName = akKeymasterTentTypeTraits[type].m_name;
        if (aNumKeymasterTents[type] > 0) {
            if (aNumBorderGuards[type] == 0 && aNumBorderGates[type] == 0)
                _addNote(TFormattedString(kKeymastersTentButNoBorderFmtStr, colorName, colorName));
        } else {
            if (aNumBorderGuards[type] > 0)
                _addNote(TFormattedString(kBorderGuardButNoKeymastersTentFmtStr, colorName, colorName));
            if (aNumBorderGates[type] > 0)
                _addNote(TFormattedString(kBorderGateButNoKeymastersTentFmtStr, colorName, colorName));
        }
    }
    if (numHutsOfMagi > 0) {
        if (numEyesOfMagi == 0)
            _addNote(kHutOfTheMagiButNoEyeStr);
    } else if (numEyesOfMagi > 0)
        _addNote(kEyeOfTheMagiButNoHutStr);

    bitset<kNumArtifacts> availableArtifacts = ~_m_map.getDisabledArtifacts();
    for (int artifact = 0; artifact < kNumArtifacts; ++artifact)
        if (akArtifactTraits[artifact].m_disabled || akArtifactTraits[artifact].m_class & ArtifactClassSpecial)
            availableArtifacts.set(artifact, false);
    for (i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TQuestLocation* pQuestLocation = dynamic_cast<const TQuestLocation*>(layer.getPObject(*iter));
            if (pQuestLocation == NULL)
                continue;
            if (pQuestLocation->getPQuest() == NULL) {
                TTilePoint loc = layer.getObjectLoc(*iter) - pQuestLocation->getTriggerLoc();
                TFormattedString locStr(kObjectAtLocationFmtStr, pQuestLocation->getTypeName().c_str(), loc.x(),
                                        loc.y(), i);
                _addNote(TFormattedString(kQuestObjectHasNoQuestFmtStr, (LPCTSTR)locStr));
            } else if (dynamic_cast<const TQuestBringArtifacts*>(pQuestLocation->getPQuest()) != NULL) {
                const multiset<TArtifact>& artifacts =
                    static_cast<const TQuestBringArtifacts*>(pQuestLocation->getPQuest())->getArtifacts();
                for (multiset<TArtifact>::const_iterator pArtifact = artifacts.begin(); pArtifact != artifacts.end();
                     ++pArtifact) {
                    availableArtifacts[*pArtifact] = false;
                    if (!_isArtifactOnMap(*pArtifact, dynamic_cast<const TSeersHut*>(pQuestLocation))) {
                        TTilePoint loc = layer.getObjectLoc(*iter) - pQuestLocation->getTriggerLoc();
                        TFormattedString locStr(kObjectAtLocationFmtStr, pQuestLocation->getTypeName().c_str(),
                                                loc.x(), loc.y(), i);
                        _addNote(TFormattedString(kSeersHutQuestArtifactNotOnMapFmtStr, (LPCTSTR)locStr,
                                                  akArtifactTraits[*pArtifact].m_name));
                    }
                }
            }
        }
    }

    const TVictoryCondition* pVictoryCondition = _m_map.getPVictoryCondition();
    if (pVictoryCondition != NULL) {
        if (dynamic_cast<const TVCAquireArtifact*>(pVictoryCondition) != NULL)
            availableArtifacts[static_cast<const TVCAquireArtifact*>(pVictoryCondition)->getArtifact()] = false;
        else if (dynamic_cast<const TVCTransportArtifact*>(pVictoryCondition) != NULL)
            availableArtifacts[static_cast<const TVCTransportArtifact*>(pVictoryCondition)->getArtifact()] = false;
    }
    if (!availableArtifacts.any())
        _addNote(kAllArtifactsDisabledOrReservedStr);

    if (_m_map.getNumPlayableSlots() == 1) {
        if (pVictoryCondition == NULL)
            _addNote(kOnePlayerPresentButNoSpecialVictoryConditionStr);
        else if (pVictoryCondition->getBAllowNormalVictory())
            _addNote(kOnePlayerPresentButNormalVictoryNotDisabledStr);
    }
    if (pVictoryCondition != NULL)
        pVictoryCondition->accept(this);
    const TLossCondition* pLossCondition = _m_map.getPLossCondition();
    if (pLossCondition != NULL)
        pLossCondition->accept(this);

    if (_m_numNotes == 0)
        _addNote(kThereAreNoProblemsWithMapStr);
    return _m_notes;
}

VA(0x0047d594, 0x86)
CString TMapValidationFunc::_createTownStr(const TTown& town, const TTilePoint& loc, bool bSecondLayer)
{
    CString result;
    result.Format(kObjectAtLocationFmtStr, town.getTownTypeTraits().m_pName, loc.x(), loc.y(), bSecondLayer);
    return result;
}

VA(0x0047d61a, 0x1cd)
CString TMapValidationFunc::_createHeroStr(const TBasicHero& hero, const TTilePoint& loc, bool bSecondLayer) const
{
    CString heroStr;
    const TIdentifiedHero* pIdentifiedHero = dynamic_cast<const TIdentifiedHero*>(&hero);
    if (pIdentifiedHero != NULL) {
        const std::string& name = pIdentifiedHero->getBCustomName()
                                      ? pIdentifiedHero->getName()
                                      : _m_map.getHeroPrototype(pIdentifiedHero->getHeroID()).getName();
        heroStr.Format(kSpecificHeroAndClassFmtStr, name.c_str(),
                       THero::s_akClassTraits[pIdentifiedHero->getHeroClass()].m_name);
    } else {
        const THeroPlaceholder* pPlaceholder = dynamic_cast<const THeroPlaceholder*>(&hero);
        if (pPlaceholder != NULL) {
            bool bHasHero = pPlaceholder->getHeroID() != -1;
            if (bHasHero) {
                THeroID heroID = pPlaceholder->getHeroID();
                const THero::TTraits& traits = THero::s_akTraits[heroID];
                const THero::TClassTraits& classTraits = THero::s_akClassTraits[traits.m_class];
                heroStr.Format(kSpecificHeroAndClassFmtStr, _m_map.getHeroPrototype(heroID).getName().c_str(),
                               classTraits.m_name);
            } else
                heroStr = akAdvObjectTypeTraits[HERO_PLACEHOLDER].m_name;
        } else {
            const THero* pRandomHero = static_cast<const THero*>(&hero);
            if (pRandomHero->getBCustomName())
                heroStr.Format(kSpecificHeroAndClassFmtStr, pRandomHero->getName().c_str(),
                               akAdvObjectTypeTraits[RANDOM_HERO].m_name);
            else
                heroStr = akAdvObjectTypeTraits[RANDOM_HERO].m_name;
        }
    }
    CString result;
    result.Format(kObjectAtLocationFmtStr, (LPCTSTR)heroStr, loc.x(), loc.y(), bSecondLayer);
    return result;
}

VA(0x0047d7e7, 0x62)
CString TMapValidationFunc::_createTeamStr(unsigned int teamNum)
{
    CString result;
    result.Format(kTeamFmtStr, teamNum + 1);
    return result;
}

VA(0x0047d849, 0x26)
void TMapValidationFunc::_addNote(const CString& note)
{
    _m_notes += note;
    _m_notes += "\r\n";
    ++_m_numNotes;
}

VA(0x0047d86f, 0x1cf)
bool TMapValidationFunc::_isArtifactOnMap(TArtifact whichArtifact, const TSeersHut* pExcludedHut) const
{
    if (whichArtifact == ARTIFACT_HOLY_GRAIL && (_m_map.isGrailOnMap() || _m_map.getNumObelisksOnMap() > 0))
        return true;
    unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
    for (unsigned int i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TGameObject* pObject = layer.getPObject(*iter);
            if (const TGameArtifact* pArtifact = dynamic_cast<const TGameArtifact*>(pObject)) {
                if (pArtifact->getType() == ARTIFACT && pArtifact->getArtifactType() == whichArtifact)
                    return true;
            } else {
                const THero* pHero = dynamic_cast<const THero*>(pObject);
                if (pHero == NULL) {
                    if (const TTown* pTown = dynamic_cast<const TTown*>(pObject))
                        pHero = pTown->getPVisitingHero();
                }
                if (pHero != NULL) {
                    if (pHero->hasArtifact(whichArtifact))
                        return true;
                } else if (const TMonster* pMonster = dynamic_cast<const TMonster*>(pObject)) {
                    if (pMonster->getArtifact() == whichArtifact)
                        return true;
                } else if (const TBlackBox* pBlackBox = dynamic_cast<const TBlackBox*>(pObject)) {
                    const vector<TArtifact>& artifacts = pBlackBox->getContents().getArtifacts();
                    for (vector<TArtifact>::const_iterator it = artifacts.begin(); it != artifacts.end(); ++it)
                        if (*it == whichArtifact)
                            return true;
                } else if (const TSeersHut* pSeersHut = dynamic_cast<const TSeersHut*>(pObject)) {
                    if (pSeersHut != pExcludedHut && pSeersHut->getPReward() != NULL) {
                        if (const TSeersHut::TRewardArtifact* pReward
                            = dynamic_cast<const TSeersHut::TRewardArtifact*>(pSeersHut->getPReward()))
                            if (pReward->getArtifact() == whichArtifact)
                                return true;
                    }
                }
            }
        }
    }
    return false;
}

VA(0x0047da3e, 0x86)
bool TMapValidationFunc::_playerMayBeHuman(TPlayer whichPlayer) const
{
    const TTeamInfo& teamInfo = _m_map.getTeamInfo();
    if (teamInfo.getBHasTeams()) {
        unsigned int team = teamInfo.getPlayerTeam(whichPlayer);
        for (unsigned int player = 0; player < kNumPlayers; ++player)
            if (_m_map.isPlayerPresent(TPlayer(player)) && teamInfo.getPlayerTeam(TPlayer(player)) == team
                && _m_map.getPlayers()[player].getBHumanPlayable())
                return true;
        return false;
    }
    return _m_map.getPlayers()[whichPlayer].getBHumanPlayable();
}

// Complete passes the map's width as both dimensions to the neighbour
// test (Loki passes the height second); the editor's maps are square.
VA(0x0047dac4, 0x3fe)
void TMapValidationFunc::_checkForUnreachableObjects()
{
    DATA(0x0053d9ac) static const int akDirOrder[] = { 2, 3, 4, 5, 6, 7, 0, 1 };

    unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
    for (unsigned int i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TGameObject& obj = layer.getObject(*iter);
            if (!obj.hasTrigger() || obj.getType() == EYE_OF_MAGI)
                continue;
            TTilePoint objMapLoc = layer.getObjectLoc(*iter);
            unsigned int numDirs = akAdvObjectTypeTraits[obj.getType()].m_enterableFromNorth ? 8 : 5;
            bool bReachable = false;
            for (unsigned int objY = 0; objY < obj.getHeight(); ++objY) {
                for (unsigned int objX = 0; objX < obj.getWidth(); ++objX) {
                    if (!obj.getBCellTrigger(objX, objY))
                        continue;
                    TTilePoint triggerMapLoc = objMapLoc - TTilePoint(objX, objY);
                    bool bTriggerOnWater = layer.getCell(triggerMapLoc).getTerrainType() == eTerrainWater;
                    bool abAdjDir[8];
                    computeAdjacentDirs(_m_map.getWidth(), _m_map.getWidth(), triggerMapLoc.x(), triggerMapLoc.y(),
                                        abAdjDir);
                    for (unsigned int k = 0; k < numDirs; ++k) {
                        int dir = akDirOrder[k];
                        if (!abAdjDir[dir])
                            continue;
                        TTilePoint adjMapLoc = TPoint<int>(triggerMapLoc) + akAdjOffset[dir];
                        TTerrainType adjTerrainType = layer.getCell(adjMapLoc).getTerrainType();
                        if (adjTerrainType == eTerrainRock || (adjTerrainType == eTerrainWater) != bTriggerOnWater)
                            continue;
                        unsigned int numAdjObjs = layer.getNumObjectIDsAtCell(adjMapLoc.x(), adjMapLoc.y());
                        unsigned int j;
                        for (j = 0; j < numAdjObjs; ++j) {
                            unsigned int adjObjID = layer.getObjectIDAtCell(adjMapLoc.x(), adjMapLoc.y(), j);
                            const TGameObject& adjObj = layer.getObject(adjObjID);
                            if (akAdvObjectTypeTraits[adjObj.getType()].m_clearedOnVisit)
                                continue;
                            TTilePoint adjObjLoc = layer.getObjectLoc(adjObjID);
                            unsigned int adjObjX = adjObjLoc.x() - adjMapLoc.x();
                            unsigned int adjObjY = adjObjLoc.y() - adjMapLoc.y();
                            if (adjObj.getBCellTrigger(adjObjX, adjObjY)) {
                                int adjObjType = adjObj.getType();
                                if (akAdvObjectTypeTraits[adjObjType].m_blocksLanding
                                    || (!akAdvObjectTypeTraits[adjObjType].m_enterableFromNorth
                                        && (dir == 3 || dir == 4 || dir == 5)))
                                    break;
                            } else if (!adjObj.getBCellPassable(adjObjX, adjObjY))
                                break;
                        }
                        if (j == numAdjObjs) {
                            bReachable = true;
                            goto reachable;
                        }
                    }
                }
            }
        reachable:
            if (!bReachable) {
                objMapLoc = objMapLoc - obj.getTriggerLoc();
                CString locStr;
                locStr.Format(kObjectAtLocationFmtStr, obj.getTypeName().c_str(), objMapLoc.x(), objMapLoc.y(), i);
                CString note;
                note.Format(kObjectIsUnreachableFmtStr, (LPCTSTR)locStr);
                _addNote(note);
            }
        }
    }
}

VA(0x0047dec2, 0x306)
void TMapValidationFunc::visit(const TVCAquireArtifact& vc)
{
    if (_isArtifactOnMap(vc.getArtifact())) {
        unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
        for (unsigned int i = 0; i < numLayers; ++i) {
            const TGameMap::TLayer& layer = _m_map.getLayer(i);
            for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
                const THero* pHero = dynamic_cast<const THero*>(layer.getPObject(*iter));
                if (pHero == NULL) {
                    const TTown* pTown = dynamic_cast<const TTown*>(layer.getPObject(*iter));
                    if (pTown != NULL)
                        pHero = pTown->getPVisitingHero();
                }
                if (pHero != NULL && pHero->getOwner() != ePlayerNone
                    && (vc.getBAppliesToComputer() || _playerMayBeHuman(pHero->getOwner()))
                    && pHero->hasArtifact(vc.getArtifact())) {
                    TTilePoint loc = layer.getObjectLoc(*iter) - layer.getObject(*iter).getTriggerLoc();
                    CString heroStr;
                    const TIdentifiedHero* pIdentifiedHero = dynamic_cast<const TIdentifiedHero*>(pHero);
                    if (pIdentifiedHero != NULL) {
                        const std::string& name = pIdentifiedHero->getBCustomName()
                                                      ? pIdentifiedHero->getName()
                                                      : _m_map.getHeroPrototype(pIdentifiedHero->getHeroID()).getName();
                        heroStr.Format(kSpecificHeroAndClassFmtStr, name.c_str(),
                                       THero::s_akClassTraits[pIdentifiedHero->getHeroClass()].m_name);
                    } else if (pHero->getBCustomName())
                        heroStr.Format(kSpecificHeroAndClassFmtStr, pHero->getName().c_str(),
                                       akAdvObjectTypeTraits[RANDOM_HERO].m_name);
                    else
                        heroStr = akAdvObjectTypeTraits[RANDOM_HERO].m_name;
                    CString locStr;
                    locStr.Format(kObjectAtLocationFmtStr, (LPCTSTR)heroStr, loc.x(), loc.y(), i);
                    const char* artifactName = akArtifactTraits[vc.getArtifact()].m_name;
                    CString note;
                    note.Format(kSVCAquireArtifactButHeroHasArtifactFmtStr, artifactName, (LPCTSTR)locStr);
                    _addNote(note);
                }
            }
        }
    } else {
        CString note;
        note.Format(kSVCAquireArtifactButArtifactNotPresentFmtStr, akArtifactTraits[vc.getArtifact()].m_name);
        _addNote(note);
    }
}

VA(0x0047e1c8, 0x509)
void TMapValidationFunc::visit(const TVCAccumulateCreature& vc)
{
    TArray<unsigned int, kNumPlayers> aNumCreatures(0);
    unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
    for (unsigned int i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            if (const TTown* pTown = dynamic_cast<const TTown*>(layer.getPObject(*iter))) {
                if (pTown->getOwner() != ePlayerNone) {
                    aNumCreatures[pTown->getOwner()] += _countCreatures(vc.getCreatureType(), pTown->getGarrison());
                    if (pTown->getPVisitingHero() != NULL && pTown->getPVisitingHero()->getBCustomArmy())
                        aNumCreatures[pTown->getOwner()]
                            += _countCreatures(vc.getCreatureType(), pTown->getPVisitingHero()->getArmy());
                }
            } else if (const THero* pHero = dynamic_cast<const THero*>(layer.getPObject(*iter))) {
                if (pHero->getOwner() != ePlayerNone && pHero->getBCustomArmy())
                    aNumCreatures[pHero->getOwner()] += _countCreatures(vc.getCreatureType(), pHero->getArmy());
            } else if (const TGarrison* pGarrison = dynamic_cast<const TGarrison*>(layer.getPObject(*iter))) {
                if (pGarrison->getOwner() != ePlayerNone)
                    aNumCreatures[pGarrison->getOwner()] += _countCreatures(vc.getCreatureType(), pGarrison->getArmy());
            }
        }
    }
    const TTeamInfo& teamInfo = _m_map.getTeamInfo();
    if (teamInfo.getBHasTeams()) {
        TArray<unsigned int, TTeamInfo::s_kMaxTeams> aNumTeamCreatures(0);
        TArray<bool, TTeamInfo::s_kMaxTeams> abTeamMayBeHuman(false);
        for (unsigned int player = 0; player < kNumPlayers; ++player) {
            if (_m_map.isPlayerPresent(TPlayer(player))) {
                unsigned int team = teamInfo.getPlayerTeam(TPlayer(player));
                aNumTeamCreatures[team] += aNumCreatures[player];
                if (_m_map.getPlayers()[player].getBHumanPlayable())
                    abTeamMayBeHuman[team] = true;
            }
        }
        for (unsigned int team = 0; team < teamInfo.getNumTeams(); ++team) {
            if ((vc.getBAppliesToComputer() || abTeamMayBeHuman[team]) && aNumTeamCreatures[team] >= vc.getQuantity()) {
                CString note;
                note.Format(kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr, vc.getQuantity(),
                            getArmyName(vc.getCreatureType(), vc.getQuantity()), (LPCTSTR)_createTeamStr(team),
                            aNumTeamCreatures[team], getArmyName(vc.getCreatureType(), aNumTeamCreatures[team]));
                _addNote(note);
            }
        }
    } else {
        for (unsigned int player = 0; player < kNumPlayers; ++player) {
            if (_m_map.isPlayerPresent(TPlayer(player))
                && (vc.getBAppliesToComputer() || _m_map.getPlayers()[player].getBHumanPlayable())
                && aNumCreatures[player] >= vc.getQuantity()) {
                CString note;
                note.Format(kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr, vc.getQuantity(),
                            getArmyName(vc.getCreatureType(), vc.getQuantity()), akPlayerTraits[player].m_pName,
                            aNumCreatures[player], getArmyName(vc.getCreatureType(), aNumCreatures[player]));
                _addNote(note);
            }
        }
    }
}

VA(0x0047e6d1, 0x25e)
void TMapValidationFunc::visit(const TVCUpgradeTown& vc)
{
    DATA(0x0053d9cc) static const TBuilding akHallBuildings[] = { eBuildingTownHall, eBuildingCityHall, eBuildingCapitol };
    DATA(0x0053d9d8) static const TBuilding akCastleBuildings[] = { eBuildingFort, eBuildingCitadel, eBuildingCastle };

    TBuilding hallBuilding = akHallBuildings[vc.getHallLevel()];
    TBuilding castleBuilding = akCastleBuildings[vc.getCastleLevel()];
    const TMapObjectRef& townRef = vc.getTownRef();
    const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(townRef));
    const TArray<TTown::TBuildingState, TTown::s_kNumBuildings>& aBuildingStates = pTown->getBuildingStates();
    if (aBuildingStates[hallBuilding].getBBuilt() && aBuildingStates[castleBuilding].getBBuilt()) {
        CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                         townRef.getBSecondLayer());
        const TTown::TBuildingTraits& hallTraits = pTown->getTownTypeTraits().m_akBuildingTraits[hallBuilding];
        const TTown::TBuildingTraits& castleTraits = pTown->getTownTypeTraits().m_akBuildingTraits[castleBuilding];
        CString note;
        note.Format(kSVCUpgradeTownBuildingsAlreadyBuiltFmtStr, hallTraits.m_pName, castleTraits.m_pName,
                    (LPCTSTR)townStr);
        _addNote(note);
    } else if (pTown->getBIsBuildingDisabled(hallBuilding) || pTown->getBIsBuildingDisabled(castleBuilding)) {
        CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                         townRef.getBSecondLayer());
        const TTown::TBuildingTraits& hallTraits = pTown->getTownTypeTraits().m_akBuildingTraits[hallBuilding];
        const TTown::TBuildingTraits& castleTraits = pTown->getTownTypeTraits().m_akBuildingTraits[castleBuilding];
        CString note;
        note.Format(kSVCUpgradeTownBuildingsDisabledFmtStr, hallTraits.m_pName, castleTraits.m_pName,
                    (LPCTSTR)townStr);
        _addNote(note);
    }
}

VA(0x0047e92f, 0x281)
void TMapValidationFunc::visit(const TVCBuildHolyGrailStruct& vc)
{
    const TMapObjectRef& townRef = vc.getTownRef();
    if (townRef != TMapObjectRef()) {
        const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(townRef));
        if (pTown->getBuildingStates()[eBuildingGrail].getBBuilt()) {
            CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                             townRef.getBSecondLayer());
            CString note;
            note.Format(kSVCBuildGrailStructAlreadyBuiltFmtStr, (LPCTSTR)townStr);
            _addNote(note);
        } else if (pTown->getBuildingStates()[eBuildingGrail].getBDisabled()) {
            CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                             townRef.getBSecondLayer());
            CString note;
            note.Format(kSVCBuildGrailStructDisabledFmtStr, (LPCTSTR)townStr);
            _addNote(note);
        }
    } else {
        bool bCanBuild = false;
        unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
        for (unsigned int i = 0; i < numLayers && !bCanBuild; ++i) {
            const TGameMap::TLayer& layer = _m_map.getLayer(i);
            for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
                const TTown* pTown = dynamic_cast<const TTown*>(layer.getPObject(*iter));
                if (pTown != NULL && !pTown->getBuildingStates()[eBuildingGrail].getBBuilt()
                    && !pTown->getBuildingStates()[eBuildingGrail].getBDisabled()) {
                    bCanBuild = true;
                    break;
                }
            }
        }
        if (!bCanBuild)
            _addNote(kSVCBuildGrailStructNowhereToBuildStr);
    }
}

VA(0x0047ebcd, 0x165)
void TMapValidationFunc::visit(const TVCDefeatHero& vc)
{
    const TMapObjectRef& heroRef = vc.getHeroRef();
    const TBasicHero* pHero = dynamic_cast<const TBasicHero*>(_m_map.getPObject(heroRef));
    if (pHero == NULL)
        pHero = dynamic_cast<const TTown*>(_m_map.getPObject(heroRef))->getPVisitingHero();
    if (pHero->getOwner() != ePlayerNone && _playerMayBeHuman(pHero->getOwner())) {
        CString heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                         heroRef.getBSecondLayer());
        CString note;
        note.Format(kSVCDefeatHeroBelongsToHumanFmtStr, (LPCTSTR)heroStr, akPlayerTraits[pHero->getOwner()].m_pName);
        _addNote(note);
    }
}

VA(0x0047ed32, 0x1bd)
void TMapValidationFunc::visit(const TVCCaptureTown& vc)
{
    const TMapObjectRef& townRef = vc.getTownRef();
    const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(townRef));
    if (pTown->getOwner() != ePlayerNone) {
        if (vc.getBAppliesToComputer()) {
            CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                             townRef.getBSecondLayer());
            CString note;
            note.Format(kSVCCaptureTownAlreadyOwnedFmtStr, (LPCTSTR)townStr, akPlayerTraits[pTown->getOwner()].m_pName);
            _addNote(note);
        } else if (_playerMayBeHuman(pTown->getOwner())) {
            CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                             townRef.getBSecondLayer());
            CString note;
            note.Format(kSVCCaptureTownOwnedByHumanFmtStr, (LPCTSTR)townStr, akPlayerTraits[pTown->getOwner()].m_pName);
            _addNote(note);
        }
    }
}

VA(0x0047eeef, 0x2ac)
void TMapValidationFunc::visit(const TVCFlagAllCreatureGenerators& vc)
{
    unsigned int numGenerators = 0;
    TArray<unsigned int, kNumPlayers> aNumOwned(0);
    unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
    for (unsigned int i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TGenerator* pGenerator = dynamic_cast<const TGenerator*>(layer.getPObject(*iter));
            if (pGenerator != NULL) {
                ++numGenerators;
                if (pGenerator->getOwner() != ePlayerNone)
                    ++aNumOwned[pGenerator->getOwner()];
            }
        }
    }
    if (numGenerators == 0) {
        _addNote(kSVCFlagGeneratorsNoGeneratorsOnMapStr);
    } else {
        const TTeamInfo& teamInfo = _m_map.getTeamInfo();
        if (teamInfo.getBHasTeams()) {
            TArray<unsigned int, TTeamInfo::s_kMaxTeams> aNumTeamOwned(0);
            TArray<bool, TTeamInfo::s_kMaxTeams> abTeamMayBeHuman(false);
            for (unsigned int player = 0; player < kNumPlayers; ++player) {
                if (_m_map.isPlayerPresent(TPlayer(player))) {
                    unsigned int team = teamInfo.getPlayerTeam(TPlayer(player));
                    aNumTeamOwned[team] += aNumOwned[player];
                    if (_m_map.getPlayers()[player].getBHumanPlayable())
                        abTeamMayBeHuman[team] = true;
                }
            }
            for (unsigned int team = 0; team < teamInfo.getNumTeams(); ++team) {
                if (aNumTeamOwned[team] > 0) {
                    if (aNumTeamOwned[team] == numGenerators && (vc.getBAppliesToComputer() || abTeamMayBeHuman[team])) {
                        CString note;
                        note.Format(kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr, (LPCTSTR)_createTeamStr(team));
                        _addNote(note);
                    }
                    break;
                }
            }
        } else {
            for (unsigned int player = 0; player < kNumPlayers; ++player) {
                if (aNumOwned[player] > 0) {
                    if (aNumOwned[player] == numGenerators && _m_map.isPlayerPresent(TPlayer(player))
                        && (vc.getBAppliesToComputer() || _m_map.getPlayers()[player].getBHumanPlayable())) {
                        CString note;
                        note.Format(kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr, akPlayerTraits[player].m_pName);
                        _addNote(note);
                    }
                    break;
                }
            }
        }
    }
}

VA(0x0047f19b, 0x2cd)
void TMapValidationFunc::visit(const TVCFlagAllMines& vc)
{
    unsigned int numMines = 0;
    TArray<unsigned int, kNumPlayers> aNumOwned(0);
    unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
    for (unsigned int i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TMine* pMine = dynamic_cast<const TMine*>(layer.getPObject(*iter));
            if (pMine != NULL && (pMine->getType() == MINE || pMine->getType() == ABANDONED_MINE)) {
                ++numMines;
                if (pMine->getOwner() != ePlayerNone)
                    ++aNumOwned[pMine->getOwner()];
            }
        }
    }
    if (numMines == 0) {
        _addNote(kSVCFlagMinesNoMinesOnMapStr);
    } else {
        const TTeamInfo& teamInfo = _m_map.getTeamInfo();
        if (teamInfo.getBHasTeams()) {
            TArray<unsigned int, TTeamInfo::s_kMaxTeams> aNumTeamOwned(0);
            TArray<bool, TTeamInfo::s_kMaxTeams> abTeamMayBeHuman(false);
            for (unsigned int player = 0; player < kNumPlayers; ++player) {
                if (_m_map.isPlayerPresent(TPlayer(player))) {
                    unsigned int team = teamInfo.getPlayerTeam(TPlayer(player));
                    aNumTeamOwned[team] += aNumOwned[player];
                    if (_m_map.getPlayers()[player].getBHumanPlayable())
                        abTeamMayBeHuman[team] = true;
                }
            }
            for (unsigned int team = 0; team < teamInfo.getNumTeams(); ++team) {
                if (aNumTeamOwned[team] > 0) {
                    if (aNumTeamOwned[team] == numMines && (vc.getBAppliesToComputer() || abTeamMayBeHuman[team])) {
                        CString note;
                        note.Format(kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr, (LPCTSTR)_createTeamStr(team));
                        _addNote(note);
                    }
                    break;
                }
            }
        } else {
            for (unsigned int player = 0; player < kNumPlayers; ++player) {
                if (aNumOwned[player] > 0) {
                    if (aNumOwned[player] == numMines && _m_map.isPlayerPresent(TPlayer(player))
                        && (vc.getBAppliesToComputer() || _m_map.getPlayers()[player].getBHumanPlayable())) {
                        CString note;
                        note.Format(kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr, akPlayerTraits[player].m_pName);
                        _addNote(note);
                    }
                    break;
                }
            }
        }
    }
}

VA(0x0047f468, 0x74)
void TMapValidationFunc::visit(const TVCTransportArtifact& vc)
{
    if (!_isArtifactOnMap(vc.getArtifact())) {
        CString note;
        note.Format(kSVCTransportArtifactNotPresentFmtStr, akArtifactTraits[vc.getArtifact()].m_name);
        _addNote(note);
    }
}

VA(0x0047f4dc, 0x48c)
void TMapValidationFunc::visit(const TLCLoseTown& lc)
{
    const TMapObjectRef& townRef = lc.getTownRef();
    const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(townRef));
    if (pTown->getOwner() == ePlayerNone) {
        CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                         townRef.getBSecondLayer());
        CString note;
        note.Format(kSLCLoseTownNotOwnedFmtStr, (LPCTSTR)townStr);
        _addNote(note);
    } else {
        const TTeamInfo& teamInfo = _m_map.getTeamInfo();
        if (teamInfo.getBHasTeams()) {
            bitset<TTeamInfo::s_kMaxTeams> abTeamMayBeHuman;
            unsigned int team;
            for (unsigned int player = 0; player < kNumPlayers; ++player) {
                if (_m_map.isPlayerPresent(TPlayer(player))) {
                    team = teamInfo.getPlayerTeam(TPlayer(player));
                    if (_m_map.getPlayers()[player].getBHumanPlayable())
                        abTeamMayBeHuman[team] = true;
                }
            }
            team = teamInfo.getPlayerTeam(pTown->getOwner());
            if (!abTeamMayBeHuman[team]) {
                CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                                 townRef.getBSecondLayer());
                CString note;
                note.Format(kSLCLoseTownTeamNotHumanFmtStr, (LPCTSTR)townStr, (LPCTSTR)_createTeamStr(team));
                _addNote(note);
            } else if (abTeamMayBeHuman.count() > 1) {
                CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                                 townRef.getBSecondLayer());
                CString note;
                note.Format(kSLCLoseTownMultipleHumanTeamsFmtStr, (LPCTSTR)townStr);
                _addNote(note);
            }
        } else if (!_m_map.getPlayers()[pTown->getOwner()].getBHumanPlayable()) {
            CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                             townRef.getBSecondLayer());
            CString note;
            note.Format(kSLCLoseTownPlayerNotHumanFmtStr, (LPCTSTR)townStr, akPlayerTraits[pTown->getOwner()].m_pName);
            _addNote(note);
        } else {
            const TArray<TPlayerInfo, kNumPlayers>& aPlayers = _m_map.getPlayers();
            unsigned int playerNum = 0;
            while (!aPlayers[playerNum].getBHumanPlayable())
                ++playerNum;
            for (++playerNum; playerNum < kNumPlayers; ++playerNum) {
                if (aPlayers[playerNum].getBHumanPlayable()) {
                    CString townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                                     townRef.getBSecondLayer());
                    CString note;
                    note.Format(kSLCLoseTownMultipleHumanPlayersFmtStr, (LPCTSTR)townStr);
                    _addNote(note);
                    break;
                }
            }
        }
    }
}

VA(0x0047f968, 0x52c)
void TMapValidationFunc::visit(const TLCLoseHero& lc)
{
    const TMapObjectRef& heroRef = lc.getHeroRef();
    const TBasicHero* pHero = dynamic_cast<const TBasicHero*>(_m_map.getPObject(heroRef));
    if (pHero == NULL)
        pHero = dynamic_cast<const TTown*>(_m_map.getPObject(heroRef))->getPVisitingHero();
    if (pHero->getOwner() == ePlayerNone) {
        CString heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                         heroRef.getBSecondLayer());
        CString note;
        note.Format(kSLCLoseHeroNotOwnedFmtStr, (LPCTSTR)heroStr);
        _addNote(note);
    } else {
        const TTeamInfo& teamInfo = _m_map.getTeamInfo();
        if (teamInfo.getBHasTeams()) {
            bitset<TTeamInfo::s_kMaxTeams> abTeamMayBeHuman;
            unsigned int team;
            for (unsigned int player = 0; player < kNumPlayers; ++player) {
                if (_m_map.isPlayerPresent(TPlayer(player))) {
                    team = teamInfo.getPlayerTeam(TPlayer(player));
                    if (_m_map.getPlayers()[player].getBHumanPlayable())
                        abTeamMayBeHuman[team] = true;
                }
            }
            team = teamInfo.getPlayerTeam(pHero->getOwner());
            if (!abTeamMayBeHuman[team]) {
                CString heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                                 heroRef.getBSecondLayer());
                CString note;
                note.Format(kSLCLoseHeroTeamNotHumanFmtStr, (LPCTSTR)heroStr, (LPCTSTR)_createTeamStr(team));
                _addNote(note);
            } else if (abTeamMayBeHuman.count() > 1) {
                CString heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                                 heroRef.getBSecondLayer());
                CString note;
                note.Format(kSLCLoseHeroMultipleHumanTeamsFmtStr, (LPCTSTR)heroStr);
                _addNote(note);
            }
        } else if (!_m_map.getPlayers()[pHero->getOwner()].getBHumanPlayable()) {
            CString heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                             heroRef.getBSecondLayer());
            CString note;
            note.Format(kSLCLoseHeroPlayerNotHumanFmtStr, (LPCTSTR)heroStr, akPlayerTraits[pHero->getOwner()].m_pName);
            _addNote(note);
        } else {
            const TArray<TPlayerInfo, kNumPlayers>& aPlayers = _m_map.getPlayers();
            unsigned int playerNum = 0;
            while (!aPlayers[playerNum].getBHumanPlayable())
                ++playerNum;
            for (++playerNum; playerNum < kNumPlayers; ++playerNum) {
                if (aPlayers[playerNum].getBHumanPlayable()) {
                    CString heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                                     heroRef.getBSecondLayer());
                    CString note;
                    note.Format(kSLCLoseHeroMultipleHumanPlayersFmtStr, (LPCTSTR)heroStr);
                    _addNote(note);
                    break;
                }
            }
        }
    }
}
