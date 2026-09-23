    // kb.obj's SendPlayerLost builds this at all three DisplayLCWinLoss
    // arms. The member's own default constructor runs before the body's
    // assignment - retail stores Type/-1, GameLost/0 and playerLoser/-1
    // into the frame copy and then overwrites all 36 bytes with the
    // rep movsd, which is what proves the two-statement body rather than
    // a member-initialiser.
    CPlayerLostMsg(int loser, LossConditionStruct& lossConditionStruct)
      : CNetMsg(RS_PLAYER_LOST, sizeof(CPlayerLostMsg))
    {
        this->m_loser = loser;
        m_lossCondition = lossConditionStruct;
    }
