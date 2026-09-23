    // DC header inline (cmbtmgr.h:1525, dc 0x27f64). mark_teleport's
    // retail expansion retains the ValidHex bounds checks and the two
    // invisible edge columns, 0 and 16 of each 17-cell row.
    static bool inInvisibleColumn(int index)
    {
        if (!validHex(index))
            return false;
        int column = gridX(index);
        return column == 0 || column == COMBAT_GRID_LAST_COLUMN;
    }
