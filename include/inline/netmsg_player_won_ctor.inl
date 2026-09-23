    // Dreamcast netmsg.h:505 fixes the reference parameter and statement
    // order. Complete expands this constructor into DisplayVCWinLoss while
    // retaining or expanding the CNetMsg base constructor per call site.
    CPlayerWonMsg(int gamePos,
                  VictoryConditionStruct& victoryConditionStruct)
      : CNetMsg(RS_PLAYER_WON, sizeof(CPlayerWonMsg))
    {
        this->m_gamePos = gamePos;
        m_victoryCondition = victoryConditionStruct;
    }
