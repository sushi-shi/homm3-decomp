// CATALOG: A13
// PHENOMENON: `#pragma inline_depth` is positionally scoped - two functions
//   in one TU compile under different budgets. depth(0) suppresses all
//   expansion in functions that follow it; depth(255) restores the default.
//   This is the retail-shape instrument behind mousemgr::CheckUpdate EXACT,
//   the armygrp::TSplitWindow adapter and the ai_player::make_gift closure.
// NOTE: inline_depth(1) canNOT pin a depth-2 leaf standalone: the one-pass
//   inliner folds leaves into their callers' stored bodies bottom-up
//   (measured: an f->g->h chain under depth(1) still has no calls), so only
//   the depth-0 stop is probeable in a single TU.
// OBSERVABLE: pinned_caller keeps `call ?mid`; deep_caller has no call.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(pinned_caller): call\s+\?mid@@YIHH@Z
// EXPECT-NOT-ASM(deep_caller): call
int gAcc;
int mid(int x) { return (x + gAcc) * 2; }
#pragma inline_depth(0)
int pinned_caller(int x) { return mid(x) + 1; }
#pragma inline_depth(255)
int deep_caller(int x) { return mid(x) + 1; }
