// MapCell.h:769 in the DC roster (dc 0x2e48), i.e. a header inline of
// this class - and retail keeps no out-of-line row for it either.
// advManager::ProcessDeSelect's elevation-toggle arm expands it in
// place: `movzx edx,[gpGame+0x1fc48] / inc edx / cmp edx,1 / jle`, the
// zero-extended flag plus one, tested against one. Gated to the
// compilation personalities whose call sites prove the expansion
// (victorylossconditions' z bound in CheckForDefeatedMonsterWin is
// the same movzx/inc shape, 2026-08-20).
inline int NewfullMap::getNumLevels() { return m_hasTwoLevels + 1; }
