# VC6 candidate source lines

`homm3 sema diff --source` reads the parallel `/Z7` object's classic COFF
`IMAGE_LINENUMBER` records. A stored line of zero anchors a function symbol;
ordinary records add their stored line to the function's `.bf` opening line.
VC6 uses **0x7fff for a zero relative line**, allowing later instructions to
return to that opening line without being interpreted as another anchor.

The pinned SP3 compiler proves the encoding with this control:

```cpp
void zeroRelative(volatile int* dst)
{
    dst[0] = 1;
#line 2
    dst[1] = 2;
#line 8
    dst[2] = 3;
}
```

With `/O2 /Ob2 /Oy- /Op /ML /Gr /GX /Gy /Z7 /FAs`, `.bf` is line 2.
The second store at offset 6 has stored line 0x7fff; the compiler's assembly
listing labels it line 2. Changing only `#line 2` to `#line 3` changes that
record to 1. Both functions have identical 21-byte bodies, SHA-256
`efd96d94536a7ef0515b4b22ae952081e7fd73486c3116fc5e64313b3f21dda0`.

The same encoding occurs naturally at offset 0x8b of mapcell's
`CObjectType(TObjectType*)` candidate: the loop's `xor ebx, ebx` is attributed
to the opening brace. Treating 0x7fff as an ordinary delta produced an
out-of-range source line. The reader decodes this one value explicitly;
other deltas remain intact. Source-range validation and byte identity
between matching and debug functions remain required.
