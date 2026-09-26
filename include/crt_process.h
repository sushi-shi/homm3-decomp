// crt_process.h - the CRT thread entry points used by the game.

// Keep these thread-entry ABI declarations in one reviewed owner without
// pulling <process.h> and its additional declarations into every consumer.
#ifndef HOMM3_CRT_PROCESS_H
#define HOMM3_CRT_PROCESS_H

typedef unsigned (__stdcall* H3ThreadStart)(void*);
typedef void (__cdecl* H3ThreadStartCdecl)(void*);

extern "C" {
unsigned long __cdecl _beginthread(
    H3ThreadStartCdecl startAddress, unsigned stackSize, void* arguments);
void __cdecl _endthread();
unsigned long __cdecl _beginthreadex(
    void* security, unsigned stackSize, H3ThreadStart startAddress,
    void* arguments, unsigned initFlag, unsigned* threadAddress);
void __cdecl _endthreadex(unsigned returnValue);
}

#endif  /* HOMM3_CRT_PROCESS_H */
