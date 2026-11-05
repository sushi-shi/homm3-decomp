#!/usr/bin/env python3
"""Convert the HoMM2 combat creatures, heroes, missiles and effects into HoMM3 DEFs + a LOD.

Reads the HoMM2 Gold archive (heroes2.agg) and writes:

  build/h2mash.lod            HoMM3 LOD (zlib members) with the DEFs below
  build/h2mash-assets.json    per-DEF manifest + suggested CRANIM rows
  build/h2mash-preview/*.png  contact sheets (one row per DEF group)
  build/h2mash-defs/*.DEF     the loose DEF files (same bytes as in the LOD)

Formats (see docs/formats/lod-def-formats.md and the HoMM2 source tree):

* AGG: u16 count, count x {u32 hash, u32 offset, u32 size}; the names are
  a trailing table of count x 15 bytes (13-byte name + 2 pad).  Every name is
  verified against MAKEFILEID (Misc.cpp).
* ICN: u16 frames, u32 size, frames x {i16 x, i16 y, i16 w, i16 h, u8 type,
  u32 off}; RLE per Icon2b.cpp (0x00 EOL, 0x01-0x7f literal, 0x80 end,
  0x81-0xbf skip, 0xc0 shadow/outline, 0xc1 long fill, 0xc2-0xff fill).
  In the 0xc0 command, bit 0x40 = shadow (dim palette level (b&0x3c)>>2;
  only level 0, ~57% luminance, occurs) and bit 0x80 = outline pixel that is
  painted with the highlight colour when the stack is selected.
* *FRM.BIN: 821-byte SMonFrameInfo (include/SOURCE/ARMY.h).
* DEF: 0x310-byte header + groups; type 0x42 (creature) / 0x40 (missile)
  frames use the 32-byte cropped header and encoding 1 ("general RLE",
  CSpriteFrame::encodeGeneral): indices < 10 are control codes, runs of
  literal pixels use code 255.  HoMM3 creature control indices
  (CSpriteFrame::drawCreatureImpl): 0 transparent, 1 shadow 75%,
  4 shadow 50%, 5 selection outline (transparent unless selected),
  6 outline over 50% shadow, 7 outline over 75% shadow.
* LOD: 0x5c header ("LOD\\0", version 200, count), 0x20-byte entries
  {name[16], offset, size, attributes (=DEF type), csize}; LODFile::find binary-searches
  with _strcmpi, so entries are sorted case-insensitively.

The HoMM3 combat anchor is canvas (196, 267) = hex (refX, refY)
(army::drawToBuffer); HoMM2 draws ICN frames relative to (m_x, m_y), which is
the same hex point (gridLeft+22, gridTop+42) in both games, and both games use
44x42 hex steps.  So an ICN frame with offset (ox, oy) lands at canvas
(196+ox, 267+oy) unchanged.

Deterministic: no timestamps, fixed zlib level, sorted output.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
WORKTREE = HERE.parents[1]
DEFAULT_AGG = Path("/home/sheep/Projects/homm2/homm2-linked-game/DATA/heroes2.agg")

# ---------------------------------------------------------------------------
# HoMM3 conventions
# ---------------------------------------------------------------------------
CANVAS_W, CANVAS_H = 450, 400      # every retail creature DEF (CPKMAN, CCRUSD...)
ANCHOR_X, ANCHOR_Y = 196, 267      # army::drawToBuffer: x-196 / y-267
HEX_STEP_X = 44                    # cs_walk displacement over one hex
MISSILE_CANVAS = 30                # PLCBOWX.DEF is 30x30, 9 frames, type 0x40

DEF_TYPE_SPRITE = 0x40
DEF_TYPE_CREATURE = 0x42

IDX_TRANSPARENT = 0
IDX_SHADOW75 = 1
IDX_SHADOW50 = 4
IDX_OUTLINE = 5
IDX_OUTLINE_SHADOW50 = 6
# retail creature DEF palette slots 0..9 (identical in CPKMAN/CLCBOW/CCRUSD...)
SPECIAL_PALETTE = [
    (0, 255, 255), (255, 150, 255), (255, 100, 255), (255, 50, 255),
    (255, 0, 255), (255, 255, 0), (180, 0, 255), (0, 255, 0),
    (255, 128, 255), (255, 128, 255),
]

# HoMM3 creature_seqid (include/csprite.h)
CS_WALK, CS_FIDGET, CS_WAIT, CS_WINCE, CS_DEFEND, CS_DEATH = 0, 1, 2, 3, 4, 5
CS_TURN_RF, CS_TURN_FR, CS_TURN_LF, CS_TURN_FL = 7, 8, 9, 10
CS_ATTACK_UR, CS_ATTACK_R, CS_ATTACK_DR = 11, 12, 13
CS_RANGE_UR, CS_RANGE_R, CS_RANGE_DR = 14, 15, 16
CS_PREWALK, CS_POSTWALK = 20, 21
GROUP_NAMES = {
    0: "walk", 1: "fidget/mouse-over", 2: "standing", 3: "wince", 4: "defend",
    5: "death", 7: "turn r->f", 8: "turn f->r", 9: "turn l->f", 10: "turn f->l",
    11: "attack up", 12: "attack straight", 13: "attack down",
    14: "shoot up", 15: "shoot straight", 16: "shoot down",
    20: "start moving", 21: "stop moving",
}

# HoMM2 ARMY_ANIMATION_* (include/SOURCE/ARMY.h)
H2_WALK_BEGIN, H2_WALK_BEGIN_STANDING, H2_WALK_MIDDLE = 0, 1, 2
H2_WALK_END, H2_WALK_END_STANDING, H2_WALK_STAND = 3, 4, 5
H2_STAND, H2_IDLE_FIRST = 7, 8
H2_DEATH, H2_WINCE, H2_WINCE_RETURN = 13, 14, 15
H2_ATTACK_UP, H2_ATTACK_FWD, H2_ATTACK_DOWN = 16, 20, 24
H2_SHOOT_UP, H2_SHOOT_FWD, H2_SHOOT_DOWN = 28, 30, 32

# ---------------------------------------------------------------------------
# Creature table: all 66 HoMM2 creatures in HoMM2 id order (KB_TYPES.h
# CREATURE_PEASANT=0 .. CREATURE_WATER_ELEMENTAL=65); HoMM3 id = 150 + id.
# ICN = cMonFilename, BIN = cArmyFrameFileNames, snd = gMonsterDatabase
# spriteName, race/flags/speed = gMonsterDatabase (all KB.cpp).  The original
# HEROES2W.EXE agrees: orcs use the "elf_" samples and the bone dragon the
# "drgn" ones (ORC_*.82M / BONE*.82M ship in the AGG but are never loaded).
# base = the speed ModifyFrameInfo() (ARMY.cpp) compares against: the
# previous id for the upgraded units, the fire elemental for the other
# elementals, None when HoMM2 leaves the BIN timings alone.
# ---------------------------------------------------------------------------
K, B, S, W, Z, N, X = range(7)       # FACTION_KNIGHT .. FACTION_NECROMANCER, FACTION_NEUTRAL
RACE_NAMES = ["knight", "barbarian", "sorceress", "warlock", "wizard", "necromancer", "neutral"]
F_WIDE, F_FLY, F_SHOOT, F_BREATH, F_UNDEAD = 1, 2, 4, 8, 0x400      # MONSTER_FLAGS_*


class H2Creature:
    __slots__ = ("h2id", "key", "name", "icn", "bin", "def_name", "tag", "snd", "race", "flags",
                 "speed", "base_speed", "level")

    def __init__(self, h2id, key, name, icn, bin_name, def_name, tag, snd, race, flags, speed, base_speed,
                 level):
        self.h2id, self.key, self.name, self.icn, self.bin = h2id, key, name, icn, bin_name
        self.def_name, self.tag, self.snd, self.race, self.flags = def_name, tag, snd, race, flags
        self.speed, self.base_speed, self.level = speed, base_speed, level

    wide = property(lambda s: bool(s.flags & F_WIDE))
    flying = property(lambda s: bool(s.flags & F_FLY))
    shooter = property(lambda s: bool(s.flags & F_SHOOT))
    breath = property(lambda s: bool(s.flags & F_BREATH))
    undead = property(lambda s: bool(s.flags & F_UNDEAD))


_CREATURE_ROWS = [
    # key, display, ICN, BIN, DEF (8.3), frame tag (5), snd, race, flags, speed, base, HoMM3 level
    ("peasant", "Peasant", "PEASANT", "PEAS_FRM", "H2CPEAS", "H2PEA", "psnt", K, 0, 2, None, 0),
    ("archer", "Archer", "ARCHER", "ARCHRFRM", "H2CARCHR", "H2ARC", "arch", K, F_SHOOT, 2, None, 1),
    ("ranger", "Ranger", "ARCHER2", "ARCHRFRM", "H2CRANGR", "H2RNG", "arch", K, F_SHOOT, 4, 2, 1),
    ("pikeman", "Pikeman", "PIKEMAN", "PIKMNFRM", "H2CPIKE", "H2PIK", "pike", K, 0, 4, None, 2),
    ("veteran_pikeman", "Veteran Pikeman", "PIKEMAN2", "PIKMNFRM", "H2CVPIKE", "H2VPK", "pike", K, 0, 5, 4, 2),
    ("swordsman", "Swordsman", "SWORDSMN", "SWRDSFRM", "H2CSWRD", "H2SWD", "swdm", K, 0, 4, None, 3),
    ("master_swordsman", "Master Swordsman", "SWORDSM2", "SWRDSFRM", "H2CMSWRD", "H2MSW", "swdm", K, 0, 5, 4, 3),
    ("cavalry", "Cavalry", "CAVALRYR", "CVLRYFRM", "H2CCAVLR", "H2CAV", "cavl", K, F_WIDE, 6, None, 4),
    ("champion", "Champion", "CAVALRYB", "CVLR2FRM", "H2CCHAMP", "H2CHA", "cavl", K, F_WIDE, 7, 6, 4),
    ("paladin", "Paladin", "PALADIN", "PALADFRM", "H2CPALAD", "H2PAL", "pldn", K, 0, 5, None, 5),
    ("crusader", "Crusader", "PALADIN2", "PALADFRM", "H2CCRUSD", "H2CRU", "pldn", K, 0, 6, 5, 5),
    ("goblin", "Goblin", "GOBLIN", "GOBLNFRM", "H2CGOBLN", "H2GOB", "gbln", B, 0, 4, None, 0),
    ("orc", "Orc", "ORC", "ORC__FRM", "H2CORC", "H2ORC", "elf_", B, F_SHOOT, 2, None, 1),
    ("orc_chief", "Orc Chief", "ORC2", "ORC__FRM", "H2CORCCH", "H2ORK", "elf_", B, F_SHOOT, 3, 2, 1),
    ("wolf", "Wolf", "WOLF", "WOLF_FRM", "H2CWOLF", "H2WLF", "wolf", B, F_WIDE, 6, None, 2),
    ("ogre", "Ogre", "OGRE", "OGRE_FRM", "H2COGRE", "H2OGR", "ogre", B, 0, 2, None, 3),
    ("ogre_lord", "Ogre Lord", "OGRE2", "OGRE_FRM", "H2COGRLD", "H2OGL", "ogre", B, 0, 4, 2, 3),
    ("troll", "Troll", "TROLL", "TROLLFRM", "H2CTROLL", "H2TRL", "trll", B, F_SHOOT, 4, None, 4),
    ("war_troll", "War Troll", "TROLL2", "TROLLFRM", "H2CWTROL", "H2WTR", "trll", B, F_SHOOT, 5, 4, 4),
    ("cyclops", "Cyclops", "CYCLOPS", "CYCLOFRM", "H2CCYCLO", "H2CYC", "cycl", B, F_BREATH, 5, None, 5),
    ("sprite", "Sprite", "SPRITE", "SPRITFRM", "H2CSPRIT", "H2SPR", "sprt", S, F_FLY, 4, None, 0),
    ("dwarf", "Dwarf", "DWARF", "DWARFFRM", "H2CDWARF", "H2DWF", "dwrf", S, 0, 2, None, 1),
    ("battle_dwarf", "Battle Dwarf", "DWARF2", "DWARFFRM", "H2CBDWRF", "H2BDW", "dwrf", S, 0, 4, 2, 1),
    ("elf", "Elf", "ELF", "ELF__FRM", "H2CELF", "H2ELF", "elf_", S, F_SHOOT, 4, None, 2),
    ("grand_elf", "Grand Elf", "ELF2", "ELF__FRM", "H2CGELF", "H2GEL", "elf_", S, F_SHOOT, 6, 4, 2),
    ("druid", "Druid", "DRUID", "DRUIDFRM", "H2CDRUID", "H2DRU", "drui", S, F_SHOOT, 5, None, 3),
    ("greater_druid", "Greater Druid", "DRUID2", "DRUIDFRM", "H2CGDRUI", "H2GDR", "drui", S, F_SHOOT, 6, 5, 3),
    ("unicorn", "Unicorn", "UNICORN", "UNICOFRM", "H2CUNICO", "H2UNI", "unic", S, F_WIDE, 5, None, 4),
    ("phoenix", "Phoenix", "PHOENIX", "PHOENFRM", "H2CPHOEN", "H2PHX", "phoe", S, F_WIDE | F_FLY | F_BREATH, 7, None, 5),
    ("centaur", "Centaur", "CENTAUR", "CENTRFRM", "H2CCENTR", "H2CEN", "cntr", W, F_WIDE | F_SHOOT, 4, None, 0),
    ("gargoyle", "Gargoyle", "GARGOYLE", "GARGLFRM", "H2CGARGL", "H2GRG", "garg", W, F_FLY, 6, None, 1),
    ("griffin", "Griffin", "GRIFFIN", "GRIFFFRM", "H2CGRIFF", "H2GRF", "grif", W, F_WIDE | F_FLY, 4, None, 2),
    ("minotaur", "Minotaur", "MINOTAUR", "MINOTFRM", "H2CMINOT", "H2MIN", "mino", W, 0, 4, None, 3),
    ("minotaur_king", "Minotaur King", "MINOTAU2", "MINOTFRM", "H2CMINOK", "H2MNK", "mino", W, 0, 6, 4, 3),
    ("hydra", "Hydra", "HYDRA", "HYDRAFRM", "H2CHYDRA", "H2HYD", "hydr", W, F_WIDE, 2, None, 4),
    ("green_dragon", "Green Dragon", "DRAGGREE", "DRAGGFRM", "H2CDRGGR", "H2DGG", "drgn", W, F_WIDE | F_FLY | F_BREATH, 4, None, 5),
    ("red_dragon", "Red Dragon", "DRAGRED", "DRAGRFRM", "H2CDRGRD", "H2DGR", "drgn", W, F_WIDE | F_FLY | F_BREATH, 5, None, 5),
    ("black_dragon", "Black Dragon", "DRAGBLAK", "DRAGBFRM", "H2CDRGBK", "H2DGB", "drgn", W, F_WIDE | F_FLY | F_BREATH, 6, None, 5),
    ("halfling", "Halfling", "HALFLING", "HALFLFRM", "H2CHALFL", "H2HAL", "half", Z, F_SHOOT, 3, None, 0),
    ("boar", "Boar", "BOAR", "BOAR_FRM", "H2CBOAR", "H2BOR", "boar", Z, F_WIDE, 6, None, 1),
    ("iron_golem", "Iron Golem", "GOLEM", "GOLEMFRM", "H2CIGOLM", "H2IGL", "golm", Z, 0, 2, None, 2),
    ("steel_golem", "Steel Golem", "GOLEM2", "GOLEMFRM", "H2CSGOLM", "H2SGL", "golm", Z, 0, 3, 2, 2),
    ("roc", "Roc", "ROC", "ROC__FRM", "H2CROC", "H2ROC", "roc_", Z, F_WIDE | F_FLY, 4, None, 3),
    ("mage", "Mage", "MAGE1", "MAGE1FRM", "H2CMAGE", "H2MAG", "mage", Z, F_SHOOT, 5, None, 4),
    ("archmage", "Archmage", "MAGE2", "MAGE1FRM", "H2CAMAGE", "H2AMG", "mage", Z, F_SHOOT, 6, 5, 4),
    ("giant", "Giant", "TITANBLU", "TITANFRM", "H2CGIANT", "H2GIA", "titn", Z, 0, 4, None, 5),
    ("titan", "Titan", "TITANBLA", "TITA2FRM", "H2CTITAN", "H2TIT", "titn", Z, F_SHOOT, 6, None, 5),
    ("skeleton", "Skeleton", "SKELETON", "SKEL_FRM", "H2CSKEL", "H2SKL", "skel", N, F_UNDEAD, 4, None, 0),
    ("zombie", "Zombie", "ZOMBIE", "ZOMB_FRM", "H2CZOMB", "H2ZOM", "zomb", N, F_UNDEAD, 2, None, 1),
    ("mutant_zombie", "Mutant Zombie", "ZOMBIE2", "ZOMB_FRM", "H2CMZOMB", "H2MZM", "zomb", N, F_UNDEAD, 4, 2, 1),
    ("mummy", "Mummy", "MUMMYW", "MUMMYFRM", "H2CMUMMY", "H2MUM", "mumy", N, F_UNDEAD, 4, None, 2),
    ("royal_mummy", "Royal Mummy", "MUMMY2", "MUMMYFRM", "H2CRMUMY", "H2RMM", "mumy", N, F_UNDEAD, 5, 4, 2),
    ("vampire", "Vampire", "VAMPIRE", "VAMPIFRM", "H2CVAMP", "H2VAM", "vamp", N, F_FLY | F_UNDEAD, 4, None, 3),
    ("vampire_lord", "Vampire Lord", "VAMPIRE2", "VAMPIFRM", "H2CVAMPL", "H2VML", "vamp", N, F_FLY | F_UNDEAD, 5, 4, 3),
    ("lich", "Lich", "LICH", "LICH_FRM", "H2CLICH", "H2LCH", "lich", N, F_SHOOT | F_UNDEAD, 5, None, 4),
    ("power_lich", "Power Lich", "LICH2", "LICH_FRM", "H2CPLICH", "H2PLC", "lich", N, F_SHOOT | F_UNDEAD, 6, 5, 4),
    ("bone_dragon", "Bone Dragon", "DRAGBONE", "DRABNFRM", "H2CDRGBN", "H2DGN", "drgn", N, F_WIDE | F_FLY | F_UNDEAD, 4, None, 5),
    ("rogue", "Rogue", "ROGUE", "ROGUEFRM", "H2CROGUE", "H2ROG", "rogu", X, 0, 5, None, 1),
    ("nomad", "Nomad", "NOMAD", "NOMADFRM", "H2CNOMAD", "H2NOM", "nmad", X, F_WIDE, 6, None, 2),
    ("ghost", "Ghost", "GHOST", "GHOSTFRM", "H2CGHOST", "H2GHO", "ghst", X, F_FLY | F_UNDEAD, 5, None, 3),
    ("genie", "Genie", "GENIE", "GENIEFRM", "H2CGENIE", "H2GEN", "geni", X, F_FLY, 6, None, 5),
    ("medusa", "Medusa", "MEDUSA", "MEDUSFRM", "H2CMEDUS", "H2MED", "meds", X, F_WIDE, 4, None, 3),
    ("earth_elemental", "Earth Elemental", "EELEM", "FELEMFRM", "H2CEELEM", "H2EEL", "eelm", X, 0, 3, 5, 3),
    ("air_elemental", "Air Elemental", "AELEM", "FELEMFRM", "H2CAELEM", "H2AEL", "aelm", X, 0, 6, 5, 3),
    ("fire_elemental", "Fire Elemental", "FELEM", "FELEMFRM", "H2CFELEM", "H2FEL", "felm", X, 0, 5, None, 3),
    ("water_elemental", "Water Elemental", "WELEM", "FELEMFRM", "H2CWELEM", "H2WEL", "welm", X, 0, 4, 5, 3),
]
CREATURES = [H2Creature(i, key, name, icn + ".ICN", bin_name + ".BIN", def_name + ".DEF", tag, snd, race,
                        flags, speed, base, level)
             for i, (key, name, icn, bin_name, def_name, tag, snd, race, flags, speed, base, level)
             in enumerate(_CREATURE_ROWS)]
assert len(CREATURES) == 66
REFERENCE_DEFS = ["CPKMAN.DEF", "CLCBOW.DEF", "CSWORD.DEF", "CCAVLR.DEF", "CCRUSD.DEF"]

# HoMM2 missiles (army::LoadResources picks one per shooter type).
MISSILES = [
    # key, display, ICN, DEF, frame tag
    ("arrow", "Archer/Ranger arrow", "ARCH_MSL.ICN", "H2CARROW.DEF", "H2ARW"),
    ("lich", "Lich/Power Lich cloud ball", "LICH_MSL.ICN", "H2MLICH.DEF", "H2MLI"),
    ("elf", "Elf arrow (default: elves, centaurs, mages)", "ELF__MSL.ICN", "H2MELF.DEF", "H2MEL"),
    ("orc", "Orc/Orc Chief arrow", "ORC__MSL.ICN", "H2MORC.DEF", "H2MOR"),
    ("druid", "Druid/Greater Druid bolt", "DRUIDMSL.ICN", "H2MDRUID.DEF", "H2MDR"),
    ("troll", "Troll/War Troll rock", "TROLLMSL.ICN", "H2MTROLL.DEF", "H2MTR"),
    ("halfling", "Halfling stone", "HALFLMSL.ICN", "H2MHALF.DEF", "H2MHA"),
    ("titan", "Giant/Titan lightning", "TITANMSL.ICN", "H2MTITAN.DEF", "H2MTI"),
]
MISSILE_BY_ICN = {m[2]: m for m in MISSILES}


def missile_icn(c: H2Creature):
    """army::LoadResources (ARMY.cpp) missile choice; None for non-shooters."""
    if not c.shooter:
        return None
    if c.key in ("giant", "titan"):
        return "TITANMSL.ICN"
    if c.key == "halfling":
        return "HALFLMSL.ICN"
    if c.key in ("archer", "ranger"):
        return "ARCH_MSL.ICN"
    if c.key in ("lich", "power_lich"):
        return "LICH_MSL.ICN"
    if c.key in ("orc", "orc_chief"):
        return "ORC__MSL.ICN"
    if c.key in ("druid", "greater_druid"):
        return "DRUIDMSL.ICN"
    if c.key in ("troll", "war_troll"):
        return "TROLLMSL.ICN"
    return "ELF__MSL.ICN"


def missile_def(c: H2Creature):
    icn = missile_icn(c)
    return MISSILE_BY_ICN[icn][3] if icn else None


# ---------------------------------------------------------------------------
# HoMM2 readers
# ---------------------------------------------------------------------------
def make_file_id(name: str) -> int:
    """MAKEFILEID (src/BASE/Misc.cpp)."""
    fid = 0
    total = 0
    for ch in reversed(name.upper().encode("ascii")):
        fid = ((fid << 5) + (fid >> 25)) & 0xFFFFFFFF
        total += ch
        fid = (fid + ch + total) & 0xFFFFFFFF
    return fid


class Agg:
    def __init__(self, path: Path):
        self.data = path.read_bytes()
        d = self.data
        (count,) = struct.unpack_from("<H", d, 0)
        table_off = len(d) - 15 * count
        self.entries: dict[str, tuple[int, int]] = {}
        for i in range(count):
            fid, off, size = struct.unpack_from("<3I", d, 2 + 12 * i)
            raw = d[table_off + 15 * i: table_off + 15 * i + 13]
            name = raw.split(b"\0")[0].decode("ascii").upper()
            if make_file_id(name) != fid:
                raise ValueError(f"AGG name/hash mismatch for {name}")
            if off + size > len(d):
                raise ValueError(f"AGG member {name} out of range")
            self.entries[name] = (off, size)

    def get(self, name: str) -> bytes:
        off, size = self.entries[name.upper()]
        return self.data[off: off + size]


def read_palette(agg: Agg) -> list[tuple[int, int, int]]:
    raw = agg.get("KB.PAL")
    # 6-bit VGA components -> 8 bit with full range replication
    return [tuple(((raw[3 * i + j] & 63) << 2) | ((raw[3 * i + j] & 63) >> 4) for j in range(3))
            for i in range(256)]


# decoded pixel values: -1 transparent, 0..255 colour, plus the specials below
PX_SHADOW = 256
PX_OUTLINE = 257
PX_OUTLINE_SHADOW = 258


class IcnFrame:
    __slots__ = ("x", "y", "w", "h", "pixels")

    def __init__(self, x, y, w, h, pixels):
        self.x, self.y, self.w, self.h, self.pixels = x, y, w, h, pixels


def read_icn(agg: Agg, name: str) -> list[IcnFrame]:
    d = agg.get(name)
    count, total = struct.unpack_from("<HI", d, 0)
    frames = []
    for i in range(count):
        x, y, w, h, ftype, off = struct.unpack_from("<hhhhBI", d, 6 + 13 * i)
        if ftype & 0x20:
            raise ValueError(f"{name}[{i}]: monochrome ICN frames are not supported")
        px = [-1] * (w * h)
        p = 6 + off
        cx = cy = 0

        def put(val, n):
            nonlocal cx
            for _ in range(n):
                if 0 <= cx < w and 0 <= cy < h:
                    px[cy * w + cx] = val
                cx += 1

        while True:
            c = d[p]
            p += 1
            if c == 0x80:
                break
            if c == 0x00:
                cx = 0
                cy += 1
            elif c < 0x80:
                for k in range(c):
                    put(d[p + k], 1)
                p += c
            elif c < 0xC0:
                cx += c & 0x3F
            elif c == 0xC0:
                b = d[p]
                p += 1
                n = b & 3
                if not n:
                    n = d[p]
                    p += 1
                shadow = bool(b & 0x40)
                outline = bool(b & 0x80)
                if outline and shadow:
                    put(PX_OUTLINE_SHADOW, n)
                elif outline:
                    put(PX_OUTLINE, n)
                elif shadow:
                    put(PX_SHADOW, n)
                else:
                    cx += n
            elif c == 0xC1:
                n = d[p]
                put(d[p + 1], n)
                p += 2
            else:
                put(d[p], c & 0x3F)
                p += 1
        frames.append(IcnFrame(x, y, w, h, px))
    return frames


def read_frame_info(agg: Agg, name: str) -> dict:
    d = agg.get(name)
    if len(d) != 821:
        raise ValueError(f"{name}: expected 821-byte SMonFrameInfo, got {len(d)}")
    r: dict = {}
    r["spell_effect"] = list(struct.unpack_from("<hh", d, 1))
    p = 5
    r["walk_x_offsets"] = [list(struct.unpack_from("<16b", d, p + 16 * i)) for i in range(6)]
    p += 96 + 16
    r["idle_count"] = d[p]
    p += 1
    r["idle_chances"] = [round(v, 4) for v in struct.unpack_from("<10f", d, p)]
    p += 40
    r["stand_still_delay"], r["walk_duration"], r["attack_duration"], r["flight_speed"] = \
        struct.unpack_from("<4i", d, p)
    p += 16
    r["missile_offsets"] = [list(struct.unpack_from("<hh", d, p + 4 * i)) for i in range(3)]
    p += 12
    r["projectile_dirs"] = d[p]
    p += 1
    r["projectile_angles"] = [round(v, 4) for v in struct.unpack_from("<12f", d, p)]
    p += 48
    r["quantity_x"] = list(struct.unpack_from("<2i", d, p))
    p += 8
    counts = struct.unpack_from("<34b", d, p)
    p += 34
    r["seq"] = [list(struct.unpack_from("<16b", d, p + 16 * i))[: max(0, counts[i])] for i in range(34)]
    return r


# ---------------------------------------------------------------------------
# Palette mapping HoMM2 -> DEF
# ---------------------------------------------------------------------------
def build_def_palette(h2pal):
    pal = list(SPECIAL_PALETTE) + [h2pal[i] for i in range(10, 256)]
    remap = list(range(256))
    for i in range(10):
        # HoMM2 0..9 are all black system colours; map to nearest real colour
        best = min(range(10, 256), key=lambda j: (sum((a - b) ** 2 for a, b in zip(h2pal[i], h2pal[j])), j))
        remap[i] = best
    return pal, remap


def to_def_index(v: int, remap) -> int:
    if v < 0:
        return IDX_TRANSPARENT
    if v == PX_SHADOW:
        return IDX_SHADOW50
    if v == PX_OUTLINE:
        return IDX_OUTLINE
    if v == PX_OUTLINE_SHADOW:
        return IDX_OUTLINE_SHADOW50
    return remap[v]


# ---------------------------------------------------------------------------
# DEF frame encoding (encoding 1, CSpriteFrame::encodeGeneral)
# ---------------------------------------------------------------------------
def encode_general(rows: list[bytes]) -> bytes:
    h = len(rows)
    offsets = []
    body = bytearray()
    for row in rows:
        offsets.append(4 * h + len(body))
        x = 0
        w = len(row)
        while x < w:
            code = row[x] if row[x] < 10 else 255
            run = 1
            while x + run < w and run < 256:
                nxt = row[x + run] if row[x + run] < 10 else 255
                if nxt != code:
                    break
                run += 1
            body.append(code)
            body.append(run - 1)
            if code == 255:
                body += row[x: x + run]
            x += run
    return struct.pack(f"<{h}I", *offsets) + bytes(body)


def decode_general(data: bytes, w: int, h: int) -> list[bytearray]:
    rows = []
    for y in range(h):
        (off,) = struct.unpack_from("<I", data, 4 * y)
        row = bytearray()
        p = off
        while len(row) < w:
            code, run = data[p], data[p + 1] + 1
            p += 2
            if code == 255:
                row += data[p: p + run]
                p += run
            else:
                row += bytes([code]) * run
        if len(row) != w:
            raise ValueError("run crosses row end")
        rows.append(row)
    return rows


class DefFrame:
    """A full-canvas frame stored cropped."""

    def __init__(self, name, full_w, full_h, cx, cy, rows):
        self.name, self.full_w, self.full_h = name, full_w, full_h
        self.cx, self.cy, self.rows = cx, cy, rows

    @property
    def cw(self):
        return len(self.rows[0]) if self.rows else 0

    @property
    def ch(self):
        return len(self.rows)

    def encode(self) -> bytes:
        data = encode_general(self.rows)
        return struct.pack("<8I", len(data), 1, self.full_w, self.full_h,
                           self.cw, self.ch, self.cx, self.cy) + data


CLIPPED_PIXELS: list = []


def icn_center_x(fr: IcnFrame) -> float:
    xs = [i % fr.w for i, v in enumerate(fr.pixels) if 0 <= v < 256]
    return fr.x + (min(xs) + max(xs)) / 2 if xs else fr.x + fr.w / 2


def place_icn_frame(fr: IcnFrame, remap, full_w, full_h, ox, oy):
    """Return (cx, cy, rows) of the cropped frame placed at origin (ox, oy)."""
    pts = {}
    for yy in range(fr.h):
        for xx in range(fr.w):
            v = fr.pixels[yy * fr.w + xx]
            if v < 0:
                continue
            idx = to_def_index(v, remap)
            if idx == IDX_TRANSPARENT:
                continue
            X, Y = ox + fr.x + xx, oy + fr.y + yy
            if not (0 <= X < full_w and 0 <= Y < full_h):
                # SWORDSMN/SWORDSM2 frame 15 carries two stray pixels ~230px
                # up-left of the body; anything off the HoMM3 canvas is dropped.
                CLIPPED_PIXELS.append((fr.x, fr.y, xx, yy))
                continue
            pts[(X, Y)] = idx
    if not pts:
        # retail crop() of an empty map keeps the full extent; use a 1x1 hole
        return ox, oy, [bytes([0])]
    x0 = min(p[0] for p in pts)
    x1 = max(p[0] for p in pts)
    y0 = min(p[1] for p in pts)
    y1 = max(p[1] for p in pts)
    rows = []
    for Y in range(y0, y1 + 1):
        rows.append(bytes(pts.get((X, Y), 0) for X in range(x0, x1 + 1)))
    return x0, y0, rows


def build_def(def_type, full_w, full_h, palette, groups) -> bytes:
    """groups: list of (gid, [DefFrame]) ; frames with equal names share data."""
    head = bytearray(struct.pack("<4I", def_type, full_w, full_h, len(groups)))
    for c in palette:
        head += bytes(c)
    assert len(head) == 0x310
    tables_len = sum(16 + 17 * len(frames) for _, frames in groups)
    payload_off = 0x310 + tables_len
    blobs = bytearray()
    offsets_by_name: dict[str, int] = {}
    tables = bytearray()
    for gid, frames in groups:
        tables += struct.pack("<4I", gid, len(frames), 0, 0)
        for fr in frames:
            nm = fr.name.encode("ascii")
            assert len(nm) <= 12
            tables += nm + b"\0" * (13 - len(nm))
        for fr in frames:
            if fr.name not in offsets_by_name:
                offsets_by_name[fr.name] = payload_off + len(blobs)
                blobs += fr.encode()
            tables += struct.pack("<I", offsets_by_name[fr.name])
    return bytes(head + tables + blobs)


def parse_def(b: bytes) -> dict:
    """Independent reader used for round-trip validation."""
    t, w, h, ng = struct.unpack_from("<4I", b, 0)
    pal = [tuple(b[16 + 3 * i: 19 + 3 * i]) for i in range(256)]
    p = 0x310
    groups = []
    for _ in range(ng):
        gid, nf, _u1, _u2 = struct.unpack_from("<4I", b, p)
        p += 16
        names = [b[p + 13 * i: p + 13 * i + 13].split(b"\0")[0].decode() for i in range(nf)]
        p += 13 * nf
        offs = struct.unpack_from(f"<{nf}I", b, p)
        p += 4 * nf
        frames = []
        for nm, o in zip(names, offs):
            size, enc, fw, fh, cw, ch, cx, cy = struct.unpack_from("<8I", b, o)
            if enc != 1:
                raise ValueError(f"unexpected encoding {enc}")
            data = b[o + 32: o + 32 + size]
            if len(data) != size:
                raise ValueError("frame data truncated")
            rows = decode_general(data, cw, ch)
            frames.append(dict(name=nm, offset=o, size=size, full=(fw, fh),
                               crop=(cx, cy, cw, ch), rows=rows))
        groups.append(dict(id=gid, frames=frames))
    return dict(type=t, width=w, height=h, palette=pal, groups=groups)


# ---------------------------------------------------------------------------
# LOD writer / reader
# ---------------------------------------------------------------------------
def build_lod(members: dict[str, bytes], version=200) -> bytes:
    names = sorted(members, key=lambda n: n.lower())
    header = bytearray(b"LOD\0" + struct.pack("<II", version, len(names)))
    header += b"\0" * (0x5C - len(header))
    dir_len = 0x20 * len(names)
    off = 0x5C + dir_len
    directory = bytearray()
    payload = bytearray()
    for nm in names:
        raw = members[nm]
        comp = zlib.compress(raw, 9)
        enc = nm.encode("ascii")
        assert len(enc) < 16
        # retail H3sprite.lod (version 200) stores the DEF type as the attribute
        # and 16/17 for 8-bit/24-bit LOD bitmaps (H3bitmap.lod)
        if nm.upper().endswith(".DEF"):
            attr = struct.unpack_from("<I", raw, 0)[0]
        elif nm.upper().endswith(".PCX"):
            size, w, h = struct.unpack_from("<3I", raw, 0)
            attr = 16 if size == w * h else 17
        else:
            attr = 0
        directory += enc + b"\0" * (16 - len(enc))
        directory += struct.pack("<4I", off + len(payload), len(raw), attr, len(comp))
        payload += comp
    return bytes(header + directory + payload)


def read_lod(b: bytes) -> dict[str, bytes]:
    if b[:4] != b"LOD\0":
        raise ValueError("not a LOD")
    _ver, count = struct.unpack_from("<II", b, 4)
    out = {}
    prev = None
    for i in range(count):
        e = 0x5C + 0x20 * i
        nm = b[e: e + 16].split(b"\0")[0].decode()
        off, size, _attr, csize = struct.unpack_from("<4I", b, e + 16)
        if prev is not None and prev.lower() > nm.lower():
            raise ValueError("LOD directory not sorted")
        prev = nm
        raw = b[off: off + (csize or size)]
        data = zlib.decompress(raw) if csize else raw
        if len(data) != size:
            raise ValueError(f"{nm}: size mismatch")
        out[nm] = data
    return out


def lod_member(path: Path, name: str) -> bytes | None:
    """Extract one member from a retail LOD (reference comparison only)."""
    if not path.is_file():
        return None
    with path.open("rb") as f:
        head = f.read(0x5C)
        (count,) = struct.unpack_from("<I", head, 8)
        directory = f.read(0x20 * count)
        for i in range(count):
            e = 0x20 * i
            if directory[e: e + 16].split(b"\0")[0].decode().lower() == name.lower():
                off, size, _attr, csize = struct.unpack_from("<4I", directory, e + 16)
                f.seek(off)
                raw = f.read(csize or size)
                return zlib.decompress(raw) if csize else raw
    return None


# ---------------------------------------------------------------------------
# Conversion
# ---------------------------------------------------------------------------
class FramePool:
    """Assigns globally unique frame names (ResourceManager caches frames by name)."""

    def __init__(self, tag, icn, remap, full_w, full_h, ox, oy):
        self.tag, self.icn, self.remap = tag, icn, remap
        self.full_w, self.full_h, self.ox, self.oy = full_w, full_h, ox, oy
        self.cache: dict[tuple[int, int], DefFrame] = {}

    def get(self, icn_index: int, dx: int = 0) -> DefFrame:
        key = (icn_index, dx)
        if key not in self.cache:
            name = f"{self.tag}{len(self.cache):03d}.pcx"
            cx, cy, rows = place_icn_frame(self.icn[icn_index], self.remap, self.full_w,
                                           self.full_h, self.ox + dx, self.oy)
            self.cache[key] = DefFrame(name, self.full_w, self.full_h, cx, cy, rows)
        return self.cache[key]


def convert_creature(agg, c: H2Creature, palette, remap):
    key, display, icn_name, bin_name, def_name, tag = c.key, c.name, c.icn, c.bin, c.def_name, c.tag
    shooter, wide, speed, base_speed = c.shooter, c.wide, c.speed, c.base_speed
    icn = read_icn(agg, icn_name)
    info = read_frame_info(agg, bin_name)
    seq = info["seq"]
    pool = FramePool(tag, icn, remap, CANVAS_W, CANVAS_H, ANCHOR_X, ANCHOR_Y)
    groups = []
    mapping = {}

    def add(gid, parts, desc, frames_dx=None):
        frames = []
        for i, fi in enumerate(parts):
            dx = frames_dx[i] if frames_dx else 0
            frames.append(pool.get(fi, dx))
        if frames:
            groups.append((gid, frames))
            mapping[gid] = desc

    stand = seq[H2_STAND] or [1]

    if c.flying:
        # Flyers: HoMM2 army::FlyTo and HoMM3 army::fly are the same loop:
        # straight hops of flightSpeed / m_flightPixelSpan pixels, one pass of
        # the walk frames per hop, the sprite moved by the engine (cs_walk has
        # no per-frame displacement for flyers in army::drawToBuffer either).
        # Take-off (walk-begin) plays on the source hex before the hops, the
        # landing (walk-end-standing) on the destination hex after them; the
        # flyers' walk x-offsets are all zero, so no frame is shifted.
        walk = seq[H2_WALK_MIDDLE] or seq[H2_WALK_STAND] or stand
        add(CS_WALK, walk, "h2 seq 2 walk-middle = flight loop (no shift: the engine moves flyers)")
    else:
        # cs_walk: HoMM2 bakes the one-hex advance into the walk-middle frames and
        # draws them without engine displacement; HoMM3 adds i*44/n itself, so pull
        # each frame back by the same amount to reproduce the HoMM2 positions.
        walk = seq[H2_WALK_MIDDLE] or seq[H2_WALK_STAND] or stand
        n = len(walk)
        # When HoMM2 plays walk-begin frames first (swordsmen), the middle cycle
        # starts already part-way across the hex; recentre it on the standing pose
        # using the mean sprite centre so the HoMM3 loop stays on its hex.
        lead = 0
        if seq[H2_WALK_BEGIN]:
            mean = sum(icn_center_x(icn[f]) - i * HEX_STEP_X / n for i, f in enumerate(walk)) / n
            lead = round(mean - icn_center_x(icn[stand[0]]))
        add(CS_WALK, walk, f"h2 seq 2 walk-middle (frame i shifted by -i*44/n - {lead})",
            [-(i * HEX_STEP_X // n) - lead for i in range(n)])

    idles = [seq[H2_IDLE_FIRST + i] for i in range(min(5, info["idle_count"])) if seq[H2_IDLE_FIRST + i]]
    fidget = max(idles, key=len) if idles else stand
    fidget_src = (H2_IDLE_FIRST + idles.index(fidget)) if idles else H2_STAND
    add(CS_FIDGET, fidget, f"h2 seq {fidget_src} (longest idle)")
    add(CS_WAIT, stand, "h2 seq 7 static")
    wince = seq[H2_WINCE] + seq[H2_WINCE_RETURN]
    add(CS_WINCE, wince or stand, "h2 seq 14+15 wince+return")
    # HoMM2 has no defend pose: hold the wince pose
    hold = seq[H2_WINCE] + seq[H2_WINCE][-1:] * 2 + seq[H2_WINCE_RETURN] if seq[H2_WINCE] else stand
    add(CS_DEFEND, hold, "h2 seq 14 (+last frame held x2) +15")
    add(CS_DEATH, seq[H2_DEATH] or stand, "h2 seq 13 death")
    for gid in (CS_TURN_RF, CS_TURN_FR, CS_TURN_LF, CS_TURN_FL):
        add(gid, stand[:1], "h2 seq 7 static (HoMM2 has no turn)")
    # Melee: HoMM2 attack (16/20/24) + return (17/21/25).  The breath
    # variants of the breath-attack creatures (18/19, 22/23, 26/27: cyclops,
    # phoenix, dragons) are the same strike aimed at two hexes; HoMM3 plays
    # its normal attack groups for a breath attack, so they map there too.
    for gid, h2 in ((CS_ATTACK_UR, H2_ATTACK_UP), (CS_ATTACK_R, H2_ATTACK_FWD), (CS_ATTACK_DR, H2_ATTACK_DOWN)):
        frames = seq[h2] + seq[h2 + 1]
        if not frames:  # fall back to the straight attack
            frames = seq[H2_ATTACK_FWD] + seq[H2_ATTACK_FWD + 1]
        add(gid, frames or stand, f"h2 seq {h2}+{h2 + 1} attack+return")
    if shooter:
        for gid, h2 in ((CS_RANGE_UR, H2_SHOOT_UP), (CS_RANGE_R, H2_SHOOT_FWD), (CS_RANGE_DR, H2_SHOOT_DOWN)):
            add(gid, seq[h2] + seq[h2 + 1], f"h2 seq {h2}+{h2 + 1} shoot+return")
    post = seq[H2_WALK_END_STANDING] or seq[H2_WALK_END]
    post_src = H2_WALK_END_STANDING if seq[H2_WALK_END_STANDING] else H2_WALK_END
    if c.flying:
        if seq[H2_WALK_BEGIN]:
            add(CS_PREWALK, seq[H2_WALK_BEGIN], "h2 seq 0 walk-begin = take-off on the source hex")
        if post:
            add(CS_POSTWALK, post, f"h2 seq {post_src} walk-end = landing on the destination hex")
    else:
        # start/stop moving are drawn on the hex without engine displacement;
        # HoMM2's walk-begin/end frames carry their own advance (walk_x_offsets).
        if seq[H2_WALK_BEGIN]:
            add(CS_PREWALK, seq[H2_WALK_BEGIN], "h2 seq 0 walk-begin (minus x-offset)",
                [-info["walk_x_offsets"][H2_WALK_BEGIN][i] for i in range(len(seq[H2_WALK_BEGIN]))])
        # HoMM2 appends walk-end(-standing) to the last hex's cycle, i.e. drawn
        # relative to the hex being left; HoMM3 draws cs_postwalk on the arrival hex.
        if post:
            add(CS_POSTWALK, post, f"h2 seq {post_src} walk-end (shifted -44 onto the arrival hex)",
                [-HEX_STEP_X] * len(post))

    groups.sort(key=lambda g: g[0])
    def_bytes = build_def(DEF_TYPE_CREATURE, CANVAS_W, CANVAS_H, palette, groups)

    # Suggested CRANIM row (HoMM3 units: fidget*9000 ms, walk/attack*500 ms).
    walk_ms, attack_ms = info["walk_duration"], info["attack_duration"]
    if base_speed is not None:  # HoMM2 ModifyFrameInfo
        diff = speed - base_speed
        attack_ms = int(attack_ms * 0.78) if key == "ranger" else int(attack_ms * (1.0 - diff * 0.08))
        walk_ms = int(walk_ms * (1.0 - diff * 0.12))
    angles = info["projectile_angles"][: info["projectile_dirs"]] if shooter else []
    cranim = {
        "time_between_fidgets": round(info["stand_still_delay"] / 9000.0, 2),
        "walk_animation_time": round(walk_ms / 500.0, 2),
        "attack_animation_time": round(attack_ms / 500.0, 2),
        # HoMM2 SMonFrameInfo.flightSpeed == HoMM3 m_flightPixelSpan (pixels per hop)
        "flight_animation_distance": float(info["flight_speed"]) if c.flying else 0.0,
        "missile_offsets_upper_right": info["missile_offsets"][0] if shooter else [0, 0],
        "missile_offsets_right": info["missile_offsets"][1] if shooter else [0, 0],
        "missile_offsets_lower_right": info["missile_offsets"][2] if shooter else [0, 0],
        "missile_frame_angles": (angles + [0.0] * 12)[:12],
        "troop_count_location_offset": 0,
        "attack_climax_frame": len(seq[H2_SHOOT_FWD]) if shooter else 0,
    }
    meta = {
        "key": key, "name": display, "def": def_name, "type": DEF_TYPE_CREATURE,
        "h2_id": c.h2id, "h3_id": H2_CREATURE_BASE + c.h2id, "race": RACE_NAMES[c.race],
        "source": {"icn": icn_name, "bin": bin_name, "icn_frames": len(icn)},
        "wide": wide, "flying": c.flying, "shooter": shooter, "breath": c.breath, "undead": c.undead,
        "sound_prefix": H2_SOUND_PREFIX[c.snd],
        "missile_def": missile_def(c),
        "missile_icn": missile_icn(c),
        "group_mapping": {str(g): f"{GROUP_NAMES.get(g, '?')}: {mapping[g]}" for g, _ in groups},
        "h2_frame_info": {k: v for k, v in info.items() if k not in ("seq", "walk_x_offsets")},
        "h2_sequences": {str(i): s for i, s in enumerate(seq) if s},
        "cranim_suggestion": cranim,
    }
    return def_name, def_bytes, meta


def convert_missile(agg, spec, palette, remap):
    key, display, icn_name, def_name, tag = spec
    icn = read_icn(agg, icn_name)
    # canvas big enough for every frame centred on the HoMM2 origin
    half = MISSILE_CANVAS // 2
    for fr in icn:
        half = max(half, -fr.x, -fr.y, fr.x + fr.w, fr.y + fr.h)
    size = 2 * half
    pool = FramePool(tag, icn, remap, size, size, half, half)
    frames = [pool.get(i) for i in range(len(icn))]
    def_bytes = build_def(DEF_TYPE_SPRITE, size, size, palette, [(0, frames)])
    users = [c for c in CREATURES if missile_icn(c) == icn_name]
    meta = {
        "key": key, "name": display, "def": def_name, "type": DEF_TYPE_SPRITE,
        "source": {"icn": icn_name, "icn_frames": len(icn)},
        "used_by": {str(H2_CREATURE_BASE + c.h2id): c.name for c in users},
        "group_mapping": {"0": "missile frames in m_arrowAngle order (90 deg up .. -90 deg down), facing right; centre = flight point"},
    }
    if any(c.key in ("mage", "archmage") for c in users):
        meta["note"] = ("HoMM2 loads elf__msl.icn for mages too, but draws their shot as a DoBolt "
                        "(ARMY.cpp); the missile is never shown for them")
    return def_name, def_bytes, meta


# ---------------------------------------------------------------------------
# PNG previews
# ---------------------------------------------------------------------------
def write_png(path: Path, w: int, h: int, rgb_rows: list[bytes]):
    def chunk(t, data):
        c = struct.pack(">I", len(data)) + t + data
        return c + struct.pack(">I", zlib.crc32(t + data) & 0xFFFFFFFF)
    raw = b"".join(b"\0" + r for r in rgb_rows)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    path.write_bytes(png)


FONT = {  # 3x5 digits
    "0": "111101101101111", "1": "010110010010111", "2": "111001111100111",
    "3": "111001111001111", "4": "101101111001001", "5": "111100111001111",
    "6": "111100111101111", "7": "111001001001001", "8": "111101111101111",
    "9": "111101111001111",
}


class Canvas:
    def __init__(self, w, h, bg):
        self.w, self.h = w, h
        self.px = [list(bg) for _ in range(w * h)]

    def set(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y * self.w + x] = list(c)

    def get(self, x, y):
        return self.px[y * self.w + x]

    def text(self, x, y, s, c, scale=2):
        for ch in s:
            g = FONT.get(ch)
            if g:
                for i, bit in enumerate(g):
                    if bit == "1":
                        for sy in range(scale):
                            for sx in range(scale):
                                self.set(x + (i % 3) * scale + sx, y + (i // 3) * scale + sy, c)
            x += 4 * scale

    def line(self, x0, y0, x1, y1, c):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            self.set(round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n), c)

    def save(self, path):
        rows = [bytes(v for p in self.px[y * self.w:(y + 1) * self.w] for v in p) for y in range(self.h)]
        write_png(path, self.w, self.h, rows)


def blit_frame(cv: Canvas, frame: dict, palette, ox, oy, win, selected=False):
    """Draw a parsed DEF frame the way drawCreatureImpl does (no outline colour)."""
    wx0, wy0, wx1, wy1 = win
    cx, cy, cw, ch = frame["crop"]
    for r, row in enumerate(frame["rows"]):
        Y = cy + r
        if not (wy0 <= Y < wy1):
            continue
        for c, v in enumerate(row):
            X = cx + c
            if not (wx0 <= X < wx1):
                continue
            px, py = ox + X - wx0, oy + Y - wy0
            if v >= 10:
                cv.set(px, py, palette[v])
            elif v in (1, 7) and not (selected and v == 7):
                o = cv.get(px, py)
                cv.set(px, py, [o[0] * 3 // 4, o[1] * 3 // 4, o[2] * 3 // 4])
            elif v in (4, 6) and not (selected and v == 6):
                o = cv.get(px, py)
                cv.set(px, py, [o[0] // 2, o[1] // 2, o[2] // 2])
            elif selected and v in (5, 6, 7):
                cv.set(px, py, (255, 255, 0))


def draw_hex(cv, ox, oy, win):
    """HoMM3 hex around ref point (ANCHOR) in window coordinates."""
    ulx, uly = ANCHOR_X - 22 - win[0] + ox, ANCHOR_Y - 42 - win[1] + oy
    pts = [(ulx + 22, uly), (ulx + 44, uly + 10), (ulx + 44, uly + 42), (ulx + 22, uly + 52),
           (ulx, uly + 42), (ulx, uly + 10)]
    for i in range(6):
        (a, b), (c, d) = pts[i], pts[(i + 1) % 6]
        cv.line(a, b, c, d, (40, 90, 40))
    ax, ay = ANCHOR_X - win[0] + ox, ANCHOR_Y - win[1] + oy
    for k in range(-3, 4):
        cv.set(ax + k, ay, (255, 0, 0))
        cv.set(ax, ay + k, (255, 0, 0))


def overview_sheet(items, path: Path, win, scale=2, cols=12):
    """Standing frame (+ selected outline) of each DEF, scaled, in rows of `cols`."""
    wx0, wy0, wx1, wy1 = win
    cw, ch = wx1 - wx0, wy1 - wy0
    cols = min(cols, len(items))
    rows = -(-len(items) // cols)
    small = Canvas(cols * cw, 2 * ch * rows, (112, 140, 96))
    for i, (parsed, gid) in enumerate(items):
        g = next(g for g in parsed["groups"] if g["id"] == gid)
        x0, y0 = (i % cols) * cw, (i // cols) * 2 * ch
        for row, sel in ((0, False), (1, True)):
            draw_hex(small, x0, y0 + row * ch, win)
            blit_frame(small, g["frames"][0], parsed["palette"], x0, y0 + row * ch, win, selected=sel)
    big = Canvas(small.w * scale, small.h * scale + 0, (0, 0, 0))
    for y in range(big.h):
        for x in range(big.w):
            big.px[y * big.w + x] = small.px[(y // scale) * small.w + x // scale]
    for i in range(1, cols):
        for y in range(big.h):
            big.set(i * cw * scale, y, (40, 40, 40))
    for r in range(1, rows):
        for x in range(big.w):
            big.set(x, r * 2 * ch * scale, (200, 200, 200))
    big.save(path)


def creature_window(parsed, margin=6):
    """Preview window covering the frames of a creature DEF (at least the
    Knight-era window), so dragons and other big sprites are not cut off.
    Capped round the anchor: a few HoMM2 frames carry stray pixels ~250px
    away (SWORDSMN/ELF/DRUID/SKELETON), which would blow the sheet up."""
    x0, y0, x1, y1 = ANCHOR_X - 90, ANCHOR_Y - 130, ANCHOR_X + 140, ANCHOR_Y + 30
    for g in parsed["groups"]:
        for fr in g["frames"]:
            cx, cy, cw, ch = fr["crop"]
            if cw * ch <= 1:
                continue
            x0, y0 = min(x0, cx - margin), min(y0, cy - margin)
            x1, y1 = max(x1, cx + cw + margin), max(y1, cy + ch + margin)
    return (max(0, x0, ANCHOR_X - 140), max(0, y0, ANCHOR_Y - 190),
            min(parsed["width"], x1, ANCHOR_X + 200), min(parsed["height"], y1, ANCHOR_Y + 50))


def contact_sheet(parsed: dict, path: Path, win, hexes=True):
    wx0, wy0, wx1, wy1 = win
    cell_w, cell_h = wx1 - wx0, wy1 - wy0
    groups = parsed["groups"]
    cols = max(len(g["frames"]) for g in groups) + (1 if hexes else 0)
    label_w = 30
    W = label_w + cols * (cell_w + 2)
    H = len(groups) * (cell_h + 2)
    cv = Canvas(W, H, (112, 140, 96))
    for gi, g in enumerate(groups):
        y = gi * (cell_h + 2)
        cv.text(2, y + 4, str(g["id"]), (0, 0, 0))
        frames = list(g["frames"])
        for fi, fr in enumerate(frames):
            x = label_w + fi * (cell_w + 2)
            if hexes:
                draw_hex(cv, x, y, win)
            blit_frame(cv, fr, parsed["palette"], x, y, win)
        if hexes and g["id"] == CS_WAIT:  # extra cell: standing frame, selected outline
            x = label_w + len(frames) * (cell_w + 2)
            draw_hex(cv, x, y, win)
            blit_frame(cv, frames[0], parsed["palette"], x, y, win, selected=True)
        for fi in range(cols):
            x = label_w + fi * (cell_w + 2) + cell_w
            for yy in range(cell_h + 2):
                cv.set(x, y + yy, (60, 60, 60))
                cv.set(x + 1, y + yy, (60, 60, 60))
        for xx in range(W):
            cv.set(xx, y + cell_h, (60, 60, 60))
            cv.set(xx, y + cell_h + 1, (60, 60, 60))
    cv.save(path)


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def validate_def(name: str, data: bytes, meta: dict):
    parsed = parse_def(data)
    errs = []
    if parsed["type"] != meta["type"]:
        errs.append("type")
    for g in parsed["groups"]:
        for fr in g["frames"]:
            cx, cy, cw, ch = fr["crop"]
            if fr["full"] != (parsed["width"], parsed["height"]):
                errs.append(f"{fr['name']}: full size")
            if cx + cw > parsed["width"] or cy + ch > parsed["height"]:
                errs.append(f"{fr['name']}: crop outside canvas")
            # re-encode must reproduce the stored bytes exactly
            if encode_general(fr["rows"]) != data[fr["offset"] + 32: fr["offset"] + 32 + fr["size"]]:
                errs.append(f"{fr['name']}: re-encode mismatch")
    if errs:
        raise ValueError(f"{name}: " + "; ".join(errs[:5]))
    return parsed


# ---------------------------------------------------------------------------
# Creature portrait DEFs (CPRSMALL / TWCRPORT / CPRLARGE)
# ---------------------------------------------------------------------------
# The engine indexes these single-group DEFs by frame = creature id + 2 (frame
# 0 "Blk" backdrop, frame 1 "XXX" empty).  The mashup registers HoMM2 creature
# k as HoMM3 id H2_CREATURE_BASE + k, so frames 152..217 hold the 66 HoMM2
# creature portraits; any gap before them (frame 151 = id 149) reuses the empty frame.
H2_CREATURE_BASE = 150
H2_PORTRAIT_COUNT = 66            # every HoMM2 creature, ids 0..65
STRIP_RACE_FRAME_OFFSET = 4       # STRIP.ICN frame = race + 4 (strip.h; STRIP.cpp per creature)
PORTRAIT_DEFS = [
    # DEF, HoMM2 source kind, new-frame name prefix (+3 digits +".pcx")
    ("CPRSMALL.DEF", "small", "H2PS"),
    ("TWCRPORT.DEF", "large", "H2PL"),
    ("CPRLARGE.DEF", "large", "H2PX"),   # not shipped by Complete; used only if present
]


class RawFrame:
    """A retail frame copied verbatim (cropped header + payload)."""

    def __init__(self, name, blob):
        self.name, self.blob = name, blob

    def encode(self):
        return self.blob


def raw_def_frames(b: bytes):
    """Retail single-pass reader returning (header, [(gid, [(name, blob)])])."""
    t, w, h, ng = struct.unpack_from("<4I", b, 0)
    p = 0x310
    groups = []
    for _ in range(ng):
        gid, nf, _u1, _u2 = struct.unpack_from("<4I", b, p)
        p += 16
        names = [b[p + 13 * i: p + 13 * i + 13].split(b"\0")[0].decode("latin1") for i in range(nf)]
        p += 13 * nf
        offs = struct.unpack_from(f"<{nf}I", b, p)
        p += 4 * nf
        frames = []
        for nm, o in zip(names, offs):
            (size,) = struct.unpack_from("<I", b, o)
            frames.append((nm, b[o: o + 32 + size]))
        groups.append((gid, frames))
    return (t, w, h, [tuple(b[16 + 3 * i: 19 + 3 * i]) for i in range(256)]), groups


def area_resample(src, sw, sh, dw, dh, x0=0.0, y0=0.0, fx=None, fy=None):
    """Box-filter resample of an RGBA float image (premultiplied colour)."""
    fx = fx if fx is not None else sw / dw
    fy = fy if fy is not None else sh / dh
    out = []
    for oy in range(dh):
        sy0, sy1 = y0 + oy * fy, y0 + (oy + 1) * fy
        for ox in range(dw):
            sx0, sx1 = x0 + ox * fx, x0 + (ox + 1) * fx
            acc = [0.0, 0.0, 0.0, 0.0]
            area = 0.0
            for yy in range(max(0, int(sy0)), min(sh, int(-(-sy1 // 1)))):
                wy = min(sy1, yy + 1) - max(sy0, yy)
                if wy <= 0:
                    continue
                for xx in range(max(0, int(sx0)), min(sw, int(-(-sx1 // 1)))):
                    wx = min(sx1, xx + 1) - max(sx0, xx)
                    if wx <= 0:
                        continue
                    wgt = wx * wy
                    r, g, bb, a = src[yy * sw + xx]
                    acc[0] += r * a * wgt
                    acc[1] += g * a * wgt
                    acc[2] += bb * a * wgt
                    acc[3] += a * wgt
                    area += wgt
            if acc[3] > 0:
                out.append((acc[0] / acc[3], acc[1] / acc[3], acc[2] / acc[3], acc[3] / area))
            else:
                out.append((0.0, 0.0, 0.0, 0.0))
    return out


def nearest_index_fn(palette):
    cands = list(range(10, 256))   # 0..9 are control codes in encoding 1
    cache = {}

    def nearest(rgb):
        key = (int(rgb[0]) >> 1, int(rgb[1]) >> 1, int(rgb[2]) >> 1)
        if key not in cache:
            r, g, b = rgb

            def dist(i):
                pr, pg, pb = palette[i]
                rm = (r + pr) / 2
                return ((2 + rm / 256) * (r - pr) ** 2 + 4 * (g - pg) ** 2
                        + (2 + (255 - rm) / 256) * (b - pb) ** 2, i)
            cache[key] = min(cands, key=dist)
        return cache[key]
    return nearest


def icn_rgba(fr: IcnFrame, h2pal, w, h, ox, oy, under=None):
    """Compose an ICN frame into an RGBA float canvas (shadows darken `under`)."""
    img = list(under) if under else [(0.0, 0.0, 0.0, 0.0)] * (w * h)
    for yy in range(fr.h):
        for xx in range(fr.w):
            v = fr.pixels[yy * fr.w + xx]
            X, Y = ox + fr.x + xx, oy + fr.y + yy
            if v < 0 or not (0 <= X < w and 0 <= Y < h):
                continue
            if v < 256:
                img[Y * w + X] = (*map(float, h2pal[v]), 1.0)
            elif v in (PX_SHADOW, PX_OUTLINE_SHADOW) and img[Y * w + X][3] > 0:
                r, g, b, a = img[Y * w + X]
                img[Y * w + X] = (r * 0.57, g * 0.57, b * 0.57, a)
    return img


def portrait_pixels(kind, agg, h2pal, k, fw, fh):
    """RGBA float image (fw x fh) for HoMM2 creature k."""
    if kind == "small":
        # mons32.icn, as the HoMM2 casualty list draws it: centred, bottom-aligned
        fr = read_icn_cached(agg, "MONS32.ICN")[k]
        w, h = fr.w, fr.h
        img = icn_rgba(fr, h2pal, w, h, -fr.x, -fr.y)
        scale = min(1.0, fw / w, fh / h)
        dw, dh = max(1, round(w * scale)), max(1, round(h * scale))
        if (dw, dh) != (w, h):
            img = area_resample(img, w, h, dw, dh)
        out = [(0.0, 0.0, 0.0, 0.0)] * (fw * fh)
        ox, oy = (fw - dw) // 2, fh - dh
        for y in range(dh):
            for x in range(dw):
                out[(oy + y) * fw + ox + x] = img[y * dw + x]
        return out
    # large: hero-strip cell = STRIP.ICN backdrop of the creature's own race
    # (race + STRIP_RACE_FRAME_OFFSET, neutral = frame 10) + monh%04d.icn on top
    bg = read_icn_cached(agg, "STRIP.ICN")[CREATURES[k].race + STRIP_RACE_FRAME_OFFSET]
    w, h = bg.w, bg.h
    img = icn_rgba(bg, h2pal, w, h, 0, 0)
    img = icn_rgba(read_icn_cached(agg, f"MONH{k:04d}.ICN")[0], h2pal, w, h, 0, 0, under=img)
    f = w / fw                       # fit width, centre-crop height
    y0 = max(0.0, (h - fh * f) / 2)
    return area_resample(img, w, h, fw, fh, 0.0, y0, f, f)


_ICN_CACHE: dict = {}


def read_icn_cached(agg, name):
    if name not in _ICN_CACHE:
        _ICN_CACHE[name] = read_icn(agg, name)
    return _ICN_CACHE[name]


def convert_portrait(agg, h2pal, def_name, kind, prefix, retail: bytes, source: str):
    (dtype, fw, fh, pal), groups = raw_def_frames(retail)
    if len(groups) != 1:
        raise ValueError(f"{def_name}: expected one group, got {len(groups)}")
    gid, rframes = groups[0]
    frames = [RawFrame(nm, blob) for nm, blob in rframes]
    retail_count = len(frames)
    first = H2_CREATURE_BASE + 2
    if retail_count > first:
        raise ValueError(f"{def_name}: retail already has {retail_count} frames")
    empty = frames[1] if len(frames) > 1 else frames[0]   # "XXX": the empty slot
    while len(frames) < first:
        frames.append(empty)
    nearest = nearest_index_fn(pal)
    for k in range(H2_PORTRAIT_COUNT):
        img = portrait_pixels(kind, agg, h2pal, k, fw, fh)
        idx = [nearest(p[:3]) if p[3] >= 0.5 else IDX_TRANSPARENT for p in img]
        rows = [bytes(idx[y * fw:(y + 1) * fw]) for y in range(fh)]
        # crop like CSpriteFrame::crop (retail small portraits are cropped)
        ys = [y for y in range(fh) if any(rows[y])]
        xs = [x for x in range(fw) if any(r[x] for r in rows)]
        if ys and xs:
            rows = [r[xs[0]: xs[-1] + 1] for r in rows[ys[0]: ys[-1] + 1]]
            cx, cy = xs[0], ys[0]
        else:
            rows, cx, cy = [bytes([0])], 0, 0
        frames.append(DefFrame(f"{prefix}{H2_CREATURE_BASE + k:03d}.pcx", fw, fh, cx, cy, rows))
    data = build_def(dtype, fw, fh, pal, [(gid, frames)])
    meta = {
        "key": def_name.split(".")[0].lower(), "name": f"creature portraits ({kind})",
        "def": def_name, "type": dtype, "source": {"retail": source,
            "h2": "MONS32.ICN[k]" if kind == "small" else "STRIP.ICN[race + 4] + MONHnnnn.ICN"},
        "retail_frames": retail_count, "padding_frames": first - retail_count,
        "h2_frames": {str(first + k): f"HoMM2 creature {k} {CREATURES[k].name} ({RACE_NAMES[CREATURES[k].race]}"
                                      + (f", STRIP.ICN[{CREATURES[k].race + STRIP_RACE_FRAME_OFFSET}]"
                                         if kind != "small" else "")
                                      + f") -> HoMM3 id {H2_CREATURE_BASE + k}"
                      for k in range(H2_PORTRAIT_COUNT)},
        "group_mapping": {str(gid): "frame = creature id + 2 (retail frames kept verbatim)"},
    }
    return def_name, data, meta


def validate_portrait(name, data, retail):
    parsed = parse_def(data)
    ref = parse_def(retail)
    new, old = parsed["groups"][0]["frames"], ref["groups"][0]["frames"]
    if parsed["palette"] != ref["palette"] or parsed["type"] != ref["type"]:
        raise ValueError(f"{name}: header/palette changed")
    for a, b in zip(new, old):
        if (a["name"], a["crop"], a["rows"]) != (b["name"], b["crop"], b["rows"]):
            raise ValueError(f"{name}: retail frame {b['name']} not preserved")
    for fr in new[len(old):]:
        cx, cy, cw, ch = fr["crop"]
        if cx + cw > parsed["width"] or cy + ch > parsed["height"]:
            raise ValueError(f"{name}: {fr['name']} crop outside canvas")
    if len(new) < H2_CREATURE_BASE + 2 + H2_PORTRAIT_COUNT:
        raise ValueError(f"{name}: too few frames")
    return parsed


def portrait_sheet(parsed, path, first, last, scale):
    W, H = parsed["width"], parsed["height"]
    frames = parsed["groups"][0]["frames"]
    idxs = list(range(first, min(last, len(frames))))
    cols = 12
    rows = -(-len(idxs) // cols)
    cell_w, cell_h = W + 4, H + 14
    small = Canvas(cols * cell_w, rows * cell_h, (60, 60, 60))
    for n, i in enumerate(idxs):
        x, y = (n % cols) * cell_w + 2, (n // cols) * cell_h + 12
        for yy in range(H):
            for xx in range(W):
                small.set(x + xx, y + yy, (255, 0, 255) if ((xx // 4 + yy // 4) & 1) else (220, 0, 220))
        blit_frame(small, frames[i], parsed["palette"], x, y, (0, 0, W, H))
        small.text(x, y - 10, str(i), (255, 255, 255) if i < H2_CREATURE_BASE + 2 else (255, 255, 0), scale=1)
    big = Canvas(small.w * scale, small.h * scale, (0, 0, 0))
    for yy in range(big.h):
        for xx in range(big.w):
            big.px[yy * big.w + xx] = small.px[(yy // scale) * small.w + xx // scale]
    big.save(path)


# ---------------------------------------------------------------------------
# Combat heroes (H2CHERO/H2CHERON/H2CHEROW.DEF) and hero portraits
# (HPLH2KIL/HPSH2KIL, HPLH2SAN/HPSH2SAN, HPLH2ALA/HPSH2ALA .pcx)
# ---------------------------------------------------------------------------
# Retail CH00.DEF: type 0x49, 150x175 canvas, groups 0 stand, 1 fidget,
# 2 "doh" (own stack lost), 3 "yeah" (enemy stack lost), 4 cast
# (drawing.cpp COMBAT_HERO_FRAME_*; army.cpp sets m_playDoh/m_playYeah).
# The left hero canvas is drawn at screen (-43,-19), the right one mirrored at
# (693,-19); the battlefield flag CmFlagL sits at (29,20) and hangs from CH00's
# pole top at canvas (89,35).
HERO_CANVAS = (150, 175)
# HoMM2 frames share one box per class with the origin at the hooves' bottom
# centre.  Put the Knight's lance tip (ICN x 9..13, y -111) on CH00's pole
# top (89, 35) so the HoMM3 flag hangs from it; the hooves then land on
# y=145, CH00's bottom row.  HoMM2 draws every class at the same screen point
# (COMBAT_HERO_LEFT_X/Y) under the same herofl%02d.icn flag overlay, so the
# same origin keeps the other classes where HoMM2 has them.
HERO_ORIGIN = (78, 146)


class CombatHero:
    """One HoMM2 hero class: cmbthro%c.icn (cHeroTypeInitial, CMBTMGR.cpp)
    and its sCmbtHero row (KB.cpp): {x[2], y[2]} cast-hand offsets (x[0], y[0]
    = the forward "low" cast pose, relative to the hero's draw point) + the
    12 sequences 0 stand, 1 own loss, 2 opponent loss, 3/4 no-target cast
    (+return), 5/6 forward cast (+return), 7/8 high cast (+return),
    9/10 idles."""

    def __init__(self, key, name, def_name, icn, tag, sprite_index, cast_offset, seqs):
        self.key, self.name, self.def_name, self.icn, self.tag = key, name, def_name, icn, tag
        self.sprite_index, self.cast_offset, self.seq = sprite_index, cast_offset, seqs

    @property
    def groups(self):
        s = self.seq
        return [
            (0, s[0], "stand: h2 seq 0"),
            (1, s[9] + s[0] + s[10], "fidget: h2 idle seq 9, stand, idle seq 10"),
            (2, s[1], "doh / own stack lost: h2 seq 1 (loss reaction)"),
            (3, s[2], "yeah / enemy stack lost: h2 seq 2 (opponent-loss reaction)"),
            (4, s[5] + s[6], "cast: h2 seq 5 (forward cast) + seq 6 (return)"),
        ]

    @property
    def cast_point(self):
        return (HERO_ORIGIN[0] + self.cast_offset[0], HERO_ORIGIN[1] + self.cast_offset[1])

    @property
    def cast_frame(self):
        return len(self.seq[5])     # spells.cpp plays frames [0, castFrame) then releases


COMBAT_HEROES = [
    CombatHero("kilburn_combat_hero", "HoMM2 Knight combat hero", "H2CHERO.DEF", "CMBTHROK.ICN", "H2HRO",
               0, (18, -61), {
                   0: [1], 1: [2, 3, 4, 5, 4, 5, 4, 3, 2], 2: [6, 7, 8, 9, 8, 9, 8, 7, 6],
                   3: [10, 11], 4: [10], 5: [6, 12, 13], 6: [12, 6], 7: [2, 14], 8: [2],
                   9: [15, 16, 17], 10: [18, 19]}),
    CombatHero("necromancer_combat_hero", "HoMM2 Necromancer combat hero", "H2CHERON.DEF", "CMBTHRON.ICN",
               "H2HRN", 5, (17, -61), {
                   0: [1], 1: [2, 3, 4, 5, 4, 5, 4, 3, 2], 2: [6, 7, 6, 7, 6, 7],
                   3: [7, 8, 9, 10, 11], 4: [10, 9, 7], 5: [7, 12, 13, 14, 15], 6: [7],
                   7: [7, 12, 13, 14, 16], 8: [7], 9: [17], 10: [18, 19]}),
    CombatHero("warlock_combat_hero", "HoMM2 Warlock combat hero", "H2CHEROW.DEF", "CMBTHROW.ICN",
               "H2HRW", 3, (18, -65), {
                   0: [1], 1: [2, 3, 4, 5, 4, 5, 4, 3, 2], 2: [6, 7, 8, 9, 10, 9, 8, 7, 6],
                   3: [6, 7, 11, 12], 4: [11, 6], 5: [6, 7, 13], 6: [6], 7: [6, 14], 8: [6],
                   9: [15, 16], 10: [6]}),
]
HERO_DEF = COMBAT_HEROES[0].def_name

PORTRAITS = [
    # name, size, source ICN, frame.  HoMM2 heroes are port%04d.icn by
    # hero::m_portrait, which h2core sets to the hero id (h2bridge.cpp).
    ("HPLH2KIL.pcx", (58, 64), "PORT0000.ICN", 0),   # hero 0 Lord Kilburn: hero screen / combat info
    ("HPSH2KIL.pcx", (48, 32), "PORT0000.ICN", 0),   # downscaled: MINIPORT is only 30x22
    ("HPLH2SAN.pcx", (58, 64), "PORT0052.ICN", 0),   # hero 52 Sandro (Necromancer)
    ("HPSH2SAN.pcx", (48, 32), "PORT0052.ICN", 0),
    ("HPLH2ALA.pcx", (58, 64), "PORT0028.ICN", 0),   # hero 28 Alamar (Warlock)
    ("HPSH2ALA.pcx", (48, 32), "PORT0028.ICN", 0),
]


def convert_combat_hero(agg, hero: CombatHero, palette, remap):
    icn = read_icn(agg, hero.icn)
    w, h = HERO_CANVAS
    pool = FramePool(hero.tag, icn, remap, w, h, *HERO_ORIGIN)
    groups = [(gid, [pool.get(f) for f in seq]) for gid, seq, _ in hero.groups]
    data = build_def(0x49, w, h, palette, groups)
    cast = hero.cast_point
    meta = {
        "key": hero.key, "name": hero.name, "def": hero.def_name, "type": 0x49,
        "source": {"icn": hero.icn, "sequences": f"sCmbtHero[{hero.sprite_index}] (KB.cpp)"},
        "group_mapping": {str(g): d for g, _, d in hero.groups},
        "h2_unused": "seq 3/4 (no-target cast) and 7/8 (high cast): HoMM3 has one cast group",
        "origin": list(HERO_ORIGIN),
        "h2_cast_offset": list(hero.cast_offset),
        "combat_hero_sprite_entry": {"def": hero.def_name, "castX": cast[0], "castY": cast[1],
                                     "castFrame": hero.cast_frame},
    }
    return hero.def_name, data, meta


def encode_bitmap8(w, h, idx, palette) -> bytes:
    """LOD 'pcx': {dataSize=w*h, w, h}, 8-bit pixels, 768-byte RGB palette (GetBitmap816)."""
    return struct.pack("<3I", w * h, w, h) + bytes(idx) + b"".join(bytes(c) for c in palette)


def decode_bitmap(b: bytes):
    size, w, h = struct.unpack_from("<3I", b, 0)
    if size == w * h:
        pal = [tuple(b[12 + size + 3 * i: 15 + size + 3 * i]) for i in range(256)]
        if len(b) != 12 + size + 768:
            raise ValueError("bad 8-bit bitmap length")
        return w, h, [pal[v] for v in b[12:12 + size]]
    if size == 3 * w * h:
        return w, h, [tuple(b[12 + 3 * i: 15 + 3 * i]) for i in range(w * h)]
    raise ValueError("unknown bitmap layout")


def convert_portrait_bitmap(agg, h2pal, palette, name, size, icn_name, frame):
    fr = read_icn(agg, icn_name)[frame]
    img = icn_rgba(fr, h2pal, fr.w, fr.h, -fr.x, -fr.y)
    dw, dh = size
    # cover the target: crop the spare width centred, the spare height
    # mostly from the bottom (keeps the head in the wide small portrait)
    f = min(fr.w / dw, fr.h / dh)
    x0, y0 = (fr.w - dw * f) / 2, (fr.h - dh * f) * 0.3
    out = area_resample(img, fr.w, fr.h, dw, dh, x0, y0, f, f)
    nearest = nearest_index_fn(palette)
    idx = [nearest(p[:3]) for p in out]
    data = encode_bitmap8(dw, dh, idx, palette)
    meta = {"name": name, "size": [dw, dh], "format": "8-bit indexed + 768-byte palette (like HPL000KN.pcx)",
            "source": f"{icn_name}[{frame}] {fr.w}x{fr.h}, box-filtered x{1 / f:.3f}"}
    return name, data, meta


# ---------------------------------------------------------------------------
# HoMM2 combat spell visuals -> HoMM3 spell-effect DEFs (H2SEnn.DEF)
# ---------------------------------------------------------------------------
# Retail effect DEFs (C02spF0, C04spF0, C11spA1, ...) are type 0x40, one
# group, a canvas cropped round the effect, encoding 1.  g_spellEffectTraits
# placement words (army::drawToBuffer / combatManager::spellEffect):
#   0 OVERHEAD  stack: x = midX - W/2, y = bottomY - H     hex: y = refY + 10 - H
#   1 CENTERED  stack: centre on (midX, midY)              hex: centre on (refX, refY - 37)
#   2 ABOVE     stack: x = midX - W/2, y = topY - H
#   3 FLANK     stack: x = frontX (- W when facing left),  y centred on midY
#   +0x100      drawSpellEffect alpha: every literal pixel blended 50%.
# drawSpellEffect without alpha paints literal runs only (control indices
# are transparent), so HoMM2 "dim" pixels can only be reproduced through the
# alpha path: they are written as black and the placement gets +0x100.
#
# HoMM2 anchors (army::DrawToBuffer + GetPowBaseY, SPELLS.cpp):
#   most effects   (MidX, MidY)                         -> CENTERED, exact
#   bless/curse/berserk/paralyze/hypnotize/dragon slayer (MidX, TopY) -> ABOVE
#   stone/steel skin (MidX, hex y + 5)                  -> OVERHEAD
#   shield (RightX = front edge, MidY)                  -> FLANK, exact
#   blind  (x + spellEffectX, y + spellEffectY): eye height, ~24 px above MidY
#   fireball/fireblast/cold ring/meteor (hex x, hex y - 17) -> hex CENTERED, exact
# When an effect reaches below its HoMM2 anchor where the HoMM3 mode needs it
# to end (ABOVE/OVERHEAD), the whole effect is lifted by that overhang.
H2_POW_FRAMES = [10, 10, 10, 10, 10, 10, 10, 10, 10, 8, 8, 10, 10, 10, 10, 15,
                 10, 10, 10, 10, 10, 16, 16, 14, 19, 22, 10, 17, 10, 12, 11, 16]   # giNumPowFrames
H2_COMBAT_FX = ["", "MAGIC01", "MAGIC02", "MAGIC03", "MAGIC04", "MAGIC05", "MAGIC06", "MAGIC07",
                "MAGIC08", "RAINBLUK", "CLOUDLUK", "MORALEG", "MORALEB", "REDDEATH", "REDFIRE",
                "SPARKS", "ELECTRIC", "PHYSICAL", "BLUEFIRE", "ICECLOUD", "LICHCLOD", "BLESS",
                "BERZERK", "SHIELD", "HASTE", "PARALYZE", "HYPNOTIZ", "DRAGSLAY", "BLIND", "CURSE",
                "STONSKIN", "STELSKIN"]                                            # gCombatFxNames
H2_FX_TOP = {21, 22, 25, 26, 27, 29}          # GetPowBaseY -> TopY
H2_FX_SKIN = {30, 31}                         # GetPowBaseY -> hex y + 5
H2_FX_SHIELD = 23
H2_FX_BLIND = 28
BLIND_LIFT = 24                               # knight spellEffectY (-57..-63) vs MidY (~-35)
AREA_LIFT = 37 - 17                           # hex CENTERED (refY-37) vs HoMM2 (y-17)
STORM_W, STORM_H = 800, 556                   # HoMM3 combat area (armageddon tiling bounds)

# extra (non CombatEffectType) visuals, numbered after the 32 effect types
FX_FIREBALL, FX_FIREBLAST, FX_COLDRING, FX_METEOR, FX_STORM = 32, 33, 34, 35, 36
FX_COLDRAY, FX_DISRRAY, FX_MAGICARROW, FX_RESURRECT = 37, 38, 39, 40


def fx_def_name(n):
    return f"H2SE{n:02d}.DEF"


def synth_frame(w, h, x, y, pixels):
    return IcnFrame(x, y, w, h, pixels)


def mirror_compose(fr: IcnFrame) -> IcnFrame:
    """Frame drawn normally and FlipIconToBitmap'ed at the same point (cold ring)."""
    pts = {}
    for yy in range(fr.h):
        for xx in range(fr.w):
            v = fr.pixels[yy * fr.w + xx]
            if v >= 0:
                ox, oy = fr.x + xx, fr.y + yy
                pts[(ox, oy)] = v
                pts.setdefault((-ox, oy), v)
    x0 = min(p[0] for p in pts)
    x1 = max(p[0] for p in pts)
    y0 = min(p[1] for p in pts)
    y1 = max(p[1] for p in pts)
    w, h = x1 - x0 + 1, y1 - y0 + 1
    px = [-1] * (w * h)
    for (ox, oy), v in pts.items():
        px[(oy - y0) * w + ox - x0] = v
    return synth_frame(w, h, x0, y0, px)


def storm_compose(tiles, frame) -> IcnFrame:
    """ElementalStorm: 54px grid, tile frame = (frame + 3*col + row) % 10, over the HoMM3 field."""
    px = [-1] * (STORM_W * STORM_H)
    for row in range(-(-STORM_H // 54)):
        for col in range(-(-STORM_W // 54)):
            t = tiles[(frame + 3 * col + row) % len(tiles)]
            for yy in range(t.h):
                Y = row * 54 + t.y + yy
                if not 0 <= Y < STORM_H:
                    continue
                for xx in range(t.w):
                    X = col * 54 + t.x + xx
                    v = t.pixels[yy * t.w + xx]
                    if v >= 0 and 0 <= X < STORM_W:
                        px[Y * STORM_W + X] = v
    return synth_frame(STORM_W, STORM_H, 0, 0, px)


def frames_extent(frames):
    xs, ys = [], []
    for fr in frames:
        for i, v in enumerate(fr.pixels):
            if v >= 0:
                xs.append(fr.x + i % fr.w)
                ys.append(fr.y + i // fr.w)
    return min(xs), min(ys), max(xs), max(ys)


def fx_canvas(frames, mode, k=0, base=0):
    """(W, H, ax, ay): canvas pixel = HoMM2 offset + (ax, ay)."""
    x0, y0, x1, y1 = frames_extent(frames)
    if mode == "flank":
        ax = max(0, -x0)
        W = ax + x1 + 1
    elif mode == "screen":
        return STORM_W, STORM_H, 0, 0
    else:
        half = max(-x0, x1 + 1, 1)
        W, ax = 2 * half, half
    if mode in ("center", "flank", "missile"):
        half = max(-y0 - k, y1 + 1 + k, 1)
        H, ay = 2 * half, half + k
    elif mode == "above":            # canvas bottom = HoMM2 anchor (TopY), lifted by the overhang
        s = max(0, y1 + 1)
        H = s - y0
        ay = H - s
    elif mode == "overhead":         # canvas bottom = bottomY = HoMM2 anchor - base
        s = max(0, y1 + base + 1)
        H = s - base - y0
        ay = H + base - s
    else:
        raise ValueError(mode)
    return W, H, ax, ay


def build_fx_def(n, frames, seq, mode, h2pal, palette, remap, k=0, base=0, tag=None):
    shadowy = any(v in (PX_SHADOW, PX_OUTLINE_SHADOW) for fr in frames for v in fr.pixels)
    black = min(range(10, 256), key=lambda i: (sum(palette[i]), i))
    W, H, ax, ay = fx_canvas([frames[i] for i in sorted(set(seq))], mode, k, base)
    tag = tag or f"H2E{n:02d}"
    cache = {}
    out = []
    for fi in seq:
        if fi not in cache:
            fr = frames[fi]
            pts = {}
            for yy in range(fr.h):
                for xx in range(fr.w):
                    v = fr.pixels[yy * fr.w + xx]
                    if v < 0 or v == PX_OUTLINE:
                        continue
                    idx = black if v >= 256 else remap[v]
                    X, Y = ax + fr.x + xx, ay + fr.y + yy
                    if 0 <= X < W and 0 <= Y < H:
                        pts[(X, Y)] = idx
            if pts:
                cx0 = min(p[0] for p in pts)
                cx1 = max(p[0] for p in pts)
                cy0 = min(p[1] for p in pts)
                cy1 = max(p[1] for p in pts)
                rows = [bytes(pts.get((X, Y), 0) for X in range(cx0, cx1 + 1)) for Y in range(cy0, cy1 + 1)]
            else:
                cx0 = cy0 = 0
                rows = [bytes([0])]
            cache[fi] = DefFrame(f"{tag}{len(cache):03d}.pcx", W, H, cx0, cy0, rows)
        out.append(cache[fi])
    data = build_def(DEF_TYPE_SPRITE, W, H, palette, [(0, out)])
    return data, {"canvas": [W, H], "anchor": [ax, ay], "frames": len(seq), "unique_frames": len(cache),
                  "shadow_as_black_alpha": shadowy}


PLACEMENT = {"overhead": 0, "center": 1, "above": 2, "flank": 3, "area": 1, "missile": None, "screen": None}


def convert_spell_effects(agg, h2pal, palette, remap):
    """Returns [(def_name, data, meta)]; meta carries the placement word."""
    out = []

    def add(n, frames, seq, mode, src, k=0, base=0, note=""):
        cmode = "center" if mode == "area" else mode
        data, info = build_fx_def(n, frames, seq, cmode, h2pal, palette, remap, k, base)
        place = PLACEMENT[mode]
        if place is not None and info["shadow_as_black_alpha"]:
            place |= 0x100
        meta = {"def": fx_def_name(n), "type": DEF_TYPE_SPRITE, "source": src, "mode": mode,
                "placement": place, **info}
        if note:
            meta["note"] = note
        out.append((fx_def_name(n), data, meta))

    for e in range(1, 32):
        if H2_COMBAT_FX[e] + ".ICN" not in agg.entries:
            continue                  # e.g. MAGIC05: referenced by the table, absent from the AGG
        icn = read_icn(agg, H2_COMBAT_FX[e] + ".ICN")
        seq = list(range(min(H2_POW_FRAMES[e], len(icn))))
        if e in H2_FX_TOP:
            add(e, icn, seq, "above", f"{H2_COMBAT_FX[e]}.ICN (anchor MidX, TopY)")
        elif e in H2_FX_SKIN:
            add(e, icn, seq, "overhead", f"{H2_COMBAT_FX[e]}.ICN (anchor MidX, hex y+5)", base=5)
        elif e == H2_FX_SHIELD:
            add(e, icn, seq, "flank", "SHIELD.ICN (anchor RightX, MidY)")
        elif e == H2_FX_BLIND:
            add(e, icn, seq, "center", "BLIND.ICN (HoMM2 anchor: per-creature eye offset)", k=-BLIND_LIFT,
                note=f"drawn {BLIND_LIFT}px above midY to approximate HoMM2's eye anchor")
        else:
            add(e, icn, seq, "center", f"{H2_COMBAT_FX[e]}.ICN (anchor MidX, MidY)")

    fb = read_icn(agg, "FIREBALL.ICN")
    add(FX_FIREBALL, fb, list(range(12)), "area", "FIREBALL.ICN (hex x, hex y-17)", k=AREA_LIFT)
    fb2 = read_icn(agg, "FIREBAL2.ICN")
    add(FX_FIREBLAST, fb2, list(range(12)), "area", "FIREBAL2.ICN (hex x, hex y-17)", k=AREA_LIFT)
    cr = [mirror_compose(f) for f in read_icn(agg, "COLDRING.ICN")[:7]]
    add(FX_COLDRING, cr, list(range(7)), "area", "COLDRING.ICN drawn + mirrored (hex x, hex y-17)", k=AREA_LIFT)
    met = read_icn(agg, "METEOR.ICN")
    add(FX_METEOR, met, list(range(10)) * 2, "area", "METEOR.ICN, 2 passes (hex x, hex y-17)", k=AREA_LIFT)
    tiles = read_icn(agg, "STORM.ICN")
    storm = [storm_compose(tiles, f) for f in range(10)]
    add(FX_STORM, storm, list(range(10)) * 6, "screen", "STORM.ICN tiled 54px, frame (f+3col+row)%10, 6 passes",
        note="pre-composited for the 800x556 field: one armageddon-style tile covers the screen")
    add(FX_COLDRAY, read_icn(agg, "COLDRAY.ICN"), list(range(10)), "missile",
        "COLDRAY.ICN: DoBlast travelling sprite, frame = step*10/steps, 18px step, flipped when moving left")
    add(FX_DISRRAY, read_icn(agg, "DISRRAY.ICN"), list(range(10)), "missile",
        "DISRRAY.ICN: DoBlast travelling sprite, 24px step; its dim pixels are black -> draw with alpha")
    add(FX_MAGICARROW, read_icn(agg, "KEEP.ICN"), list(range(9)), "missile",
        "KEEP.ICN: magic-arrow missile, 9 angle frames 90,68.5,45,20.8,0,-20.8,-45,-68.5,-90")
    add(FX_RESURRECT, read_icn(agg, "YINYANG.ICN"), list(range(22)), "center",
        "YINYANG.ICN at (MidX, MidY), 22 frames; mostly dim pixels -> alpha")
    return out


# ---------------------------------------------------------------------------
# HoMM2 morale / luck visuals
# ---------------------------------------------------------------------------
# CMBTMGR.cpp CheckApplyGood/BadMorale: SpellEffect(MORALEG / MORALEB) at
# (MidX, MidY); ARMY.cpp CheckLuck: bad luck = SpellEffect(CLOUDLUK) at
# (MidX, MidY), good luck = combatManager::DoLuck, a procedural rainbow
# DoBolt from the top screen edge down to 5px above the stack's head.
# Retail HoMM3 plays its own via stack spellEffect(): good morale 20
# (C09spW0), bad morale 30 (C14spE0), good luck eSpellEffectFortune 18
# (C09spA0), all placement 1; HoMM3 has no bad-luck visual.
RAINBOW = [0xbc, 0xba, 0xce, 0xca, 0xc8, 0x6f, 0x71, 0x73, 0xde, 0x5a, 0x9f, 0x47, 0x4a, 0x8d, 0x8f]
LUCK_TARGET = (300, 273 - 70 - 5)       # representative mid-field stack: hex y 273, 70px tall
LUCK_PASS_STRIDE = 3                    # HoMM2 30ms per pass vs HoMM3 spellEffect 100ms per frame


def rainbow_passes():
    """Replay DoLuck's DoBolt (facing right, field left of x=480): cumulative pixel sets per pass.
    Pixel offsets are relative to the bolt's end point (stack MidX, head - 5)."""
    tx, ty = LUCK_TARGET
    sx, sy = tx + ty // 2 + 150, 0
    end = (tx, ty)
    ang = -40                            # startX > endX flips the 40/40 angle pair
    total = int(math.sqrt((sx - tx) ** 2 + (sy - ty) ** 2))
    st = {"cx": float(sx), "cy": float(sy), "px": sx, "py": sy, "near": False, "nearest": 0,
          "done": False, "base": 0.0}

    def reset():
        if st["near"]:
            return
        dist = int(math.sqrt((end[0] - st["px"]) ** 2 + (end[1] - st["py"]) ** 2))
        ratio = 0.0 if dist > total else (total - dist) / total
        st["base"] = math.atan2(end[0] - st["px"], end[1] - st["py"])
        st["base"] += (ang / 100.0) * ((2.0 - ratio) / 1.5)      # forceAngle=1 -> always bent

    reset()
    pix = {}
    passes = []
    while not st["done"]:
        prev = (st["px"], st["py"])
        for _ in range(10):              # drawDistance = angleDistance = 10
            st["cx"] += math.sin(st["base"])
            st["cy"] += math.cos(st["base"])
            st["px"], st["py"] = int(st["cx"]), int(st["cy"])
            if (st["px"], st["py"]) == prev:
                continue
            prev = (st["px"], st["py"])
            for off in range(-7, 8):     # width 15, spread horizontally (start not on an edge)
                pix[(st["px"] + off - tx, st["py"] - ty)] = RAINBOW[off + 7]   # RAINBOW_FORWARD
            d = abs(end[0] - st["px"]) + abs(end[1] - st["py"])
            if st["near"]:
                if d > st["nearest"] + 1 or d <= 2:
                    st["done"] = True
                    break
                if d < st["nearest"]:
                    st["nearest"] = d
            elif d < 15:
                st["near"], st["nearest"] = True, d
        passes.append(dict(pix))
        if not st["done"]:
            reset()
    return passes


def rainbow_frames():
    passes = rainbow_passes()
    keep = passes[LUCK_PASS_STRIDE - 1::LUCK_PASS_STRIDE]
    if keep[-1] is not passes[-1]:
        keep.append(passes[-1])
    keep += [passes[-1]] * 2             # brief hold before the redraw clears it
    frames = []
    for pts in keep:
        # offsets relative to the HoMM3 ABOVE anchor (topY): the bolt ends 5px above the head
        xs = [p[0] for p in pts]
        ys = [p[1] - 5 for p in pts]
        x0, y0 = min(xs), min(ys)
        w, h = max(xs) - x0 + 1, max(ys) - y0 + 1
        px = [-1] * (w * h)
        for (x, y), v in pts.items():
            px[(y - 5 - y0) * w + x - x0] = v
        frames.append(synth_frame(w, h, x0, y0, px))
    return frames, len(passes)


MORALE_LUCK = [  # key, DEF, tag, source, mode, sound, retail entry
    ("good_morale", "H2SEMRLG.DEF", "H2EMG", "MORALEG.ICN", "center", "H2BGOODMRLE", "20 C09spW0 Mirth (1)"),
    ("bad_morale", "H2SEMRLB.DEF", "H2EMB", "MORALEB.ICN", "center", "H2BBADMRLE", "30 C14spE0 Sorrow (1)"),
    ("good_luck", "H2SELCKG.DEF", "H2ELG", None, "above", "H2BGOODLUCK", "18 C09spA0 Fortune (1)"),
    ("bad_luck", "H2SELCKB.DEF", "H2ELB", "CLOUDLUK.ICN", "center", "H2BBADLUCK", "none (HoMM3 has no bad-luck visual)"),
]


def convert_morale_luck(agg, h2pal, palette, remap):
    out, table = [], {}
    for key, name, tag, icn_name, mode, sound, retail in MORALE_LUCK:
        if icn_name:
            icn = read_icn(agg, icn_name)
            e = H2_COMBAT_FX.index(icn_name.split(".")[0])
            seq = list(range(min(H2_POW_FRAMES[e], len(icn))))
            src = f"{icn_name} at (MidX, MidY), SpellEffect"
            frames = icn
        else:
            frames, npass = rainbow_frames()
            seq = list(range(len(frames)))
            src = (f"DoLuck rainbow DoBolt replayed ({npass} passes of 10px, every {LUCK_PASS_STRIDE}rd kept "
                   f"+2 hold), from the top edge {LUCK_TARGET[1] // 2 + 150}px ahead to 5px above the head; "
                   f"geometry of a stack whose head is {LUCK_TARGET[1]}px below the screen top")
        data, info = build_fx_def(0, frames, seq, mode, h2pal, palette, remap, tag=tag)
        place = PLACEMENT[mode]
        meta = {"def": name, "type": DEF_TYPE_SPRITE, "source": src, "mode": mode, "placement": place, **info}
        out.append((name, data, meta))
        table[key] = {"def": name, "placement": place, "sound": sound, "retail_counterpart": retail,
                      "note": ("ABOVE: canvas bottom on the stack's top, mirrored when facing left like HoMM2"
                               if mode == "above" else "CENTERED on (midX, midY), as HoMM2 and retail")}
    return out, table


# HoMM2 lich ranged attack (army::SpecialAttack, ARMY.cpp): after the
# LICH_MSL missile lands, PowEffect(COMBAT_EFFECT_LICH_CLOUD) draws
# LICHCLOD.ICN at (target hex x, hex y - PROJECTILE_TARGET_Y_OFFSET (17))
# over the target and its 6 neighbours (all damaged), while the lich's
# "lichexpl" sample (ARMY_SAMPLE_EXTRA_ONE) plays.  That is the fireball
# anchor, so the DEF is built like the area spells: hex-target CENTERED,
# lifted by AREA_LIFT onto HoMM3's (refX, refY - 37).
LICH_CLOUD = ("H2SELICH.DEF", "H2ELI", 20)     # DEF, frame tag, CombatEffectType


def convert_lich_cloud(agg, h2pal, palette, remap):
    name, tag, effect = LICH_CLOUD
    icn = read_icn(agg, H2_COMBAT_FX[effect] + ".ICN")
    seq = list(range(min(H2_POW_FRAMES[effect], len(icn))))
    data, info = build_fx_def(0, icn, seq, "center", h2pal, palette, remap, k=AREA_LIFT, tag=tag)
    place = PLACEMENT["area"]
    if info["shadow_as_black_alpha"]:
        place |= 0x100
    meta = {"def": name, "type": DEF_TYPE_SPRITE, "mode": "area", "placement": place,
            "source": f"{H2_COMBAT_FX[effect]}.ICN (COMBAT_EFFECT_LICH_CLOUD) at (hex x, hex y-17) of the lich's target",
            "sound": sound_name_creature("lich", "EXPL"),
            "sound_source": "LICHEXPL.82M (lich ARMY_SAMPLE_EXTRA_ONE)",
            "placement_note": "hex-target spellEffect(effect, hex): centred on (refX, refY-37) of the target hex",
            **info}
    return name, data, meta


def fx_entry(fx, sound, kind, **extra):
    d = {"def": fx_def_name(fx) if fx is not None else None, "sound": sound, "kind": kind}
    d.update(extra)
    return d


H2_SPELLS = [  # id, name, gsSpellInfo soundName
    (0, "Fireball", "fireball"), (1, "Fireblast", "fireball"), (2, "Lightning Bolt", "lightblt"),
    (3, "Chain Lightning", "chainlte"), (4, "Teleport", "telptout"), (5, "Cure", "cure"),
    (6, "Mass Cure", "masscure"), (7, "Resurrect", "resurect"), (8, "True Resurrect", "resurtru"),
    (9, "Haste", "haste"), (10, "Mass Haste", "masshast"), (11, "Slow", "slow"),
    (12, "Mass Slow", "massslow"), (13, "Blind", "blind"), (14, "Bless", "bless"),
    (15, "Mass Bless", "massbles"), (16, "Stoneskin", "stonskin"), (17, "Steelskin", "stelskin"),
    (18, "Curse", "curse"), (19, "Mass Curse", "masscurs"), (20, "Holy Word", "masscurs"),
    (21, "Holy Shout", "masscurs"), (22, "Anti-Magic", "antimagk"), (23, "Dispel Magic", "dipmagk"),
    (24, "Mass Dispel", "dipmagk"), (25, "Magic Arrow", "magcarow"), (26, "Berserker", "berzerk"),
    (27, "Armageddon", "armgedn"), (28, "Elemental Storm", "storm"), (29, "Meteor Shower", "meteor~1"),
    (30, "Paralyze", "paralize"), (31, "Hypnotize", "hypnotiz"), (32, "Cold Ray", "coldray"),
    (33, "Cold Ring", "coldring"), (34, "Disrupting Ray", "disruptr"), (35, "Death Ripple", "mnrdeath"),
    (36, "Death Wave", "mnrdeath"), (37, "Dragon Slayer", "drgnslay"), (38, "Blood Lust", "bloodlus"),
    (39, "Animate Dead", "resurect"), (40, "Mirror Image", "mirrorim"), (41, "Shield", "shield"),
    (42, "Mass Shield", "shield"), (43, "Summon Earth Elemental", "sumnelm"),
    (44, "Summon Air Elemental", "sumnelm"), (45, "Summon Fire Elemental", "sumnelm"),
    (46, "Summon Water Elemental", "sumnelm"), (47, "Earthquake", "erthquak"),
]
H2_SPELL_FX = {  # id -> (effect, kind, extra)
    0: (FX_FIREBALL, "area", {}), 1: (FX_FIREBLAST, "area", {}),
    2: (15, "bolt", {"bolt": "HoMM2 DoBolt: branch dist 150, branch len 100, width 9->2, angle -40..40, angle dist 30, colour BOLT_COLOR_LIGHTNING(301)"}),
    3: (15, "bolt", {"bolt": "HoMM2 ChainLightning: up to 4 targets, bolt len 80, width 9->2, angle 10..80; sparks on every hit stack (ShowMassSpell)"}),
    4: (None, "stack", {"note": "RippleCreature distortion out/in, no ICN; second sound H2STELPTIN on arrival"}),
    5: (1, "stack", {}), 6: (1, "stack", {"mass": True}),
    7: (FX_RESURRECT, "stack", {"note": "creature plays its death sequence backwards under the effect"}),
    8: (FX_RESURRECT, "stack", {}), 9: (24, "stack", {}), 10: (24, "stack", {"mass": True}),
    11: (2, "stack", {}), 12: (2, "stack", {"mass": True}), 13: (28, "stack", {}),
    14: (21, "stack", {}), 15: (21, "stack", {"mass": True}), 16: (30, "stack", {}), 17: (31, "stack", {}),
    18: (29, "stack", {}), 19: (29, "stack", {"mass": True}),
    20: (8, "screen", {"note": "screen colour blur (Blur 0,-2,-2) then the effect on every undead stack"}),
    21: (8, "screen", {"note": "screen colour blur then the effect on every undead stack"}),
    22: (6, "stack", {}), 23: (7, "stack", {}), 24: (7, "stack", {"mass": True}),
    25: (None, "bolt", {"missile_def": fx_def_name(FX_MAGICARROW), "missile_angles": [90, 68.5, 45, 20.8, 0, -20.8, -45, -68.5, -90]}),
    26: (22, "stack", {}),
    27: (None, "screen", {"note": "no ICN: palette flash/fade over the battlefield (combatManager::Armageddon)"}),
    28: (FX_STORM, "screen", {}), 29: (FX_METEOR, "area", {}), 30: (25, "stack", {}), 31: (26, "stack", {}),
    32: (19, "bolt", {"missile_def": fx_def_name(FX_COLDRAY), "missile_step": 18}),
    33: (FX_COLDRING, "area", {}),
    34: (None, "bolt", {"missile_def": fx_def_name(FX_DISRRAY), "missile_step": 24, "note": "target then ripples (RippleCreature)"}),
    35: (13, "screen", {"note": "screen Ripple(1) distortion then the effect on every living stack"}),
    36: (13, "screen", {"note": "screen Ripple(2) distortion then the effect on every living stack"}),
    37: (27, "stack", {}),
    38: (None, "stack", {"note": "no ICN: red palette tint on the stack (BloodLustEffect)"}),
    39: (FX_RESURRECT, "stack", {}),
    40: (None, "stack", {"note": "no ICN: image fades in next to the stack"}),
    41: (23, "stack", {}), 42: (23, "stack", {"mass": True}),
    43: (None, "stack", {}), 44: (None, "stack", {}), 45: (None, "stack", {}), 46: (None, "stack", {}),
    47: (20, "area", {"note": "LICHCLOD.ICN puffs over damaged castle walls (siege only)"}),
}


def h2_spell_visuals(fx_meta):
    by_def = {m["def"]: m for m in fx_meta}
    table = {}
    for sid, name, snd in H2_SPELLS:
        fx, kind, extra = H2_SPELL_FX[sid]
        entry = {"name": name, **fx_entry(fx, sound_name_spell(snd), kind, **extra)}
        m = by_def.get(entry["def"]) if entry["def"] else None
        entry["placement"] = m["placement"] if m else None
        if kind == "area" and m:
            entry["placement_note"] = "hex-target spellEffect(effect, hex): centred on (refX, refY-37)"
        if m and m["placement"] is not None and m["placement"] & 0x100:
            entry["alpha"] = True
        table[str(sid)] = entry
    return table


# ---------------------------------------------------------------------------
# HoMM2 sounds -> HoMM3 .snd (IMA ADPCM WAV, like heroes3.snd)
# ---------------------------------------------------------------------------
# HoMM2 *.82M = raw unsigned 8-bit mono 22050 Hz (sample.cpp: '8' 8-bit,
# '2' 22050, 'M' mono).  Every retail heroes3.snd / h3ab_ahd.snd member is a
# RIFF WAVE, format 0x11 (IMA ADPCM) mono 22050 Hz, 4 bits, block 512
# (1017 samples), fmt chunk 20 bytes + fact chunk.  The .snd is
# {u32 count; count x {char name[40] ("NAME\0WAV"), u32 offset, u32 size}}
# then the data; GetSoundFile strips the requested extension and _stricmp's
# the stored name.
H2_SOUND_PREFIX = {  # HoMM2 spriteName (gMonsterDatabase) -> HoMM3 m_samplePrefix
    "psnt": "H2PS", "arch": "H2AR", "pike": "H2PK", "swdm": "H2SW", "cavl": "H2CV", "pldn": "H2PL",
    "gbln": "H2GB", "elf_": "H2EL", "wolf": "H2WF", "ogre": "H2OG", "trll": "H2TR", "cycl": "H2CY",
    "sprt": "H2SP", "dwrf": "H2DW", "drui": "H2DR", "unic": "H2UN", "phoe": "H2PH",
    "cntr": "H2CN", "garg": "H2GA", "grif": "H2GR", "mino": "H2MI", "hydr": "H2HY", "drgn": "H2DG",
    "half": "H2HA", "boar": "H2BO", "golm": "H2GO", "roc_": "H2RC", "mage": "H2MA", "titn": "H2TI",
    "skel": "H2SK", "zomb": "H2ZO", "mumy": "H2MU", "vamp": "H2VA", "lich": "H2LI",
    "rogu": "H2RG", "nmad": "H2NM", "ghst": "H2GH", "geni": "H2GE", "meds": "H2ME",
    "eelm": "H2EE", "aelm": "H2AE", "felm": "H2FE", "welm": "H2WE",
}
assert len(set(H2_SOUND_PREFIX.values())) == len(H2_SOUND_PREFIX)
assert {c.snd for c in CREATURES} == set(H2_SOUND_PREFIX)
# army::loadResources asks for MOVE ATTK WNCE KILL DFND (+SHOT for shooters).
HOMM3_SUFFIXES = ["MOVE", "ATTK", "WNCE", "KILL", "DFND", "SHOT"]
# HoMM2-only samples (army::LoadResources): vampires' EXT1 (take-off) and
# EXT2 (landing) replace MOVE on the first/last flight hop (FlyTo); the
# lich's EXPL plays with the lich cloud.  The host plays these itself.
H2_EXTRA_SUFFIXES = ["EXT1", "EXT2", "EXPL"]


def sound_name_creature(h2snd, suffix):
    return H2_SOUND_PREFIX[h2snd] + suffix
BATTLE_SOUNDS = [("PREBATTL", "pre-battle fanfare"), ("GOODMRLE", "good morale"),
                 ("BADMRLE", "bad morale"), ("GOODLUCK", "good luck"), ("BADLUCK", "bad luck"),
                 ("RSBRYFZL", "spell resisted / fizzle (ShowSpellCastFailure)"),
                 ("TELPTIN", "teleport arrival (second teleport sound)")]


def sound_name_spell(h2name):
    return "H2S" + "".join(c for c in h2name.upper() if c.isalnum())


IMA_STEPS = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60,
             66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337,
             371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552,
             1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894,
             6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
             22385, 24623, 27086, 29794, 32767]
IMA_INDEX = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]
IMA_BLOCK = 512
IMA_SPB = (IMA_BLOCK - 4) * 2 + 1         # 1017 samples per mono block


def ima_encode(pcm: list[int]) -> bytes:
    out = bytearray()
    index = 0
    n = len(pcm)
    padded = pcm + [0] * ((-n) % IMA_SPB)
    for b0 in range(0, len(padded), IMA_SPB):
        pred = padded[b0]
        out += struct.pack("<hBB", pred, index, 0)
        nib = []
        for s in padded[b0 + 1: b0 + IMA_SPB]:
            step = IMA_STEPS[index]
            diff = s - pred
            code = 0
            if diff < 0:
                code = 8
                diff = -diff
            delta = step >> 3
            if diff >= step:
                code |= 4
                diff -= step
                delta += step
            if diff >= step >> 1:
                code |= 2
                diff -= step >> 1
                delta += step >> 1
            if diff >= step >> 2:
                code |= 1
                delta += step >> 2
            pred = pred - delta if code & 8 else pred + delta
            pred = max(-32768, min(32767, pred))
            index = max(0, min(88, index + IMA_INDEX[code]))
            nib.append(code)
        for i in range(0, len(nib), 2):
            out.append(nib[i] | (nib[i + 1] << 4))
    return bytes(out)


def ima_decode(data: bytes, count: int) -> list[int]:
    out = []
    for b0 in range(0, len(data), IMA_BLOCK):
        blk = data[b0: b0 + IMA_BLOCK]
        pred, index = struct.unpack_from("<hB", blk, 0)
        out.append(pred)
        for byte in blk[4:]:
            for code in (byte & 15, byte >> 4):
                step = IMA_STEPS[index]
                delta = step >> 3
                if code & 4:
                    delta += step
                if code & 2:
                    delta += step >> 1
                if code & 1:
                    delta += step >> 2
                pred = pred - delta if code & 8 else pred + delta
                pred = max(-32768, min(32767, pred))
                index = max(0, min(88, index + IMA_INDEX[code]))
                out.append(pred)
    return out[:count]


def wav_ima(pcm: list[int]) -> bytes:
    data = ima_encode(pcm)
    fmt = struct.pack("<HHIIHHHH", 0x11, 1, 22050, 11100, IMA_BLOCK, 4, 2, IMA_SPB)
    body = (b"WAVE" + b"fmt " + struct.pack("<I", len(fmt)) + fmt
            + b"fact" + struct.pack("<II", 4, len(pcm)) + b"data" + struct.pack("<I", len(data)) + data)
    return b"RIFF" + struct.pack("<I", len(body)) + body


def pcm_from_82m(raw: bytes) -> list[int]:
    return [(v - 128) << 8 for v in raw]


def build_snd(members: dict[str, bytes]) -> bytes:
    names = sorted(members, key=str.upper)
    head = bytearray(struct.pack("<I", len(names)))
    off = 4 + 48 * len(names)
    blobs = bytearray()
    for nm in names:
        field = nm.encode("ascii") + b"\0WAV"
        assert len(nm) < 36
        head += field + b"\0" * (40 - len(field)) + struct.pack("<II", off + len(blobs), len(members[nm]))
        blobs += members[nm]
    return bytes(head + blobs)


def read_snd(b: bytes) -> dict[str, bytes]:
    (n,) = struct.unpack_from("<I", b, 0)
    out = {}
    for i in range(n):
        e = 4 + 48 * i
        nm = b[e: e + 40].split(b"\0")[0].decode("ascii")
        off, size = struct.unpack_from("<II", b, e + 40)
        out[nm] = b[off: off + size]
    return out


def convert_sounds(agg):
    members = {}
    sources = {}

    def put(key, src):
        pcm = pcm_from_82m(agg.get(src))
        members[key] = wav_ima(pcm)
        sources[key] = pcm
    meta = {"creatures": {}, "spells": {}, "battle": {}}
    aliases = {}
    for h2p, p in H2_SOUND_PREFIX.items():
        users = [c for c in CREATURES if c.snd == h2p]
        avail = {}
        for suf in HOMM3_SUFFIXES + H2_EXTRA_SUFFIXES:
            nm = f"{h2p.upper()}{suf}.82M"
            if nm in agg.entries:
                avail[suf] = nm
        for need in ("MOVE", "ATTK", "WNCE", "KILL"):
            if need not in avail:
                raise ValueError(f"{h2p}: missing {need} sample")
        if any(c.shooter for c in users) and "SHOT" not in avail:
            raise ValueError(f"{h2p}: shooter without a SHOT sample")
        if "DFND" not in avail:
            avail["DFND"] = avail["WNCE"]     # HoMM2 has no defend sample; defend anim = wince
        for suf, src in avail.items():
            put(p + suf, src)
            aliases[p + suf] = src
    for c in CREATURES:
        p = H2_SOUND_PREFIX[c.snd]
        wanted = HOMM3_SUFFIXES[:5] + (["SHOT"] if c.shooter else [])
        if c.key in ("vampire", "vampire_lord"):
            wanted += ["EXT1", "EXT2"]
        if c.key in ("lich", "power_lich"):
            wanted += ["EXPL"]
        missing = [s for s in wanted if p + s not in aliases]
        if missing:
            raise ValueError(f"{c.name}: no {missing} sample")
        meta["creatures"][str(H2_CREATURE_BASE + c.h2id)] = {
            "creature": c.name, "prefix": p, "h2_prefix": c.snd,
            "samples": {s: aliases[p + s] for s in wanted}}
    for sid, name, snd in H2_SPELLS:
        nm = snd.upper() + ".82M"
        key = sound_name_spell(snd)
        if key not in members:
            put(key, nm)
        meta["spells"][key] = nm
    for snd, desc in BATTLE_SOUNDS:
        key = ("H2S" if snd in ("RSBRYFZL", "TELPTIN") else "H2B") + snd
        put(key, snd + ".82M")
        meta["battle"][key] = {"source": snd + ".82M", "use": desc}
    return members, meta, sources


def validate_snd(snd: bytes, members, agg_sources):
    back = read_snd(snd)
    if back != members:
        raise ValueError("snd round-trip mismatch")
    worst = 0.0
    for nm, wav in members.items():
        if wav[:4] != b"RIFF" or wav[8:16] != b"WAVEfmt ":
            raise ValueError(f"{nm}: not RIFF/WAVE")
        tag, ch, sr, _br, ba, bits = struct.unpack_from("<HHIIHH", wav, 20)
        if (tag, ch, sr, ba, bits) != (0x11, 1, 22050, IMA_BLOCK, 4):
            raise ValueError(f"{nm}: unexpected format")
        (count,) = struct.unpack_from("<I", wav, 48)
        (dlen,) = struct.unpack_from("<I", wav, 56)
        pcm = ima_decode(wav[60: 60 + dlen], count)
        ref = agg_sources[nm]
        if len(pcm) != len(ref):
            raise ValueError(f"{nm}: sample count")
        err = sum((a - b) ** 2 for a, b in zip(pcm, ref)) / max(1, len(ref))
        sig = sum(b * b for b in ref) / max(1, len(ref))
        if sig > 0:
            worst = max(worst, err / sig)
    return worst


def fx_preview(parsed, path, anchor, max_cells=24):
    """Unique frames of an effect DEF on grey, HoMM2 anchor marked in red."""
    W, H = parsed["width"], parsed["height"]
    seen, frames = set(), []
    for fr in parsed["groups"][0]["frames"]:
        if fr["name"] not in seen:
            seen.add(fr["name"])
            frames.append(fr)
    frames = frames[:max_cells]
    scale_down = 2 if W * H > 300 * 300 else 1
    if scale_down > 1:
        frames = frames[:2]
    cw, ch = W // scale_down, H // scale_down
    cols = min(len(frames), 8)
    rows = -(-len(frames) // cols)
    cv = Canvas(cols * (cw + 2), rows * (ch + 2), (60, 60, 60))
    for n, fr in enumerate(frames):
        x0, y0 = (n % cols) * (cw + 2), (n // cols) * (ch + 2)
        full = Canvas(W, H, (112, 140, 96))
        blit_frame(full, fr, parsed["palette"], 0, 0, (0, 0, W, H))
        for k in range(-3, 4):
            full.set(anchor[0] + k, anchor[1], (255, 0, 0))
            full.set(anchor[0], anchor[1] + k, (255, 0, 0))
        for yy in range(ch):
            for xx in range(cw):
                cv.set(x0 + xx, y0 + yy, full.get(xx * scale_down, yy * scale_down))
    cv.save(path)


def fx_compare(pairs, path):
    """Middle frame of each HoMM2 effect next to a comparable retail one."""
    cells = []
    for ours, ref in pairs:
        row = []
        for d in (ours, ref):
            if d is None:
                continue
            fr = d["groups"][0]["frames"][len(d["groups"][0]["frames"]) // 2]
            c = Canvas(d["width"], d["height"], (112, 140, 96))
            blit_frame(c, fr, d["palette"], 0, 0, (0, 0, d["width"], d["height"]))
            row.append(c)
        cells.append(row)
    W = max(sum(c.w + 4 for c in row) for row in cells)
    H = sum(max(c.h for c in row) + 4 for row in cells)
    cv = Canvas(W, H, (40, 40, 40))
    y = 0
    for row in cells:
        x = 0
        for c in row:
            for yy in range(c.h):
                for xx in range(c.w):
                    cv.set(x + xx, y + yy, c.get(xx, yy))
            x += c.w + 4
        y += max(c.h for c in row) + 4
    cv.save(path)


def hero_compare(ours, ch00_raw, flag_raw, path, scale=2):
    """Each of our heroes' group 0 and cast frames next to CH00, with the
    CmFlagL flag at its battlefield offset (29+43, 20+19) and the
    castX/castY point marked.  ours: [(parsed, (castX, castY), castFrame)]."""
    sheets = list(ours)
    if ch00_raw:
        sheets.append((parse_def(ch00_raw), (92, 67), 5))
    flag = parse_def(flag_raw) if flag_raw else None
    W, H = HERO_CANVAS
    cols = 3
    small = Canvas(cols * (W + 2), len(sheets) * (H + 2), (60, 60, 60))
    for r, (parsed, (cx, cy), cf) in enumerate(sheets):
        groups = {g["id"]: g["frames"] for g in parsed["groups"]}
        cells = [groups[0][0], groups[4][min(cf, len(groups[4]) - 1)], groups[4][max(cf - 1, 0)]]
        for c, fr in enumerate(cells):
            x, y = c * (W + 2), r * (H + 2)
            for yy in range(H):
                for xx in range(W):
                    small.set(x + xx, y + yy, (112, 140, 96))
            if flag:
                blit_frame(small, flag["groups"][0]["frames"][0], flag["palette"], x + 72, y + 39, (0, 0, W, H))
            blit_frame(small, fr, parsed["palette"], x, y, (0, 0, W, H))
            if c > 0:
                for k in range(-3, 4):
                    small.set(x + cx + k, y + cy, (255, 0, 0))
                    small.set(x + cx, y + cy + k, (255, 0, 0))
    big = Canvas(small.w * scale, small.h * scale, (0, 0, 0))
    for yy in range(big.h):
        for xx in range(big.w):
            big.px[yy * big.w + xx] = small.px[(yy // scale) * small.w + xx // scale]
    big.save(path)


def bitmap_compare(ours, retail, path, scale=3):
    imgs = [decode_bitmap(b) for b in ours + [r for r in retail if r]]
    W = sum(w for w, _, _ in imgs) + 4 * len(imgs)
    H = max(h for _, h, _ in imgs)
    cv = Canvas(W * scale, H * scale, (40, 40, 40))
    x = 0
    for w, h, rgb in imgs:
        for yy in range(h * scale):
            for xx in range(w * scale):
                cv.set(x + xx, yy, rgb[(yy // scale) * w + xx // scale])
        x += (w + 4) * scale
    cv.save(path)


def check_frame_names(members):
    """ResourceManager caches DEF frames by name across every DEF: each of
    our frame names must denote one picture."""
    seen = {}
    for nm, data in members.items():
        if not nm.upper().endswith(".DEF"):
            continue
        for gid, frames in raw_def_frames(data)[1]:
            for fname, blob in frames:
                if not fname.upper().startswith("H2"):
                    continue          # retail portrait frames kept verbatim
                key = fname.lower()
                if key in seen and seen[key][1] != blob:
                    raise ValueError(f"frame name {fname} used by {seen[key][0]} and {nm} for different pictures")
                seen.setdefault(key, (nm, blob))
    return len(seen)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--agg", type=Path, default=DEFAULT_AGG)
    ap.add_argument("--build-dir", type=Path, default=WORKTREE / "build")
    ap.add_argument("--no-preview", action="store_true")
    ap.add_argument("--h3sprite", type=Path,
                    default=WORKTREE / "build/mashup-runtime/Data/H3sprite.lod",
                    help="retail sprite LOD (portrait sources, reference overview)")
    ap.add_argument("--h3bitmap", type=Path,
                    default=WORKTREE / "build/mashup-runtime/Data/H3bitmap.lod",
                    help="retail bitmap LOD (portrait comparison preview only)")
    ap.add_argument("--h3ab-spr", type=Path,
                    default=WORKTREE / "build/mashup-runtime/Data/H3ab_spr.lod",
                    help="second retail sprite LOD, searched after --h3sprite")
    args = ap.parse_args(argv)

    agg = Agg(args.agg)
    h2pal = read_palette(agg)
    palette, remap = build_def_palette(h2pal)

    outputs = []
    for spec in CREATURES:
        outputs.append(convert_creature(agg, spec, palette, remap))
    for spec in MISSILES:
        outputs.append(convert_missile(agg, spec, palette, remap))
    for hero in COMBAT_HEROES:
        outputs.append(convert_combat_hero(agg, hero, palette, remap))

    build = args.build_dir
    defs_dir = build / "h2mash-defs"
    prev_dir = build / "h2mash-preview"
    defs_dir.mkdir(parents=True, exist_ok=True)
    members = {}
    manifest = {
        "lod": "h2mash.lod",
        "canvas": [CANVAS_W, CANVAS_H],
        "anchor": {"x": ANCHOR_X, "y": ANCHOR_Y,
                   "note": "canvas point drawn at hex (refX, refY); HoMM2 ICN origin placed here"},
        "palette": {"0": "transparent", "1": "shadow 75% (unused by H2 art)", "4": "shadow 50% (H2 dim level 0)",
                    "5": "selection outline", "6": "selection outline over 50% shadow",
                    "10-255": "HoMM2 KB.PAL entries 10-255 (H2 indices 0-9 are black, remapped)"},
        "frame_encoding": 1,
        "defs": [],
    }
    for name, data, meta in outputs:
        parsed = validate_def(name, data, meta)
        (defs_dir / name).write_bytes(data)
        members[name] = data
        meta = dict(meta)
        meta["canvas"] = [parsed["width"], parsed["height"]]
        meta["size"] = len(data)
        meta["groups"] = {str(g["id"]): len(g["frames"]) for g in parsed["groups"]}
        meta["frame_names"] = {str(g["id"]): [f["name"] for f in g["frames"]] for g in parsed["groups"]}
        if meta["type"] == DEF_TYPE_CREATURE:
            st = next(g for g in parsed["groups"] if g["id"] == CS_WAIT)["frames"][0]
            cx, cy, cw, ch = st["crop"]
            meta["standing_bbox"] = [cx, cy, cx + cw, cy + ch]
            meta["image_height"] = ANCHOR_Y - cy  # army::m_imageHeight = 267 - croppedY
        manifest["defs"].append(meta)
        if not args.no_preview:
            prev_dir.mkdir(parents=True, exist_ok=True)
            if meta["type"] == DEF_TYPE_CREATURE:
                contact_sheet(parsed, prev_dir / (Path(name).stem + ".png"), creature_window(parsed))
            else:
                contact_sheet(parsed, prev_dir / (Path(name).stem + ".png"),
                              (0, 0, parsed["width"], parsed["height"]), hexes=False)

    # Portrait DEFs: full replacement copies of the retail files.  The engine
    # walks its sprite LODs in order (findSpriteResource), so take the first
    # retail archive that has the member, exactly as the game would.
    manifest["portraits"] = []
    for def_name, kind, prefix in PORTRAIT_DEFS:
        retail = source = None
        for lod_path in (args.h3sprite, args.h3ab_spr):
            retail = lod_member(lod_path, def_name)
            if retail:
                source = lod_path.name
                break
        if not retail:
            manifest["portraits"].append({"def": def_name, "skipped": "not in retail sprite LODs"})
            continue
        name, data, meta = convert_portrait(agg, h2pal, def_name, kind, prefix, retail, source)
        parsed = validate_portrait(name, data, retail)
        (defs_dir / name).write_bytes(data)
        members[name] = data
        meta["canvas"] = [parsed["width"], parsed["height"]]
        meta["size"] = len(data)
        meta["groups"] = {str(g["id"]): len(g["frames"]) for g in parsed["groups"]}
        manifest["portraits"].append(meta)
        if not args.no_preview:
            first = H2_CREATURE_BASE + 2
            portrait_sheet(parsed, prev_dir / (Path(name).stem + ".png"), first - 22,
                           first + H2_PORTRAIT_COUNT, 3 if kind == "small" else 2)

    if not args.no_preview:
        # overview: our creatures next to retail HoMM3 analogues (if available)
        win = (ANCHOR_X - 60, ANCHOR_Y - 110, ANCHOR_X + 90, ANCHOR_Y + 20)
        items = []
        refs = {}
        for ref in REFERENCE_DEFS:
            raw = lod_member(args.h3sprite, ref)
            if raw:
                refs[ref] = parse_def(raw)
        for meta in manifest["defs"]:
            if meta["type"] == DEF_TYPE_CREATURE:
                items.append((parse_def(members[meta["def"]]), CS_WAIT))
        for ref, parsed in sorted(refs.items()):
            items.append((parsed, CS_WAIT))
            st = next(g for g in parsed["groups"] if g["id"] == CS_WAIT)["frames"][0]
            cx, cy, cw, ch = st["crop"]
            manifest.setdefault("reference_standing_bbox", {})[ref] = [cx, cy, cx + cw, cy + ch]
        overview_sheet(items, prev_dir / "_overview.png", win)
        # the same at a window big enough for dragons, hydra and other large sprites
        overview_sheet(items, prev_dir / "_overview_large.png",
                       (ANCHOR_X - 110, ANCHOR_Y - 175, ANCHOR_X + 150, ANCHOR_Y + 25), scale=1)

    # Hero portraits (LOD bitmaps) + combat-hero comparison against retail CH00
    manifest["bitmaps"] = []
    for name, size, icn_name, frame in PORTRAITS:
        name, data, meta = convert_portrait_bitmap(agg, h2pal, palette, name, size, icn_name, frame)
        w, h, rgb = decode_bitmap(data)
        if (w, h) != tuple(size) or len(rgb) != w * h:
            raise ValueError(f"{name}: bitmap round-trip failed")
        (defs_dir / name).write_bytes(data)
        members[name] = data
        manifest["bitmaps"].append(meta)
    if not args.no_preview:
        hero_compare([(parse_def(members[hr.def_name]), hr.cast_point, hr.cast_frame) for hr in COMBAT_HEROES],
                     lod_member(args.h3sprite, "CH00.DEF"),
                     lod_member(args.h3sprite, "CmFlagL.def"), prev_dir / "_hero_vs_CH00.png")
        bitmap_compare([members[n] for n, *_ in PORTRAITS],
                       [lod_member(args.h3bitmap, n) for n in ("HPL000KN.pcx", "HPS000KN.pcx",
                                                                "HPL001KN.pcx", "HPS001KN.pcx")],
                       prev_dir / "_hero_portraits.png")

    # HoMM2 combat spell visuals
    fx_meta = []
    for name, data, meta in convert_spell_effects(agg, h2pal, palette, remap):
        parsed = validate_def(name, data, meta)
        (defs_dir / name).write_bytes(data)
        members[name] = data
        fx_meta.append(meta)
        if not args.no_preview:
            fx_preview(parsed, prev_dir / ("fx_" + Path(name).stem + ".png"), meta["anchor"])
    ml_defs, ml_table = convert_morale_luck(agg, h2pal, palette, remap)
    for name, data, meta in ml_defs:
        parsed = validate_def(name, data, meta)
        (defs_dir / name).write_bytes(data)
        members[name] = data
        fx_meta.append(meta)
        if not args.no_preview:
            fx_preview(parsed, prev_dir / ("fx_" + Path(name).stem + ".png"), meta["anchor"])
    manifest["h2_morale_luck"] = ml_table
    name, data, meta = convert_lich_cloud(agg, h2pal, palette, remap)
    parsed = validate_def(name, data, meta)
    (defs_dir / name).write_bytes(data)
    members[name] = data
    fx_meta.append(meta)
    manifest["h2_lich_cloud"] = {k: meta[k] for k in ("def", "placement", "sound", "sound_source", "canvas",
                                                       "anchor", "frames", "source", "placement_note")}
    if not args.no_preview:
        fx_preview(parsed, prev_dir / ("fx_" + Path(name).stem + ".png"), meta["anchor"])
    manifest["spell_effects"] = fx_meta
    manifest["h2_spell_visuals"] = h2_spell_visuals(fx_meta)
    if not args.no_preview:
        pairs = [("H2SE21.DEF", "C01spW0.def"), ("H2SE28.DEF", "C02spF0.def"),
                 ("H2SE33.DEF", "C04spF0.def"), ("H2SE15.DEF", "C11spA1.def"),
                 ("H2SE23.DEF", "C13spE0.def"), ("H2SE07.DEF", "C05spW0.def")]
        fx_compare([(parse_def(members[a]), parse_def(lod_member(args.h3sprite, b)) if lod_member(args.h3sprite, b) else None)
                    for a, b in pairs], prev_dir / "_fx_vs_retail.png")

    # HoMM2 sounds -> build/h2mash.snd
    snd_members, snd_meta, snd_sources = convert_sounds(agg)
    snd = build_snd(snd_members)
    worst = validate_snd(snd, snd_members, snd_sources)
    (build / "h2mash.snd").write_bytes(snd)
    snd_meta["archive"] = "h2mash.snd"
    snd_meta["count"] = len(snd_members)
    snd_meta["format"] = "RIFF WAVE IMA ADPCM (0x11) mono 22050 Hz 4-bit, block 512 / 1017 samples, like heroes3.snd"
    snd_meta["worst_relative_error"] = round(worst, 5)
    manifest["h2_sounds"] = snd_meta

    # Summary tables for the host (creature table rows, g_combatHeroSprites rows)
    by_def = {m["def"]: m for m in manifest["defs"]}
    manifest["h2_creatures"] = {
        str(H2_CREATURE_BASE + c.h2id): {
            "h2_id": c.h2id, "name": c.name, "race": RACE_NAMES[c.race], "def": c.def_name,
            "sound_prefix": H2_SOUND_PREFIX[c.snd], "missile_def": missile_def(c),
            "wide": c.wide, "flying": c.flying, "shooter": c.shooter, "undead": c.undead,
            "image_height": by_def[c.def_name]["image_height"],
            "portrait_frame": H2_CREATURE_BASE + c.h2id + 2}
        for c in CREATURES}
    manifest["combat_heroes"] = [by_def[hr.def_name]["combat_hero_sprite_entry"] for hr in COMBAT_HEROES]
    check_frame_names(members)

    lod = build_lod(members)
    back = read_lod(lod)
    if back != members:
        raise ValueError("LOD round-trip mismatch")
    (build / "h2mash.lod").write_bytes(lod)
    (build / "h2mash-assets.json").write_text(json.dumps(manifest, indent=1) + "\n")
    if CLIPPED_PIXELS:
        print(f"note: dropped {len(CLIPPED_PIXELS)} off-canvas stray pixel(s)")
    print(f"wrote {build / 'h2mash.lod'} ({len(lod)} bytes, {len(members)} members)")
    for meta in manifest["defs"]:
        print(f"  {meta['def']:<13} {meta['canvas'][0]}x{meta['canvas'][1]} groups "
              + " ".join(f"{g}:{n}" for g, n in meta["groups"].items()))
    for meta in manifest["portraits"]:
        if "skipped" in meta:
            print(f"  {meta['def']:<13} skipped ({meta['skipped']})")
        else:
            print(f"  {meta['def']:<13} {meta['canvas'][0]}x{meta['canvas'][1]} from {meta['source']['retail']}: "
                  f"{meta['retail_frames']} retail + {meta['padding_frames']} pad + {H2_PORTRAIT_COUNT} HoMM2 frames")
    return 0


if __name__ == "__main__":
    sys.exit(main())
