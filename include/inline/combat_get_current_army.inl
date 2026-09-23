    // DC header inline (cmbtmgr.h:1478, dc 0x27efc); the DC xref graph
    // lists it among DoCompAI's callees and retail carries no
    // out-of-line copy, so it is the /Ob2 inline-away case.
    army* getCurrentArmy() { return &m_armies[m_actingSide][m_actingSlot]; }
