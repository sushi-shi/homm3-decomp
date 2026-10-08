// FindDlg.cpp - Loki h3maped object 78: the find dialog. The findable
// entries are built once, from the object type names (less the kinds the
// other tables name and the terrain overlays) and the artifact, creature
// bank, generator, garrison, hero class, mine, monster, resource and town
// tables. The assert lines come from the retail immediates.
#include "editor/stdafx.h"

namespace {
#include <gtk/gtk.h>
}

#include "editor/FindDlg.h"

#include <bitset>
#include <vector>

#include "adventureobjecttype.h"
#include "artifact.h"
#include "objnames.h"
#include "editor/cppbridge.h"
#include "editor/GameResource.h"
#include "editor/ObjectSpecializations.h"
#include "creaturetype.h"
#include "editor/Hero.h"
#include "editor/Town.h"

namespace {
// A findable object kind: its name, object type and extra (-1 for any).
struct TFindWhatEntry {
    TFindWhatEntry(const char* name, int type, int extra) : m_name(name), m_type(type), m_extra(extra) {}

    const char* m_name;
    int m_type;
    int m_extra;
};

class TAFindWhatEntry : public vector<TFindWhatEntry> {
public:
    TAFindWhatEntry();
};

TAFindWhatEntry::TAFindWhatEntry()
{
    static const TAdventureObjectType akSpecificType[] = {
        NOTHING, ANCHOR_POINT, CREATURE_BANK, CREATURE_GENERATOR_1, CREATURE_GENERATOR_2, CREATURE_GENERATOR_3,
        CREATURE_GENERATOR_4, GARRISON, TERRAIN_RIVER_1, TERRAIN_RIVER_2, TERRAIN_RIVER_3, TERRAIN_RIVER_4,
        TERRAIN_ROAD_1, TERRAIN_ROAD_2, TERRAIN_ROAD_3
    };
    bitset<MAX_EVENT_TYPE> specificTypes;
    for (unsigned int i = 0; i < sizeof(akSpecificType) / sizeof(akSpecificType[0]); i++)
        specificTypes[akSpecificType[i]] = true;
    for (int type = 0; type < MAX_EVENT_TYPE; type++)
        if (!specificTypes[type] && *akAdvObjectTypeTraits[type].m_name != '\0')
            push_back(TFindWhatEntry(akAdvObjectTypeTraits[type].m_name, type, -1));
    for (unsigned int artifact = 0; artifact < kNumArtifacts; artifact++)
        if ((akArtifactTraits[artifact].m_class & ArtifactClassSpecial) == 0)
            push_back(TFindWhatEntry(akArtifactTraits[artifact].m_name, ARTIFACT, artifact));
    for (unsigned int bank = 0; bank < kNumCreatureBankTypes; bank++)
        push_back(TFindWhatEntry(akCreatureBankTypeTraits[bank].m_name, CREATURE_BANK, bank));
    unsigned int generator;
    for (generator = 0; generator < TGenerator::s_kNumGenerator1Types; generator++)
        push_back(TFindWhatEntry(TGenerator::s_akGenerator1TypeTraits[generator].m_name, CREATURE_GENERATOR_1,
                                 generator));
    for (generator = 0; generator < TGenerator::s_kNumGenerator4Types; generator++)
        push_back(TFindWhatEntry(TGenerator::s_akGenerator4TypeTraits[generator].m_name, CREATURE_GENERATOR_4,
                                 generator));
    for (unsigned int garrison = 0; garrison < TGarrison::s_kNumTypes; garrison++)
        push_back(TFindWhatEntry(TGarrison::s_akTypeTraits[garrison].m_name, GARRISON, garrison));
    for (unsigned int heroClass = 0; heroClass < kNumHeroClasses; heroClass++)
        push_back(TFindWhatEntry(THero::s_akClassTraits[heroClass].m_name, HERO, heroClass));
    for (unsigned int mine = 0; mine < TMine::s_kNumMineTypes; mine++)
        push_back(TFindWhatEntry(TMine::s_akMineTypeTraits[mine].m_name, MINE, mine));
    for (unsigned int creature = 0; creature < kNumCreatureTypes; creature++)
        push_back(TFindWhatEntry(akCreatureTypeTraits[creature].m_name, MONSTER, creature));
    for (unsigned int resource = 0; resource < kNumGameResourceTypes; resource++)
        push_back(TFindWhatEntry(akGameResourceTypeTraits[resource].m_name, RESOURCE, resource));
    for (unsigned int town = 0; town < kNumTownTypes; town++)
        push_back(TFindWhatEntry(TTown::s_akTypeTraits[town].m_pName, TOWN, town));
}

inline const TAFindWhatEntry& getAFindWhatEntry()
{
    static TAFindWhatEntry aFindWhatEntry;
    return aFindWhatEntry;
}
}

TFindDlg::TFindDlg(int findType, int findExtra)
    : _m_findType(findType), _m_findExtra(findExtra), _m_bSearchBackwards(false)
{
    OnInitDialog();
    GtkWidget* pDialog = _widget("find_dlg");
    gtk_widget_show(pDialog);
}

BOOL TFindDlg::OnInitDialog()
{
    static const TAFindWhatEntry& kaFindWhatEntry = getAFindWhatEntry();
    GtkCombo* combo = GTK_COMBO(_widget("find_combo"));
    GtkList* l = GTK_LIST(combo->list);
#line 228
    assert(l != NULL);
    emptyList(l);
    int numItems = 0;
    unsigned int entryNum;
    GList* items = NULL;
    for (entryNum = 0; entryNum < kaFindWhatEntry.size(); entryNum++) {
        items = g_list_append(items, (gpointer) kaFindWhatEntry[entryNum].m_name);
        _m_entryItemData[numItems] = entryNum;
        numItems++;
    }
    gtk_combo_set_popdown_strings(combo, items);
    return TRUE;
}

void TFindDlg::OnOK()
{
    GtkCombo* combo = GTK_COMBO(_widget("find_combo"));
    GtkList* l = GTK_LIST(combo->list);
#line 279
    assert(l != NULL);
    int curSel = getCurrentSelection(l);
#line 282
    assert(curSel != -1);
    unsigned int entryNum = _m_entryItemData[curSel];
#line 285
    assert(entryNum < getAFindWhatEntry().size());
    const TFindWhatEntry& entry = getAFindWhatEntry()[entryNum];
    _m_findType = entry.m_type;
    _m_findExtra = entry.m_extra;
}
