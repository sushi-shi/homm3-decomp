/* rva.h - the annotation contract (port plan P0.2, gruntz template).
 *
 * RVA(rva, size)      this function definition is matched to the pinned
 *                     retail image (HEROES3.EXE sha 057c9d88..., base
 *                     0x400000) at that rva/size.
 * DATA(rva)           reserved for matched globals (no uses yet).
 * DC_ONLY(off, cb)    the function is evidenced only in the Dreamcast
 *                     build so far (CodeView proc at .text offset/cb);
 *                     it makes NO claim about the retail image.
 *
 * All three compile out under VC6: annotations are consumed by the label
 * map generator (later phase), never by the compiler.
 */
#ifndef HOMM3_RVA_H
#define HOMM3_RVA_H

#define RVA(rva, size)
#define DATA(rva)
#define DC_ONLY(off, cb)

#endif
