    // E:\gamedcs\Hero.h:718, dc 0x2308c
    TSkillMastery getSpellLevel(SpellID spell) const
    {
        return getSpellLevel(spell, getSpecialTerrain());
    }
