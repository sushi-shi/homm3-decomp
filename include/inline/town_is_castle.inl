    // E:\gamedcs\Town.h:337. Public ?IsCastle@town@@QBA_NXZ proves bool;
    // the DC T_UCHAR return record is lowered, as for hasBuilding.
    bool isCastle() const
    {
        return hasBuilding(CASTLE_FORT_ID, 0)
            || hasBuilding(CASTLE_CITADEL_ID, 0)
            || hasBuilding(CASTLE_CASTLE_ID, 0);
    }
