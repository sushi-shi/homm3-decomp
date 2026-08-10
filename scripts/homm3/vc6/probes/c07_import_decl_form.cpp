// CATALOG: C7
// PHENOMENON: the import-declaration form is per-declaration TU state.
//   A __declspec(dllimport) declaration emits the IAT form
//   `call DWORD PTR __imp__X@N`; a plain extern declaration of another
//   import emits the thunk form `call _X@N` - both in ONE TU
//   (timeGetTime: kbwin IAT vs button/misc/mousemgr thunk).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(sample): call\s+DWORD PTR __imp__timeGetTime@0
// EXPECT-ASM(sample): call\s+_timeKillEvent@4
// EXPECT-NOT-ASM(sample): call\s+_timeGetTime@0
extern "C" __declspec(dllimport) unsigned long __stdcall timeGetTime(void);
extern "C" unsigned int __stdcall timeKillEvent(unsigned int id);
unsigned long gT;
void sample(unsigned int id) {
    gT = timeGetTime();
    timeKillEvent(id);
}
