// MapValidation.cpp - Loki h3maped object 77: the map validation function,
// the editor's "Validate map" report. Every problem becomes one line of
// text; the victory and loss conditions are checked by visiting them.
// Assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <algorithm>
#include <bitset>
#include <functional>
#include <string>

#include "adventureobjecttype.h"
#include "artifact.h"
#include "creaturetype.h"
#include "objnames.h"
#include "editor/BlackBox.h"
#include "editor/GameMap.h"
#include "editor/GameObject.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/MapValidation.h"
#include "editor/Monster.h"
#include "editor/ObjectSpecializations.h"
#include "editor/SeersHut.h"
#include "editor/TilePoint.h"
#include "editor/Town.h"

TMapValidationFunc::TMapValidationFunc(const TGameMap& map) : _m_map(map), _m_numNotes(0)
{
}

string TMapValidationFunc::operator()()
{
    if (_m_map.getNumPlayableSlots() == 0)
        _addNote(kThereAreNoPlayersOnMapStr);
    else if (_m_map.getNumTownsOnMap() == 0)
        _addNote(kThereAreNoTownsOnMapStr);

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
    for (unsigned int i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TFlaggableObject* pFlaggable = dynamic_cast<const TFlaggableObject*>(layer.getPObject(*iter));
            if (pFlaggable != NULL && pFlaggable->getOwner() != ePlayerNone)
                ownersMask[pFlaggable->getOwner()] = true;
        }
    }
    for (unsigned int player = 0; player < kNumPlayers; ++player) {
        if (ownersMask[player] && !_m_map.isPlayerPresent(TPlayer(player))) {
            char buf[512];
            snprintf(buf, sizeof(buf), kPlayerOwnsObjectsButNotPresentFmtStr, akPlayerTraits[player].m_pName);
            buf[sizeof(buf) - 1] = '\0';
            _addNote(buf);
        }
    }

    for (i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TTown* pTown = dynamic_cast<const TTown*>(layer.getPObject(*iter));
            if (pTown != NULL && pTown->getBuildingStates()[eBuildingShipyard].getBBuilt()) {
                TTilePoint loc = layer.getObjectLoc(*iter) - pTown->getTriggerLoc();
                unsigned int shoreY = loc.y() + 2;
                unsigned int shoreLeftX = loc.x() - 1;
                unsigned int shoreRightX = loc.x() + 1;
                if (shoreY >= _m_map.getHeight()
                    || ((shoreLeftX >= _m_map.getWidth()
                         || layer.getCell(shoreLeftX, shoreY).getTerrainType() != eTerrainWater)
                        && (shoreRightX >= _m_map.getWidth()
                            || layer.getCell(shoreRightX, shoreY).getTerrainType() != eTerrainWater))) {
                    char locBuf[512];
                    snprintf(locBuf, sizeof(locBuf), kObjectAtLocationFmtStr, pTown->getTownTypeTraits().m_pName,
                             loc.x(), loc.y(), i);
                    locBuf[sizeof(locBuf) - 1] = '\0';
                    char buf[512];
                    snprintf(buf, sizeof(buf), kTownHasShipyardButIsLandlockedFmtStr, locBuf);
                    buf[sizeof(buf) - 1] = '\0';
                    _addNote(buf);
                }
            }
        }
    }

    TArray<unsigned int, 2> aNumGates(0);
    TArray<unsigned int, kNumMonolithTypes> aNumMonoliths(0);
    TArray<unsigned int, kNumOneWayMonolithTypes> aNumEntrances(0);
    TArray<unsigned int, kNumOneWayMonolithTypes> aNumExits(0);
    unsigned int numWhirlpools = 0;
    for (i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TGameObject& obj = layer.getObject(*iter);
            switch (obj.getType()) {
            case UNDERGROUND_GATE:
                ++aNumGates[i];
                break;
            case LITH_TWOWAY:
#line 213
                assert(obj.getExtra() >= 0 && obj.getExtra() < kNumMonolithTypes);
                ++aNumMonoliths[obj.getExtra()];
                break;
            case LITH_ONEWAY_ENTRANCE:
#line 218
                assert(obj.getExtra() >= 0 && obj.getExtra() < kNumOneWayMonolithTypes);
                ++aNumEntrances[obj.getExtra()];
                break;
            case LITH_ONEWAY_EXIT:
#line 223
                assert(obj.getExtra() >= 0 && obj.getExtra() < kNumOneWayMonolithTypes);
                ++aNumExits[obj.getExtra()];
                break;
            case WHIRLPOOL:
                ++numWhirlpools;
                break;
            }
        }
    }
    for (unsigned int type = 0; type < kNumMonolithTypes; ++type) {
        if (aNumMonoliths[type] == 1) {
            char buf[512];
            snprintf(buf, sizeof(buf), kOnlyOneMonolithOnMapFmtStr, akMonolithTypeTraits[type].m_name);
            buf[sizeof(buf) - 1] = '\0';
            _addNote(buf);
        }
    }
    for (type = 0; type < kNumOneWayMonolithTypes; ++type) {
        const char* typeName = akOneWayMonolithTypeTraits[type].m_name;
        if (aNumEntrances[type] != 0 && aNumExits[type] == 0) {
            char buf[512];
            snprintf(buf, sizeof(buf), kMonolithEntranceButNoExitFmtStr, typeName, typeName);
            buf[sizeof(buf) - 1] = '\0';
            _addNote(buf);
        } else if (aNumEntrances[type] == 0 && aNumExits[type] != 0) {
            char buf[512];
            snprintf(buf, sizeof(buf), kMonolithExitButNoEntranceFmtStr, typeName, typeName);
            buf[sizeof(buf) - 1] = '\0';
            _addNote(buf);
        }
    }
    if (numLayers == 1) {
        if (aNumGates[0] != 0)
            _addNote(kSubterraneanGatesOnOneLayerMapStr);
    } else {
#line 284
        assert(numLayers == 2);
        if (aNumGates[0] > aNumGates[1])
            _addNote(kMoreSubterraneanGatesAboveThanBelowStr);
        else if (aNumGates[0] < aNumGates[1])
            _addNote(kMoreSubterraneanGatesBelowThanAboveStr);
    }
    if (numWhirlpools == 1)
        _addNote(kOneWhirlpoolStr);

    for (i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TSeersHut* pSeersHut = dynamic_cast<const TSeersHut*>(layer.getPObject(*iter));
            if (pSeersHut != NULL && pSeersHut->getQuestArtifact() != eArtifactNone
                && !_isArtifactOnMap(pSeersHut->getQuestArtifact(), pSeersHut)) {
                TTilePoint loc = layer.getObjectLoc(*iter) - pSeersHut->getTriggerLoc();
                char locBuf[512];
                snprintf(locBuf, sizeof(locBuf), kObjectAtLocationFmtStr, akAdvObjectTypeTraits[SEER].m_name,
                         loc.x(), loc.y(), i);
                char buf[512];
                snprintf(buf, sizeof(buf), kSeersHutQuestArtifactNotOnMapFmtStr, locBuf,
                         akArtifactTraits[pSeersHut->getQuestArtifact()].m_name);
                buf[sizeof(buf) - 1] = '\0';
                _addNote(buf);
            }
        }
    }

    const TVictoryCondition* pVictoryCondition = _m_map.getPVictoryCondition();
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

unsigned int TMapValidationFunc::_countCreatures(TCreatureType creatureType, const TArmy& army)
{
#line 365
    assert(creatureType >= 0 && creatureType < kNumCreatureTypes);
    unsigned int count = 0;
    for (unsigned int i = 0; i < TArmy::s_kNumCreatureStacks; ++i)
        if (army[i].getCreatureType() == creatureType)
            count += army[i].getQuantity();
    return count;
}

string TMapValidationFunc::_createTownStr(const TTown& town, const TPoint<unsigned int>& loc, bool bSecondLayer)
{
    string result;
    char buf[512];
    snprintf(buf, sizeof(buf), kObjectAtLocationFmtStr, town.getTownTypeTraits().m_pName, loc.x(), loc.y(),
             bSecondLayer);
    result = buf;
    return result;
}

string TMapValidationFunc::_createHeroStr(const THero& hero, const TPoint<unsigned int>& loc, bool bSecondLayer)
{
    char buf[512];
    string heroStr;
    if (hero.getClass() < kNumHeroClasses || hero.getBCustomName()) {
        snprintf(buf, sizeof(buf), kSpecificHeroAndClassFmtStr, hero.getName().c_str(),
                 hero.getClassTraits().m_name);
        heroStr = buf;
    } else {
        heroStr = akAdvObjectTypeTraits[RANDOM_HERO].m_name;
    }
    string result;
    snprintf(buf, sizeof(buf), kObjectAtLocationFmtStr, heroStr.c_str(), loc.x(), loc.y(), bSecondLayer);
    result = buf;
    return result;
}

string TMapValidationFunc::_createTeamStr(unsigned int teamNum)
{
#line 429
    assert(teamNum < TTeamInfo::s_kMaxTeams);
    string result;
    char buf[512];
    snprintf(buf, sizeof(buf), kTeamFmtStr, teamNum + 1);
    result = buf;
    return result;
}

void TMapValidationFunc::_addNote(char* note)
{
    _m_notes += note;
    _m_notes += "\n";
    ++_m_numNotes;
}

bool TMapValidationFunc::_isArtifactOnMap(TArtifact whichArtifact, const TSeersHut* pExcludedHut) const
{
#line 451
    assert(whichArtifact >= 0 && whichArtifact < kNumArtifacts);
    assert(whichArtifact == eArtifactHolyGrail || ( akArtifactTraits[ whichArtifact ].m_class & ArtifactClassSpecial ) == 0);
    if (whichArtifact == eArtifactHolyGrail && (_m_map.isGrailOnMap() || _m_map.getNumObelisksOnMap() != 0))
        return true;
    unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
    for (unsigned int i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TGameObject& obj = layer.getObject(*iter);
            if (const TGameArtifact* pArtifact = dynamic_cast<const TGameArtifact*>(&obj)) {
                if (pArtifact->getType() == ARTIFACT && pArtifact->getArtifactType() == whichArtifact)
                    return true;
            } else {
                const THero* pHero = dynamic_cast<const THero*>(&obj);
                if (pHero == NULL) {
                    if (const TTown* pTown = dynamic_cast<const TTown*>(&obj))
                        pHero = pTown->getPVisitingHero();
                }
                if (pHero != NULL) {
                    if (pHero->getBHasArtifact(whichArtifact))
                        return true;
                } else if (const TMonster* pMonster = dynamic_cast<const TMonster*>(&obj)) {
                    if (pMonster->getArtifact() == whichArtifact)
                        return true;
                } else if (const TBlackBox* pBlackBox = dynamic_cast<const TBlackBox*>(&obj)) {
                    const vector<TArtifact>& artifacts = pBlackBox->getContents().getArtifacts();
                    for (vector<TArtifact>::const_iterator it = artifacts.begin(); it != artifacts.end(); ++it)
                        if (*it == whichArtifact)
                            return true;
                } else if (const TSeersHut* pSeersHut = dynamic_cast<const TSeersHut*>(&obj)) {
                    if (pSeersHut != pExcludedHut && pSeersHut->getPQuestReward() != NULL) {
                        if (const TSeersHut::TArtifactReward* pReward
                            = dynamic_cast<const TSeersHut::TArtifactReward*>(pSeersHut->getPQuestReward()))
                            if (pReward->getArtifact() == whichArtifact)
                                return true;
                    }
                }
            }
        }
    }
    return false;
}

bool TMapValidationFunc::_playerMayBeHuman(TPlayer whichPlayer) const
{
#line 533
    assert(whichPlayer >= 0 && whichPlayer < kNumPlayers);
    assert(_m_map.isPlayerPresent( whichPlayer ));
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

void TMapValidationFunc::_checkForUnreachableObjects()
{
    static const int akDirOrder[] = { 2, 3, 4, 5, 6, 7, 0, 1 };

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
#line 599
                    assert(triggerMapLoc.x() < _m_map.getWidth());
                    assert(triggerMapLoc.y() < _m_map.getHeight());
                    bool bTriggerOnWater = layer.getCell(triggerMapLoc).getTerrainType() == eTerrainWater;
                    bool abAdjDir[8];
                    computeAdjacentDirs(_m_map.getWidth(), _m_map.getHeight(), triggerMapLoc.x(), triggerMapLoc.y(),
                                        abAdjDir);
                    for (unsigned int k = 0; k < numDirs; ++k) {
                        int dir = akDirOrder[k];
                        if (!abAdjDir[dir])
                            continue;
                        TTilePoint adjMapLoc = TPoint<int>(triggerMapLoc) + akAdjOffset[dir];
                        TTerrainType adjTerrainType = layer.getCell(adjMapLoc).getTerrainType();
                        if (adjTerrainType == eTerrainRock || (adjTerrainType == eTerrainWater) != bTriggerOnWater)
                            continue;
                        unsigned int numAdjObjs = layer.getNumObjectIDsAtCell(adjMapLoc);
                        unsigned int j;
                        for (j = 0; j < numAdjObjs; ++j) {
                            unsigned int adjObjID = layer.getObjectIDAtCell(adjMapLoc, j);
                            const TGameObject& adjObj = layer.getObject(adjObjID);
                            if (akAdvObjectTypeTraits[adjObj.getType()].m_clearedOnVisit)
                                continue;
                            TTilePoint adjObjLoc = layer.getObjectLoc(adjObjID);
                            unsigned int adjObjX = adjObjLoc.x() - adjMapLoc.x();
                            unsigned int adjObjY = adjObjLoc.y() - adjMapLoc.y();
#line 633
                            assert(adjObj.getBCellPlaced( adjObjX, adjObjY ));
                            if (adjObj.getBCellTrigger(adjObjX, adjObjY)) {
#line 636
                                assert(!adjObj.getBCellPassable( adjObjX, adjObjY ));
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
                objMapLoc -= obj.getTriggerLoc();
                char locBuf[512];
                snprintf(locBuf, sizeof(locBuf), kObjectAtLocationFmtStr, obj.getTypeName().c_str(), objMapLoc.x(),
                         objMapLoc.y(), i);
                char buf[512];
                snprintf(buf, sizeof(buf), kObjectIsUnreachableFmtStr, locBuf);
                buf[sizeof(buf) - 1] = '\0';
                _addNote(buf);
            }
        }
    }
}

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
                    && pHero->getBHasArtifact(vc.getArtifact())) {
                    TTilePoint loc = layer.getObjectLoc(*iter) - layer.getObject(*iter).getTriggerLoc();
                    string heroStr;
                    char heroBuf[512];
                    if (pHero->getClass() < kNumHeroClasses || pHero->getBCustomName()) {
                        snprintf(heroBuf, sizeof(heroBuf), kSpecificHeroAndClassFmtStr, pHero->getName().c_str(),
                                 pHero->getClassTraits().m_name);
                        heroStr = heroBuf;
                    } else {
                        heroStr = akAdvObjectTypeTraits[RANDOM_HERO].m_name;
                    }
                    char locBuf[512];
                    snprintf(locBuf, sizeof(locBuf), kObjectAtLocationFmtStr, heroStr.c_str(), loc.x(), loc.y(), i);
                    const char* artifactName = akArtifactTraits[vc.getArtifact()].m_name;
                    char buf[512];
                    snprintf(buf, sizeof(buf), kSVCAquireArtifactButHeroHasArtifactFmtStr, artifactName, locBuf);
                    _addNote(buf);
                }
            }
        }
    } else {
        char buf[512];
        snprintf(buf, sizeof(buf), kSVCAquireArtifactButArtifactNotPresentFmtStr,
                 akArtifactTraits[vc.getArtifact()].m_name);
        buf[sizeof(buf) - 1] = '\0';
        _addNote(buf);
    }
}

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
                    if (pTown->getPVisitingHero() != NULL)
                        aNumCreatures[pTown->getOwner()]
                            += _countCreatures(vc.getCreatureType(), pTown->getPVisitingHero()->getArmy());
                }
            } else if (const THero* pHero = dynamic_cast<const THero*>(layer.getPObject(*iter))) {
                if (pHero->getOwner() != ePlayerNone)
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
                char buf[512];
                snprintf(buf, sizeof(buf), kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr, vc.getQuantity(),
                         GetArmyName(vc.getCreatureType(), vc.getQuantity()), _createTeamStr(team).c_str(),
                         aNumTeamCreatures[team], GetArmyName(vc.getCreatureType(), aNumTeamCreatures[team]));
                buf[sizeof(buf) - 1] = '\0';
                _addNote(buf);
            }
        }
    } else {
        for (unsigned int player = 0; player < kNumPlayers; ++player) {
            if (_m_map.isPlayerPresent(TPlayer(player))
                && (vc.getBAppliesToComputer() || _m_map.getPlayers()[player].getBHumanPlayable())
                && aNumCreatures[player] >= vc.getQuantity()) {
                char buf[512];
                snprintf(buf, sizeof(buf), kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr, vc.getQuantity(),
                         GetArmyName(vc.getCreatureType(), vc.getQuantity()), akPlayerTraits[player].m_pName,
                         aNumCreatures[player], GetArmyName(vc.getCreatureType(), aNumCreatures[player]));
                buf[sizeof(buf) - 1] = '\0';
                _addNote(buf);
            }
        }
    }
}

void TMapValidationFunc::visit(const TVCAccumulateResource& vc)
{
}

void TMapValidationFunc::visit(const TVCUpgradeTown& vc)
{
    static const TBuilding akHallBuildings[] = { eBuildingTownHall, eBuildingCityHall, eBuildingCapitol };
    static const TBuilding akCastleBuildings[] = { eBuildingFort, eBuildingCitadel, eBuildingCastle };

    TBuilding hallBuilding = akHallBuildings[vc.getHallLevel()];
    TBuilding castleBuilding = akCastleBuildings[vc.getCastleLevel()];
    const TMapObjectRef& townRef = vc.getTownRef();
    const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(townRef));
#line 908
    assert(pTown != NULL);
    const TArray<TTown::TBuildingState, kNumBuildings>& aBuildingStates = pTown->getBuildingStates();
    if (aBuildingStates[hallBuilding].getBBuilt() && aBuildingStates[castleBuilding].getBBuilt()) {
        string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                        townRef.getBSecondLayer());
        const TTown::TBuildingTraits& hallTraits = pTown->getTownTypeTraits().m_akBuildingTraits[hallBuilding];
        const TTown::TBuildingTraits& castleTraits = pTown->getTownTypeTraits().m_akBuildingTraits[castleBuilding];
        char buf[512];
        snprintf(buf, sizeof(buf), kSVCUpgradeTownBuildingsAlreadyBuiltFmtStr, hallTraits.m_pName,
                 castleTraits.m_pName, townStr.c_str());
        buf[sizeof(buf) - 1] = '\0';
        _addNote(buf);
    } else if (pTown->getBIsBuildingDisabled(hallBuilding) || pTown->getBIsBuildingDisabled(castleBuilding)) {
        string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                        townRef.getBSecondLayer());
        const TTown::TBuildingTraits& hallTraits = pTown->getTownTypeTraits().m_akBuildingTraits[hallBuilding];
        const TTown::TBuildingTraits& castleTraits = pTown->getTownTypeTraits().m_akBuildingTraits[castleBuilding];
        char buf[512];
        snprintf(buf, sizeof(buf), kSVCUpgradeTownBuildingsDisabledFmtStr, hallTraits.m_pName, castleTraits.m_pName,
                 townStr.c_str());
        buf[sizeof(buf) - 1] = '\0';
        _addNote(buf);
    }
}

void TMapValidationFunc::visit(const TVCBuildHolyGrailStruct& vc)
{
    const TMapObjectRef& townRef = vc.getTownRef();
    if (townRef != TMapObjectRef()) {
        const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(townRef));
#line 959
        assert(pTown != NULL);
        if (pTown->getBuildingStates()[eBuildingGrail].getBBuilt()) {
            string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                            townRef.getBSecondLayer());
            char buf[512];
            snprintf(buf, sizeof(buf), kSVCBuildGrailStructAlreadyBuiltFmtStr, townStr.c_str());
            buf[sizeof(buf) - 1] = '\0';
            _addNote(buf);
        } else if (pTown->getBuildingStates()[eBuildingGrail].getBDisabled()) {
            string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                            townRef.getBSecondLayer());
            char buf[512];
            snprintf(buf, sizeof(buf), kSVCBuildGrailStructDisabledFmtStr, townStr.c_str());
            buf[sizeof(buf) - 1] = '\0';
            _addNote(buf);
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

void TMapValidationFunc::visit(const TVCDefeatHero& vc)
{
    const TMapObjectRef& heroRef = vc.getHeroRef();
    const THero* pHero = dynamic_cast<const THero*>(_m_map.getPObject(heroRef));
    if (pHero == NULL) {
        const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(heroRef));
#line 1032
        assert(pTown != NULL && pTown->getPVisitingHero() != NULL);
        pHero = pTown->getPVisitingHero();
    }
#line 1035
    assert(pHero != NULL);
    if (pHero->getOwner() != ePlayerNone && _playerMayBeHuman(pHero->getOwner())) {
        string heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                        heroRef.getBSecondLayer());
        char buf[512];
        snprintf(buf, sizeof(buf), kSVCDefeatHeroBelongsToHumanFmtStr, heroStr.c_str(),
                 akPlayerTraits[pHero->getOwner()].m_pName);
        _addNote(buf);
    }
}

void TMapValidationFunc::visit(const TVCCaptureTown& vc)
{
    const TMapObjectRef& townRef = vc.getTownRef();
    const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(townRef));
#line 1063
    assert(pTown != NULL);
    if (pTown->getOwner() != ePlayerNone) {
        if (vc.getBAppliesToComputer()) {
            string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                            townRef.getBSecondLayer());
            char buf[512];
            snprintf(buf, sizeof(buf), kSVCCaptureTownAlreadyOwnedFmtStr, townStr.c_str(),
                     akPlayerTraits[pTown->getOwner()].m_pName);
            _addNote(buf);
        } else if (_playerMayBeHuman(pTown->getOwner())) {
            string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                            townRef.getBSecondLayer());
            char buf[512];
            snprintf(buf, sizeof(buf), kSVCCaptureTownOwnedByHumanFmtStr, townStr.c_str(),
                     akPlayerTraits[pTown->getOwner()].m_pName);
            _addNote(buf);
        }
    }
}

void TMapValidationFunc::visit(const TVCDefeatMonster& vc)
{
}

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
                if (aNumTeamOwned[team] != 0) {
                    if (aNumTeamOwned[team] == numGenerators && (vc.getBAppliesToComputer() || abTeamMayBeHuman[team])) {
                        char buf[512];
                        snprintf(buf, sizeof(buf), kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr,
                                 _createTeamStr(team).c_str());
                        _addNote(buf);
                    }
                    break;
                }
            }
        } else {
            for (unsigned int player = 0; player < kNumPlayers; ++player) {
                if (aNumOwned[player] != 0) {
                    if (aNumOwned[player] == numGenerators && _m_map.isPlayerPresent(TPlayer(player))
                        && (vc.getBAppliesToComputer() || _m_map.getPlayers()[player].getBHumanPlayable())) {
                        char buf[512];
                        snprintf(buf, sizeof(buf), kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr,
                                 akPlayerTraits[player].m_pName);
                        _addNote(buf);
                    }
                    break;
                }
            }
        }
    }
}

void TMapValidationFunc::visit(const TVCFlagAllMines& vc)
{
    unsigned int numMines = 0;
    TArray<unsigned int, kNumPlayers> aNumOwned(0);
    unsigned int numLayers = _m_map.isTwoLayer() ? 2 : 1;
    for (unsigned int i = 0; i < numLayers; ++i) {
        const TGameMap::TLayer& layer = _m_map.getLayer(i);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TMine* pMine = dynamic_cast<const TMine*>(layer.getPObject(*iter));
            if (pMine != NULL && pMine->getType() == MINE) {
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
                if (aNumTeamOwned[team] != 0) {
                    if (aNumTeamOwned[team] == numMines && (vc.getBAppliesToComputer() || abTeamMayBeHuman[team])) {
                        char buf[512];
                        snprintf(buf, sizeof(buf), kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr,
                                 _createTeamStr(team).c_str());
                        _addNote(buf);
                    }
                    break;
                }
            }
        } else {
            for (unsigned int player = 0; player < kNumPlayers; ++player) {
                if (aNumOwned[player] != 0) {
                    if (aNumOwned[player] == numMines && _m_map.isPlayerPresent(TPlayer(player))
                        && (vc.getBAppliesToComputer() || _m_map.getPlayers()[player].getBHumanPlayable())) {
                        char buf[512];
                        snprintf(buf, sizeof(buf), kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr,
                                 akPlayerTraits[player].m_pName);
                        _addNote(buf);
                    }
                    break;
                }
            }
        }
    }
}

void TMapValidationFunc::visit(const TVCTransportArtifact& vc)
{
    if (!_isArtifactOnMap(vc.getArtifact())) {
        char buf[512];
        snprintf(buf, sizeof(buf), kSVCTransportArtifactNotPresentFmtStr, akArtifactTraits[vc.getArtifact()].m_name);
        _addNote(buf);
    }
}

void TMapValidationFunc::visit(const TLCLoseTown& lc)
{
    const TMapObjectRef& townRef = lc.getTownRef();
    const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(townRef));
#line 1332
    assert(pTown != NULL);
    if (pTown->getOwner() == ePlayerNone) {
        string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                        townRef.getBSecondLayer());
        char buf[512];
        snprintf(buf, sizeof(buf), kSLCLoseTownNotOwnedFmtStr, townStr.c_str());
        _addNote(buf);
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
#line 1367
            assert(abTeamMayBeHuman.count() > 0);
            team = teamInfo.getPlayerTeam(pTown->getOwner());
            if (!abTeamMayBeHuman[team]) {
                string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                                townRef.getBSecondLayer());
                char buf[512];
                snprintf(buf, sizeof(buf), kSLCLoseTownTeamNotHumanFmtStr, townStr.c_str(), _createTeamStr(team).c_str());
                _addNote(buf);
            } else if (abTeamMayBeHuman.count() > 1) {
                string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                                townRef.getBSecondLayer());
                char buf[512];
                snprintf(buf, sizeof(buf), kSLCLoseTownMultipleHumanTeamsFmtStr, townStr.c_str());
                _addNote(buf);
            }
        } else if (!_m_map.getPlayers()[pTown->getOwner()].getBHumanPlayable()) {
            string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                            townRef.getBSecondLayer());
            char buf[512];
            snprintf(buf, sizeof(buf), kSLCLoseTownPlayerNotHumanFmtStr, townStr.c_str(),
                     akPlayerTraits[pTown->getOwner()].m_pName);
            _addNote(buf);
        } else {
            const TArray<TPlayerInfo, kNumPlayers>& aPlayers = _m_map.getPlayers();
            unsigned int playerNum = 0;
            while (!aPlayers[playerNum].getBHumanPlayable()) {
                ++playerNum;
#line 1426
                assert(playerNum < kNumPlayers);
            }
            for (++playerNum; playerNum < kNumPlayers; ++playerNum) {
                if (aPlayers[playerNum].getBHumanPlayable()) {
                    string townStr = _createTownStr(*pTown, _m_map.getObjectLoc(townRef) - pTown->getTriggerLoc(),
                                                    townRef.getBSecondLayer());
                    char buf[512];
                    snprintf(buf, sizeof(buf), kSLCLoseTownMultipleHumanPlayersFmtStr, townStr.c_str());
                    _addNote(buf);
                    break;
                }
            }
        }
    }
}

void TMapValidationFunc::visit(const TLCLoseHero& lc)
{
    const TMapObjectRef& heroRef = lc.getHeroRef();
    const THero* pHero = dynamic_cast<const THero*>(_m_map.getPObject(heroRef));
    if (pHero == NULL) {
        const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(heroRef));
#line 1461
        assert(pTown != NULL);
        pHero = pTown->getPVisitingHero();
    }
#line 1464
    assert(pHero != NULL);
    if (pHero->getOwner() == ePlayerNone) {
        string heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                        heroRef.getBSecondLayer());
        char buf[512];
        snprintf(buf, sizeof(buf), kSLCLoseHeroNotOwnedFmtStr, heroStr.c_str());
        _addNote(buf);
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
#line 1502
            assert(abTeamMayBeHuman.count() > 0);
            team = teamInfo.getPlayerTeam(pHero->getOwner());
            if (!abTeamMayBeHuman[team]) {
                string heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                                heroRef.getBSecondLayer());
                char buf[512];
                snprintf(buf, sizeof(buf), kSLCLoseHeroTeamNotHumanFmtStr, heroStr.c_str(), _createTeamStr(team).c_str());
                _addNote(buf);
            } else if (abTeamMayBeHuman.count() > 1) {
                string heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                                heroRef.getBSecondLayer());
                char buf[512];
                snprintf(buf, sizeof(buf), kSLCLoseHeroMultipleHumanTeamsFmtStr, heroStr.c_str());
                _addNote(buf);
            }
        } else if (!_m_map.getPlayers()[pHero->getOwner()].getBHumanPlayable()) {
            string heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                            heroRef.getBSecondLayer());
            char buf[512];
            snprintf(buf, sizeof(buf), kSLCLoseHeroPlayerNotHumanFmtStr, heroStr.c_str(),
                     akPlayerTraits[pHero->getOwner()].m_pName);
            _addNote(buf);
        } else {
            const TArray<TPlayerInfo, kNumPlayers>& aPlayers = _m_map.getPlayers();
            unsigned int playerNum = 0;
            while (!aPlayers[playerNum].getBHumanPlayable()) {
                ++playerNum;
#line 1570
                assert(playerNum < kNumPlayers);
            }
            for (++playerNum; playerNum < kNumPlayers; ++playerNum) {
                if (aPlayers[playerNum].getBHumanPlayable()) {
                    string heroStr = _createHeroStr(*pHero, _m_map.getObjectLoc(heroRef) - _m_map.getPObject(heroRef)->getTriggerLoc(),
                                                    heroRef.getBSecondLayer());
                    char buf[512];
                    snprintf(buf, sizeof(buf), kSLCLoseHeroMultipleHumanPlayersFmtStr, heroStr.c_str());
                    _addNote(buf);
                    break;
                }
            }
        }
    }
}

void TMapValidationFunc::visit(const TLCTimeExpires& lc)
{
}
