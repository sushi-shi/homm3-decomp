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
    enum { s_kMaxQuantity = 9999 };

    // An empty stack (the army's constructor 0x40234e fills its slots).
    TCreatureStack() : _m_creatureType(CREATURE_NONE), _m_quantity(0) {}

    // The army editor's stores (h3maped 0x406891, and 0x499e5b where the
    // quantity setter folds).
    void setCreatureType(TCreatureType newCreatureType);
    void setQuantity(unsigned int newQuantity);

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

// The random creatures a stack may hold (crgenerc.txt, read by an
// unnamed object at h3maped 0x458605): name and creature type.
struct TRandomCreatureTraits {
    const char* m_name;
    TCreatureType m_creatureType;
};

enum { kNumRandomCreatureTypes = 14 };

// h3maped 0x58b170: points at the rows.
extern const TRandomCreatureTraits* akRandomCreatureTraits;

#endif  /* HOMM3_EDITOR_ARMY_H */
