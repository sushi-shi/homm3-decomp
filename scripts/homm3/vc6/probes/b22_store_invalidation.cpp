// CATALOG: B22
// PHENOMENON: the store-invalidation rule behind the aliasing "semantics,
//   not allocation" family (town::Deallocate, cmbtmgr::RemoveArmyFromGrid,
//   inputmgr bufferBusy, findpath base reload). VC6 CSEs a repeated
//   pointer-derived load (no_store folds two reads of m->num into one load +
//   shl), but ANY intervening store kills the CSE and forces a reload -
//   including, measured here, a char-array store that may alias the field.
//   (Also measured while reducing: a store to an unrelated named int global
//   invalidates too - VC6's invalidation is address-blind conservative, with
//   no type-based refinement.)
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-COUNT-ASM(with_store): 2 movsx\s+e(?:[a-d]x|si|di), BYTE PTR \[ecx\]
// EXPECT-COUNT-ASM(no_store): 1 movsx\s+e(?:[a-d]x|si|di), BYTE PTR \[ecx\]
// EXPECT-ASM(no_store): shl\s+eax, 1
struct mgrC { char num; char ids[8]; };
int with_store(mgrC* m, int k) {
    int n = m->num;
    m->ids[k] = 0;
    return n + m->num;
}
int no_store(mgrC* m) {
    int n = m->num;
    return n + m->num;
}
