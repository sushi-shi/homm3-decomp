    VA(0x004317d0, 0x26)  // hd-crossbuild + exact body/callers x15, dc 0x2eb0
    hero* getHero(int which)
    {
        if (which == -1)
            return 0;
        return m_heroes + which;
    }
