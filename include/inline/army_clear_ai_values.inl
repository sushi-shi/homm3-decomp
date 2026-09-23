    // E:\gamedcs\Army.h:752, dc 0x27ccc. The older DC helper clears
    // expectedDamage, target, time and value. Complete's x86 findAITargets
    // at 0x422b20 and Mac at 0:0x25100 instead clear target, value,
    // possibleTargets and time in this order, leaving expectedDamage intact.
inline void army::clearAIValues()
    {
        m_aiTarget = 0;
        m_aiTargetValue = 0;
        m_aiPossibleTargets = 0;
        m_aiTargetTime = 0;
    }
