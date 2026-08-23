// crt_stdio.h - the CRT stream/memory surface, declared WITHOUT pulling
// <stdio.h> or <string.h>.
// HAND-OWNED after admission.
//
// Why this header exists rather than the real CRT headers: the
// include-set sensitivity class (initialize_game_data precedent,
// byte-proven) makes the COUNT OF USER-DEFINED TYPE DEFINITIONS visible
// in a TU a real codegen input, with no semantic change anywhere. A
// measurement in this tree on 2026-08-08 cost initialize_game_data
// 100.0 -> 96.09 for a single added type definition in a shared
// header's closure. <stdio.h> would add its own definitions to every
// TU that needs so much as fopen.
//
// A bare `struct _iobuf;` FORWARD declaration is in the proven-inert
// set (blank lines, comments, typedefs, `extern int` and bare
// `struct X;` do NOT move the class), so this header buys the CRT calls
// with no type DEFINITION at all. _CRTIMP is empty under /ML, so these
// match the real declarations and the CRT's own linkage.
//
// Same doctrine as winmm_thunks.h: a declaration whose SPELLING is
// load-bearing lives in one reviewed header, not re-typed per .cpp.
// Include it only in the TUs that actually call these.
#ifndef HOMM3_CRT_STDIO_H
#define HOMM3_CRT_STDIO_H

extern "C" {
struct _iobuf;
typedef struct _iobuf FILE;
FILE* __cdecl fopen(const char* filename, const char* mode);
int __cdecl fseek(FILE* stream, long offset, int origin);
long __cdecl ftell(FILE* stream);
int __cdecl fclose(FILE* stream);
int __cdecl sprintf(char* buffer, const char* format, ...);
// VC6's va_list is char*.  Keep the ABI spelling here without pulling
// <stdarg.h> and its include-set surface into every consumer of this header.
int __cdecl vsprintf(char* buffer, const char* format, char* arguments);
void* __cdecl memset(void* dest, int fill, unsigned int count);
char* __cdecl _getcwd(char* buffer, int maxlen);
int __cdecl _strcmpi(const char* lhs, const char* rhs);
char* __cdecl strcat(char* dest, const char* src);
char* __cdecl strcpy(char* dest, const char* src);
unsigned int __cdecl strlen(const char* text);
}

#define SEEK_SET 0
#define SEEK_END 2

#endif  /* HOMM3_CRT_STDIO_H */
