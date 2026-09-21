#include "va.h"
#include "includes.h"

#include <algorithm>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "seerhut.h"

#include "advmgr.h"
#include "ai_player.h"
#include "creaturetype.h"
#include "game.h"
#include "hero.h"
#include "herospec.h"
#include "quest.h"
#include "resourcemanager.h"
#include "seerhuttext.h"
#include "textresource.h"
#include "winmgr.h"

void aiEquipArtifacts(hero* currentHero);
void aiJoinDecision(hero* currentHero, TCreatureType creature, int amount);
void doMonsterJoinDialog(hero* currentHero, TCreatureType creature,
                            int amount);

// The retail factory is frameless under /GX even though its new-expressions
// construct vector-owning classes, so this TU saw the nothrow deallocator
// declaration used by the other frameless-construction compilands.
__declspec(nothrow) void __cdecl operator delete(void* value);

// DC dialog ownership changed with the quest representation. In the old
// TSeerHut (dc0x12d238..0x12d4dc), one artifact and text-row byte drive
// progress/proposal, acceptance/refusal and immediate-reward prompts.
// Complete doSeerEvent0x573670 loads the quest pointer and invokes its
// virtual slots4/5 for proposal/progress. The artifact quest owns its vector
// of requirements and generated/custom text in0x56f8a0/0x56fbc0. The hut
// marks visits before checking satisfaction, then uses one completion offer;
// declining it returns, without the old separate refusal/acceptance dialogs.
// Thus DoAlreadyHaveProposalDialog and the two acknowledgement helpers
// belong to the retired single-artifact interaction, not missing wrappers.
//
// DC SaveSeerList0x12d7e8/LoadSeerList0x12d854 are static methods that
// always select gpGame's seer pool and check each old record result. Complete
// NewfullMap::save0x4fdf40/load0x4fdbc0 operate on this map's +0x60 pool.
// Both retain the two-byte count; load passes saveVersion and registers each
// new polymorphic quest in this map's +0xb0 object-data pool. Those receiver
// and ownership changes supersede the old global static list interfaces.

// --- retail's virtual quest family ------------------------------------
// The Dreamcast port has no counterpart for any of these: its TSeerHut
// carries the quest inline and switches on a type word. Retail's ten quest
// classes each own a 15-slot vtable, and the slots reconstructed below are
// the ones whose bodies are self-identifying.

// Slot 2 is the "does this hero satisfy the quest" predicate, and the
// bodies here name their own classes: the experience quest compares the
// short at hero+0x55 (hero::level), the be-hero quest the dword at hero+0x1a
// (hero::id), and the belong-to-player quest the signed byte at hero+0x22
// (hero::owner) - three different widths at three fields hero.h already
// names, each matching its class. Slot 8 is the type discriminator, a bare
// constant; all ten leaf bodies read 1..9 in vtable address order, which is
// the h3m quest-type enumeration (see quest.h).

// Slots 11 and 12 are the two deserializers, and they come as a family: each
// leaf reads its own payload through TAbstractFile slot 1 and then chains to
// the base body (0x56cd00 / 0x56ce50). The two differ by encoding width at
// every class that has both - be-hero reads a short in slot 11 and a byte in
// slot 12, the experience quest a short and a dword - which is what fixes
// slot 11 as the savegame reader (it also carries the format version) and
// slot 12 as the h3m reader.

// Retail-only rows: no Dreamcast roster entry corresponds to any of them.

// The AI's resource valuation, 0x526cc0: player index in ecx, a seven-entry
// cost vector in edx, summed against the per-player multiplier table the
// same 0x168-byte player stride reaches. Declared here rather than pulled in
// from a header - seerhut.cpp is its only consumer in this tree.
int aiResourceCost(int player, const int* costs);

type_quest* createQuest(int questType, unsigned char flags);

std::string formatString(const char* format, ...);

// The seven localized resource names.  The resource-quest string builders
// walk this array in lockstep with their seven-dword payload.
DATA(0x006a5e64) extern const char* g_resourceNames[7];

// The nine compass phrases describing a quest monster's map region.
// Retail reaches every cell directly from the initializer below; their
// clockwise order is north, north-east, east, south-east, south, south-west,
// west, north-west, then centre.
DATA(0x006a5c48) extern const char* g_questMonsterDirections[9];

// kb.obj's centred message box, 0x4f6570 - kb.h declares it, but the ten
// quest dialog bodies below are this compiland's only consumers of that
// header and the include-set residual class makes a one-line declaration
// the cheaper edge, exactly as format_string above.
void normalDialog(const char* text, int mbType, int x, int y,
    int resType1, int resExtra1, int resType2, int resExtra2,
    int special, int timeout, int resType3, int resExtra3);
void extendedDialog(const char* text,
    std::vector<type_dialog_resource>& resources,
    long x, long y, long timeout);

// Retail 0x56c3e0. Pull seerhut.txt out of the resource cache, fill both
// three-column tables from it, then walk every row

// Complete loads quest text from a spreadsheet; Dreamcast initializes a fixed table.
// E:\gamedcs\seerhut.cpp:50, dc 0x12cd28
VA(0x0056c3e0, 0x183)  // anchor-string(seerhut.txt) + anchor-callee(LoadSeerHutTextColumn)
unsigned char initializeSeerHutText()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00683214, seerHutSpreadsheetName, "seerhut.txt"));
    if (!sheet)
        return 0;

    if (sheet->getNumberOfRows() < 60)
        return 0;

    for (int c = 0; c < 3; ++c) {
        loadSeerHutTextColumn(sheet, &g_seerHutTextB[c], c + 1);
        loadSeerHutTextColumn(sheet, &g_seerHutTextA[c], c + 4);
    }

    for (int row = 50; row < sheet->getNumberOfRows(); ++row) {
        const char* name = sheet->getRow(row)[0];
        if (!name[0] || name[0] == ' ')
            continue;
        g_seerHutNames.insert(g_seerHutNames.end(), name);
    }

    sheet->dispose();
    return 1;
}

VA_COMPGEN(0x0056cbe0, 0x21, SCALAR_DELETING_DTOR, type_quest)

VA(0x0056cb80, 0x5F)  // sole callee of all nine factory arms
type_quest::type_quest(unsigned char flags)
{
    m_seerHut = flags;
    m_textVariant = rand() % 3;
    m_limit = -1;
}

// The common virtual base destructor. Its three std::string members unwind
// in reverse order.
VA(0x0056cc10, 0xA0)
type_quest::~type_quest()
{
}

VA(0x0056ccb0, 0x44)
unsigned char type_quest::hasExpired() const
{
    if (m_limit < 0)
        return 0;
    return m_limit < g_game->getCurrentTurn();
}

// Slot 11's base body is the savegame counterpart of LoadFromMap: flag,
// text-table row, deadline, then the same three length-prefixed strings.
// Both bodies belong to Complete's replacement quest hierarchy; no Dreamcast
// type_quest procedure supplies a source dossier.

// Retail's EH states are 0 / -1 / 1 / -1 / 2: each returned string dies
// before the next read. The first two temporaries share [ebp-0x1c] and the
// third uses [ebp-0x2c]. Binding all three by const reference at function
// scope raises MAX to 49.7043% / 41.9271%, but keeps three objects alive
// together and contradicts those cleanups. Those peaks remain history; the
// expression lifetimes restore the two-slot layout and cleanup ordering.

// Two inline boundaries remain: retail calls the FIRST temporary's _Tidy
// and expands the other two, while retaining all three string assign calls.
// The candidate expands that first cleanup and the final assign. The old
// note claiming three retail _Tidy calls was wrong. Negative controls on
// LoadFromMap: assign(ReadLengthPrefixedString(file)) and a value-returning
// scalar reader are byte-flat to plain assignment (10.50%); separate
// block-local string values give 6.64%, like block-local const references.
// None recovers the missing boundaries, and a longer lifetime is not a fix.

// The short-lived form also recycles the incoming argument slot for the
// row and deadline. Its byte-typed row lowers to retail's dword load and
// mask; explicitly reading one byte into an int and masking is byte-flat,
// so that instruction choice does not prove the source buffer was an int.
// Earlier controls on the reference-bound Load were that dword row (48.83%),
// one shared scalar block (49.60%), and hoisting extra (49.60%), against the
// 49.7043% peak. Those results do not justify retaining the wrong lifetimes.
VA(0x0056cd00, 0x14F)  // anchor-vtable 0x64174c slot 11 + the chain from all eight leaf Loads, retail-only
void type_quest::load(TAbstractFile* file, int version)
{
    {
        unsigned char flag;
        file->read(&flag, sizeof(flag));
        m_seerHut = flag != 0;
    }
    {
        unsigned char row;
        file->read(&row, sizeof(row));
        m_textVariant = row;
    }
    {
        int extra;
        file->read(&extra, sizeof(extra));
        m_limit = extra;
    }
    m_proposalText = readLengthPrefixedString(file);
    m_progressText = readLengthPrefixedString(file);
    m_completionText = readLengthPrefixedString(file);
}

// Slot 12's base body: the h3m form. Same three strings, but only the
// deadline ahead of them - the selector and the table row are savegame-only,
// which is the split quest.h records between the two loaders.
// The current expression lifetimes match retail's 0x20 frame and EH states
// 0/-1/1/-1/2; the first two results reuse one slot and the third uses a
// second. See Load above for the two remaining inline boundaries and the
// preserved 41.9271% peak from the rejected longer-lived form.
// C2 measures cb=147, budget=1000 and six root candidates. The first _Tidy
// gets 200 units for its 152-unit body; the final three-argument assign gets
// 348 for its 307-unit body. Both expand, contrary to these retail sites.
// Making the existing ReadLengthPrefixedString definition visible is byte-
// and trace-neutral: C1 does not admit that body as an inline candidate.
// A value-returning forwarding reader retains an extra wrapper call and
// still expands the final assign (23.1563%). A read-into-string-reference
// helper and a shared three-string reader remain out of line, contrary to
// retail's expanded read/assign/cleanup sequence. Neither is retained.
// Individual proposal/progress/completion const-reference scopes preserve
// cleanup order but score 8.6146/9.2396/9.6667%, below the 10.50% expression
// body. A shared assignment helper taking const string& reproduces that
// expression body byte for byte; taking string by value remains called and
// removes the caller's EH sequence (33.5625%, 112 bytes versus retail 286).
// Directly reading the deadline member loses the incoming-slot scratch and
// scores 2.6771%. None supplies the missing inline boundaries.
// /Ob1 is byte-identical to the configured /Ob2 body. /Os retains all
// three _Tidy calls and uses __EH_prolog (30.4375%); retail retains only
// the first cleanup and emits its prologue inline. Neither policy explains it.
VA(0x0056ce50, 0x11E)  // anchor-vtable 0x64174c slot 12 + the chain from all eight leaf LoadFromMaps, retail-only
void type_quest::loadFromMap(TAbstractFile* file)
{
    {
        int extra;
        file->read(&extra, sizeof(extra));
        m_limit = extra;
    }
    m_proposalText = readLengthPrefixedString(file);
    m_progressText = readLengthPrefixedString(file);
    m_completionText = readLengthPrefixedString(file);
}

VA(0x0056cf70, 0xCD)
void type_quest::save(TAbstractFile* file)
{
    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}

// The deadline suffix both base dialog getters below append when the quest
// is dated. It is the only reader of the text row's LAST column: retail
// takes it at `[row + 0x334]`, which is 51 * sizeof(std::string) plus the
// _Ptr member, and - unlike every other row read in this file - WITHOUT a
// quest_type() call, so the ternary on field_04/field_38 is spelled inline
// with the 832-byte product duplicated into both arms, exactly as
// quest_text()'s own note describes. Naming the remaining-day subtraction at
// this lifetime raises retail similarity from 85.1124% to 93.4944%. Moving it
// earlier, naming the text variant too, or constructing text directly from the
// separator loses agreement; the combined direct/copy-initialization controls
// score 75.2472%.

VA(0x0056d040, 0x1F7)  // anchor-caller(both base dialog getters) + the row-column-51 read, retail-only
std::string type_quest::getTimeLimitText()
{
    int days = static_cast<short>(
        (g_game->m_month * 4 + g_game->m_week - 5) * 7
        + g_game->m_day);
    std::string text;
    text = DATA_COMPGEN(0x00660330, questTimeLimitSeparator, " ");
    int remainingDays = m_limit - days;
    const std::string* row =
        m_seerHut ? g_questTextA[m_textVariant] : g_questTextB[m_textVariant];
    text += formatString(row[QUEST_TEXT_TIME_LIMIT].c_str(), remainingDays);
    return text;
}

VA(0x0056d240, 0xCA)
std::string type_quest::getProposalDialogText()
{
    if (m_limit < 0)
        return m_progressText;
    return m_progressText + getTimeLimitText();
}

VA(0x0056d310, 0xCA)
std::string type_quest::getProgressDialogText()
{
    if (m_limit < 0)
        return m_proposalText;
    return m_proposalText + getTimeLimitText();
}

VA(0x0056d3e0, 0x2A)
std::string type_experience_quest::getRequirementText()
{
    return formatString(DATA_COMPGEN(0x00660a1c, decimalFormat, "%d"),
                         m_requiredLevel);
}
VA(0x0056d410, 0x72)
std::string type_experience_quest::getQuestDescription()
{
    return formatString(questText(QUEST_TEXT_DESCRIPTION).c_str(),
                         m_requiredLevel);
}

VA(0x0056d490, 0x1A)
unsigned char type_experience_quest::isSatisfied(hero* currentHero)
{
    return currentHero->m_level >= m_requiredLevel;
}
VA(0x0056d4b0, 0x9F)
void type_experience_quest::doProposalDialog(hero* currentHero)
{
    normalDialog(getProposalDialogText().c_str(), 1, -1, -1, 0x11,
                 m_requiredLevel, -1, 0, -1, 0, -1, 0);
}

VA(0x0056d550, 0x9B)
void type_experience_quest::doProgressDialog()
{
    normalDialog(getProgressDialogText().c_str(), 1, -1, -1, 0x11,
                 m_requiredLevel, -1, 0, -1, 0, -1, 0);
}

VA(0x0056d5f0, 0x35)
void type_experience_quest::load(TAbstractFile* file, int version)
{
    unsigned short level;

    file->read(&level, sizeof(level));
    m_requiredLevel = level;
    type_quest::load(file, version);
}

VA(0x0056d630, 0xE1)
void type_experience_quest::save(TAbstractFile* file)
{
    short level = m_requiredLevel;
    file->write(&level, sizeof(level));

    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}
VA(0x0056d720, 0x23E)
void type_experience_quest::setDefaultText()
{
    const std::string* texts = questTexts();
    if (m_proposalText.length() == 0)
        m_proposalText = formatString(texts[QUEST_TEXT_PROPOSAL].c_str(),
                              m_requiredLevel);
    if (m_progressText.length() == 0)
        m_progressText = formatString(texts[QUEST_TEXT_PROGRESS].c_str(),
                              m_requiredLevel);
    if (m_completionText.length() == 0)
        m_completionText = formatString(texts[QUEST_TEXT_COMPLETION].c_str(),
                              m_requiredLevel);
}

VA(0x0056d960, 0x22)
std::string type_skill_quest::getRequirementText()
{
    return skillRequirementText(m_requiredSkills);
}
VA(0x0056d990, 0xD3)
std::string type_skill_quest::getQuestDescription()
{
    return formatString(questText(QUEST_TEXT_DESCRIPTION).c_str(),
                         skillRequirementText(m_requiredSkills).c_str());
}

// E:\gamedcs\seerhut.cpp
VA(0x0056da70, 0x60)  // anchor-vtable 0x6417c4 slot 2, retail-only
unsigned char type_skill_quest::isSatisfied(hero* currentHero)
{
    for (int i = 0; i < 4; ++i) {
        int have = currentHero->getPrimarySkill(i);
        if (have < m_requiredSkills[i])
            return 0;
    }
    return 1;
}

// Slot 4 reports only the primary-skill requirements the visiting hero still
// lacks. A custom progress line takes the direct path; otherwise retail
// formats the missing-skill list into the table's progress column and appends
// the common deadline suffix before showing the same pictures.

VA(0x0056dad0, 0x28C)  // anchor-vtable 0x6417c4 slot 4 + exact HD structural twin
void type_skill_quest::doProposalDialog(hero* currentHero)
{
    signed char missing[4];
    for (int i = 0; i < 4; ++i) {
        int have = currentHero->getPrimarySkill(i);
        // The actual requirement is a signed byte. The old const int& bound
        // a converted temporary; it never referred back to the byte field.
        const signed char& required = m_requiredSkills[i];
        missing[i] = required > have ? required : 0;
    }

    if (m_progressText.length() > 0) {
        std::string text = getProposalDialogText();
        const char* textPointer = text.c_str();
        {
            std::vector<type_dialog_resource> dialogResources;
            type_dialog_resource resource;
            for (int i = 0; i < 4; ++i) {
                if (missing[i] > 0) {
                    resource.m_resource = 0x1f + i;
                    resource.m_qualifier = 0x10000
                        | static_cast<unsigned short>(missing[i]);
                    type_dialog_resource* position = dialogResources.end();
                    dialogResources.insert(position, resource);
                }
            }
            extendedDialog(
                textPointer, dialogResources, -1, -1, 0);
        }
    } else {
        std::string requirement = skillRequirementText(missing);
        const char* requirementPointer = requirement.c_str();
        const std::string* texts = questTexts();
        std::string text = formatString(
            texts[QUEST_TEXT_PROGRESS].c_str(), requirementPointer);
        text += getTimeLimitText();
        const char* textPointer = text.c_str();
        {
            std::vector<type_dialog_resource> dialogResources;
            type_dialog_resource resource;
            for (int i = 0; i < 4; ++i) {
                if (missing[i] > 0) {
                    resource.m_resource = 0x1f + i;
                    resource.m_qualifier = 0x10000
                        | static_cast<unsigned short>(missing[i]);
                    type_dialog_resource* position = dialogResources.end();
                    dialogResources.insert(position, resource);
                }
            }
            extendedDialog(
                textPointer, dialogResources, -1, -1, 0);
        }
    }
}

// Slot 5 presents one primary-skill picture for every positive requirement.
// The picture class advances from 0x1f with the skill index, while the
// qualifier packs the displayed value below a high-word one.
// Residual (96.6180%, 2026-09-07): the resource vector has an inner scope,
// ending before the lifetime-extended text. This removes the three post-delete
// zero stores and restores retail's 0x30 frame and EBX loop down-counter
// (76.2135 -> 94.2360%). Declaring the skill cursor before the vector also
// restores its initialization schedule (96.6180%). The three-variable walk
// preserves retail's inc-cursor/dec-count loop and picture-id induction.
// Controls: indexed i < 4 within the scope gives 82.5169%; a named string
// instead of the const reference is byte-flat at 94.2360%. Moving the picture
// resource outside the loop is byte-flat after the cursor-order repair.
// Remaining: c_str reloads the return slot instead of dereferencing returned
// EAX, and string destruction uses ECX rather than retail's EAX/ECX pair.
// The earlier bare c_str pointer destroyed the temporary before the loop and
// is invalid; mutable/const lifetime-extending references gave the same bytes.
// This Complete quest has no Dreamcast counterpart to settle the source form.
// E:\gamedcs\seerhut.cpp
VA(0x0056dd60, 0xF5)  // anchor-vtable 0x6417c4 slot 5 + dialog picture rows, retail-only
void type_skill_quest::doProgressDialog()
{
    const std::string& text = getProgressDialogText();
    const char* textPointer = text.c_str();
    {
        const signed char* skill = m_requiredSkills;
        std::vector<type_dialog_resource> dialogResources;
        int picture = 0x1f;
        int remaining = 4;
        do {
            if (*skill > 0) {
                type_dialog_resource resource;
                resource.m_resource = picture;
                resource.m_qualifier = 0x10000
                    | static_cast<unsigned short>(*skill);
                dialogResources.push_back(resource);
            }
            ++skill;
            ++picture;
            --remaining;
        } while (remaining);
        extendedDialog(textPointer, dialogResources, -1, -1, 0);
    }
}

VA(0x0056de60, 0x29)
void type_skill_quest::load(TAbstractFile* file, int version)
{
    file->read(m_requiredSkills, sizeof(m_requiredSkills));
    type_quest::load(file, version);
}

VA(0x0056de90, 0x25)
void type_skill_quest::loadFromMap(TAbstractFile* file)
{
    file->read(m_requiredSkills, sizeof(m_requiredSkills));
    type_quest::loadFromMap(file);
}

VA(0x0056dec0, 0xD7)
void type_skill_quest::save(TAbstractFile* file)
{
    file->write(m_requiredSkills, sizeof(m_requiredSkills));

    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}
VA(0x0056dfa0, 0x124)
std::string type_skill_quest::skillRequirementText(
    const signed char (&skills)[4])
{
    std::vector<std::string> requirements;
    for (int i = 0; i < 4; ++i) {
        if (skills[i] > 0) {
            requirements.push_back(formatString(
                DATA_COMPGEN(0x00683220, skillRequirementFormat, "%s %i"),
                g_primarySkillNames[i], m_requiredSkills[i]));
        }
    }
    return joinTextList(requirements);
}
VA(0x0056e0d0, 0x169)  // anchor-vtable 0x6417c4 slot 14 + the shared text-table shape, retail-only
void type_skill_quest::setDefaultText()
{
    const std::string* texts = questTexts();
    std::string requirement = skillRequirementText(m_requiredSkills);

    if (m_proposalText.length() == 0)
        m_proposalText = formatString(texts[QUEST_TEXT_PROPOSAL].c_str(),
                                     requirement.c_str());
    if (m_completionText.length() == 0) {
        std::string formatted =
            formatString(texts[QUEST_TEXT_COMPLETION].c_str(),
                          requirement.c_str());
        m_completionText = formatted;
    }
}

VA(0x0056e240, 0xF2)
std::string type_defeat_hero_quest::getRequirementText()
{
    return g_game->getHero(m_defeatedHero)->m_name;
}

VA(0x0056e340, 0x8B)
std::string type_defeat_hero_quest::getQuestDescription()
{
    return formatString(questText(QUEST_TEXT_DESCRIPTION).c_str(),
                         g_game->getHero(m_defeatedHero)->m_name);
}

VA(0x0056e3d0, 0x06)
int type_defeat_hero_quest::questType()
{
    return 3;
}

VA(0x0056e3e0, 0x29)
unsigned char type_defeat_hero_quest::isSatisfied(hero* currentHero)
{
    if (currentHero->m_owner < 0)
        return 0;
    return (m_satisfiedMask & (1 << currentHero->m_owner)) != 0;
}
VA(0x0056e410, 0x99)
void type_defeat_hero_quest::doProposalDialog(hero* currentHero)
{
    normalDialog(getProposalDialogText().c_str(), 1, -1, -1, -1,
                 0, -1, 0, -1, 0, -1, 0);
}

VA(0x0056e4b0, 0x95)
void type_defeat_hero_quest::doProgressDialog()
{
    normalDialog(getProgressDialogText().c_str(), 1, -1, -1, -1,
                 0, -1, 0, -1, 0, -1, 0);
}

VA(0x0056e550, 0x26)
void type_defeat_hero_quest::notifyHeroDefeated(int heroId, int player)
{
    if (player >= 0 && heroId == m_defeatedHero)
        m_satisfiedMask |= 1 << player;
}

VA(0x0056e580, 0x6E)
void type_defeat_hero_quest::load(TAbstractFile* file, int version)
{
    {
        short id;

        file->read(&id, sizeof(id));
        m_defeatedHero = id;
    }
    if (version < 30 || (version > 30 && version < 36)) {
        m_satisfiedMask = 0;
    } else {
        unsigned char mask;

        file->read(&mask, sizeof(mask));
        m_satisfiedMask = mask;
    }
    type_quest::load(file, version);
}

VA(0x0056e5f0, 0xF4)
void type_defeat_hero_quest::save(TAbstractFile* file)
{
    short id = m_defeatedHero;
    file->write(&id, sizeof(id));
    {
        unsigned char mask = static_cast<unsigned char>(m_satisfiedMask);
        file->write(&mask, sizeof(mask));
    }

    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}
VA(0x0056e6f0, 0x29E)
void type_defeat_hero_quest::setDefaultText()
{
    const std::string* texts = questTexts();
    hero* defeatedHero;
    for (m_defeatedHero = game::HERO_COUNT - 1; m_defeatedHero > -1;
         --m_defeatedHero) {
        defeatedHero = g_game->getHero(m_defeatedHero);
        if (defeatedHero->m_order == m_mapHero)
            break;
    }
    if (m_defeatedHero != -1 && m_proposalText.length() == 0)
        m_proposalText = formatString(texts[QUEST_TEXT_PROPOSAL].c_str(),
                              defeatedHero->m_name);
    if (m_defeatedHero != -1 && m_progressText.length() == 0)
        m_progressText = formatString(texts[QUEST_TEXT_PROGRESS].c_str(),
                              defeatedHero->m_name);
    if (m_defeatedHero != -1 && m_completionText.length() == 0)
        m_completionText = formatString(texts[QUEST_TEXT_COMPLETION].c_str(),
                              defeatedHero->m_name);
}

VA(0x0056ea30, 0xF9)
std::string type_monster_quest::getRequirementText()
{
    const char* name = m_monsterId >= 0 && m_monsterId <= 0x96
                           ? g_creatureTypeTraits[m_monsterId].m_pluralName
                           : g_emptyRolloverText;
    return name;
}

VA(0x0056eb30, 0x90)
std::string type_monster_quest::getQuestDescription()
{
    return formatString(
        questText(QUEST_TEXT_DESCRIPTION).c_str(),
        m_monsterId >= 0 && m_monsterId <= 0x96
            ? g_creatureTypeTraits[m_monsterId].m_pluralName
            : g_emptyRolloverText);
}

VA(0x0056ebc0, 0x06)
int type_monster_quest::questType()
{
    return 4;
}

VA(0x0056ebd0, 0x26)
unsigned char type_monster_quest::isSatisfied(hero* currentHero)
{
    int owner = currentHero->m_owner;

    if (owner < 0)
        return 0;
    return owner == m_defeatedBy;
}
VA(0x0056ec00, 0x9F)
void type_monster_quest::doProposalDialog(hero* currentHero)
{
    normalDialog(getProposalDialogText().c_str(), 1, -1, -1, 0x15,
                 m_monsterId, -1, 0, -1, 0, -1, 0);
}

VA(0x0056eca0, 0x9B)
void type_monster_quest::doProgressDialog()
{
    normalDialog(getProgressDialogText().c_str(), 1, -1, -1, 0x15,
                 m_monsterId, -1, 0, -1, 0, -1, 0);
}

VA(0x0056ed40, 0x45)
void type_monster_quest::notifyMonsterDefeated(TQuestPosition where,
                                                int player)
{
    if (m_defeatedBy >= 0)
        return;
    if (m_position.m_x != where.m_x)
        return;
    if (m_position.m_y != where.m_y)
        return;
    if (m_position.m_z != where.m_z)
        return;
    m_defeatedBy = player;
}

VA(0x0056ed90, 0x51)
void type_monster_quest::load(TAbstractFile* file, int version)
{
    file->read(&m_position, sizeof(m_position));
    {
        short id;

        file->read(&id, sizeof(id));
        m_monsterId = id;
    }
    {
        signed char killer;

        file->read(&killer, sizeof(killer));
        m_defeatedBy = killer;
    }
    type_quest::load(file, version);
}

VA(0x0056edf0, 0x2B)
void type_experience_quest::loadFromMap(TAbstractFile* file)
{
    int level;

    file->read(&level, sizeof(level));
    m_requiredLevel = level;
    type_quest::loadFromMap(file);
}

VA(0x0056ee20, 0xFE)
void type_monster_quest::save(TAbstractFile* file)
{
    file->write(&m_position, sizeof(m_position));
    {
        short id = m_monsterId;
        file->write(&id, sizeof(id));
    }
    {
        unsigned char killer = static_cast<unsigned char>(m_defeatedBy);
        file->write(&killer, sizeof(killer));
    }

    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}

// Slot 14 does more than the other default-text initializers because a map
// stores this quest's target as an editor object reference. Retail first
// resolves that reference through game::monsterIdentifiers, rejects the
// all-minus-one point, reads the creature type from the referenced map cell,
// and describes the point by map third (plus an underground suffix). Those
// two strings are the varargs for each of the three localized text columns.

// The former inline-depth diagnostic around the middle-north assignment was
// removed. The ordinary const-char assignment improves the unpinned build and
// keeps the same source form as the other eight direction arms.
// Residual: `worldMap.cell(position)` below.
// Retail expands the packed-point wrapper and CALLS the three-scalar
// accessor (0x408770); this compile expands both. The peak came from a
// per-TU declaration-only view of cell(int,int,int) - an imposed inline
// decision, not a source fact - retired 2026-09-05 with game.h's fork.
// E:\gamedcs\seerhut.cpp
VA(0x0056ef20, 0x57C)  // anchor-vtable 0x64183c slot 14 + quest-monster pool
void type_monster_quest::setDefaultText()
{
    m_position = g_game->gameFn004CEF10(m_mapMonster);
    if (m_position.m_x < 0)
        return;

    const char* monsterName;
    const std::string* texts =
        questTextRow() + QUEST_TEXT_COLUMNS * questType();
    m_monsterId = g_game->m_worldMap.cell(m_position)->m_objectIndex;
    monsterName = m_monsterId >= 0 && m_monsterId <= 0x96
                      ? g_creatureTypeTraits[m_monsterId].m_pluralName
                      : g_emptyRolloverText;

    std::string direction;
    if (m_position.m_x < g_mapWidth / 3) {
        if (m_position.m_y < g_mapHeight / 3)
            direction = g_questMonsterDirections[7];
        else if (m_position.m_y > (2 * g_mapHeight) / 3)
            direction = g_questMonsterDirections[5];
        else
            direction = g_questMonsterDirections[6];
    } else if (m_position.m_x > (2 * g_mapWidth) / 3) {
        if (m_position.m_y < g_mapHeight / 3)
            direction = g_questMonsterDirections[1];
        else if (m_position.m_y > (2 * g_mapHeight) / 3)
            direction = g_questMonsterDirections[3];
        else
            direction = g_questMonsterDirections[2];
    } else {
        if (m_position.m_y < g_mapHeight / 3)
            direction = g_questMonsterDirections[0];
        else if (m_position.m_y > (2 * g_mapHeight) / 3)
            direction = g_questMonsterDirections[4];
        else
            direction = g_questMonsterDirections[8];
    }

    if (m_position.m_z)
        direction += DATA_COMPGEN(0x00683228, questUndergroundSuffix,
                                  " underground");

    if (m_proposalText.length() == 0)
        m_proposalText = formatString(texts[QUEST_TEXT_PROPOSAL].c_str(),
                                     monsterName, direction.c_str());
    if (m_progressText.length() == 0)
        m_progressText = formatString(texts[QUEST_TEXT_PROGRESS].c_str(),
                                     monsterName, direction.c_str());
    if (m_completionText.length() == 0)
        m_completionText = formatString(texts[QUEST_TEXT_COMPLETION].c_str(),
                                       monsterName, direction.c_str());
}

VA_COMPGEN(0x0056e990, 0xA0, IMPLICIT_DTOR, type_experience_quest)

// The vector-owning artifact leaf has its own wrapper/body pair.
VA_COMPGEN(0x0056f4a0, 0x21, SCALAR_DELETING_DTOR, type_artifact_quest)
VA_COMPGEN(0x0056f4d0, 0xC2, IMPLICIT_DTOR, type_artifact_quest)

VA(0x0056f5a0, 0x4C)
int type_artifact_quest::getAIValue(int player)
{
    int total = 0;

    for (unsigned i = 0; i < m_artifacts.size(); ++i) {
        type_artifact wanted(m_artifacts[i]);

        total += aiGetValueOfArtifact(wanted, player);
    }
    return total;
}
VA(0x0056f5f0, 0x136)
std::string type_artifact_quest::getRequirementText()
{
    std::vector<std::string> requirements;
    for (unsigned i = 0; i < m_artifacts.size(); ++i)
        requirements.push_back(g_artifactTraits[m_artifacts[i]].m_name);
    return joinTextList(requirements);
}
VA(0x0056f730, 0xCF)
std::string type_artifact_quest::getQuestDescription()
{
    return formatString(questText(QUEST_TEXT_DESCRIPTION).c_str(),
                         getRequirementText().c_str());
}

VA(0x0056f800, 0x58)
unsigned char type_artifact_quest::isSatisfied(hero* currentHero)
{
    if (m_artifacts.size() == 0)
        return 0;
    for (unsigned i = 0; i < m_artifacts.size(); ++i)
        if (!currentHero->hasArtifact(m_artifacts[i]))
            return 0;
    return 1;
}

VA(0x0056f860, 0x37)
void type_artifact_quest::takePayment(hero* currentHero)
{
    for (unsigned i = 0; i < m_artifacts.size(); ++i)
        currentHero->removeArtifact(m_artifacts[i]);
}

// Slot 4 filters the payload to the artifacts the visiting hero still lacks.
// Retail keeps both that id vector and its parallel name vector alive across
// the text choice.  Each arm then owns its own picture vector: the generated
// arm formats the missing-name list into the progress template, while a
// custom progress string is passed through directly.

// Computing the five-column group once fixes the retail
// quest-text copy schedule. The remaining Dinkumware inline-budget class
// inlines both dialog-vector destructors and the final trivial artifact-vector
// destroy where retail calls them, and selects the count-taking insert in the
// custom arm where retail selects the position/value overload. The former
// inline-depth diagnostic on the generated loop is removed; retain the natural
// loop and source-shaped lifetimes instead of manufacturing cleanup flow.
// E:\gamedcs\seerhut.cpp
VA(0x0056f8a0, 0x313)  // anchor-vtable 0x641878 slot 4 + artifact picture class, retail-only
void type_artifact_quest::doProposalDialog(hero* currentHero)
{
    std::vector<TArtifact> missingArtifacts;
    std::vector<std::string> requirements;
    const char* textPointer;
    for (unsigned i = 0; i < m_artifacts.size(); ++i) {
        if (!currentHero->hasArtifact(m_artifacts[i])) {
            missingArtifacts.push_back(m_artifacts[i]);
            requirements.push_back(g_artifactTraits[m_artifacts[i]].m_name);
        }
    }

    if (m_progressText.length() == 0) {
        const std::string* texts = questTexts();
        std::string textFormat = texts[QUEST_TEXT_PROGRESS];
        std::string text = formatString(
            textFormat.c_str(),
            joinTextList(requirements).c_str());
        textPointer = text.c_str();
        std::vector<type_dialog_resource> dialogResources;
        type_dialog_resource resource;
        for (unsigned i = 0; i < missingArtifacts.size(); ++i) {
            resource.m_resource = 8;
            resource.m_qualifier = missingArtifacts[i];
            dialogResources.push_back(resource);
        }
        extendedDialog(textPointer, dialogResources, -1, -1, 0);
    } else {
        textPointer = m_progressText.c_str();
        std::vector<type_dialog_resource> dialogResources;
        type_dialog_resource resource;
        for (unsigned i = 0; i < missingArtifacts.size(); ++i) {
            resource.m_resource = 8;
            resource.m_qualifier = missingArtifacts[i];
            // DEPTH LADDER (docs/vc6/inliner.md 6b): this append alone is
            // spelled `insert(end(), x)`; the two in the sibling arm above
            // stay `push_back`.  89.1000 -> 92.0556.  Per-site: the two
            // sibling sites give 91.6667 each, all three together 85.7667,
            // and a greedy second round over the survivors finds nothing.
            dialogResources.insert(dialogResources.end(), resource);
        }
        extendedDialog(textPointer, dialogResources, -1, -1, 0);
    }
}

// Slot 5 uses extended-dialog artifact pictures: class 8 and the artifact id
// as qualifier, one row per element in the quest payload.
// Residual (89.8471%): an inner resource-vector scope removes its three
// post-delete zero stores and restores retail's ESI/EDI saves and ECX zero,
// raising 75.2353% without changing destruction order. All twelve CFG blocks
// still agree. Flattening the scope restores the old bytes. Named mutable
// or const strings, aggregate resource initialization, and single-element
// insert(end(), resource) are byte-identical with the scope. Moving the
// resource outside the loop is byte-identical without it; counted insert
// instead expands to 25 blocks and is rejected. Keep canonical push_back.
// Remaining: c_str reloads the return slot rather than dereferencing EAX;
// insert argument scheduling and string cleanup registers differ. This
// Complete quest has no Dreamcast counterpart to settle its source scopes.
// E:\gamedcs\seerhut.cpp
VA(0x0056fbc0, 0xE6)  // anchor-vtable 0x641878 slot 5 + artifact picture class, retail-only
void type_artifact_quest::doProgressDialog()
{
    const std::string& text = getProgressDialogText();
    const char* textPointer = text.c_str();
    {
        std::vector<type_dialog_resource> dialogResources;
        for (unsigned i = 0; i < m_artifacts.size(); ++i) {
            type_dialog_resource resource;
            resource.m_resource = 8;
            resource.m_qualifier = m_artifacts[i];
            dialogResources.push_back(resource);
        }
        extendedDialog(textPointer, dialogResources, -1, -1, 0);
    }
}
VA(0x0056fcb0, 0x1EE)
void type_artifact_quest::load(TAbstractFile* file, int version)
{
    int i;
    {
        int count;
        file->read(&count, sizeof(unsigned char));
        i = count & 0xff;
    }
    for (; i > 0; --i) {
        short id;

        file->read(&id, sizeof(id));
        TArtifact artifact;
        {
            artifact = TArtifact(id);
        }
        m_artifacts.push_back(artifact);
    }
    type_quest::load(file, version);
}

VA(0x0056fea0, 0x1FA)
void type_artifact_quest::loadFromMap(TAbstractFile* file)
{
    int i;
    {
        int count;
        file->read(&count, sizeof(unsigned char));
        i = count & 0xff;
    }
    for (; i > 0; --i) {
        short id;

        file->read(&id, sizeof(id));
        TArtifact artifact;
        {
            artifact = TArtifact(id);
        }
        m_artifacts.push_back(artifact);
        g_game->m_artifactDisabled[artifact] = 1;
    }
    type_quest::loadFromMap(file);
}

VA(0x005700a0, 0x121)
void type_artifact_quest::save(TAbstractFile* file)
{
    unsigned char count = static_cast<unsigned char>(m_artifacts.size());
    file->write(&count, sizeof(count));
    for (unsigned int i = 0; i < m_artifacts.size(); i++) {
        short id = m_artifacts[i];
        file->write(&id, sizeof(id));
    }

    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}
VA(0x005701d0, 0x199)
void type_artifact_quest::setDefaultText()
{
    const std::string* texts = questTexts();
    std::string requirement;
    requirement = getRequirementText();

    if (m_proposalText.length() == 0)
        m_proposalText = formatString(texts[QUEST_TEXT_PROPOSAL].c_str(),
                                     requirement.c_str());
    if (m_completionText.length() == 0)
        m_completionText = formatString(texts[QUEST_TEXT_COMPLETION].c_str(),
                                       requirement.c_str());
}

// The two-vector creature leaf has the second distinct implicit
// wrapper/body pair in the family.
VA_COMPGEN(0x00570370, 0x21, SCALAR_DELETING_DTOR, type_creature_quest)
VA_COMPGEN(0x005703a0, 0xD7, IMPLICIT_DTOR, type_creature_quest)

VA(0x00570480, 0x55)
int type_creature_quest::getAIValue(int player)
{
    int total = 0;

    for (unsigned i = 0; i < m_types.size(); ++i)
        total += g_creatureTypeTraits[m_types[i]].m_aiValue * m_counts[i];
    return total;
}
VA(0x005704e0, 0x1A7)
std::string type_creature_quest::getRequirementText()
{
    std::string requirement;
    std::vector<std::string> requirements;
    for (unsigned i = 0; i < m_types.size(); ++i) {
        requirement = formatString(
            DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
            m_counts[i], getArmyName(m_types[i], m_counts[i]));
        requirements.push_back(requirement);
    }
    return joinTextList(requirements);
}
VA(0x00570690, 0xCF)
std::string type_creature_quest::getQuestDescription()
{
    return formatString(questText(QUEST_TEXT_DESCRIPTION).c_str(),
                         getRequirementText().c_str());
}

VA(0x00570760, 0x60)
unsigned char type_creature_quest::isSatisfied(hero* currentHero)
{
    if (m_types.size() == 0)
        return 0;
    for (unsigned i = 0; i < m_types.size(); ++i)
        if (currentHero->m_army.getCreatureTotal(m_types[i]) < m_counts[i])
            return 0;
    return 1;
}

VA(0x005707c0, 0xB3)
void type_creature_quest::takePayment(hero* currentHero)
{
    for (unsigned i = 0; i < m_types.size(); ++i) {
        for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
            if (currentHero->m_army.m_armies[slot] != m_types[i])
                continue;
            if (currentHero->m_army.m_numTroops[slot] > m_counts[i]) {
                currentHero->m_army.m_numTroops[slot] -= m_counts[i];
                break;
            }
            m_counts[i] -= currentHero->m_army.m_numTroops[slot];
            currentHero->m_army.dismiss(slot);
        }
    }
}

// Slot 4 lists only stacks for which the visitor's whole army is below the
// quest count. The display keeps the full required count (not the deficit),
// both in the localized text fragment and in the packed creature picture.

VA(0x00570880, 0x2F8)
void type_creature_quest::doProposalDialog(hero* currentHero)
{
    std::string text;
    std::vector<std::string> requirements;
    std::vector<type_dialog_resource> dialogResources;
    type_dialog_resource resource;

    for (unsigned i = 0; i < m_types.size(); ++i) {
        if (currentHero->m_army.getCreatureTotal(m_types[i]) < m_counts[i]) {
            text = formatString(
                DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
                m_counts[i], getArmyName(m_types[i], m_counts[i]));
            requirements.push_back(text);

            resource.m_resource = 0x15;
            resource.m_qualifier =
                (static_cast<unsigned long>(
                    static_cast<unsigned short>(m_counts[i])) << 16)
                | static_cast<unsigned short>(m_types[i]);
            dialogResources.push_back(resource);
        }
    }

    if (m_progressText.length() == 0) {
        const std::string* texts = questTexts();
        std::string textFormat = texts[QUEST_TEXT_PROGRESS];
        text = formatString(
            textFormat.c_str(),
            joinTextList(requirements).c_str());
    } else {
        text = m_progressText;
    }
    extendedDialog(text.c_str(), dialogResources, -1, -1, 0);
}

// Slot 5 shows every requested creature stack. With no custom proposal text,
// retail appends the deadline suffix to the proposal template before filling
// in the joined stack list; otherwise it uses the base's dated proposal-text
// getter. Picture qualifiers use the same unsigned 16|16 packing as slot 4.

// The vectors need their own inner scope so their cleanup precedes the final
// text cleanup. Retail retains the element destructor and expands the final
// string destructor; the unpinned compiler currently makes the opposite pair
// of inline decisions. The former end-of-scope inline-depth diagnostic is not
// source evidence and has been removed.
// E:\gamedcs\seerhut.cpp
VA(0x00570b80, 0x2D5)  // anchor-vtable 0x6418b4 slot 5 + creature picture class, retail-only
void type_creature_quest::doProgressDialog()
{
    std::string text;
    {
        std::vector<std::string> requirements;
        std::vector<type_dialog_resource> dialogResources;
        type_dialog_resource resource;

        for (unsigned i = 0; i < m_types.size(); ++i) {
            text = formatString(
                DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
                m_counts[i], getArmyName(m_types[i], m_counts[i]));
            requirements.push_back(text);

            resource.m_resource = 0x15;
            resource.m_qualifier =
                (static_cast<unsigned long>(
                    static_cast<unsigned short>(m_counts[i])) << 16)
                | static_cast<unsigned short>(m_types[i]);
            dialogResources.push_back(resource);
        }

        if (m_proposalText.length() == 0) {
            std::string textFormat =
                questTextRow()[QUEST_TEXT_COLUMNS * questType()
                                 + QUEST_TEXT_PROPOSAL];
            if (m_limit >= 0)
                textFormat += getTimeLimitText();
            text = formatString(
                textFormat.c_str(),
                joinTextList(requirements).c_str());
        } else {
            text = getProgressDialogText();
        }
        extendedDialog(text.c_str(), dialogResources, -1, -1, 0);
    }
}

VA(0x00570e60, 0x208)
void type_creature_quest::load(TAbstractFile* file, int version)
{
    int i;
    {
        int count;
        file->read(&count, sizeof(unsigned char));
        i = count & 0xff;
    }
    while (i--) {
        int type;
        int number;

        file->read(&type, sizeof(short));
        TCreatureType creature;
        {
            creature = TCreatureType(type & 0xffff);
        }
        file->read(&number, sizeof(number));
        int amount = number;
        m_counts.push_back(amount);
        m_types.push_back(creature);
    }
    type_quest::load(file, version);
}

VA(0x00571070, 0x20A)
void type_creature_quest::loadFromMap(TAbstractFile* file)
{
    int i;
    {
        int count;
        file->read(&count, sizeof(unsigned char));
        i = count & 0xff;
    }
    while (i--) {
        int type;
        int number;

        file->read(&type, sizeof(short));
        TCreatureType creature;
        {
            creature = TCreatureType(type & 0xffff);
        }
        file->read(&number, sizeof(short));
        int amount = number & 0xffff;
        m_counts.push_back(amount);
        m_types.push_back(creature);
    }
    type_quest::loadFromMap(file);
}

VA(0x00571280, 0x137)
void type_creature_quest::save(TAbstractFile* file)
{
    unsigned char count = static_cast<unsigned char>(m_types.size());
    file->write(&count, sizeof(count));
    for (unsigned int i = 0; i < m_types.size(); i++) {
        {
            short value = m_types[i];
            file->write(&value, sizeof(value));
        }
        {
            int value = m_counts[i];
            file->write(&value, sizeof(value));
        }
    }

    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}
VA(0x005713c0, 0x16A)
void type_creature_quest::setDefaultText()
{
    if (m_completionText.length() == 0) {
        std::string requirement = getRequirementText();
        const std::string* texts = questTexts();
        std::string text = texts[QUEST_TEXT_COMPLETION];
        m_completionText = formatString(text.c_str(),
                                       requirement.c_str());
    }
}

VA_COMPGEN(0x00571530, 0x21, SCALAR_DELETING_DTOR, type_experience_quest)

VA(0x00571560, 0x12)
int type_resource_quest::getAIValue(int player)
{
    return aiResourceCost(player, m_resources);
}
VA(0x00571580, 0x15A)
std::string type_resource_quest::getRequirementText()
{
    std::vector<std::string> requirements;
    std::string requirement;
    for (int i = 0; i <= 6; i++) {
        if (m_resources[i] > 0) {
            requirement = formatString(
                DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
                m_resources[i], g_resourceNames[i]);
            requirements.push_back(requirement);
        }
    }
    return joinTextList(requirements);
}
VA(0x005716e0, 0xCF)
std::string type_resource_quest::getQuestDescription()
{
    return formatString(questText(QUEST_TEXT_DESCRIPTION).c_str(),
                         getRequirementText().c_str());
}

// Vtable 0x6418f0 slot 2. A hero without an owner cannot pay; otherwise
// every one of the seven treasury balances must cover the quest price.
VA(0x005717b0, 0x4B)  // anchor-vtable
unsigned char type_resource_quest::isSatisfied(hero* currentHero)
{
    int owner = currentHero->m_owner;
    if (owner < 0)
        return 0;

    long* playerResources = g_game->m_players[owner].m_resources;
    int i = 0;
    while (i <= 6) {
        if (playerResources[i] < m_resources[i])
            return 0;
        ++i;
    }
    return 1;
}

VA(0x00571800, 0x3D)  // anchor-vtable
void type_resource_quest::takePayment(hero* currentHero)
{
    int* questResource = m_resources;
    long* playerResource = g_game->m_players[currentHero->m_owner].m_resources;
    int count = 7;
    do {
        *playerResource++ -= *questResource++;
    } while (--count);
}

VA(0x00571840, 0x29D)
void type_resource_quest::doProposalDialog(hero* currentHero)
{
    std::vector<std::string> requirements;
    std::string text;
    std::vector<type_dialog_resource> dialogResources;
    type_dialog_resource resource;
    long* playerResources =
        g_game->m_players[currentHero->m_owner].m_resources;

    for (int i = 0; i <= 6; ++i) {
        if (playerResources[i] < m_resources[i]) {
            text = formatString(
                DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
                m_resources[i], g_resourceNames[i]);
            requirements.push_back(text);

            resource.m_resource = i;
            resource.m_qualifier = m_resources[i];
            dialogResources.push_back(resource);
        }
    }

    if (m_progressText.length() == 0) {
        const std::string* texts = questTexts();
        std::string textFormat = texts[QUEST_TEXT_PROGRESS];
        text = formatString(
            textFormat.c_str(),
            joinTextList(requirements).c_str());
    } else {
        text = m_progressText;
    }
    extendedDialog(text.c_str(), dialogResources, -1, -1, 0);
}

VA(0x00571ae0, 0x9A)  // anchor-vtable
void type_resource_quest::doProgressDialog()
{
    std::vector<type_dialog_resource> dialogResources;
    for (int i = 0; i <= 6; ++i) {
        if (m_resources[i] > 0) {
            type_dialog_resource resource;
            resource.m_resource = i;
            resource.m_qualifier = m_resources[i];
            dialogResources.push_back(resource);
        }
    }
    extendedDialog(m_proposalText.c_str(), dialogResources, -1, -1, 0);
}

VA(0x00571b80, 0x29)
void type_resource_quest::load(TAbstractFile* file, int version)
{
    file->read(m_resources, sizeof(m_resources));
    type_quest::load(file, version);
}

VA(0x00571bb0, 0x25)
void type_resource_quest::loadFromMap(TAbstractFile* file)
{
    file->read(m_resources, sizeof(m_resources));
    type_quest::loadFromMap(file);
}

VA(0x00571be0, 0xD7)
void type_resource_quest::save(TAbstractFile* file)
{
    file->write(m_resources, sizeof(m_resources));

    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}
VA(0x00571cc0, 0x23E)
void type_resource_quest::setDefaultText()
{
    std::vector<std::string> requirements;
    std::string requirement;
    const std::string* texts =
        questTextRow() + QUEST_TEXT_COLUMNS * questType();
    for (int i = 0; i <= 6; i++) {
        if (m_resources[i] > 0) {
            requirement = formatString(
                DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
                m_resources[i], g_resourceNames[i]);
            requirements.push_back(requirement);
        }
    }
    requirement = joinTextList(requirements);
    if (m_proposalText.length() == 0)
        m_proposalText = formatString(texts[QUEST_TEXT_PROPOSAL].c_str(),
                              requirement.c_str());
    if (m_completionText.length() == 0)
        m_completionText = formatString(texts[QUEST_TEXT_COMPLETION].c_str(),
                              requirement.c_str());
}

VA(0x00571f00, 0x19)
unsigned char type_be_hero_quest::isSatisfied(hero* currentHero)
{
    return currentHero->m_id == m_requiredHero;
}
VA(0x00571f20, 0x99)
void type_be_hero_quest::doProposalDialog(hero* currentHero)
{
    normalDialog(getProposalDialogText().c_str(), 1, -1, -1, -1,
                 0, -1, 0, -1, 0, -1, 0);
}

VA(0x00571fc0, 0x95)
void type_be_hero_quest::doProgressDialog()
{
    normalDialog(getProgressDialogText().c_str(), 1, -1, -1, -1,
                 0, -1, 0, -1, 0, -1, 0);
}
VA(0x00572060, 0xF2)
std::string type_be_hero_quest::getRequirementText()
{
    return g_game->getHero(m_requiredHero)->m_name;
}

VA(0x00572160, 0x8B)
std::string type_be_hero_quest::getQuestDescription()
{
    return formatString(questText(QUEST_TEXT_DESCRIPTION).c_str(),
                         g_game->getHero(m_requiredHero)->m_name);
}

VA(0x005721f0, 0x06)
int type_be_hero_quest::questType()
{
    return 8;
}

VA(0x00572200, 0x30)
void type_be_hero_quest::load(TAbstractFile* file, int version)
{
    short id;

    file->read(&id, sizeof(id));
    m_requiredHero = id;
    type_quest::load(file, version);
}

VA(0x00572230, 0x31)
void type_be_hero_quest::loadFromMap(TAbstractFile* file)
{
    unsigned char id;

    file->read(&id, sizeof(id));
    m_requiredHero = id;
    type_quest::loadFromMap(file);
}
VA(0x00572270, 0x276)
void type_be_hero_quest::setDefaultText()
{
    hero* requiredHero = g_game->getHero(m_requiredHero);
    const std::string* texts = questTexts();
    if (m_proposalText.length() == 0)
        m_proposalText = formatString(texts[QUEST_TEXT_PROPOSAL].c_str(),
                              requiredHero->m_name);
    if (m_progressText.length() == 0)
        m_progressText = formatString(texts[QUEST_TEXT_PROGRESS].c_str(),
                              requiredHero->m_name);
    if (m_completionText.length() == 0)
        m_completionText = formatString(texts[QUEST_TEXT_COMPLETION].c_str(),
                              requiredHero->m_name);
}

VA(0x005724f0, 0x1A)
unsigned char type_belong_to_player_quest::isSatisfied(hero* currentHero)
{
    return currentHero->m_owner == m_requiredOwner;
}
VA(0x00572510, 0x99)
void type_belong_to_player_quest::doProposalDialog(hero* currentHero)
{
    normalDialog(getProposalDialogText().c_str(), 1, -1, -1, -1,
                 0, -1, 0, -1, 0, -1, 0);
}

VA(0x005725b0, 0x95)
void type_belong_to_player_quest::doProgressDialog()
{
    normalDialog(getProgressDialogText().c_str(), 1, -1, -1, -1,
                 0, -1, 0, -1, 0, -1, 0);
}

VA(0x00572650, 0x20)
std::string type_belong_to_player_quest::getRequirementText()
{
    return std::string();
}

VA(0x00572670, 0x19D)
std::string type_belong_to_player_quest::getQuestDescription()
{
    std::string requirement = g_playerColorNames[m_requiredOwner];
    std::transform(requirement.begin(), requirement.end(),
                   requirement.begin(), ::tolower);
    return formatString(questText(QUEST_TEXT_DESCRIPTION).c_str(),
                         requirement.c_str());
}

VA(0x00572810, 0x06)
int type_belong_to_player_quest::questType()
{
    return 9;
}

VA(0x00572820, 0x35)
void type_belong_to_player_quest::load(TAbstractFile* file, int version)
{
    unsigned char owner;

    file->read(&owner, sizeof(owner));
    m_requiredOwner = owner;
    type_quest::load(file, version);
}

VA(0x00572860, 0xE0)
void type_belong_to_player_quest::save(TAbstractFile* file)
{
    unsigned char owner = static_cast<unsigned char>(m_requiredOwner);
    file->write(&owner, sizeof(owner));

    {
        unsigned char flag = m_seerHut;
        file->write(&flag, sizeof(flag));
    }
    {
        unsigned char row = static_cast<unsigned char>(m_textVariant);
        file->write(&row, sizeof(row));
    }
    {
        int extra = m_limit;
        file->write(&extra, sizeof(extra));
    }
    {
        int length = m_proposalText.length();
        file->write(&length, sizeof(length));
        file->write(m_proposalText.c_str(), m_proposalText.length());
    }
    {
        int length = m_progressText.length();
        file->write(&length, sizeof(length));
        file->write(m_progressText.c_str(), m_progressText.length());
    }
    {
        int length = m_completionText.length();
        file->write(&length, sizeof(length));
        file->write(m_completionText.c_str(), m_completionText.length());
    }
}
VA(0x00572940, 0x204)
void type_belong_to_player_quest::setDefaultText()
{
    std::string requirement = g_playerColorNames[m_requiredOwner];
    std::transform(requirement.begin(), requirement.end(),
                   requirement.begin(), ::tolower);
    const std::string* texts =
        questTextRow() + QUEST_TEXT_COLUMNS * questType();
    if (m_proposalText.length() == 0)
        m_proposalText = formatString(texts[QUEST_TEXT_PROPOSAL].c_str(),
                              requirement.c_str());
    if (m_progressText.length() == 0)
        m_progressText = formatString(texts[QUEST_TEXT_PROGRESS].c_str(),
                              requirement.c_str());
    if (m_completionText.length() == 0)
        m_completionText = formatString(texts[QUEST_TEXT_COMPLETION].c_str(),
                              requirement.c_str());
}

VA(0x00572b50, 0xD)
TQuestGuard::TQuestGuard()
{
    m_quest = 0;
    m_visitedPlayers = 0;
}

VA(0x00572b60, 0x1FE)
void TQuestGuard::doEvent(hero* currentHero, bool humanPlayer,
                          NewmapCell* eventCell, type_point point)
{
    if (!m_quest)
        return;

    bool expired = false;
    if (m_quest->m_limit >= 0) {
        expired = m_quest->m_limit < static_cast<short>(
            (g_game->m_month * 4 + g_game->m_week - 5) * 7
            + g_game->m_day);
    }

    if (expired) {
        if (humanPlayer) {
            const std::string* row = m_quest->m_seerHut
                ? g_questTextA[m_quest->m_textVariant]
                : g_questTextB[m_quest->m_textVariant];
            normalDialog(row[type_quest::QUEST_TEXT_EXPIRED].c_str(),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
        return;
    }

    if (humanPlayer) {
        if (!(m_visitedPlayers & (1 << currentHero->m_owner)))
            m_quest->doProgressDialog();
        else if (!m_quest->isSatisfied(currentHero))
            m_quest->doProposalDialog(currentHero);
    }

    m_visitedPlayers |= 1 << g_netLocalGamePos;
    if (!m_quest->isSatisfied(currentHero))
        return;

    if (humanPlayer) {
        normalDialog(m_quest->getCompletionText().c_str(),
                     2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT
            && g_windowManager->m_dialogReturn != DIALOG_RETURN_CHOICE_1)
            return;
    }

    m_quest->takePayment(currentHero);
    g_advManager->eraseAndFizzle(eventCell, point, 1);
    m_quest = 0;
}

// The quest log's line for this guard, and the ONE reader of the text
// table's fifth column: the quest's own requirement line formatted into
// whatever that column holds. Nullary where the quick-info and rollover
// pair above take a player, which is what TQuestLogWindow's call site
// pushes.

VA(0x00572d60, 0xE0)
std::string TQuestGuard::questGuardFn00572D60()
{
    return formatString(
        m_quest->questText(type_quest::QUEST_TEXT_LOG).c_str(),
        m_quest->getRequirementText().c_str());
}

VA(0x00572e40, 0x1FF)
std::string TQuestGuard::questGuardFn00572E40(int player)
{
    std::string text;
    text = g_questGuardName;

    if ((m_visitedPlayers & (1 << static_cast<unsigned char>(player))) && m_quest) {
        text += DATA_COMPGEN(0x006603b0, questGuardQuickInfoSeparator, "\n\n");
        text += m_quest->getQuestDescription();
    }

    return text;
}

VA(0x00573040, 0x1FF)
std::string TQuestGuard::questGuardFn00573040(int player)
{
    std::string text;
    text = g_questGuardName;

    if ((m_visitedPlayers & (1 << static_cast<unsigned char>(player))) && m_quest) {
        text += DATA_COMPGEN(0x00660330, questGuardRolloverSeparator, " ");
        text += m_quest->getQuestDescription();
    }

    return text;
}

__forceinline type_experience_quest::type_experience_quest(
    unsigned char flags)
    : type_quest(flags)
{
    m_requiredLevel = 0;
}

__forceinline type_skill_quest::type_skill_quest(unsigned char flags)
    : type_quest(flags)
{
    memset(m_requiredSkills, 0, sizeof(m_requiredSkills));
}

__forceinline type_defeat_hero_quest::type_defeat_hero_quest(
    unsigned char flags)
    : type_quest(flags)
{
    m_mapHero = 0;
    m_defeatedHero = -1;
    m_satisfiedMask = 0;
}

__forceinline type_monster_quest::type_monster_quest(unsigned char flags)
    : type_quest(flags)
{
    m_position.m_x = (m_monsterId = m_defeatedBy = -1);
}

type_artifact_quest::type_artifact_quest(unsigned char flags)
    : type_quest(flags)
{
}

// Both legacy seer-hut readers capture the artifact argument before base/member
// construction. Retail 0x574610 and 0x574a90 keep the append, text-row
// override, disabled-artifact store, and direct SetDefaultText call inside the
// allocation-success arm. These support a shared single-artifact constructor.
type_artifact_quest::type_artifact_quest(
    unsigned char flags, TArtifact artifact, int textRow)
    : type_quest(flags)
{
    m_artifacts.push_back(artifact);
    m_textVariant = textRow;
    g_game->m_artifactDisabled[artifact] = 1;
    setDefaultText();
}

__forceinline type_creature_quest::type_creature_quest(unsigned char flags)
    : type_quest(flags)
{
}

__forceinline type_resource_quest::type_resource_quest(unsigned char flags)
    : type_quest(flags)
{
    memset(m_resources, 0, sizeof(m_resources));
}

__forceinline type_be_hero_quest::type_be_hero_quest(unsigned char flags)
    : type_quest(flags), m_requiredHero(-1)
{
}

__forceinline type_belong_to_player_quest::type_belong_to_player_quest(
    unsigned char flags)
    : type_quest(flags), m_requiredOwner(0)
{
}

VA(0x00573240, 0x23C)  // hd-crossbuild + nine vtables + four callers
type_quest* createQuest(int questType, unsigned char flags)
{
    switch (questType) {
    case QUEST_EXPERIENCE:
        return new type_experience_quest(flags);
    case QUEST_PRIMARY_SKILLS:
        return new type_skill_quest(flags);
    case QUEST_DEFEAT_HERO:
        return new type_defeat_hero_quest(flags);
    case QUEST_DEFEAT_MONSTER:
        return new type_monster_quest(flags);
    case QUEST_ARTIFACTS:
        return new type_artifact_quest(flags);
    case QUEST_CREATURES:
        return new type_creature_quest(flags);
    case QUEST_RESOURCES:
        return new type_resource_quest(flags);
    case QUEST_BE_HERO:
        return new type_be_hero_quest(flags);
    case QUEST_BELONG_TO_PLAYER:
        return new type_belong_to_player_quest(flags);
    }
    return 0;
}

VA(0x00573480, 0x52)  // hd-crossbuild
int TQuestGuard::load(TAbstractFile* infile, int saveVersion)
{
    {
        unsigned char questType;
        infile->read(&questType, sizeof(questType));
        m_quest = createQuest(questType, 0);
        if (m_quest)
            m_quest->load(infile, saveVersion);
    }
    {
        unsigned char visited;
        int result = infile->read(&visited, sizeof(visited));
        m_visitedPlayers = visited;
        return result;
    }
}

// The guard's h3m reader and its savegame writer, both reached from
// mapcell/NewfullMap. Each is one byte of quest TYPE through the factory
// and then the quest's own slot 12 or slot 13 - which is a second,
// independent witness for slot 13 being the serializer: this body picks
// it out of the vtable at +0x34 right after writing the type byte.

VA(0x005734e0, 0x3B)
void TQuestGuard::read(TAbstractFile* infile)
{
    unsigned char questType;

    infile->read(&questType, sizeof(questType));
    m_quest = createQuest(questType, 0);
    if (m_quest)
        m_quest->loadFromMap(infile);
}

VA(0x00573520, 0x5E)
int TQuestGuard::save(TAbstractFile* outfile)
{
    if (!m_quest) {
        unsigned char questType = 0;
        outfile->write(&questType, sizeof(questType));
    } else {
        unsigned char questType = static_cast<unsigned char>(m_quest->questType());
        outfile->write(&questType, sizeof(questType));
        m_quest->save(outfile);
    }

    {
        unsigned char visited = m_visitedPlayers;
        return outfile->write(&visited, sizeof(visited));
    }
}

// Original: TSeerHut::SetRandomName; seerhut.cpp:139, dc 0x12d084
// DC uses one static TPickANumber(0,47). Complete read0x574610 expands
// the same static reference interface with the revised dynamic name table:
// construct availability, remove names used by this map, then select one.
inline void TSeerHut::setRandomName(TSeerHut& thisHut)
{
    std::vector<unsigned char> nameAvailable(g_seerHutNamesPointer->size());
    unsigned int name;
    for (name = 0; name < nameAvailable.size(); ++name)
        nameAvailable[name] = 1;

    unsigned int hut;
    for (hut = 0; hut < g_game->m_worldMap.m_seerHutList.size(); ++hut)
        nameAvailable[g_game->m_worldMap.m_seerHutList[hut].m_nameIndex] = 0;

    int pick = rand()
        % (nameAvailable.size() - g_game->m_worldMap.m_seerHutList.size());
    unsigned int chosen;
    for (chosen = 0; chosen < nameAvailable.size(); ++chosen) {
        if (nameAvailable[chosen]) {
            if (--pick < 0)
                break;
        }
    }
    thisHut.m_nameIndex = chosen;
}

VA(0x005735a0, 0xC3)
int TSeerHut::getValue(hero* currentHero)
{
    int value = m_reward.getValue(currentHero);

    if (!(m_visitedPlayers & (1 << currentHero->m_owner)))
        return cppMax(value, 20);

    if (m_quest && !m_quest->hasExpired()
        && m_quest->isSatisfied(currentHero))
        return value - m_quest->getAIValue(currentHero->m_owner);

    return 0;
}

// Dreamcast preserves the public member name and `(hero*, bool)` signature.
// Retail's much larger Complete-era body independently proves the split
// quest/reward model: it drives the same quest slots as TQuestGuard::DoEvent,
// prices the reward for the AI, and consumes the quest only after payment and
// reward application. A hut with no quest records the visit and shows one of
// three randomized empty-hut lines to a human player.

// Retail's null-quest branch (+0x24) and expired-deadline branch (+0x66)
// both reach +0x2c7: record the visit, then show DoEmptyDialog to a human.
// An expired quest must not return before that path. The empty arm comes
// first in DC seerhut.cpp:151-154 and owns retail unwind states 0/1;
// the completion temporary is state 2. FuncInfo 0x654048 has maxState 3,
// unwind map 0x654068 and NO try blocks: no catch scope is missing.

// Complete revised the human completion arm: declining returns, while an
// accepted dialog and a sufficiently valuable AI visit converge on the same
// payment/reward tail. This also puts the completion-text temporary in the
// caller's EH state, after which VC6 naturally expands the retained
// DoEmptyDialog source call and keeps all three retail `_Tidy` boundaries.
// The resulting 59-block CFG and all 27 branches agree with retail. The only
// byte residual is a two-instruction scheduling difference inside the expanded
// ordinary getValue helper; why-reg finds the same pseudos in a different C1
// processing order, and its source-local creation-order probe regresses.

VA(0x00573670, 0x400)  // code plus two retail switch tables in the admitted row
void TSeerHut::doSeerEvent(hero* currentHero, bool humanPlayer)
{
    if (!m_quest || m_quest->hasExpired()) {
        m_visitedPlayers |= 1 << g_netLocalGamePos;
        if (humanPlayer)
            doEmptyDialog();
    } else {
        if (humanPlayer) {
            if (!(m_visitedPlayers
                  & (1 << static_cast<unsigned char>(g_netLocalGamePos))))
                m_quest->doProgressDialog();
            else if (!m_quest->isSatisfied(currentHero))
                m_quest->doProposalDialog(currentHero);
        }

        m_visitedPlayers |= 1 << g_netLocalGamePos;
        if (!m_quest->isSatisfied(currentHero))
            return;

        if (humanPlayer) {
            normalDialog(m_quest->getCompletionText().c_str(),
                         2, -1, -1, getRewardType(),
                         m_reward.getRewardExtra(currentHero),
                         -1, 0, -1, 0, -1, 0);

            if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT
                && g_windowManager->m_dialogReturn
                       != DIALOG_RETURN_CHOICE_1)
                return;
        } else if (getValue(currentHero) <= 0)
            return;

        m_quest->takePayment(currentHero);
        m_reward.giveReward(currentHero, humanPlayer);
        m_quest = 0;
        return;
    }
}

// Dreamcast preserves this private helper at dc 0x12d158 and places it after
// DoSeerEvent in the TU. Complete replaces its fixed-buffer sprintf with a
// string-returning formatter, but retail's no-quest arm corroborates the
// helper's name lookup followed by NormalDialog. Once the caller owns the
// revised completion-text lifetime, VC6 naturally expands this source boundary
// while retaining selected nested Dinkumware calls.
void TSeerHut::doEmptyDialog()
{
    std::string text;
    int textIndex = rand() % 3;
    text = formatString(
        g_questTextA[textIndex][type_quest::QUEST_TEXT_EXPIRED].c_str(),
        getName());
    normalDialog(text.c_str(), 1, -1, -1, -1, 0,
                 -1, 0, -1, 0, -1, 0);
}

// Dreamcast places this private boundary immediately after DoEmptyDialog and
// gives it the same (hero*, bool) inputs. Complete's virtual quest owns the
// completion text and its reward object owns application. Complete's shared
// human/AI reward tail supersedes this older helper boundary in DoSeerEvent;
// retain the boundary here as Dreamcast source evidence.
// Original: TSeerHut::DoCompletionDialog; seerhut.cpp:185, dc 0x12d1a8
void TSeerHut::doCompletionDialog(
    hero* currentHero, bool humanPlayer)
{
    normalDialog(m_quest->getCompletionText().c_str(),
                 2, -1, -1, getRewardType(),
                 m_reward.getRewardExtra(currentHero),
                 -1, 0, -1, 0, -1, 0);

    if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT
        || g_windowManager->m_dialogReturn == DIALOG_RETURN_CHOICE_1) {
        m_quest->takePayment(currentHero);
        m_reward.giveReward(currentHero, humanPlayer);
        m_quest = 0;
    }
}

// Dreamcast seerhut.cpp:414 (dc 0x12d758) records this as a separate,
// no-local switch helper called first by DoCompletionDialog. Retail's inlined
// copy preserves the ten reward arms and Complete's shifted skill pictures.
// Original: TSeerHut::GetRewardType; seerhut.cpp:414, dc 0x12d758
int TSeerHut::getRewardType()
{
    switch (m_reward.m_rewardType) {
    case eRewardExperience:
        return 0x11;
    case eRewardMana:
        return 0x23;
    case eRewardMorale:
        return 0x0e;
    case eRewardLuck:
        return 0x0b;
    case eRewardResource:
        return m_reward.m_value.m_resource.m_resourceType;
    case eRewardPrimarySkill:
        switch (m_reward.m_value.m_primarySkill.m_skillType) {
        case TSeerReward::ePriSkillAttack:
            return 0x1f;
        case TSeerReward::ePriSkillDefense:
            return 0x20;
        case TSeerReward::ePriSkillPower:
            return 0x21;
        case TSeerReward::ePriSkillKnowledge:
            return 0x22;
        default:
            return -1;
        }
    case eRewardSecondarySkill:
        return 0x14;
    case eRewardArtifact:
        return 8;
    case eRewardSpell:
        return 9;
    case eRewardCreature:
        return 0x15;
    default:
        return -1;
    }
}

// retail's real `AI_get_value_of_artifact(const type_artifact&, long)`
VA(0x00573a70, 0x210)
int TSeerReward::getValue(const hero* currentHero)
{
    switch (m_rewardType) {
    case eRewardExperience:
        return static_cast<int>(m_value.m_dwords[0]
            * currentHero->m_turnExperienceToRvRatio);

    case eRewardMana:
        return currentHero->getValueOfKnowledge() * m_value.m_dwords[0] / 20;

    case eRewardMorale:
        return const_cast<hero*>(currentHero)->moraleIncreaseValue(
            m_value.m_signedLow.m_bonus);

    case eRewardLuck:
        const_cast<hero*>(currentHero)->luckIncreaseValue(
            m_value.m_signedLow.m_bonus);

    case eRewardResource:
    {
        double quantity;
        playerData* player = currentHero->getPlayer();
        quantity = m_value.m_resource.m_quantity;
        return static_cast<int>(
            quantity * player->m_ai.m_resourceValue[m_value.m_resource.m_resourceType]);
    }

    case eRewardPrimarySkill:
        switch (m_value.m_primarySkill.m_skillType) {
        case ePriSkillAttack:
        case ePriSkillDefense: {
            int experience = hero::getExperienceIncrement(currentHero->m_level);
            return static_cast<int>(m_value.m_primarySkill.m_bonus
                * currentHero->m_turnExperienceToRvRatio * experience);
        }
        case ePriSkillPower:
            return m_value.m_primarySkill.m_bonus * currentHero->getValueOfPower();
        case ePriSkillKnowledge:
            return m_value.m_primarySkill.m_bonus * currentHero->getValueOfKnowledge();
        default:
            return 0;
        }

    case eRewardSecondarySkill:
        return const_cast<hero*>(currentHero)->soDGetSeerSkillValue(
            m_value.m_secondarySkill.m_skillType, m_value.m_secondarySkill.m_bonus);

    case eRewardArtifact: {
        if (const_cast<hero*>(currentHero)->getNumberInBackpack(1) >= 64)
            return 0;
        return aiGetValueOfArtifact(
            type_artifact(static_cast<TArtifact>(m_value.m_dwords[0]) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */),
            currentHero->m_owner);
    }

    case eRewardSpell:
        return currentHero->valueOfSpell(m_value.m_dwords[0]);

    case eRewardCreature:
        return g_creatureTypeTraits[m_value.m_creature.m_creatureType].m_aiValue
            * m_value.m_creature.m_count;

    default:
        return 0;
    }
}

// DC TSeerHut::GiveReward (seerhut.cpp:266, dc0x12d4dc) owns the older
// equivalent switch. Complete doSeerEvent passes this+5 at0x573919; the
// retained body reads its type at+0, proving the separate TSeerReward owner.
VA(0x00573c80, 0x290)
void TSeerReward::giveReward(hero* currentHero, bool humanPlayer)
{
    switch (m_rewardType) {
    case eRewardExperience:
        currentHero->giveExperience(static_cast<int>(
            currentHero->getExperienceBonusFactor() * m_value.m_dwords[0]),
            1, 1);
        break;

    case eRewardMana:
    {
        int amount;
        if (currentHero->m_mana + m_value.m_dwords[0] > 999)
            amount = 999 - currentHero->m_mana;
        else if (currentHero->m_mana + m_value.m_dwords[0] < 0)
            amount = currentHero->m_mana;
        else
            amount = m_value.m_dwords[0];
        currentHero->m_mana += amount;
        break;
    }

    case eRewardMorale:
        currentHero->m_moraleBonus += m_value.m_signedLow.m_bonus;
        break;

    case eRewardLuck:
        currentHero->m_luckBonus += m_value.m_signedLow.m_bonus;
        break;

    case eRewardResource:
        currentHero->giveResource(m_value.m_resource.m_resourceType,
                                  m_value.m_resource.m_quantity);
        break;

    case eRewardPrimarySkill:
        currentHero->adjustPrimarySkill(m_value.m_primarySkill.m_skillType,
                                        m_value.m_primarySkill.m_bonus);
        break;

    case eRewardSecondarySkill:
    {
        int skill = m_value.m_secondarySkill.m_skillType;
        int bonus = m_value.m_secondarySkill.m_bonus;
        if (currentHero->m_skillLevel[skill] == 0) {
            if (currentHero->m_skillCount < 8) {
                currentHero->giveSS(skill, bonus);
                break;
            }
        }
        if (currentHero->m_skillLevel[skill] > 0
            && currentHero->m_skillLevel[skill] < bonus)
            currentHero->giveSS(skill,
                bonus - currentHero->m_skillLevel[skill]);
        break;
    }

    case eRewardArtifact:
        if (currentHero->getNumberInBackpack(1) < 64) {
            type_artifact artifact(ARTIFACT_NONE);
            {
                artifact.m_artifactId = TArtifact(m_value.m_dwords[0]);
            }
            currentHero->giveArtifact(&artifact, 1, 1);
            if (!humanPlayer)
                aiEquipArtifacts(currentHero);
        }
        break;

    case eRewardSpell:
        if (currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)
            && g_spellTraits[m_value.m_dwords[0]].m_level
                <= currentHero->m_skillLevel[eSecSkillWisdom] + 2
            && !currentHero->isInSpellbook(m_value.m_dwords[0]))
            currentHero->addSpell(m_value.m_dwords[0]);
        break;

    case eRewardCreature:
        if (!currentHero->m_army.add(m_value.m_creature.m_creatureType,
                                   m_value.m_creature.m_count, -1)) {
            if (humanPlayer) {
                TCreatureType creature;
                {
                    creature = TCreatureType(m_value.m_creature.m_creatureType);
                }
                doMonsterJoinDialog(currentHero, creature,
                    m_value.m_creature.m_count);
            } else {
                TCreatureType creature;
                {
                    creature = TCreatureType(m_value.m_creature.m_creatureType);
                }
                aiJoinDecision(currentHero, creature,
                    m_value.m_creature.m_count);
            }
        }
        break;
    }
}

// DC TSeerHut::GetRewardExtra (seerhut.cpp:373, dc0x12d6d8) queries the
// same reward domain. Complete's completion offer passes this+5 to0x573f10,
// which reads type+0/payload+4/+8: the interface moved to TSeerReward.
VA(0x00573f10, 0xBC)
int TSeerReward::getRewardExtra(const hero* thisHero)
{
    switch (m_rewardType) {
    case eRewardExperience:
        return static_cast<int>(
            const_cast<hero*>(thisHero)->getExperienceBonusFactor()
            * m_value.m_dwords[0]);
    case eRewardMana:
        return m_value.m_dwords[0];
    case eRewardMorale:
    case eRewardLuck:
        return m_value.m_signedLow.m_bonus;
    case eRewardResource:
        return m_value.m_dwords[1];
    case eRewardPrimarySkill:
        return m_value.m_signedHigh.m_bonus;
    case eRewardSecondarySkill:
        return m_value.m_secondarySkill.m_skillType * 3
            + m_value.m_secondarySkill.m_bonus + 2;
    case eRewardArtifact:
    case eRewardSpell:
        return m_value.m_dwords[0];
    case eRewardCreature:
        return static_cast<unsigned short>(m_value.m_dwords[0])
            | (static_cast<unsigned short>(m_value.m_creature.m_count) << 16);
    default:
        return -1;
    }
}

VA(0x00573fd0, 0x91)  // dc 0x12d8c0
int TSeerHut::save(TAbstractFile* outfile)
{
    if (!m_quest) {
        unsigned char questType = 0;
        outfile->write(&questType, sizeof(questType));
    } else {
        unsigned char questType =
            static_cast<unsigned char>(m_quest->questType());
        outfile->write(&questType, sizeof(questType));
        m_quest->save(outfile);
    }

    outfile->write(&m_reward, sizeof(m_reward));

    {
        unsigned char value = m_completedByPlayer;
        outfile->write(&value, sizeof(value));
    }
    {
        unsigned char value = m_visitedPlayers;
        outfile->write(&value, sizeof(value));
    }
    {
        unsigned char value = m_nameIndex;
        return outfile->write(&value, sizeof(value));
    }
}

VA(0x00574070, 0x138)  // UpdateQuestLocator caller; HD twin 0x574440
std::string TSeerHut::getSeerLogText()
{
    std::string logFormat =
        m_quest->questTexts()[type_quest::QUEST_TEXT_LOG];
    return formatString(
        logFormat.c_str(),
        m_quest->getRequirementText().c_str(),
        (*g_seerHutNamesPointer)[m_nameIndex].c_str());
}

// The TQuestGuard pair's TSeerHut twin, and it splits CROSSWISE: 0x5741b0
// takes " " and is SetRolloverText's, 0x5743e0 takes "\n\n" and is
// QuickInfo's. 556 B each and, again, byte-identical apart from that one
// separator relocation.

VA(0x005741b0, 0x22C)
std::string TSeerHut::seerHutFn005741B0(int player) const
{
    if (!playerHasInfo(static_cast<unsigned char>(player)))
        return g_seerName;

    std::string text;
    text = formatString(
        g_generalText->getText(GENERAL_TEXT_SEER_HUT_NAME_FORMAT),
        (*g_seerHutNamesPointer)[m_nameIndex].c_str());

    if (m_quest) {
        text += DATA_COMPGEN(0x00660330, seerHutRolloverSeparator, " ");
        text += m_quest->getQuestDescription();
    }

    return text;
}

VA(0x005743e0, 0x22C)
std::string TSeerHut::seerHutFn005743E0(int player) const
{
    if (!playerHasInfo(static_cast<unsigned char>(player)))
        return g_seerName;

    std::string text;
    text = formatString(
        g_generalText->getText(GENERAL_TEXT_SEER_HUT_NAME_FORMAT),
        (*g_seerHutNamesPointer)[m_nameIndex].c_str());

    if (m_quest) {
        text += DATA_COMPGEN(0x006603b0, seerHutQuickInfoSeparator, "\n\n");
        text += m_quest->getQuestDescription();
    }

    return text;
}

// 0x574610, `ret 4` - the SeerHutList twin of TQuestGuard::read, reached
// from readObject's SEER arm (mapcell.cpp:4191) on the temporary hut it has
// just default-constructed. Three parts.

// The quest. Restoration of Erathia's map format has no quest record at all:
// the seer hut carries one artifact ordinal, and the reader synthesises the
// quest around it - a type_artifact_quest whose single artifact is that
// ordinal, whose field_38 text row is picked at random out of three, and
// whose artifact is struck off the map's own pool. Every later format reads
// the quest type byte and hands the record to the factory, exactly as the
// guard's reader does; the only difference from TQuestGuard::read is the
// flags argument, which is 1 here and 0 there.

// The reward. One type byte, then a jump-table switch whose ten arms are the
// TSeerRewardType roster; the two-byte artifact and creature ordinals narrow
// to one byte on Restoration of Erathia maps, which is why those two arms
// re-read gpGame->mapHeader.version. Retail reads all of them through ONE
// four-byte stack slot addressed at three widths - [ebp+8] as an int,
// [ebp+0xa] as a short and [ebp+0xb] as a signed char - so the three buffers
// below are function-scoped and let VC6 coalesce them the same way. The two
// bytes read straight after the switch are read and discarded.

// The name. Every hut takes an unused entry from gpSeerHutNames: a byte per
// name, all set, then cleared for each name the map's existing huts already
// hold, then a random one of what is left. The count subtracted from the
// name total is the hut list's own size, recomputed from a fresh gpGame -
// retail loads it twice and this transcribes both. The byte array is a real
// std::vector<unsigned char>: `_Allocate`'s own `if (_N < 0) _N = 0` clamp
// and `_Construct`'s per-element null check are both in the bytes, and it
// carries NO unwind action because nothing between its construction and its
// destruction can throw - rand() is extern "C" and nothrow under /GX.

// BLOCK SCOPE ON THE READ BUFFERS IS WORTH +6.07 (76.6567 -> 82.7295).
// Retail addresses every scalar read through the dead `infile` parameter
// home at three widths; function-scoped buffers get slots of their own at
// [ebp-1] and below and the whole frame walks. One declaration per arm is
// what puts them back.

// Residual (94.24%, raised from 86.81%): the one-byte Morale, Luck and Primary
// bonuses are unsigned-char conversions, and the two-byte creature count is an
// unsigned-short conversion. Those four source types recover the entire switch
// and tail: all 58 CFG blocks and 22 branches align, 56 blocks are byte-exact,
// and every post-constructor opcode agrees apart from the resulting eight-byte
// frame displacement. The two size-only blocks are the legacy artifact arm:
// retail expands the shared three-argument constructor but calls its nested
// type_quest(flags), while this caller expands both. The exact load sibling
// needs both expansions. Plain `inline`, moving the definition ahead of the
// base constructor, and caller inline-depth(1) are byte-flat. A constructor-level
// inline-depth(0) is the negative control: read falls to 91.86%, exact load to
// 82.72%, and is not retained. Naming the artifact falls to 93.70%; an explicit
// signed comparison is byte-flat. Keep the canonical shared constructor and
// caller-specific natural inliner state rather than pinning either caller.
// Keeping the canonical SetRandomName definition inline lets the Complete
// caller expand its revised allocation/random-selection body while preserving
// the Dreamcast-proven helper. The remaining legacy artifact-quest arm expands
// its nested type_quest construction where retail retains that call; the extra
// inline budget also leaves the name vector's element construction as a call.
VA(0x00574610, 0x480)  // anchor-caller readObject SEER arm; bracket seerhut..singleselectionpopups
void TSeerHut::read(TAbstractFile* infile)
{
    if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        int textRow = rand() % 3;
        signed char charBuffer;
        infile->read(&charBuffer, sizeof(charBuffer));
        if (charBuffer == -1) {
            m_quest = 0;
        } else {
            m_quest = new type_artifact_quest(
                1, static_cast<TArtifact>(charBuffer), textRow); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
        }
    } else {
        int intBuffer;
        infile->read(&intBuffer, 1);
        type_quest* newQuest =
            createQuest(intBuffer & 0xff, 1);
        if (newQuest)
            newQuest->loadFromMap(infile);
        m_quest = newQuest;
    }

    m_completedByPlayer = 0;
    {
        int intBuffer;
        infile->read(&intBuffer, 1);
        m_reward.m_rewardType = intBuffer & 0xff;
    }

    switch (m_reward.m_rewardType) {
    case eRewardExperience: {
        int intBuffer;
        infile->read(&intBuffer, sizeof(intBuffer));
        m_reward.m_value.m_dwords[0] = intBuffer;
        break;
    }

    case eRewardMana: {
        int intBuffer;
        infile->read(&intBuffer, sizeof(intBuffer));
        m_reward.m_value.m_dwords[0] = intBuffer;
        break;
    }

    case eRewardMorale: {
        int intBuffer;
        infile->read(&intBuffer, 1);
        m_reward.m_value.m_signedLow.m_bonus =
            static_cast<unsigned char>(intBuffer);
        break;
    }

    case eRewardLuck: {
        int intBuffer;
        infile->read(&intBuffer, 1);
        m_reward.m_value.m_signedLow.m_bonus =
            static_cast<unsigned char>(intBuffer);
        break;
    }

    case eRewardResource: {
        signed char charBuffer;
        int intBuffer;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_reward.m_value.m_resource.m_resourceType = charBuffer;
        infile->read(&intBuffer, sizeof(intBuffer));
        m_reward.m_value.m_resource.m_quantity = intBuffer;
        break;
    }

    case eRewardPrimarySkill: {
        signed char charBuffer;
        int intBuffer;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_reward.m_value.m_primarySkill.m_skillType = charBuffer;
        infile->read(&intBuffer, 1);
        m_reward.m_value.m_primarySkill.m_bonus =
            static_cast<unsigned char>(intBuffer);
        break;
    }

    case eRewardSecondarySkill: {
        signed char charBuffer;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_reward.m_value.m_secondarySkill.m_skillType = charBuffer;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_reward.m_value.m_secondarySkill.m_bonus = charBuffer;
        break;
    }

    case eRewardArtifact:
        if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            int intBuffer;
            infile->read(&intBuffer, 1);
            m_reward.m_value.m_dwords[0] = intBuffer & 0xff;
        } else {
            short shortBuffer;
            infile->read(&shortBuffer, sizeof(shortBuffer));
            m_reward.m_value.m_dwords[0] = shortBuffer;
        }
        break;

    case eRewardSpell: {
        int intBuffer;
        infile->read(&intBuffer, 1);
        m_reward.m_value.m_dwords[0] = intBuffer & 0xff;
        break;
    }

    case eRewardCreature: {
        if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            int intBuffer;
            infile->read(&intBuffer, 1);
            m_reward.m_value.m_creature.m_creatureType = intBuffer & 0xff;
        } else {
            short shortBuffer;
            infile->read(&shortBuffer, sizeof(shortBuffer));
            m_reward.m_value.m_creature.m_creatureType = shortBuffer;
        }
        int countBuffer;
        infile->read(&countBuffer, 2);
        m_reward.m_value.m_creature.m_count =
            static_cast<unsigned short>(countBuffer);
        break;
    }
    }

    {
        short shortBuffer;
        infile->read(&shortBuffer, sizeof(shortBuffer));
    }

    setRandomName(*this);
}

VA(0x00574A90, 0x24A)  // dc 0x12d8e4
void TSeerHut::load(TAbstractFile* infile, int saveVersion)
{
    if (saveVersion < 28) {
        int intBuffer;
        infile->read(&intBuffer, sizeof(intBuffer));
        infile->read(&m_reward, sizeof(m_reward));
        unsigned char noQuest;
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            noQuest = value != 0;
        }
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            m_completedByPlayer = value;
        }
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            m_visitedPlayers = value;
        }
        {
            unsigned char value;
            infile->read(&value, sizeof(value));  // reserved legacy byte
        }
        int textRow;
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            textRow = value;
        }
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            m_nameIndex = value;
        }
        if (noQuest || intBuffer == -1)
            m_quest = 0;
        else
            m_quest = new type_artifact_quest(
                1, static_cast<TArtifact>(intBuffer), textRow); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
    } else {
        type_quest* newQuest;
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            newQuest = createQuest(value, 1);
        }
        m_quest = newQuest;
        if (newQuest)
            newQuest->load(infile, saveVersion);
        infile->read(&m_reward, sizeof(m_reward));
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            m_completedByPlayer = value;
        }
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            m_visitedPlayers = value;
        }
        {
            unsigned char value;
            infile->read(&value, sizeof(value));
            m_nameIndex = value;
        }
    }
}

// COMDAT pairing: Hstd on the char instantiation, mnemonic agreement 1.000.
VA_COMPGEN(0x00574d10, 0x120, BASIC_STRING_CONCAT, char)

// COMDAT pairing: basic_string<char>'s scalar deleting destructor, agreement
// 1.000 at an exactly equal 86-byte extent. The body is _Tidy expanded in
// place - the reference-count byte at `_Ptr[-1]`, saturating at 0xff, freed
// when it reaches zero - ahead of the delete, which no other ??_G in the
// image looks like.
VA_COMPGEN(0x004b7080, 0x56, SCALAR_DELETING_DTOR, basic_string)

// COMDAT pairing: vector<basic_string<char>>::_Ufill, agreement 1.000 at an
// exactly equal 41-byte extent.
VA_COMPGEN(0x004af840, 0x29, VECTOR_UFILL, string)

// The shared single-string insertion and its retained library helpers no
// longer emit in events, but this TU naturally emits the same specializations.
// Retail calls 0x4af350 from creatureBankEvent and five quest dialogs:
// artifact/creature/resource proposals, creature progress and resource text.
// The latter callers prove this consumer independently of matching scores.
// All 1,217 instruction bytes below agree outside relocation operands.
// Allocation/deallocation, _Construct/_Ufill and the four calls within this
// chain agree; size at 0x4af4e0 is byte-identical to the retained string-vector
// size (also folded with vector<vector<hero> >). _Xran and memmove are the
// retail invalid-position throw and overlap-safe copy; npos/empty-string
// data point to 0x63a60c/0x63a608. No template body or caller is manufactured.
VA_COMPGEN(0x004af350, 0x183, VECTOR_INSERT_SINGLE, string)
VA_COMPGEN(0x004af500, 0x4D, VECTOR_DESTROY, string)
VA_COMPGEN(0x004af800, 0x38, VECTOR_UCOPY, string)
VA_COMPGEN(0x004af870, 0x154, STD_FILL, string)
VA_COMPGEN(0x004af9d0, 0x165, STD_COPY_BACKWARD, string)
