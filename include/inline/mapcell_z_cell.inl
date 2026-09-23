// MapCell.h:850, dc 0x1f974. This worker reproduces all 49 retail bytes
// at 0x408770, including the boat callers' retained zero-coordinate lookup.
// The former scalar-cell claim incorrectly distinguished 49 x86 bytes from
// 82 SH4 bytes and alleged a zCell bounds test absent on both platforms.
// Identical folded bodies cannot prove a unique original retail symbol;
// this annotation owns the emitted canonical worker, not a renamed wrapper.
VA(0x00408770, 0x31)  // exact body + anchor-callees, dc 0x1f974
inline NewmapCell* NewfullMap::zCell(int x, int y, int z)
{
    return m_cellData + x + y * m_size + z * m_size * m_size;
}
