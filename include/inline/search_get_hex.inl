    // Retail 0x4b3b90 checks receiver+0x24 for null, then indexes the
    // 30-byte pathCell array and returns ret 4. FindCombatPath calls at
    // 0x4b382f/0x4b3881/0x4b393e/0x4b3990 correspond to the four
    // expansions of DC mark_enemy's get_hex call.
    // E:\gamedcs\FindPath.h:194, dc 0x27fe8
    VA(0x004b3b90, 0x20)  // caller/get_hex correlation, dc 0x27fe8
    pathCell* getHex(long x) const
    {
        if (m_cellData == 0)
            return 0;
        return &m_cellData[x];
    }
