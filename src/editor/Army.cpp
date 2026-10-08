// Army.cpp - Loki h3maped object 4: the creature stack's validated setters
// and the binary form of a stack (type as a signed char, quantity as a
// short) and of an army (its seven stacks in order).
#include "editor/stdafx.h"

#include <assert.h>
#include <function.h>

#include "editor/Army.h"
#include "editor/RawStream.h"

void TCreatureStack::setCreatureType(TCreatureType newCreatureType)
{
#line 24
    assert(newCreatureType >= eCreatureNone && newCreatureType < kNumCreatureTypes);
    _m_creatureType = newCreatureType;
}

void TCreatureStack::setQuantity(unsigned int newQuantity)
{
#line 32
    assert(newQuantity <= s_kMaxQuantity);
    _m_quantity = newQuantity;
}

TRawOStream& operator<<(TRawOStream& stream, const TCreatureStack& stack)
{
    stream << static_cast<signed char>(stack.getCreatureType())
           << static_cast<short>(stack.getQuantity());
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TCreatureStack& stack)
{
    signed char creatureType;
    short quantity;
    stream >> creatureType >> quantity;
    stack.setCreatureType(TCreatureType(creatureType));
    stack.setQuantity(quantity);
    return stream;
}

TRawOStream& operator<<(TRawOStream& stream, const TArmy& army)
{
    writeFromIter(stream, army.begin(), army.end());
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TArmy& army)
{
    readToIter(stream, army.begin(), army.end());
    return stream;
}
