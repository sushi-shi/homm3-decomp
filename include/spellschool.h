// spellschool.h - the magic-school selector domain.
// HAND-OWNED. No compiland of its own: this header exists only so the
// ONE TSpellSchool definition can be seen from both the AI side
// (ai_tactical.h) and the hero side (hero.h) without either dragging
// the other's include closure in. The artifact.h / herospec.h /
// prefs.h precedent - a domain type that two owners need gets its own
// header rather than a second definition.
#ifndef HOMM3_SPELLSCHOOL_H
#define HOMM3_SPELLSCHOOL_H

// Magic school selector; get_protection_value (0x4396e0) takes it, and
// so does the hero-side pair hero::GetSpellSchoolLevel (0x4e5100) /
// hero::GetHighestSchool (0x4e51c0).
// Enumerator names and values are the Dreamcast roster verbatim
// (evidence/dreamcast/enums.csv); the values are a BITMASK, which is
// why those three take a "school_mask" and why eSchoolAll is 15.
// kNumSpellSchools sharing eSchoolWater's 4 is the dump's own doing,
// not a transcription slip.
// The BIT -> SECONDARY SKILL pairing is byte-proven by
// hero::GetHighestSchool, which walks the mask against the hero's own
// secondary-skill band at +0xc9: bit 1 with +0xd8 (skill 15, Air),
// bit 2 with +0xd7 (14, Fire), bit 8 with +0xda (17, Earth), bit 4
// with +0xd9 (16, Water).
// No retail byte is claimed for the SPELLING - this is DC naming
// evidence over an int-sized domain, and it changes no codegen:
// A/B-measured 2026-08-08 against the `typedef int` it replaced, ZERO
// of the 977 scored functions in all 52 units moved by so much as a
// byte.
enum TSpellSchool {
    const_invalid_school = 0,
    eSchoolAir = 1,
    eSchoolFire = 2,
    eSchoolWater = 4,
    eSchoolEarth = 8,
    eSchoolAll = 15,
    kNumSpellSchools = 4
};

#endif  /* HOMM3_SPELLSCHOOL_H */
