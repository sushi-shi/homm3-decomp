    // E:\gamedcs\Town.h:342. Public ?IsCapitol@town@@QBA_NXZ likewise
    // proves native bool. Both declarations are byte-flat in all consumers.
    bool isCapitol() const
    {
        return hasBuilding(HALL_CAPITOL_ID, 0);
    }
