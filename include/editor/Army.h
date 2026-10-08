// Army.h - a creature stack and the seven-stack army (Army.cpp; Loki
// h3maped). TCreatureStack is a creature type and a quantity; TArmy is the
// TArray of seven stacks (a town's garrison copies its eight-byte elements,
// h3maped 0x4c1b6f).
#ifndef HOMM3_EDITOR_ARMY_H
#define HOMM3_EDITOR_ARMY_H

#include "armygrp.h"
#include "editor/Array.h"

class TCreatureStack {
public:
    TCreatureType getCreatureType() const { return _m_creatureType; }
    unsigned int getQuantity() const { return _m_quantity; }

    friend bool operator==(const TCreatureStack& lhs, const TCreatureStack& rhs)
    {
        return lhs._m_creatureType == rhs._m_creatureType && lhs._m_quantity == rhs._m_quantity;
    }

private:
    TCreatureType _m_creatureType;
    unsigned int _m_quantity;
};

class TArmy : public TArray<TCreatureStack, 7> {
};

#endif  /* HOMM3_EDITOR_ARMY_H */
