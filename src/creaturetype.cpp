#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "creaturetype.h"

#include "resourcemanager.h"
#include "textresource.h"
#include "town.h"

namespace {

// Original akCreatureTypeTraits references these 150 writable rows. Retail
// initializers retain sprite/sample names and flags before crtraits.txt loads.
DATA(0x006703b8)
TCreatureTypeTraits g_creatureTypeTraitsStorage[150] = {
    { TOWN_CASTLE, 0, "pike", "cpkman.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 0, "halb", "chalbd.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 1, "lcrs", "clcbow.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 1, "hcrs", "chcbow.def", 0x8014, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 2, "grif", "cgriff.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 2, "rgrf", "crgrif.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 3, "swrd", "csword.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 3, "crus", "ccrusd.def", 0x8010, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 4, "monk", "cmonkk.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 4, "zelt", "czealt.def", 0x1014, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 5, "cava", "ccavlr.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 5, "chmp", "cchamp.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 6, "angl", "cangel.def", 0x112, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CASTLE, 6, "aagl", "crangl.def", 0x113, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 0, "cntr", "ccentr.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 0, "ecnt", "cecent.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 1, "dwrf", "cdwarf.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 1, "bdrf", "cbdwar.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 2, "welf", "celf.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 2, "gelf", "cgrelf.def", 0x8014, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 3, "pega", "cpegas.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 3, "apeg", "capegs.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 4, "tree", "ctree.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 4, "btre", "cbtree.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 5, "unic", "cunico.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 5, "wunc", "cwunic.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 6, "grdr", "cgdrag.def", 0x8000009b, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_RAMPART, 6, "godr", "cddrag.def", 0x8000009b, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 0, "agrm", "cgrema.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 0, "mgrm", "cgremm.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 1, "sgrg", "cgargo.def", 0x2, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 1, "ogrg", "cogarg.def", 0x2, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 2, "sglm", "csgole.def", 0x20400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 2, "iglm", "cigole.def", 0x20400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 3, "mage", "cmage.def", 0x1014, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 3, "amag", "camage.def", 0x1814, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 4, "geni", "cgenie.def", 0x12, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 4, "calf", "csulta.def", 0x12, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 5, "nsen", "cnaga.def", 0x10011, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 5, "ngrd", "cnagag.def", 0x10011, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 6, "ltit", "cltita.def", 0x610, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_TOWER, 6, "gtit", "cgtita.def", 0x1614, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 0, "impp", "cimp.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 0, "fmlr", "cfamil.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 1, "gogg", "cgog.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 1, "mgog", "cmagog.def", 0x100014, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 2, "hhnd", "chhoun.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 2, "cerb", "ccerbu.def", 0x90011, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 3, "shdm", "cohdem.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 3, "dhdm", "cthdem.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 4, "pfnd", "cpfien.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 4, "pfoe", "cpfoe.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 5, "efrt", "cefree.def", 0x4012, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 5, "esul", "cefres.def", 0x4012, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 6, "devl", "cdevil.def", 0x10110, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_INFERNO, 6, "advl", "cadevl.def", 0x10110, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 0, "skel", "cskele.def", 0x60400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 0, "sklw", "cwskel.def", 0x60400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 1, "zomb", "czombi.def", 0x60400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 1, "zmbl", "czomlo.def", 0x60400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 2, "wght", "cwight.def", 0x60402, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 2, "wrth", "cwrait.def", 0x60402, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 3, "vamp", "cvamp.def", 0x70402, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 3, "nosf", "cnosfe.def", 0x70402, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 4, "lich", "clich.def", 0x160404, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 4, "plch", "cplich.def", 0x160404, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 5, "bknt", "cbknig.def", 0x60401, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 5, "blrd", "cblord.def", 0x60401, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 6, "bodr", "cndrgn.def", 0x80060483, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_NECROPOLIS, 6, "ghdr", "chdrgn.def", 0x80060483, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 0, "trog", "ctrogl.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 0, "itrg", "citrog.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 1, "harp", "charpy.def", 0x12, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 1, "hhag", "charph.def", 0x10012, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 2, "bhdr", "cbehol.def", 0x1814, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 2, "evli", "ceveye.def", 0x1814, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 3, "medu", "cmedus.def", 0x1015, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 3, "medq", "cmeduq.def", 0x1015, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 4, "mino", "cminot.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 4, "mink", "cminok.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 5, "mant", "cmcore.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 5, "scrp", "ccmcor.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 6, "rddr", "crdrgn.def", 0x8000009b, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_DUNGEON, 6, "bkdr", "cbdrgn.def", 0x8000049b, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 0, "gbln", "cgobli.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 0, "hgob", "chgobl.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 1, "gwrd", "cbwlfr.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 1, "hgwr", "cuwlfr.def", 0x8011, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 2, "oorc", "corc.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 2, "orcc", "corcch.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 3, "ogre", "cogre.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 3, "ogrm", "cogmag.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 4, "rocc", "croc.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 4, "tbrd", "ctbird.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 5, "ccyc", "ccyclr.def", 0x34, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 5, "cycl", "ccycllor.def", 0x34, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 6, "ybmh", "cybehe.def", 0x91, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_STRONGHOLD, 6, "bmth", "cabehe.def", 0x91, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 0, "gnol", "cgnoll.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 0, "gnlm", "cgnolm.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 1, "pliz", "cpliza.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 1, "aliz", "caliza.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 4, "cgor", "ccgorg.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 4, "bgor", "cbgog.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 2, "dfly", "cdrfly.def", 0x12, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 2, "fdfl", "cdrfir.def", 0x12, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 3, "basl", "cbasil.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 3, "gbas", "cgbasi.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 5, "wyvn", "cwyver.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 5, "wyvm", "cwyvmn.def", 0x13, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 6, "hydr", "chydra.def", 0x90091, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_FORTRESS, 6, "chyd", "cchydr.def", 0x90091, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 1, "aelm", "caelem.def", 0x20400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 4, "eelm", "ceelem.def", 0x20400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 3, "felm", "cfelem.def", 0x24400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 2, "welm", "cwelem.def", 0x20401, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 4, "gglm", "cggole.def", 0x20400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 5, "dglm", "cdgole.def", 0x20400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 0, "pixi", "cpixie.def", 0x12, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 0, "sprt", "csprite.def", 0x10012, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 5, "psyc", "CpsyEl.def", 0xb0400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 5, "mgel", "CmagEl.def", 0xb0400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, -1, "bad1", "bad1", 0x0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 3, "icel", "cicee.def", 0x20405, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, -1, "bad2", "bad2", 0x0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 2, "magm", "cstone.def", 0x20400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, -1, "bad3", "bad3", 0x0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 1, "stor", "cstorm.def", 0x20404, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, -1, "bad4", "bad4", 0x0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 4, "ener", "cnrg.def", 0x24402, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 6, "firb", "cfbird.def", 0x409b, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { TOWN_CONFLUX, 6, "phoe", "cphx.def", 0x409b, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 6, "azur", "CADRGN.def", 0x8000009b, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 6, "crys", "Ccdrgn.def", 0x80000091, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 6, "faer", "CFDRGN.def", 0x80000093, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 6, "rust", "CRsDgn.def", 0x8000009b, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 5, "ench", "cench.def", 0x1014, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 3, "hcrs", "csharp.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 0, "half", "chalf.def", 0x14, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 0, "psnt", "cpeas.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 1, "boar", "cboar.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 2, "mumy", "cmummy.def", 0x60400, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 2, "nmad", "cnomad.def", 0x11, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 1, "agrm", "crogue.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 4, "trll", "ctroll.def", 0x10, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 0, "cata", "smcata.def", 0x20465, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 4, "ball", "smbal.def", 0x31445, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 0, "faid", "smtent.def", 0x20441, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 0, "cart", "smcart.def", 0x20440, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { eTownNeutral, 0, "", "", 0x20444, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

}

DATA(0x006747b0)
const TCreatureTypeTraits (&akCreatureTypeTraits)[150] = g_creatureTypeTraitsStorage;

void initializeCreatureTypeTraits(int id,
    const std::vector<char*, std::allocator<char*> >& values);

// Provisional name: Mac retains this source helper at 0:0x888d0, immediately
// before isBaseCreature. All three following queries call it there; VC6
// expands its two-row lookup in their Windows bodies.
MAC_ADDRESS(0x0888d0, 0x70)
static int getCreatureDwellingIndex(TCreatureType type)
{
    const TCreatureTypeTraits& traits = akCreatureTypeTraits[type];
    int townType = traits.m_townType;
    if (townType == -1)
        return -1;

    int creatureIndex = traits.m_level;
    if (type == g_dwellingType[townType][creatureIndex])
        return creatureIndex;
    creatureIndex += 7;
    if (type == g_dwellingType[townType][creatureIndex])
        return creatureIndex;
    return -1;
}

VA(0x0047b120, 0x5D)
DC_ADDRESS(0x0718fc, 0x36)
MAC_ADDRESS(0x088940, 0x3c)
int isBaseCreature(TCreatureType monType)
{
    int creatureIndex = getCreatureDwellingIndex(monType);
    return creatureIndex >= 0 && creatureIndex < 7;
}

VA(0x0047b180, 0x16)
DC_ADDRESS(0x071934, 0x12)
MAC_ADDRESS(0x08897c, 0x24)
unsigned char isSiegeWeapon(TCreatureType creature)
{
    if (creature >= CREATURE_CATAPULT && creature <= CREATURE_AMMO_CART)
        return 1;
    return 0;
}

VA(0x0047b1a0, 0x71)
DC_ADDRESS(0x071948, 0x20)
MAC_ADDRESS(0x0889a0, 0x6c)
TCreatureType upgradedCreatureType(TCreatureType type)
{
    int creatureIndex = getCreatureDwellingIndex(type);
    if (creatureIndex < 0 || creatureIndex >= 7)
        return CREATURE_NONE;
    return g_dwellingType[akCreatureTypeTraits[type].m_townType][creatureIndex + 7];
}

VA(0x0047B220, 0x6D)
MAC_ADDRESS(0x088a0c, 0x64)
TCreatureType downgradedCreatureType(TCreatureType type)
{
    int creatureIndex = getCreatureDwellingIndex(type);
    if (creatureIndex < 7)
        return CREATURE_NONE;
    return g_dwellingType[akCreatureTypeTraits[type].m_townType][creatureIndex - 7];
}

VA(0x0047b290, 0x1E9)
DC_ADDRESS(0x071968, 0x1d8)
MAC_ADDRESS(0x088a70, 0x37c)
unsigned char InitializeCreatureTypeTraitsTable()
{
    TSpreadsheetResource* traitsSheet = ResourceManager::GetSpreadsheet(
        DATA_COMPGEN(0x00675514, creatureTraitsSpreadsheetName,
                     "crtraits.txt"));
    if (!traitsSheet)
        return 0;
    if (traitsSheet->GetNumberOfRows() < 179) {
        ResourceManager::Dispose(traitsSheet);
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 14; id++, row++)
        initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 6; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 13; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    row += 3;
    { for (int i = 0; i < 5; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->GetRow(row));
    }
    ResourceManager::Dispose(traitsSheet);
    return 1;
}

namespace {

// CodeView field pStr; each loader owns its own private string class.
class TAutoStrPtr {
public:

    // E:\gamedcs\creaturetype.cpp:399
    DC_ADDRESS(0x071eec, 0x8)
    TAutoStrPtr() : m_string(0) {}

    // E:\gamedcs\creaturetype.cpp:402
    // Retail keeps this COMDAT at 0x47b7b0, directly after
    // initializeCreatureTypeTraits; identical private copies fold to it.
    VA(0x0047b7b0, 0xA)
    DC_ADDRESS(0x071ef4, 0x18)
    ~TAutoStrPtr() { delete[] m_string; }

    // E:\gamedcs\creaturetype.cpp:404
    DC_ADDRESS(0x071f0c, 0x4)
    void set(char* value) { m_string = value; }

    // E:\gamedcs\creaturetype.cpp:406
    DC_ADDRESS(0x071f10, 0x4)
    char* get() const { return m_string; }

    // Project-inferred allocation/copy used to initialize owned table text.
    void copyText(const char* source);

private:
    char* m_string;
};

// Keep the native Set/Get operations and store ownership before copying.
// Like the existing initialization sequence, this does not release an old value.
void TAutoStrPtr::copyText(const char* source)
{
    set(new char[strlen(source) + 1]);
    strcpy(get(), source);
}

}

// neighbouring column takes a dword.
VA(0x0047b480, 0x322)
DC_ADDRESS(0x071b40, 0x350)
MAC_ADDRESS(0x088dec, 0x320)
void initializeCreatureTypeTraits(int id,
    const std::vector<char*, std::allocator<char*> >& values)
{
    TCreatureTypeTraits& traits = g_creatureTypeTraitsStorage[id];

    DATA_COMPGEN_GUARD(0x00696640, creatureTypeStringsGuard,
                       creatureTypeNames)
    DATA(0x006963e8)
    static TAutoStrPtr creatureTypeNames[150];

    creatureTypeNames[id].set(new char[strlen(values[0]) + 1]);
    strcpy(creatureTypeNames[id].get(), values[0]);
    traits.m_name = creatureTypeNames[id].get();

    DATA(0x00696644)
    static TAutoStrPtr creatureTypePluralNames[150];

    creatureTypePluralNames[id].set(new char[strlen(values[1]) + 1]);
    strcpy(creatureTypePluralNames[id].get(), values[1]);
    traits.m_pluralName = creatureTypePluralNames[id].get();

    traits.m_cost[0] = atoi(values[2]);
    traits.m_cost[1] = atoi(values[3]);
    traits.m_cost[2] = atoi(values[4]);
    traits.m_cost[3] = atoi(values[5]);
    traits.m_cost[4] = atoi(values[6]);
    traits.m_cost[5] = atoi(values[7]);
    traits.m_cost[6] = atoi(values[8]);
    traits.m_baseFightValue = atoi(values[9]);
    traits.m_aiValue = atoi(values[10]);
    traits.m_growthRate = atoi(values[11]);
    traits.m_hordeGrowthRate = atoi(values[12]);
    traits.m_hitPoints = atoi(values[13]);
    traits.m_speed = atoi(values[14]);
    traits.m_attackSkill = atoi(values[15]);
    traits.m_defenseSkill = atoi(values[16]);
    traits.m_damageLowBound = atoi(values[17]);
    traits.m_damageHighBound = atoi(values[18]);
    traits.m_numShots = atoi(values[19]);
    traits.m_hasSpell = atoi(values[20]);
    traits.m_wanderingLow = atoi(values[21]);
    traits.m_wanderingHigh = atoi(values[22]);

    DATA(0x00696190)
    static TAutoStrPtr creatureTypeAbilities[150];

    creatureTypeAbilities[id].set(new char[strlen(values[23]) + 1]);
    strcpy(creatureTypeAbilities[id].get(), values[23]);
    traits.m_specialAbility = creatureTypeAbilities[id].get();
}
