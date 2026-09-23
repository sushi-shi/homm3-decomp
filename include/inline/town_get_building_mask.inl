    // DC Town.h:299/300 returns full_building_mask (+0x150 in DC).
    // Retail's +0x158 band is m_active; getBuildableMask expands this read.
    __int64 getBuildingMask() const { return m_active; }
