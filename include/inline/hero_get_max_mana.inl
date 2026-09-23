    int getMaxMana() const
    {
        return static_cast<int>(
            getPrimarySkill(3) * 10 * getIntelligenceFactor());
    }
