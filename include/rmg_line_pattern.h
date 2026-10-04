#ifndef HOMM3_RMG_LINE_PATTERN_H
#define HOMM3_RMG_LINE_PATTERN_H

// Unreflected line shapes chosen by selectRmgLinePattern; reflections supply
// the other orientations. North is up; # is a line tile.
//   END_S  END_E  NS     EW     SE     NES    ESW    CROSS
//   . . .  . . .  . # .  . . .  . . .  . # .  . . .  . # .
//   . # .  . # #  . # .  # # #  . # #  . # #  # # #  # # #
//   . # .  . . .  . # .  . . .  . # .  . # .  . # .  . # .
// SE_VARIANT is SE with the NE or SW diagonal also a line tile.
// END_S also covers an isolated tile.
enum ERmgLinePattern {
    LINE_END_S = 0,
    LINE_END_E = 1,
    LINE_NS = 2,
    LINE_EW = 3,
    LINE_SE = 4,
    LINE_SE_VARIANT = 5,
    LINE_NES = 6,
    LINE_ESW = 7,
    LINE_CROSS = 8,
    LINE_PATTERN_COUNT = 9
};

#endif
