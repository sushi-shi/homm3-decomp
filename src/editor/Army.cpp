// Army.cpp - a creature stack's and an army's binary form (h3maped
// 0x402243..0x40237c; Loki h3maped object 4). Release drops Loki's range
// asserts, so the readers store the fields directly.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Army.h"
#include "editor/RawStream.h"

VA(0x00402243, 0x52)
void TCreatureStack::read(TRawIStream* pIStream, int version)
{
    int creatureType;
    if (version >= 20) {
        short type;
        *pIStream >> type;
        creatureType = type;
    } else {
        unsigned char type;
        *pIStream >> type;
        creatureType = type;
        if (creatureType == 0xff)
            creatureType = CREATURE_NONE;
    }
    short quantity;
    *pIStream >> quantity;
    _m_creatureType = TCreatureType(creatureType);
    _m_quantity = quantity;
}

VA(0x00402295, 0x49)
void TCreatureStack::write(TRawOStream* pOStream, int version) const
{
    if (version >= 1)
        *pOStream << static_cast<short>(_m_creatureType);
    else
        *pOStream << static_cast<unsigned char>(_m_creatureType);
    *pOStream << static_cast<short>(_m_quantity);
}

VA(0x004022de, 0x4c)
TArmy::TArmy(TRawIStream* pIStream, int version)
{
    for (iterator pStack = begin(); pStack != end(); ++pStack)
        pStack->read(pIStream, version);
}

VA(0x0040232a, 0x24)
void TArmy::write(TRawOStream* pOStream, int version) const
{
    for (const_iterator pStack = begin(); pStack != end(); ++pStack)
        pStack->write(pOStream, version);
}
