    // DC-attested inline helper; SetShrineHelpText proves the direct
    // byte-indexed availability read in retail.
    unsigned char spellIsAvailable(int spell) const
    {
        return m_availableSpells[spell];
    }
