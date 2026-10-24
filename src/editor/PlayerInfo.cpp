// PlayerInfo.cpp - retained player setters (h3maped 0x4942c6..0x494308).
// The preceding 0x4942aa initializer is the standard library ctype id,
// as in TeamInfo.cpp. The original Windows file name is not recorded.
#include <bitset>
#include <string>

using namespace std;

#include "va.h"
#include "editor/GameMap.h"

// Retail folds this offset-four scalar store onto type_progress_bar::setTotal.
void TPlayerInfo::setBehaviorType(TBehaviorType newBehaviorType)
{
    _m_behaviorType = newBehaviorType;
}

VA(0x004942c6, 0x15)
void TPlayerInfo::setMainTown(const TMapObjectRef& mainTownRef)
{
    _m_bHasMainTown = true;
    _m_mainTownRef = mainTownRef;
}

VA(0x004942db, 0x18)
void TPlayerInfo::clearMainTown()
{
    _m_mainTownRef = TMapObjectRef();
    _m_bHasMainTown = false;
    _m_bGenerateHero = false;
}

VA(0x004942f3, 0x15)
void TPlayerInfo::setBGenerateHero(bool bGenerateHero)
{
    _m_bGenerateHero = bGenerateHero;
}
