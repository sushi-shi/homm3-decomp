// Army.h - a creature stack and the seven-stack army (Loki h3maped Army.cpp).
// TCreatureStack is a creature type and a quantity (eCreatureNone/0 when
// empty; the inline constructor stores -1 and 0). TArmy is the TArray of
// seven stacks: its users index it (`_m_army[ stackNum ]`) and call the
// TArray iterators and assignment on it directly.
#ifndef HOMM3_EDITOR_ARMY_H
#define HOMM3_EDITOR_ARMY_H

#include "armygrp.h"
#include "editor/Array.h"

class TRawIStream;
class TRawOStream;

class TCreatureStack {
public:
    static const unsigned int s_kMaxQuantity = 9999;

    TCreatureStack() : _m_creatureType(eCreatureNone), _m_quantity(0) {}

    TCreatureType getCreatureType() const { return _m_creatureType; }
    void setCreatureType(TCreatureType newCreatureType);
    unsigned int getQuantity() const { return _m_quantity; }
    void setQuantity(unsigned int newQuantity);

    friend bool operator==(const TCreatureStack& lhs, const TCreatureStack& rhs)
    {
        return lhs._m_creatureType == rhs._m_creatureType && lhs._m_quantity == rhs._m_quantity;
    }

private:
    TCreatureType _m_creatureType;
    unsigned int _m_quantity;
};

inline bool operator!=(const TCreatureStack& lhs, const TCreatureStack& rhs)
{
    return !(lhs == rhs);
}

TRawOStream& operator<<(TRawOStream& stream, const TCreatureStack& stack);
TRawIStream& operator>>(TRawIStream& stream, TCreatureStack& stack);

class TArmy : public TArray<TCreatureStack, 7> {
};

TRawOStream& operator<<(TRawOStream& stream, const TArmy& army);
TRawIStream& operator>>(TRawIStream& stream, TArmy& army);

#endif  /* HOMM3_EDITOR_ARMY_H */
