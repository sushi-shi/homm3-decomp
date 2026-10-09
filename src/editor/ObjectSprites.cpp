// ObjectSprites.cpp - the shared object-type and hero-flag sprite tables
// behind TObjectSpritePtr and THeroFlagSpritePtr (h3maped
// 0x48fa1b..0x48fdc9; Loki h3maped object 22). Sprites load on first lock
// and stay loaded; unlocking only checks the table. The tables are
// released by a function-local TInitializer that initializeObjectSprites
// registers for exit.
#include "editor/stdafx.h"

#include <assert.h>
#include <new>
#include <string>
#include <vector>

#include "va.h"
#include "csprite.h"
#include "exceptions.h"
#include "objecttype.h"
#include "resourcemanager.h"
#include "editor/Array.h"
#include "editor/ObjectSprites.h"

namespace {

DATA(0x005a2268) vector<const CSprite*> apObjectSprite;
DATA(0x005a2248) TArray<const CSprite*, kNumPlayers> apHeroFlagSprite(NULL);

class TInitializer {
public:
    ~TInitializer();
};

VA(0x0048fc51, 0x72)
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

VA(0x0048fcc3, 0x1c)
void initializeObjectSprites()
{
    DATA_COMPGEN_GUARD(0x005a2278, objectSpritesInitializerGuard, initializer)
    VA_COMPGEN(0x0048fcdf, 0xa, STATIC_DTOR, initializer)
    DATA(0x005a223c) static TInitializer initializer;
}

VA(0x0048fce9, 0x93)
const CSprite* TObjectSpritePtr::_lockSprite(const TObjectType& objType)
{
    unsigned int imageNum = objType.getImageNum();
    if (imageNum >= apObjectSprite.size())
        apObjectSprite.resize(imageNum + 1, NULL);
    if (apObjectSprite[imageNum] == NULL) {
        apObjectSprite[imageNum] = ResourceManager::GetSprite(objType.getImageName().c_str());
        if (apObjectSprite[imageNum] == NULL)
            throw TRuntimeError();
    }
    return apObjectSprite[imageNum];
}

void TObjectSpritePtr::_unlockSprite(const TObjectType& objType)
{
    assert(objType.getImageNum() < apObjectSprite.size());
    assert(apObjectSprite[objType.getImageNum()] != NULL);
}

VA(0x0048fd7c, 0x4d)
const CSprite* THeroFlagSpritePtr::_lockSprite(TPlayer player)
{
    assert(player >= 0 && player < kNumPlayers);
    DATA(0x0053f954) static const char* const akFlagSpriteNames[kNumPlayers] = {
        "af00e.def", "af01e.def", "af02e.def", "af03e.def",
        "af04e.def", "af05e.def", "af06e.def", "af07e.def"
    };
    if (apHeroFlagSprite[player] == NULL) {
        apHeroFlagSprite[player] = ResourceManager::GetSprite(akFlagSpriteNames[player]);
        if (apHeroFlagSprite[player] == NULL)
            throw TRuntimeError();
    }
    return apHeroFlagSprite[player];
}

void THeroFlagSpritePtr::_unlockSprite(TPlayer player)
{
    assert(player >= 0 && player < kNumPlayers);
    assert(apHeroFlagSprite[player] != NULL);
}
