// CodeView ai_creature_value.h owns this value type and its comparisons.
#ifndef HOMM3_AI_CREATURE_VALUE_H
#define HOMM3_AI_CREATURE_VALUE_H

#include "armygrp.h"

// Dreamcast records this exact 12-byte sort key; retail calculate_reserve
// copies it three dwords at a time and compares the value at +4.
// The DC decorated comparison publics encode bool (_N), despite their
// unsigned-byte debug storage records (ai_creature_value.h:29/35).
struct type_creature_value {
    TCreatureType m_type;
    long m_value;
    short m_amount;

    bool operator<(const type_creature_value& arg) const
    {
        return m_value < arg.m_value;
    }
    bool operator>(const type_creature_value& arg) const
    {
        return m_value > arg.m_value;
    }
};

#endif
