    // DC header inline (cmbtmgr.h:1460, dc 0x27ec8, 18 B). Its S_PUB32
    // identity is ?ValidHex@combatManager@@SA_NH@Z: static bool. No retail
    // body; place_shooter (0x422060) carries two copies of it, one on
    // the loop index (which VC6 strength-reduces onto the same 30-byte
    // induction variable the cellData walk uses, so it reads as a
    // `test/jl` plus `cmp 0x15ea/jge` pair) and one on the adjacent hex.
    static bool validHex(int hex)
    {
        return hex >= 0 && hex < COMBAT_GRID_CELLS;
    }
