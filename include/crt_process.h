// crt_process.h - the two CRT thread entry points used by the selection UI.
// HAND-OWNED after admission.
//
// VC6's <process.h> hides these declarations under _MT, while the retail
// singleselectionwindow.obj is a /ML compiland that still calls both symbols.
// Keep the ABI spelling in one reviewed owner instead of repeating it in a
// consumer or changing the TU's _MT preprocessor state.
#ifndef HOMM3_CRT_PROCESS_H
#define HOMM3_CRT_PROCESS_H

typedef unsigned (__stdcall* H3ThreadStart)(void*);

extern "C" {
unsigned long __cdecl _beginthreadex(
    void* security, unsigned stackSize, H3ThreadStart startAddress,
    void* arguments, unsigned initFlag, unsigned* threadAddress);
void __cdecl _endthreadex(unsigned returnValue);
}

#endif  /* HOMM3_CRT_PROCESS_H */
