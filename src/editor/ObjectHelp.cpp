// ObjectHelp.cpp - Loki h3maped object 20: displayObjectHelp. The editor
// picks the object's help topic: a block per object type with subtypes
// (artifacts, creature banks and generators, garrisons, heroes, mines,
// monsters, resources, towns) indexed by the subtype, otherwise the
// type's own topic. The Loki port has no help viewer: it prints a notice
// and exits. The topic numbers are the retail immediates; their names are
// not recorded.
#include "editor/stdafx.h"

#include <stdio.h>
#include <stdlib.h>

#include "adventureobjecttype.h"
#include "objecttype.h"
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

void displayObjectHelp(const TObjectType& objType)
{
    unsigned int helpID;
    switch (objType.getType()) {
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
    default:
        helpID = kObjectTypeHelpBase + objType.getType();
        break;
    }
    printf("This needs to print help!");
    exit(0);
}
