    // E:\gamedcs\Hero.h:707, dc 0x23058
    int getManaCost(int whichSpell) const
    {
        return getManaCost(
            whichSpell, 0,
            getSpecialTerrain());
    }
