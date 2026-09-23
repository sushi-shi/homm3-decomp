    // E:\gamedcs\Army.h:736
VA(0x00445cd0, 0x38)  // anchor-caller + exact header-inline body, dc 0x27c9c
inline int army::offsetToFront(int direction) const
    {
        if (direction >= 0 && direction <= 2)
            return 1;
        if (direction >= 3 && direction <= 5)
            return -1;
        return m_facing ? 1 : -1;
    }
