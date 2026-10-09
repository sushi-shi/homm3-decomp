// ObjectHelp.cpp - context help for an object type (h3maped
// 0x48b8ef..0x48bbbe; Loki h3maped object 20). An object type with
// subtypes (artifacts, creature banks and generators, garrisons, heroes,
// mines, monsters, resources, towns and the random dwellings) has a block
// of topics indexed by its subtype; any other type has its own topic. The
// type is the one whose objnames.txt row names it, so an alternate type
// shares its original's topics. The topic numbers are the retail
// immediates; their names are not recorded.
#include "editor/stdafx.h"

#include "va.h"
#include "objnames.h"
#include "objecttype.h"
#include "retailobjecttype.h"
#include "editor/ObjectHelp.h"

const unsigned int kObjectTypeHelpBase = 0x30000;
const unsigned int kArtifactHelpBase = 0x30400;
const unsigned int kCreatureBankHelpBase = 0x30800;
const unsigned int kCreatureGenerator1HelpBase = 0x30a00;
const unsigned int kCreatureGenerator4HelpBase = 0x30c00;
const unsigned int kGarrisonHelpBase = 0x31000;
const unsigned int kHeroHelpBase = 0x31200;
const unsigned int kMineHelpBase = 0x31400;
const unsigned int kMonsterHelpBase = 0x31600;
const unsigned int kResourceHelpBase = 0x31a00;
const unsigned int kTownHelpBase = 0x31c00;
const unsigned int kRandomDwellingLvlHelpBase = 0x31e00;
const unsigned int kRandomDwellingFactionHelpBase = 0x32000;

VA(0x0048bac3, 0xfb)
void displayObjectHelp(const TObjectType& objType)
{
    unsigned int helpID;
    int type = akAdvObjectTypeTraits[objType.getType()].m_nameRow;
    switch (type) {
    case ARTIFACT:
        helpID = kArtifactHelpBase + objType.getExtra();
        break;
    case CREATURE_BANK:
        helpID = kCreatureBankHelpBase + objType.getExtra();
        break;
    case CREATURE_GENERATOR_1:
        helpID = kCreatureGenerator1HelpBase + objType.getExtra();
        break;
    case CREATURE_GENERATOR_4:
        helpID = kCreatureGenerator4HelpBase + objType.getExtra();
        break;
    case GARRISON:
        helpID = kGarrisonHelpBase + objType.getExtra();
        break;
    case HERO:
        helpID = kHeroHelpBase + objType.getExtra();
        break;
    case MINE:
        helpID = kMineHelpBase + objType.getExtra();
        break;
    case MONSTER:
        helpID = kMonsterHelpBase + objType.getExtra();
        break;
    case RESOURCE:
        helpID = kResourceHelpBase + objType.getExtra();
        break;
    case TOWN:
        helpID = kTownHelpBase + objType.getExtra();
        break;
    case RANDOM_DWELLING_LVL:
        helpID = kRandomDwellingLvlHelpBase + objType.getExtra();
        break;
    case RANDOM_DWELLING_FACTION:
        helpID = kRandomDwellingFactionHelpBase + objType.getExtra();
        break;
    default:
        helpID = kObjectTypeHelpBase + type;
        break;
    }
    AfxGetApp()->WinHelp(helpID, HELP_CONTEXTPOPUP);
}
