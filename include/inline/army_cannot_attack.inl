    // E:\gamedcs\Army.h:855
// Complete's inlined copy in consider_single_enchantment keeps the recovered
// incapacity/attribute prefix but directly contradicts Dreamcast's final
// Psychic/Magic Elemental pair: retail compares First Aid Tent and Ammo Cart.
inline bool army::cannotAttack() const
    {
        return isIncapacitated() || is(creatureImmobilized)
               || m_creatureType == ARMY_CREATURE_FIRST_AID_TENT
               || m_creatureType == ARMY_CREATURE_AMMO_CART;
    }
