// Dinkumware string COMDAT support. The original compiland name is unknown:
// retail places this body in the broad newgame..overview link-order gap, and
// its only code reference is collate<char>::do_transform at 0x614260.
#include <va.h>
#include <string>

// Complete class instantiation is the natural VC6 boundary which retains the
// one-argument resize COMDAT. A member-only explicit instantiation is a
// measured no-op. Keeping this in a separate compiler invocation is required:
// placing the same class instantiation in objecttype.cpp perturbs the EH-state
// layout of that compiland's two otherwise exact bitset extractors. Eight
// standard-header surfaces all retain the same exact resize body; <string>
// alone is sufficient and avoids asserting unrelated template ownership.
template class std::basic_string<char>;

// Retail 0x515010 (408 B) is `basic_string<char>::resize(size_type)`.
// The bytes are Dinkumware's one-argument overload with both arms expanded:
// `_N <= _Len ? erase(_N) : append(_N - _Len, _E(0))`. The erase arm has the
// `_Xran` guard, `_Split()` and `_Eos`; the append arm has the exact two
// `_Xlen` guards and `_Tr::assign(_Ptr + _Len, _N, _C)` `rep stosb`. `ret 4`
// and the `[ebx+8]` / `[ebx+4]` reads fix the one-argument receiver ABI.
// Explicit collate instantiation expands resize and does not retain it; the
// complete basic_string instantiation emits this canonical COMDAT exactly.
VA_COMPGEN(0x00515010, 0x198, BASIC_STRING_RESIZE, char)
