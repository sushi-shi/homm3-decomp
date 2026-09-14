// artifact.h - prototypes of artifact.cpp (compiland artifact.obj)
#ifndef HOMM3_ARTIFACT_H
#define HOMM3_ARTIFACT_H

#include <bitset>
#include <va.h>

// The artifact-id domain. Added 2026-08-08 with its first consumer,
// recruit.obj's siege_artifact_to_creature (0x550360) - only the four
// war machines its jump table covers are listed; grow the roster per
// consumer, as TCreatureType and ESpellId are grown. Values are
// byte-proven by that switch (`lea eax,[ecx-3]` over four dense
// cases); the names are the Dreamcast TArtifact enumerators
// (eArtifactCatapult 3, eArtifactBallista 4, eArtifactAmmoCart 5,
// eArtifactFirstAidTent 6 in evidence/dreamcast/enums.csv) respelled
// to this tree's convention.
// Corroborated 2026-08-08 from a second side: hero.obj's two artifact
// tallies (get_equipped_artifacts 0x4d9070, get_number_in_backpack
// 0x4d90c0) both skip exactly the four consecutive ids 3,4,5,6 as the
// war machine block, independently of recruit's jump table.
// Grown 2026-08-08 for hero.obj's bonus getters, which are nothing but
// IsWieldingArtifact gates. Each block below carries its own retail
// witness; every DC spelling comes from evidence/dreamcast/enums.csv.
// PLACEMENT NOTE: armygrp.h carries a SECOND artifact roster
// (EArtifactId, the combat-side gates). These ids went here, into the
// artifact domain's own owner header, rather than there - armygrp.h is
// inside initialize.cpp's include closure through town.h, and putting
// them in EArtifactId measurably moved initialize_game_data 96.09 ->
// 94.07 through the include-set sensitivity class with no semantic
// change. artifact.h is not in that closure. Unifying the two rosters
// is a separate, measured decision.
#include "artifact_type.h"

// The per-artifact traits record. The 32-byte STRIDE is byte-proven by
// hero::IsWieldingArtifact's `shl esi,5` index, and +0x18 by the same
// body: it holds the id of the COMBINATION artifact this piece belongs
// to, -1 when the artifact is not a component of one.
// The DC's own TArtifactTraits (evidence/dreamcast/members.csv) is only
// 20 bytes - m_name 0, m_cost 4, m_allowableSlotMask 8, m_class 12,
// m_description 16 - i.e. the AB-era record without the Shadow of Death
// combination column. Retail's artraits.txt parser at 0x44cd50 independently
// confirms every DC-era member at the same offset, then writes the four
// Complete-era fields at +0x14..+0x1d. hero::remove_artifact corroborates the
// allowable-slot class, combination indices and spell-list flag.
// Before normalization (type): TArtifactTraits.
struct ArtifactTraits {
    // armyGroup::get_luck_description indexes artifact 0x55 at stride
    // 0x20 and passes +0 directly to format_string: the display name.
    const char* m_name;           // +0x00
    int m_cost;                   // +0x04
    // Equipped-slot class. remove_artifact compares this between the
    // combination artifact and each component, then uses it to index the
    // hero's per-slot equipped counts.
    int m_allowableSlotMask;      // +0x08
    int m_artifactClass;          // +0x0c
    const char* m_description;    // +0x10
    // comboType is set on an assembled artifact; targetCombo is set on each
    // component. Both are indices into gCombinationArtifacts, or -1.
    int m_comboType;              // +0x14
    int m_targetCombo;            // +0x18
    unsigned char m_disabled;     // +0x1c
    // Removing this artifact requires rebuilding the available-spell list.
    unsigned char m_givesSpells;      // +0x1d
    // Trailing alignment after the byte at +0x1d, rounding the proven
    // 0x20-byte artifact stride. NH3API explicitly leaves these two bytes unnamed.
    char m_paddingAfterGivesSpells[0x2];
};
SIZE(ArtifactTraits, 32);

// The retail artslots.txt table: 19 display names paired with the first of
// the 15 allowable-slot masks containing that physical equipment slot.
// Its 8-byte stride and both fields are written by 0x44cd50; the public name
// is preserved by the retail symbol at 0x660b64.
// Before normalization (type): TArtifactSlotTraits.
struct ArtifactSlotTraits {
    const char* m_name;
    int m_type;
};
SIZE(ArtifactSlotTraits, 8);

// The combination-artifact record. The 24-byte stride is byte-proven by
// IsWieldingArtifact's `lea r,[id + 2*id]` / `[base + 8*r]` chain, and
// +0x00 by the same body - it is the assembled artifact's own id, which
// IsWieldingArtifact recurses on. The remaining 20 bytes are the
// component mask the four retail-only combination bodies at
// 0x4dbe80..0x4dc100 walk as a bitset<144> (five dwords). This is the
// canonical record used by the artifact table and all its consumers.
// Before normalization (type): TCombinationArtifact.
struct CombinationArtifact {
    // The cinit at 0x44c960 builds each of the twelve records in a 24-byte
    // stack temporary - the id dword stored FIRST, then the component
    // builder's five-word result copied in behind it - and only then
    // `rep movsd`s the whole temporary into the table. That is an inlined
    // two-argument constructor plus the implicit copy, and VC6 cannot
    // spell it any other way: brace initialization of a record carrying a
    // bitset member is a hard C2440 for this compiler.
    CombinationArtifact(int id, const std::bitset<144>& usedComponents)
        : m_artifactId(id), m_components(usedComponents) {}

    int m_artifactId;             // +0x00
    std::bitset<144> m_components;
};
SIZE(CombinationArtifact, 24);

// Cinit-owned tables consumed by artifact.cpp's ordinary source body.
DATA(0x00693898)
extern const std::bitset<19> g_artifactSlotMasks[15];
DATA(0x006938d8)
extern const CombinationArtifact g_combinationArtifactTable[12];

// Retail .data 0x660b68 and 0x660b6c, two adjacent storage cells retail
// LOADS and then indexes (`mov eax,[0x660b68]` / `[esi + eax + 0x18]`)
// - the akHeroTraits reference-cell pattern.
// akArtifactTraits' name is DC-attested
// (?akArtifactTraits@@3AAY0HP@$$CBUTArtifactTraits@@A); its DC bound of
// 127 is AB-era and is NOT carried over, which is why this is spelled
// as a pointer rather than akHeroTraits' reference-to-array - the
// Complete-era artifact count is 144, now proved by artifact.obj's retail
// parser and table extent. The combination table has NO DC row (a Shadow of
// Death addition); its name is INVENTED. artifact.obj owns both reference
// cells and their underlying storage; the two excluded cinit tables remain a
// separate source-initializer admission.
extern const ArtifactTraits* g_artifactTraits;
extern const CombinationArtifact* g_combinationArtifacts;
extern const ArtifactSlotTraits* g_artifactSlotTraits;

// Retail .data 0x6aa9f8, defined by townmgr.cpp and consumed by the AI
// town-entry path. The record itself is completed by hero.h; an extern
// array of unknown bound can retain that single owning declaration here.
struct ArtifactRecord;
extern ArtifactRecord g_blacksmithArtifacts[];

// Four signed primary-skill deltas per artifact. remove_artifact walks all
// 144 rows when dismantling a combination; the adjacent address is a real
// retail data symbol and is used as the pointer-loop bound.
DATA(0x0063e758)
extern const signed char g_artifactPrimarySkillBonuses[][4];
DATA(0x0063e998)
extern const signed char g_artifactPrimarySkillBonusesEnd[];

// --- globals ---
// CODEVIEW(E:\gamedcs\artifact.cpp:56, dc 0x4fec0) unsigned char InitializeArtifactTraitsTable();
// CODEVIEW(E:\gamedcs\artifact.cpp:112, dc 0x50058) void InitializeArtifactTraits(int id, const std::vector<char* resource);

// --- TArtifactTraits ---
// CODEVIEW(E:\gamedcs\artifact.cpp:49, dc 0x508e4) void TArtifactTraits::TArtifactTraits();

// --- TSpreadsheetResource ---
// CODEVIEW(E:\gamedcs\TextResource.h:108, dc 0x5088c) int TSpreadsheetResource::GetNumberOfRows();
// CODEVIEW(E:\gamedcs\TextResource.h:128, dc 0x508a4) const std::vector<char* TSpreadsheetResource::GetRow(int r);

// --- `anonymous namespace' ---
// CODEVIEW(E:\gamedcs\artifact.cpp:33, dc 0x508bc) void `anonymous namespace'::TAutoStrPtr::TAutoStrPtr();
// CODEVIEW(E:\gamedcs\artifact.cpp:34, dc 0x508c4) void `anonymous namespace'::TAutoStrPtr::~TAutoStrPtr();
// CODEVIEW(E:\gamedcs\artifact.cpp:36, dc 0x508dc) void `anonymous namespace'::TAutoStrPtr::set(char* pStr);
// CODEVIEW(E:\gamedcs\artifact.cpp:38, dc 0x508e0) char* `anonymous namespace'::TAutoStrPtr::get();

// --- std ---
// CODEVIEW(..\stlport\stl_bitset.h:414, dc 0x50904) void std::bitset<18,unsigned long>::bitset<18,unsigned long>();
// CODEVIEW(..\stlport\stl_bitset.h:564, dc 0x50924) std::bitset<18,unsigned std::bitset<18,unsigned long>::operator[](__$ReturnUdt, unsigned __pos);
// CODEVIEW(..\stlport\stl_bitset.h:376, dc 0x50944) void std::bitset<18,unsigned long>::reference::~reference();
// CODEVIEW(..\stlport\stl_bitset.h:379, dc 0x50948) std::bitset<18,unsigned* std::bitset<18,unsigned long>::reference::operator=(unsigned char __x);
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0x50984) unsigned std::vector<std::vector<char *,std::allocator<char *> > *,std::allocator<std::vector<char *,std::allocator<char *> > *> >::size();
// CODEVIEW(..\stlport\stl_vector.h:204, dc 0x50990) std::vector<char** std::vector<std::vector<char *,std::allocator<char *> > *,std::allocator<std::vector<char *,std::allocator<char *> > *> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_bitset.h:107, dc 0x509b0) void std::_Base_bitset<1,unsigned long>::_Base_bitset<1,unsigned long>();
// CODEVIEW(..\stlport\stl_bitset.h:370, dc 0x509cc) void std::bitset<18,unsigned long>::reference::reference(std::bitset<18,unsigned* __b, unsigned __pos);
// CODEVIEW(..\stlport\stl_vector.h:180, dc 0x50a00) std::vector<char** std::vector<std::vector<char *,std::allocator<char *> > *,std::allocator<std::vector<char *,std::allocator<char *> > *> >::begin();
// CODEVIEW(..\stlport\stl_bitset.h:120, dc 0x50a04) unsigned long std::_Base_bitset<18,unsigned long>::_S_maskbit(unsigned __pos);
// CODEVIEW(..\stlport\stl_bitset.h:117, dc 0x50a20) unsigned std::_Base_bitset<18,unsigned long>::_S_whichbit(unsigned __pos);

#endif  /* HOMM3_ARTIFACT_H */
