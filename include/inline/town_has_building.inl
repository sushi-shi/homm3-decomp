    VA(0x004305a0, 0x66)  // hd-crossbuild + exact body/callers x18, dc 0x1fe14
    bool hasBuilding(int buildingId, bool checkIncluded) const
    {
        if (checkIncluded) {
            return (m_active & g_bitNumber[buildingId]) != 0;
        } else {
            return (m_built & g_bitNumber[buildingId]) != 0;
        }
    }
