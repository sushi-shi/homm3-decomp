// E:\gamedcs\CreatureType.h:296. Complete retains the army.obj copy;
// events.cpp also expands this at monsters_flee/join/sell_out, passing a
// literal count so each singular/plural selection folds at its call site.
VA(0x00440100, 0x3E)  // two-register /Gr ABI + trait lookup, dc 0x1ef94
inline const char* getArmyName(int type, int count)
{
    if (type < 0 || type > g_creatureTypeLast) {
        return DATA_COMPGEN(0x00691210, emptyCreatureName, "");
    } else {
        return count == 1 ? g_creatureTypeTraits[type].m_name
                          : g_creatureTypeTraits[type].m_pluralName;
    }
}
