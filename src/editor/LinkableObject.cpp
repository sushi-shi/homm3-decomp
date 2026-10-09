// LinkableObject.cpp - the map objects that carry a link id (h3maped
// 0x458f90..0x4592b8, name inferred; Armageddon's Blade on): heroes,
// towns and monsters. Each new object takes the next id of a running
// counter, skipping the no-link id; maps from version 15 store it.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/ObjectSpecializations.h"
#include "editor/RawStream.h"

DATA(0x00538a8c) const unsigned int TLinkableObject::s_kNoLinkID = 0;
DATA(0x0059fc84) unsigned int TLinkableObject::s_nextLinkID;

VA(0x00459164, 0x71)
TLinkableObject::TLinkableObject(const TObjectType& objType)
    : TGameObject(objType)
{
    assignNewLinkID();
}

VA(0x004591d5, 0xa9)
TLinkableObject::TLinkableObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
    assignNewLinkID();
    read(pIStream, version);
}

VA(0x0045927e, 0x17)
void TLinkableObject::write(TRawOStream* pOStream, int version) const
{
    if (version >= 1)
        *pOStream << _m_linkID;
}

VA(0x00459295, 0x23)
void TLinkableObject::read(TRawIStream* pIStream, int version)
{
    if (version >= 15) {
        unsigned int linkID;
        *pIStream >> linkID;
        _m_linkID = linkID;
    }
}
