#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "herodefs.h"

#include "hero.h"
#include "resourcemanager.h"
#include "textresource.h"

// Retail initial data: parsers fill names and numeric traits later; hero
// portraits, starting skills/stacks and availability already exist at startup.
DATA(0x00679dd0)
THeroTraits g_heroTraitsStorage[156] = {
    { 0, 7, THeroClass(0), 6, 1, 1, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS000Kn.PCX", "HPL000Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), 6, 1, 1, 1, 0, { 0, 0, 0 }, -1, TCreatureType(2), TCreatureType(2), TCreatureType(2), "HPS001Kn.PCX", "HPL001Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), 6, 1, 23, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(4), TCreatureType(4), "HPS002Kn.PCX", "HPL002Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), 6, 1, 5, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS003Kn.PCX", "HPL003Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), 6, 1, 13, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS004Kn.PCX", "HPL004Kn.PCX", { 1 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), 6, 1, 22, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS005Kn.PCX", "HPL005Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), 6, 1, 20, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(146), TCreatureType(4), "HPS006Kn.PCX", "HPL006Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), 6, 1, 19, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS007Kn.PCX", "HPL007Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(1), 7, 1, 27, 1, 1, { 0, 0, 0 }, 46, TCreatureType(0), TCreatureType(147), TCreatureType(4), "HPS008Cl.PCX", "HPL008Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(1), 7, 1, 4, 1, 1, { 0, 0, 0 }, 41, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS009Cl.PCX", "HPL009Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(1), 7, 1, 13, 1, 1, { 0, 0, 0 }, 45, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS010Cl.PCX", "HPL010Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(1), 7, 2, -1, 0, 1, { 0, 0, 0 }, 20, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS011Cl.PCX", "HPL011Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(1), 7, 1, 8, 1, 1, { 0, 0, 0 }, 42, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS012Cl.PCX", "HPL012Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(1), 7, 1, 11, 1, 1, { 0, 0, 0 }, 35, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS013Cl.PCX", "HPL013Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(1), 7, 1, 21, 1, 1, { 0, 0, 0 }, 48, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS014Cl.PCX", "HPL014Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(1), 7, 1, 24, 1, 1, { 0, 0, 0 }, 37, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS015Cl.PCX", "HPL015Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(2), 6, 1, 23, 1, 0, { 0, 0, 0 }, -1, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS016Rn.PCX", "HPL016Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(2), 26, 1, 9, 1, 0, { 0, 0, 0 }, -1, TCreatureType(16), TCreatureType(16), TCreatureType(16), "HPS017Rn.PCX", "HPL017Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 3, THeroClass(2), 1, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS018Rn.PCX", "HPL018Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(2), 6, 1, 4, 1, 0, { 0, 0, 0 }, -1, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS019Rn.PCX", "HPL019Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(2), 26, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS020Rn.PCX", "HPL020Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, THeroClass(2), 1, 1, 22, 1, 0, { 0, 0, 0 }, -1, TCreatureType(14), TCreatureType(18), TCreatureType(18), "HPS021Rn.PCX", "HPL021Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(2), 26, 1, 0, 1, 0, { 0, 0, 0 }, -1, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS022Rn.PCX", "HPL022Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 3, THeroClass(2), 1, 1, 2, 1, 0, { 0, 0, 0 }, -1, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS023Rn.PCX", "HPL023Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(3), 7, 1, 18, 1, 1, { 0, 0, 0 }, 55, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS024Dr.PCX", "HPL024Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(3), 7, 2, 10, 1, 1, { 0, 0, 0 }, 37, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS025Dr.PCX", "HPL025Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, THeroClass(3), 7, 1, 24, 1, 1, { 0, 0, 0 }, 42, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS026Dr.PCX", "HPL026Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(3), 7, 1, 27, 1, 1, { 0, 0, 0 }, 0, TCreatureType(14), TCreatureType(147), TCreatureType(18), "HPS027Dr.PCX", "HPL027Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(3), 7, 1, 11, 1, 1, { 0, 0, 0 }, 15, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS028Dr.PCX", "HPL028Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 3, THeroClass(3), 7, 1, 9, 1, 1, { 0, 0, 0 }, 51, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS029Dr.PCX", "HPL029Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(3), 7, 1, 25, 1, 1, { 0, 0, 0 }, 16, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS030Dr.PCX", "HPL030Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, THeroClass(3), 7, 1, 3, 1, 1, { 0, 0, 0 }, 30, TCreatureType(14), TCreatureType(16), TCreatureType(18), "HPS031Dr.PCX", "HPL031Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(4), 8, 1, 3, 1, 1, { 0, 0, 0 }, 27, TCreatureType(30), TCreatureType(30), TCreatureType(30), "HPS032Al.PCX", "HPL032Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(4), 18, 2, -1, 0, 1, { 0, 0, 0 }, 15, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS033Al.PCX", "HPL033Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(4), 8, 1, 25, 1, 1, { 0, 0, 0 }, 53, TCreatureType(28), TCreatureType(32), TCreatureType(32), "HPS034Al.PCX", "HPL034Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, THeroClass(4), 18, 1, 23, 1, 1, { 0, 0, 0 }, 27, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS035Al.PCX", "HPL035Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(4), 8, 1, 19, 1, 1, { 0, 0, 0 }, 15, TCreatureType(28), TCreatureType(146), TCreatureType(32), "HPS036Al.PCX", "HPL036Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(4), 18, 1, 26, 1, 1, { 0, 0, 0 }, 53, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS037Al.PCX", "HPL037Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(4), 8, 1, 22, 1, 1, { 0, 0, 0 }, 15, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS038Al.PCX", "HPL038Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, THeroClass(4), 18, 1, 24, 1, 1, { 0, 0, 0 }, 15, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS039Al.PCX", "HPL039Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(5), 7, 2, -1, 0, 1, { 0, 0, 0 }, 60, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS040Wz.PCX", "HPL040Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(5), 7, 1, 8, 1, 1, { 0, 0, 0 }, 46, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS041Wz.PCX", "HPL041Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(5), 7, 1, 11, 1, 1, { 0, 0, 0 }, 35, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS042Wz.PCX", "HPL042Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, THeroClass(5), 7, 1, 24, 1, 1, { 0, 0, 0 }, 51, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS043Wz.PCX", "HPL043Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(5), 7, 1, 10, 1, 1, { 0, 0, 0 }, 27, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS044Wz.PCX", "HPL044Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(5), 7, 1, 25, 1, 1, { 0, 0, 0 }, 19, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS045Wz.PCX", "HPL045Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(5), 7, 1, 4, 1, 1, { 0, 0, 0 }, 53, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS046Wz.PCX", "HPL046Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, THeroClass(5), 7, 1, 18, 1, 1, { 0, 0, 0 }, 42, TCreatureType(28), TCreatureType(30), TCreatureType(32), "HPS047Wz.PCX", "HPL047Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(6), 3, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(42), TCreatureType(46), TCreatureType(46), "HPS048Hr.PCX", "HPL048Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 2, THeroClass(6), 18, 1, 7, 1, 0, { 0, 0, 0 }, -1, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS049Hr.PCX", "HPL049Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, THeroClass(6), 23, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS050Hr.PCX", "HPL050Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(6), 19, 1, 26, 1, 0, { 0, 0, 0 }, -1, TCreatureType(42), TCreatureType(42), TCreatureType(42), "HPS051Hr.PCX", "HPL051Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 2, THeroClass(6), 18, 1, 22, 1, 0, { 0, 0, 0 }, -1, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS052Hr.PCX", "HPL052Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, THeroClass(6), 1, 1, 3, 1, 0, { 0, 0, 0 }, -1, TCreatureType(44), TCreatureType(44), TCreatureType(44), "HPS053Hr.PCX", "HPL053Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(6), 2, 1, 20, 1, 0, { 0, 0, 0 }, -1, TCreatureType(42), TCreatureType(146), TCreatureType(46), "HPS054Hr.PCX", "HPL054Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, THeroClass(6), 22, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS055Hr.PCX", "HPL055Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(7), 7, 1, 24, 1, 1, { 0, 0, 0 }, 3, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS056Dm.PCX", "HPL056Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 2, THeroClass(7), 7, 1, 18, 1, 1, { 0, 0, 0 }, 22, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS057Dm.PCX", "HPL057Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, THeroClass(7), 7, 1, 8, 1, 1, { 0, 0, 0 }, 30, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS058Dm.PCX", "HPL058Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(7), 7, 1, 10, 1, 1, { 0, 0, 0 }, 45, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS059Dm.PCX", "HPL059Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, THeroClass(7), 7, 1, 21, 1, 1, { 0, 0, 0 }, 53, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS060Dm.PCX", "HPL060Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, THeroClass(7), 7, 1, 11, 1, 1, { 0, 0, 0 }, 43, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS061Dm.PCX", "HPL061Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 2, THeroClass(7), 7, 1, 25, 1, 1, { 0, 0, 0 }, 46, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS062Dm.PCX", "HPL062Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(7), 7, 1, 6, 1, 1, { 0, 0, 0 }, 21, TCreatureType(42), TCreatureType(44), TCreatureType(46), "HPS063Dm.PCX", "HPL063Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(8), 12, 1, 26, 1, 1, { 0, 0, 0 }, 53, TCreatureType(58), TCreatureType(58), TCreatureType(58), "HPS064Dk.PCX", "HPL064Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 13, THeroClass(8), 12, 1, 20, 1, 1, { 0, 0, 0 }, 46, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS065Dk.PCX", "HPL065Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 8, THeroClass(8), 12, 1, 21, 1, 1, { 0, 0, 0 }, 54, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS066Dk.PCX", "HPL066Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(8), 12, 1, 19, 1, 1, { 0, 0, 0 }, 15, TCreatureType(56), TCreatureType(60), TCreatureType(60), "HPS067Dk.PCX", "HPL067Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 13, THeroClass(8), 12, 1, 22, 1, 1, { 0, 0, 0 }, 15, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS068Dk.PCX", "HPL068Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 8, THeroClass(8), 12, 2, -1, 1, 1, { 0, 0, 0 }, 15, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS069Dk.PCX", "HPL069Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(8), 12, 1, 22, 1, 1, { 0, 0, 0 }, 15, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS070Dk.PCX", "HPL070Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 13, THeroClass(8), 12, 1, 23, 1, 1, { 0, 0, 0 }, 27, TCreatureType(56), TCreatureType(56), TCreatureType(56), "HPS071Dk.PCX", "HPL071Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(9), 12, 1, 18, 1, 1, { 0, 0, 0 }, 24, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS072Nc.PCX", "HPL072Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 13, THeroClass(9), 12, 1, 7, 1, 1, { 0, 0, 0 }, 23, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS073Nc.PCX", "HPL073Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 8, THeroClass(9), 12, 1, 25, 1, 1, { 0, 0, 0 }, 54, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS074Nc.PCX", "HPL074Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(9), 12, 1, 11, 1, 1, { 0, 0, 0 }, 27, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS075Nc.PCX", "HPL075Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 13, THeroClass(9), 12, 1, 8, 1, 1, { 0, 0, 0 }, 39, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS076Nc.PCX", "HPL076Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 8, THeroClass(9), 12, 1, 21, 1, 1, { 0, 0, 0 }, 46, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS077Nc.PCX", "HPL077Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(9), 12, 2, -1, 0, 1, { 0, 0, 0 }, 42, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS078Nc.PCX", "HPL078Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 8, THeroClass(9), 12, 1, 24, 1, 1, { 0, 0, 0 }, 30, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS079Nc.PCX", "HPL079Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(10), 6, 1, 3, 1, 0, { 0, 0, 0 }, -1, TCreatureType(72), TCreatureType(72), TCreatureType(72), "HPS080Ov.PCX", "HPL080Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(10), 22, 1, 20, 1, 0, { 0, 0, 0 }, -1, TCreatureType(70), TCreatureType(146), TCreatureType(74), "HPS081Ov.PCX", "HPL081Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(10), 19, 1, 22, 1, 0, { 0, 0, 0 }, -1, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS082Ov.PCX", "HPL082Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(10), 6, 1, 26, 1, 0, { 0, 0, 0 }, -1, TCreatureType(70), TCreatureType(74), TCreatureType(74), "HPS083Ov.PCX", "HPL083Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(10), 22, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS084Ov.PCX", "HPL084Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(10), 19, 1, 2, 1, 0, { 0, 0, 0 }, -1, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS085Ov.PCX", "HPL085Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(10), 6, 1, 18, 1, 0, { 0, 0, 0 }, -1, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS086Ov.PCX", "HPL086Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(10), 22, 1, 19, 1, 0, { 0, 0, 0 }, -1, TCreatureType(70), TCreatureType(70), TCreatureType(70), "HPS087Ov.PCX", "HPL087Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(11), 7, 1, 18, 1, 1, { 0, 0, 0 }, 38, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS088Wl.PCX", "HPL088Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(11), 7, 1, 8, 1, 1, { 0, 0, 0 }, 27, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS089Wl.PCX", "HPL089Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(11), 7, 1, 25, 1, 1, { 0, 0, 0 }, 43, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS090Wl.PCX", "HPL090Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(11), 7, 2, -1, 0, 1, { 0, 0, 0 }, 38, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS091Wl.PCX", "HPL091Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(11), 7, 1, 11, 1, 1, { 0, 0, 0 }, 54, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS092Wl.PCX", "HPL092Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(11), 7, 1, 3, 2, 1, { 0, 0, 0 }, 23, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS093Wl.PCX", "HPL093Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(11), 7, 1, 24, 1, 1, { 0, 0, 0 }, 30, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS094Wl.PCX", "HPL094Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(11), 7, 1, 21, 1, 1, { 0, 0, 0 }, 46, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS095Wl.PCX", "HPL095Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(12), 22, 1, 10, 1, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS096Br.PCX", "HPL096Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 6, THeroClass(12), 22, 1, 20, 1, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(146), TCreatureType(88), "HPS097Br.PCX", "HPL097Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(12), 22, 1, 1, 1, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(88), TCreatureType(88), "HPS098Br.PCX", "HPL098Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(12), 22, 1, 3, 1, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS099Br.PCX", "HPL099Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 6, THeroClass(12), 22, 1, 0, 1, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(84), TCreatureType(84), "HPS100Br.PCX", "HPL100Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(12), 22, 1, 26, 1, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS101Br.PCX", "HPL101Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(12), 22, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS102Br.PCX", "HPL102Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 6, THeroClass(12), 22, 1, 19, 1, 0, { 0, 0, 0 }, -1, TCreatureType(86), TCreatureType(86), TCreatureType(86), "HPS103Br.PCX", "HPL103Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(13), 7, 1, 25, 1, 1, { 0, 0, 0 }, 43, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS104Bm.PCX", "HPL104Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(13), 7, 1, 6, 1, 1, { 0, 0, 0 }, 15, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS105Bm.PCX", "HPL105Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(13), 7, 1, 2, 1, 1, { 0, 0, 0 }, 46, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS106Bm.PCX", "HPL106Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(13), 7, 1, 19, 1, 1, { 0, 0, 0 }, 53, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS107Bm.PCX", "HPL107Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 6, THeroClass(13), 7, 1, 20, 1, 1, { 0, 0, 0 }, 44, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS108Bm.PCX", "HPL108Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 11, THeroClass(13), 7, 1, 22, 1, 1, { 0, 0, 0 }, 54, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS109Bm.PCX", "HPL109Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(13), 7, 1, 11, 1, 1, { 0, 0, 0 }, 30, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS110Bm.PCX", "HPL110Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(13), 7, 1, 26, 1, 1, { 0, 0, 0 }, 43, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS111Bm.PCX", "HPL111Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(14), 23, 1, 26, 1, 0, { 0, 0, 0 }, -1, TCreatureType(98), TCreatureType(106), TCreatureType(104), "HPS112Bs.PCX", "HPL112Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 5, THeroClass(14), 23, 1, 6, 1, 0, { 0, 0, 0 }, -1, TCreatureType(98), TCreatureType(98), TCreatureType(98), "HPS113Bs.PCX", "HPL113Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 9, THeroClass(14), 23, 1, 1, 1, 0, { 0, 0, 0 }, -1, TCreatureType(100), TCreatureType(100), TCreatureType(100), "HPS114Bs.PCX", "HPL114Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(14), 23, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS115Bs.PCX", "HPL115Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 5, THeroClass(14), 23, 1, 22, 1, 0, { 0, 0, 0 }, -1, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS116Bs.PCX", "HPL116Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 9, THeroClass(14), 23, 1, 0, 1, 0, { 0, 0, 0 }, -1, TCreatureType(98), TCreatureType(104), TCreatureType(104), "HPS117Bs.PCX", "HPL117Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(14), 23, 1, 20, 1, 0, { 0, 0, 0 }, -1, TCreatureType(98), TCreatureType(146), TCreatureType(104), "HPS118Bs.PCX", "HPL118Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 9, THeroClass(14), 23, 1, 3, 1, 0, { 0, 0, 0 }, -1, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS119Bs.PCX", "HPL119Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(15), 7, 2, -1, 1, 1, { 0, 0, 0 }, 45, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS120Wh.PCX", "HPL120Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 5, THeroClass(15), 7, 1, 8, 1, 1, { 0, 0, 0 }, 15, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS121Wh.PCX", "HPL121Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 9, THeroClass(15), 7, 1, 5, 1, 1, { 0, 0, 0 }, 54, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS122Wh.PCX", "HPL122Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(15), 7, 1, 27, 1, 1, { 0, 0, 0 }, 31, TCreatureType(98), TCreatureType(147), TCreatureType(104), "HPS123Wh.PCX", "HPL123Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 5, THeroClass(15), 7, 1, 21, 1, 1, { 0, 0, 0 }, 46, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS124Wh.PCX", "HPL124Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 9, THeroClass(15), 7, 1, 25, 1, 1, { 0, 0, 0 }, 27, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS125Wh.PCX", "HPL125Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(15), 7, 1, 24, 1, 1, { 0, 0, 0 }, 35, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS126Wh.PCX", "HPL126Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 5, THeroClass(15), 7, 1, 11, 1, 1, { 0, 0, 0 }, 46, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS127Wh.PCX", "HPL127Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(16), 22, 1, 20, 1, 0, { 0, 0, 0 }, -1, TCreatureType(118), TCreatureType(112), TCreatureType(112), "HPS000Pl.PCX", "HPL000pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(16), 19, 1, 13, 1, 0, { 0, 0, 0 }, -1, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS001pl.PCX", "HPL001pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(16), 22, 1, 20, 1, 0, { 0, 0, 0 }, -1, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS002pl.PCX", "HPL002pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(16), 19, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(118), TCreatureType(115), TCreatureType(115), "HPS003pl.PCX", "HPL003pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(16), 22, 1, 2, 1, 0, { 0, 0, 0 }, -1, TCreatureType(118), TCreatureType(112), TCreatureType(112), "HPS004pl.PCX", "HPL004pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(16), 19, 1, 13, 1, 0, { 0, 0, 0 }, -1, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS005pl.PCX", "HPL005pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(16), 22, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS006pl.PCX", "HPL006pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(16), 19, 1, 21, 1, 0, { 0, 0, 0 }, -1, TCreatureType(118), TCreatureType(115), TCreatureType(115), "HPS007pl.PCX", "HPL007pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(17), 7, 1, 14, 1, 1, { 0, 0, 0 }, 13, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS000el.PCX", "HPL000el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(17), 7, 1, 15, 1, 1, { 0, 0, 0 }, 53, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS001el.PCX", "HPL001el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(17), 7, 1, 16, 1, 1, { 0, 0, 0 }, 15, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS002el.PCX", "HPL002el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(17), 7, 1, 17, 1, 1, { 0, 0, 0 }, 46, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS003el.PCX", "HPL003el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(17), 7, 1, 14, 1, 1, { 0, 0, 0 }, 43, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS004el.PCX", "HPL004el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(17), 7, 1, 15, 1, 1, { 0, 0, 0 }, 47, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS005el.PCX", "HPL005el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(17), 7, 1, 16, 1, 1, { 0, 0, 0 }, 35, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS006el.PCX", "HPL006el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(17), 7, 1, 17, 1, 1, { 0, 0, 0 }, 54, TCreatureType(118), TCreatureType(112), TCreatureType(115), "HPS007el.PCX", "HPL007el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), 6, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS130Kn.PCX", "HPL130Kn.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(15), 7, 1, 14, 3, 1, { 0, 0, 0 }, 22, TCreatureType(98), TCreatureType(100), TCreatureType(104), "HPS000Sh.PCX", "HPL000Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), 6, 1, 22, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS128Qc.PCX", "HPL128Qc.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(5), 7, 2, -1, 0, 1, { 0, 0, 0 }, 53, TCreatureType(136), TCreatureType(136), TCreatureType(136), "HPS003Sh.PCX", "HPL003Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, THeroClass(2), 6, 1, 1, 1, 0, { 0, 0, 0 }, -1, TCreatureType(137), TCreatureType(137), TCreatureType(137), "HPS004Sh.PCX", "HPL004Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(12), 22, 2, -1, 0, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS005Sh.PCX", "HPL005Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(8), 12, 2, -1, 0, 1, { 0, 0, 0 }, 54, TCreatureType(56), TCreatureType(58), TCreatureType(60), "HPS006Sh.PCX", "HPL006Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(10), 19, 1, 13, 1, 1, { 0, 0, 0 }, 15, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS007Sh.PCX", "HPL007Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), 6, 1, 23, 1, 0, { 0, 0, 0 }, -1, TCreatureType(0), TCreatureType(2), TCreatureType(4), "HPS009Sh.PCX", "HPL009Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(10), 19, 1, 13, 1, 1, { 0, 0, 0 }, 15, TCreatureType(70), TCreatureType(72), TCreatureType(74), "HPS008Sh.PCX", "HPL008Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(12), 22, 1, 19, 1, 0, { 0, 0, 0 }, -1, TCreatureType(84), TCreatureType(86), TCreatureType(88), "HPS001Sh.PCX", "HPL001Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(6), 6, 1, 19, 1, 0, { 0, 0, 0 }, -1, TCreatureType(42), TCreatureType(46), TCreatureType(46), "HPS131Dm.PCX", "HPL131Dm.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
};
DATA(0x0067d868)
THeroClassTraits g_heroClassTraits[18] = {
    { 0, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 0, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 1, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 1, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 2, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 2, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 3, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 3, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 4, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 4, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 5, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 5, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 6, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 6, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 7, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 7, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 8, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 8, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
};

DATA(0x00698cf0) TSSkillTraits g_sSkillTraitsStorage[28];
DATA(0x0067dce8) const THeroTraits (&g_heroTraits)[156] = g_heroTraitsStorage;
DATA(0x0067dcec) const THeroClassTraits (&g_heroClasses)[18] = g_heroClassTraits;
DATA(0x0067dcf0) const TSSkillTraits (&g_sSkillTraits)[28] = g_sSkillTraitsStorage;

namespace {

// CodeView field pStr; each loader owns its own private string class.
class TAutoStrPtr {
public:
    // E:\gamedcs\herodefs.cpp:391, dc 0xd60d4
    TAutoStrPtr() : m_string(0) {}
    // E:\gamedcs\herodefs.cpp:394, dc 0xd60dc
    ~TAutoStrPtr() { delete[] m_string; }
    // E:\gamedcs\herodefs.cpp:396, dc 0xd60f4
    void set(char* value) { m_string = value; }
    // E:\gamedcs\herodefs.cpp:398, dc 0xd60f8
    char* get() const { return m_string; }

private:
    char* m_string;
};

}

static void initializeHeroTraits(int id, const TSpreadsheetResource::TStringVector& values);
static void initializeHeroClassTraits(int id, const TSpreadsheetResource::TStringVector& values);
static void initializeSSkillTraits(int id, const TSpreadsheetResource::TStringVector& values);

VA(0x004e67a0, 0x176) MAC_ADDRESS(0x1077f8, 0xd4)  // dc 0xd5a40
unsigned char initializeHeroTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0067f154, heroTraitsSpreadsheetName,
                     "hotraits.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 158) {
        resource->dispose();
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 156; ++id, ++row) {
        initializeHeroTraits(id, resource->getRow(row));
    }

    resource->dispose();
    return 1;
}

VA(0x004e6920, 0x1E2) MAC_ADDRESS(0x1078cc, 0xd4)  // dc 0xd5ab4
bool initializeHeroClassTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0067f164, heroClassTraitsSpreadsheetName,
                     "hctraits.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 20) {
        resource->dispose();
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 18; ++id, ++row) {
        initializeHeroClassTraits(id, resource->getRow(row));
    }

    resource->dispose();
    return 1;
}

VA(0x004e6b10, 0x1C8) MAC_ADDRESS(0x1079a0, 0xd4)  // dc 0xd5b28
bool initializeSSkillTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0067f174, secondarySkillTraitsSpreadsheetName,
                     "sstraits.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 30) {
        resource->dispose();
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 28; ++id, ++row) {
        initializeSSkillTraits(id, resource->getRow(row));
    }

    resource->dispose();
    return 1;
}

// The DC table loaders call these ordinary static functions. Complete's
// table bodies retain the same row parsing and private string ownership,
// including the expanded static initialization and destruction families.
// Original: InitializeHeroTraits; herodefs.cpp:409, dc 0xd5bc0
MAC_ADDRESS(0x107a74, 0x134)
static void initializeHeroTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    THeroTraits& traits = g_heroTraitsStorage[id];

    DATA_COMPGEN_GUARD(0x00698b99, heroStringsGuard, heroStrings)
    DATA(0x00698eb0)
    static TAutoStrPtr heroStrings[156];

    heroStrings[id].set(new char[strlen(values[0]) + 1]);
    strcpy(heroStrings[id].get(), values[0]);

    traits.m_defaultName = heroStrings[id].get();
    traits.m_firstStackLow = atoi(values[1]);
    traits.m_firstStackHigh = atoi(values[2]);
    traits.m_secondStackLow = atoi(values[4]);
    traits.m_secondStackHigh = atoi(values[5]);
    traits.m_thirdStackLow = atoi(values[7]);
    traits.m_thirdStackHigh = atoi(values[8]);
}

// Original: InitializeHeroClassTraits; herodefs.cpp:441, dc 0xd5d28
MAC_ADDRESS(0x107c38, 0x1d8)
static void initializeHeroClassTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    THeroClassTraits& traits = g_heroClassTraits[id];

    DATA_COMPGEN_GUARD(0x00698b9a, heroClassStringsGuard,
                      heroClassStrings)
    DATA(0x00699120)
    static TAutoStrPtr heroClassStrings[18];

    heroClassStrings[id].set(new char[strlen(values[0]) + 1]);
    strcpy(heroClassStrings[id].get(), values[0]);
    traits.m_className = heroClassStrings[id].get();
    traits.m_aggression = static_cast<float>(atof(values[1]));

    int column;
    for (column = 0; column < 4; ++column)
        traits.m_initialPrimarySkill[column] =
            static_cast<signed char>(atoi(values[column + 2]));
    for (column = 0; column < 4; ++column)
        traits.m_gainPrimarySkillChance[column] =
            static_cast<signed char>(atoi(values[column + 6]));
    for (column = 0; column < 4; ++column)
        traits.m_gainPrimarySkillChance10P[column] =
            static_cast<signed char>(atoi(values[column + 10]));
    for (column = 0; column < 28; ++column)
        traits.m_gainSecondarySkillChance[column] =
            static_cast<signed char>(atoi(values[column + 14]));
    for (column = 0; column < 9; ++column)
        traits.m_foundInTownType[column] =
            static_cast<signed char>(atoi(values[column + 42]));
}

// Original: InitializeSSkillTraits; herodefs.cpp:489, dc 0xd5ee8
MAC_ADDRESS(0x107e40, 0x16c)
static void initializeSSkillTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    TSSkillTraits& traits = g_sSkillTraitsStorage[id];

    DATA_COMPGEN_GUARD(0x00698b98, secondarySkillStringsGuard,
                      secondarySkillNames)
    DATA(0x00698b28)
    static TAutoStrPtr secondarySkillNames[28];

    secondarySkillNames[id].set(new char[strlen(values[0]) + 1]);
    strcpy(secondarySkillNames[id].get(), values[0]);
    traits.m_name = secondarySkillNames[id].get();

    DATA(0x00698b9c)
    static TAutoStrPtr secondarySkillLevelNames[28][3];

    int level;
    for (level = 0; level < 3; ++level) {
        secondarySkillLevelNames[id][level].set(
            new char[strlen(values[level + 1]) + 1]);
        strcpy(secondarySkillLevelNames[id][level].get(),
               values[level + 1]);
        traits.m_levelNames[level] =
            secondarySkillLevelNames[id][level].get();
    }
}
