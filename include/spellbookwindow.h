#ifndef HOMM3_SPELLBOOKWINDOW_H
#define HOMM3_SPELLBOOKWINDOW_H

#include "advmgr_popup.h"
#include "spellschool.h"

#include "herospec.h"
class armyGroup;
class bitmapBackedTextWidget;
class bitmapBorder;
class hero;
class iconWidget;
class textWidget;

typedef int SpellID;

// DC lays this class out over its 0x58-byte CAdvPopup and has six spell
// entries per page. Retail's constructor and GotoPage prove both divergences:
// its base is 0x60 bytes, and the loop at 0x59cf54 walks twelve SpellMap
// entries. The widget-range pointers then occupy +0xac through +0xb4, and
// the five individual widget pointers occupy +0xb8 through +0xc8.
class TSpellbookWindow : public CAdvPopup {
public:
    enum TSpellContext {
        eContextInvalid = -1,
        eContextCombat = 0,
        eContextAdventure = 1,
        eContextNeither = 2
    };

    enum TSpellContextMask {
        eCombatContextMask = 1,
        eAdventureContextMask = 2
    };

    // Dreamcast CodeView's class-local widget domain. Complete doubles the
    // spell rows per page but retains this complete 200..249 ID layout.
    enum EOtherWidgetIDs {
        BACKGROUND_ID = 200,
        SPELL_LEVEL_0_ID = 201,
        SPELL_LEVEL_1_ID = 202,
        SPELL_LEVEL_2_ID = 203,
        SPELL_LEVEL_3_ID = 204,
        SPELL_LEVEL_4_ID = 205,
        SPELL_LEVEL_5_ID = 206,
        SPELL_LEVEL_6_ID = 207,
        SPELL_LEVEL_7_ID = 208,
        SPELL_LEVEL_8_ID = 209,
        SPELL_LEVEL_9_ID = 210,
        SPELL_LEVEL_10_ID = 211,
        SPELL_LEVEL_11_ID = 212,
        SPELL_0_ID = 213,
        SPELL_1_ID = 214,
        SPELL_2_ID = 215,
        SPELL_3_ID = 216,
        SPELL_4_ID = 217,
        SPELL_5_ID = 218,
        SPELL_6_ID = 219,
        SPELL_7_ID = 220,
        SPELL_8_ID = 221,
        SPELL_9_ID = 222,
        SPELL_10_ID = 223,
        SPELL_11_ID = 224,
        SCHOOL_TABS_ID = 225,
        AIR_SCHOOL_ID = 226,
        FIRE_SCHOOL_ID = 227,
        WATER_SCHOOL_ID = 228,
        EARTH_SCHOOL_ID = 229,
        ALL_SCHOOL_ID = 230,
        COMBAT_SPELLS_ID = 231,
        ADVENTURE_SPELLS_ID = 232,
        SPELL_POINTS_ID = 233,
        PREVIOUS_PAGE_ID = 234,
        NEXT_PAGE_ID = 235,
        SCHOOL_HEADING_ID = 236,
        SPELL_0_NAME_ID = 237,
        SPELL_1_NAME_ID = 238,
        SPELL_2_NAME_ID = 239,
        SPELL_3_NAME_ID = 240,
        SPELL_4_NAME_ID = 241,
        SPELL_5_NAME_ID = 242,
        SPELL_6_NAME_ID = 243,
        SPELL_7_NAME_ID = 244,
        SPELL_8_NAME_ID = 245,
        SPELL_9_NAME_ID = 246,
        SPELL_10_NAME_ID = 247,
        SPELL_11_NAME_ID = 248,
        ROLLOVER_ID = 249
    };

    enum { SPELLS_PER_PAGE = 12 };

    class TSpellbookEntry {
    public:
        TSpellbookEntry(SpellID id, TSpellSchool school,
                        TSkillMastery mastery)
            : m_id(id), m_school(school), m_mastery(mastery)
        {
        }
        bool operator<(const TSpellbookEntry& y) const;
        SpellID m_id;
        TSpellSchool m_school;
        TSkillMastery m_mastery;
    };

    TSpellbookWindow(const hero& h, const armyGroup* g,
                     TSpellContext context, int magicTerrain);
    virtual ~TSpellbookWindow();
    virtual int open(int newPriority, unsigned char update);
    virtual void close(unsigned char update);

    // E:\gamedcs\SpellbookWindow.h:222
    void setSchool(TSpellSchool school)
    {
        m_school = school;
        s_lastSchool = school;
    }
    // E:\gamedcs\SpellbookWindow.h:230
    unsigned getSchool() const
    {
        return m_school;
    }
    // E:\gamedcs\SpellbookWindow.h:236
    void setContext(TSpellContext context)
    {
        if (context == eContextAdventure)
            m_contextMask = eAdventureContextMask;
        else
            m_contextMask = eCombatContextMask;
        s_lastContext = context;
    }
    // E:\gamedcs\SpellbookWindow.h:248
    unsigned getContextMask() const
    {
        return m_contextMask;
    }
    void gotoPage(int page);
    int getPage();
    // E:\gamedcs\SpellbookWindow.h:258
    void previousPage()
    {
        gotoPage(m_page - 1);
    }
    // E:\gamedcs\SpellbookWindow.h:264
    void nextPage()
    {
        gotoPage(m_page + 1);
    }
    static void reset();
    virtual int windowHandler(message& msg);

private:
    const TSpellContext m_allowedContext;       // +0x60
    const hero* m_hero;                         // +0x64
    const armyGroup* m_enemyGroup;              // +0x68
    int m_onMagicPlains;                        // +0x6c; retail widens DC's byte
    TSpellSchool m_school;                      // +0x70
    unsigned m_contextMask;                     // +0x74
    int m_page;                                 // +0x78
    SpellID m_spellMap[SPELLS_PER_PAGE];        // +0x7c
    iconWidget** m_spellLevelWidgets;           // +0xac
    iconWidget** m_spellIconWidgets;            // +0xb0
    textWidget** m_spellNameWidgets;            // +0xb4
    iconWidget* m_headingWidget;                // +0xb8
    bitmapBorder* m_nextPageWidget;              // +0xbc
    bitmapBorder* m_previousPageWidget;          // +0xc0
    iconWidget* m_schoolTabsWidget;              // +0xc4
    bitmapBackedTextWidget* m_rolloverWidget;    // +0xc8
    static TSpellContext s_lastContext;
    static TSpellSchool s_lastSchool;

    static int s_lastPage;

    std::string getSpellDescription(SpellID spell,
                                      const hero* currentHero,
                                      unsigned char rollover);

    int convertID2HelpID(int id) const;
    static int getPositionFromSchool(unsigned schoolMask);
    static TSpellSchool getSchoolFromPosition(int position);
    void displayNewSchool(int position);
};
SIZE(TSpellbookWindow, 0xcc);
SIZE(TSpellbookWindow::TSpellbookEntry, 0x0c);

#endif  /* HOMM3_SPELLBOOKWINDOW_H */
