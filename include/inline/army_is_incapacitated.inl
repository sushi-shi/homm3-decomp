VA(0x0041f380, 0x27)  // anchor-callee, dc 0x27d9c
inline bool army::isIncapacitated() const
    {
        return m_spellInfluence[62] || m_spellInfluence[70]
               || m_spellInfluence[74];
    }
