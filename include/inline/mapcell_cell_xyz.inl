// DC MapCell.h:897 calls zCell. The unrecorded line 896 does not prove
// a release VERIFY. Removing the inferred storage check preserves this
// helper chain and restores the boat callers' retail expansion decisions;
// the retained 49-byte arithmetic body is owned by zCell above.
inline NewmapCell* NewfullMap::cell(int x, int y, int z)
{
    return zCell(x, y, z);
}
