// ObjectSprites.cpp - Loki h3maped object 22: the shared object-type and
// hero-flag sprite tables behind TObjectSpritePtr and THeroFlagSpritePtr.
// Sprites load on first lock and stay loaded; unlocking only checks the
// table. The tables are released by a function-local TInitializer that
// initializeObjectSprites registers for exit.
#include "editor/stdafx.h"

#include <assert.h>
#include <new>
#include <string>
#include <vector>

#include "exceptions.h"
#include "objecttype.h"
#include "resourcemanager.h"
#include "editor/Array.h"
#include "editor/ObjectSprites.h"

namespace {

vector<const CSprite*> apObjectSprite;
TArray<const CSprite*, kNumPlayers> apHeroFlagSprite(NULL);

class TInitializer {
public:
    ~TInitializer();
};

TInitializer::~TInitializer()
{
    for (TArray<const CSprite*, kNumPlayers>::iterator ppFlag = apHeroFlagSprite.begin();
         ppFlag != apHeroFlagSprite.end(); ++ppFlag) {
        if (*ppFlag != NULL) {
            ResourceManager::Dispose(const_cast<CSprite*>(*ppFlag));
            *ppFlag = NULL;
        }
    }
    for (vector<const CSprite*>::iterator ppSprite = apObjectSprite.begin();
         ppSprite != apObjectSprite.end(); ++ppSprite) {
        if (*ppSprite != NULL)
            ResourceManager::Dispose(const_cast<CSprite*>(*ppSprite));
    }
    apObjectSprite.~vector<const CSprite*>();
    new (&apObjectSprite) vector<const CSprite*>();
}

}  // namespace

void initializeObjectSprites()
{
    static TInitializer initializer;
}

const CSprite* TObjectSpritePtr::_lockSprite(const TObjectType& objType)
{
    unsigned int imageNum = objType.getImageNum();
    if (imageNum >= apObjectSprite.size())
        apObjectSprite.resize(imageNum + 1, NULL);
    if (apObjectSprite[imageNum] == NULL) {
        apObjectSprite[imageNum] = ResourceManager::GetSprite(objType.getImageName().c_str());
        if (apObjectSprite[imageNum] == NULL)
#line 95
            throw TRuntimeError(__FILE__, __LINE__,
                                string("Unable to load object sprite:  \"") + objType.getImageName() + "\".");
    }
    return apObjectSprite[imageNum];
}

void TObjectSpritePtr::_unlockSprite(const TObjectType& objType)
{
#line 104
    assert(objType.getImageNum() < apObjectSprite.size());
    assert(apObjectSprite[ objType.getImageNum() ] != __null);
}

const CSprite* THeroFlagSpritePtr::_lockSprite(TPlayer player)
{
    static const char* const akFlagSpriteNames[kNumPlayers] = {
        "af00e.def", "af01e.def", "af02e.def", "af03e.def",
        "af04e.def", "af05e.def", "af06e.def", "af07e.def"
    };

#line 111
    assert(player >= 0 && player < kNumPlayers);
    if (apHeroFlagSprite[player] == NULL) {
        apHeroFlagSprite[player] = ResourceManager::GetSprite(akFlagSpriteNames[player]);
        if (apHeroFlagSprite[player] == NULL)
#line 129
            throw TRuntimeError(__FILE__, __LINE__,
                                string("Unable to load flag sprite:  \"") + akFlagSpriteNames[player] + "\".");
    }
    return apHeroFlagSprite[player];
}

void THeroFlagSpritePtr::_unlockSprite(TPlayer player)
{
#line 138
    assert(player >= 0 && player < kNumPlayers);
    assert(apHeroFlagSprite[ player ] != __null);
}
