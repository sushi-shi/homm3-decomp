// homm3.vc6.shim/sample_tu.cpp - fixed input for the shim byte-identity gate.
//
// shim/build.py compiles this TU twice with the game profile
// (/c /O2 /Ob2 /Oy- /Op /ML /Gr /GX): once through the real toolchain and
// once through the shim overlay.  The two .obj files must be byte-identical
// outside the COFF TimeDateStamp (file bytes 4..7) - see docs/vc6/shim.md.
//
// Keep this file FROZEN: the gate's value is that the input never moves.
// It deliberately has no #includes (no header-drift surface), more than one
// function (so dropping -Gy - the negative control - must change the COMDAT
// layout), an inlinable static callee, and a loop.

static unsigned mix(unsigned h, unsigned v)
{
    return (h ^ v) * 16777619u;
}

unsigned shim_gate_hash(const unsigned char *p, unsigned n)
{
    unsigned h = 2166136261u;
    while (n--)
        h = mix(h, *p++);
    return h;
}

int shim_gate_poly(int x, int y)
{
    int acc = 0;
    int i;
    for (i = 0; i < y; ++i)
        acc += (x + i) * (x - i);
    return acc ? acc : x;
}
