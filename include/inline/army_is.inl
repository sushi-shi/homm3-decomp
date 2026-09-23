    // Original: army::Is; E:\gamedcs\Army.h:765, dc 0x27ce4.
    // Any requested attribute suffices, including a combined trait mask.
inline bool army::is(unsigned attribute) const
    {
        return (m_monInfo.m_attributes & attribute) != 0;
    }
