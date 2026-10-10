// Army.h - a creature stack and the seven-stack army (Army.cpp; Loki
// h3maped). TCreatureStack is a creature type and a quantity (none and 0
// when empty); TArmy is the TArray of seven stacks (a town's garrison copies
// its eight-byte elements, h3maped 0x4c1b6f).
//
// The Windows editors read and write a stack by the map's version: from
// map version 20 the creature type is a short, before it a byte whose 0xff
// is no creature; the writers take the edition (a short from Armageddon's
// Blade on).
#ifndef HOMM3_EDITOR_ARMY_H
#define HOMM3_EDITOR_ARMY_H

#include "armygrp.h"
#include "editor/Array.h"

class TRawIStream;
class TRawOStream;

class TCreatureStack {
public:
    enum { s_kMaxQuantity = 9999 };

    // An empty stack (the army's constructor 0x40234e fills its slots).
    TCreatureStack() : _m_creatureType(CREATURE_NONE), _m_quantity(0) {}
    // A stack as the map stores it (a seer's creature reward reads one
    // straight into its new reward, h3maped 0x4b66a7).
    TCreatureStack(TRawIStream* pIStream, int version) { read(pIStream, version); }

    // The army editor's stores (h3maped 0x406891, and 0x499e5b where the
    // quantity setter folds).
    void setCreatureType(TCreatureType newCreatureType);
    void setQuantity(unsigned int newQuantity);

    TCreatureType getCreatureType() const { return _m_creatureType; }
    unsigned int getQuantity() const { return _m_quantity; }

    void read(TRawIStream* pIStream, int version);
    void write(TRawOStream* pOStream, int version) const;

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

class TArmy : public TArray<TCreatureStack, 7> {
public:
    TArmy() {}
    TArmy(TRawIStream* pIStream, int version);

    void write(TRawOStream* pOStream, int version) const;
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
