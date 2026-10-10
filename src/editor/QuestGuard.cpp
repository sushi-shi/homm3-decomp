// QuestGuard.cpp - a Quest Guard (h3maped 0x495ae9..0x495de9; Complete
// only): a quest location with nothing of its own.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/QuestGuard.h"

VA(0x00495cf7, 0x76)
TQuestGuard::TQuestGuard(const TObjectType& objType) : TGameObject(objType), TQuestLocation(objType)
{
}

VA(0x00495d6d, 0x7c)
TQuestGuard::TQuestGuard(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TQuestLocation(objType, pIStream, version)
{
}
