/* switch_fixture.c - COFF ground truth for the carve size contract.
 *
 * Compiled /O2 /Gy /c /TC by the real VC6 SP3 cl (via homm3.core.cc_wrap),
 * every function below becomes its own COMDAT section, and the section's
 * SizeOfRawData is the linker-true contribution. fixture.py asserts that the
 * switch functions' contributions INCLUDE their jump tables - the whole
 * premise of carving size = code + tables. Each case body returns a distinct
 * affine value so VC6 cannot tail-merge cases away from table form.
 *
 * dense_trailing   one dense switch -> dword jump table trailing the code
 * double_switch    two dense switches in one function -> two dword tables
 * sparse_casemap   cases spread over 0..250 -> dword table + byte case map
 * no_switch_control no tables at all - the code-only baseline
 */

int dense_trailing(int x)
{
    switch (x) {
    case 0: return x * 3 + 11;
    case 1: return x * 5 + 23;
    case 2: return x * 7 + 37;
    case 3: return x * 11 + 41;
    case 4: return x * 13 + 53;
    case 5: return x * 17 + 67;
    case 6: return x * 19 + 71;
    case 7: return x * 23 + 83;
    }
    return -1;
}

int double_switch(int x, int y)
{
    int acc = 0;
    switch (x) {
    case 0: acc = y * 3 + 101; break;
    case 1: acc = y * 5 + 103; break;
    case 2: acc = y * 7 + 107; break;
    case 3: acc = y * 11 + 109; break;
    case 4: acc = y * 13 + 113; break;
    case 5: acc = y * 17 + 127; break;
    case 6: acc = y * 19 + 131; break;
    case 7: acc = y * 23 + 137; break;
    default: acc = y - 1; break;
    }
    switch (y) {
    case 0: acc += x * 29 + 139; break;
    case 1: acc += x * 31 + 149; break;
    case 2: acc += x * 37 + 151; break;
    case 3: acc += x * 41 + 157; break;
    case 4: acc += x * 43 + 163; break;
    case 5: acc += x * 47 + 167; break;
    case 6: acc += x * 53 + 173; break;
    case 7: acc += x * 59 + 179; break;
    default: acc += x + 1; break;
    }
    return acc;
}

int sparse_casemap(int x)
{
    switch (x) {
    case 0: return x * 3 + 191;
    case 3: return x * 5 + 193;
    case 7: return x * 7 + 197;
    case 15: return x * 11 + 199;
    case 31: return x * 13 + 211;
    case 63: return x * 17 + 223;
    case 90: return x * 19 + 227;
    case 120: return x * 23 + 229;
    case 200: return x * 29 + 233;
    case 250: return x * 31 + 239;
    }
    return -2;
}

int no_switch_control(int x)
{
    return (x * 2654435761u) >> 3;
}
