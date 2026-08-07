// winmm_thunks.h - the PLAIN (thunk-form) winmm import declaration.
// HAND-OWNED after admission.
//
// Retail's TUs split on how they reach timeGetTime, and the split is
// byte-proven per TU:
//   * kbwin.obj calls through the IAT (`call [__imp__timeGetTime@0]`),
//     which needs the dllimport declaration mmsystem.h provides via
//     <windows.h> - kbwin.cpp must NEVER see a plain declaration, and
//     therefore must never include this header (directly or through
//     another header).
//   * button.obj / misc.obj / mousemgr.obj call straight into the
//     import thunk (an E8 rel32 - button::Select's match proves the
//     form), which needs a PLAIN declaration: a plain redeclaration
//     DOWNGRADES an earlier dllimport for the rest of the TU, and a
//     dllimport after a plain declaration loses. Those three .cpps
//     include this header AFTER their windows.h-bearing includes.
// (Import-call-form doctrine: .claude/skills/match - "Import call
// forms"; the original file-local declaration lived in button.cpp.)
#ifndef HOMM3_WINMM_THUNKS_H
#define HOMM3_WINMM_THUNKS_H

extern "C" unsigned long __stdcall timeGetTime();

#endif  /* HOMM3_WINMM_THUNKS_H */
