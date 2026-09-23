    // SpellID is still represented by its retail-width int domain here.
    // E:\gamedcs\Army.h:820
inline long army::getSpellTime(int spell) const
    {
        return m_spellInfluence[spell];
    }
