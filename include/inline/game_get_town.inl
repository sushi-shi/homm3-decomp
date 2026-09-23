    VA(0x0042ba30, 0x24)  // hd-crossbuild + exact body/callers x5, dc 0x2f24
    town* getTown(int townId)
    {
        if (townId == -1)
            return 0;
        return &m_towns[townId];
    }
