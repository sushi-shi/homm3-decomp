// Artifact.cpp of the Loki port (Loki object 5): the RoE artifact and
// artifact-slot traits tables, loaded from artraits.txt and artslots.txt.
#include <assert.h>
#include <bitset>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "artifact.h"

#include "resourcemanager.h"
#include "textresource.h"

namespace {

// Owns one loaded string; the loader keeps the pointers in the traits rows.
class TAutoStrPtr {
public:
    TAutoStrPtr() : str(0) {}
    ~TAutoStrPtr() { delete[] str; }
    void set(char* newStr) { str = newStr; }
    char* get() const { return str; }

private:
    char* str;
};

TArtifactSlotTraits aArtifactSlotTraitsImp[kNumArtifactSlots];

}

static TArtifactTraits aArtifactTraitsImp[kNumArtifacts];

const TArtifactSlotTraits* akArtifactSlotTraits = aArtifactSlotTraitsImp;
const TArtifactTraits* akArtifactTraits = aArtifactTraitsImp;

static void InitializeArtifactTraits(int id, const vector<char*>& resource);

bool InitializeArtifactTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet("artraits.txt");
    if (!resource)
        return false;
    if (resource->GetNumberOfRows() < 129) {
        ResourceManager::Dispose(resource);
        return false;
    }

    int row = 0;
    int id = 0;
    row += 2;
    for (int i = 0; i < kNumArtifacts; i++) {
        InitializeArtifactTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    ResourceManager::Dispose(resource);

    static TAutoStrPtr slotNames[kNumArtifactSlots];
    resource = ResourceManager::GetSpreadsheet("artslots.txt");
    if (!resource)
        return false;
    if (resource->GetNumberOfRows() < 18) {
        ResourceManager::Dispose(resource);
        return false;
    }
    for (int slot = 0; slot < kNumArtifactSlots; slot++) {
        char* name = resource->GetRow(slot)[0];
        slotNames[slot].set(new char[strlen(name) + 1]);
        strcpy(slotNames[slot].get(), name);
        aArtifactSlotTraitsImp[slot].m_name = slotNames[slot].get();
    }
    ResourceManager::Dispose(resource);
    return true;
}

static void InitializeArtifactTraits(int id, const vector<char*>& resource)
{
#line 113
    assert(id >= 0 && id < kNumArtifacts);
    assert(resource.size() >= 22);
    TArtifactTraits* const traits = &aArtifactTraitsImp[id];

    static TAutoStrPtr names[kNumArtifacts];
    names[id].set(new char[strlen(resource[0]) + 1]);
    strcpy(names[id].get(), resource[0]);
    traits->m_name = names[id].get();

    traits->m_cost = atoi(resource[1]);
    traits->m_slots[17] = *resource[2] && *resource[2] != ' ';
    traits->m_slots[16] = *resource[3] && *resource[3] != ' ';
    traits->m_slots[15] = *resource[4] && *resource[4] != ' ';
    traits->m_slots[14] = *resource[5] && *resource[5] != ' ';
    traits->m_slots[13] = *resource[6] && *resource[6] != ' ';
    traits->m_slots[12] = *resource[7] && *resource[7] != ' ';
    traits->m_slots[11] = *resource[8] && *resource[8] != ' ';
    traits->m_slots[10] = *resource[9] && *resource[9] != ' ';
    traits->m_slots[9] = *resource[10] && *resource[10] != ' ';
    traits->m_slots[8] = *resource[11] && *resource[11] != ' ';
    traits->m_slots[7] = *resource[12] && *resource[12] != ' ';
    traits->m_slots[6] = *resource[13] && *resource[13] != ' ';
    traits->m_slots[5] = *resource[14] && *resource[14] != ' ';
    traits->m_slots[4] = *resource[15] && *resource[15] != ' ';
    traits->m_slots[3] = *resource[16] && *resource[16] != ' ';
    traits->m_slots[2] = *resource[17] && *resource[17] != ' ';
    traits->m_slots[1] = *resource[18] && *resource[18] != ' ';
    traits->m_slots[0] = *resource[19] && *resource[19] != ' ';

    if (*resource[20] == 'R')
        traits->m_class = ArtifactClassRelic;
    else if (*resource[20] == 'J')
        traits->m_class = ArtifactClassMajor;
    else if (*resource[20] == 'N')
        traits->m_class = ArtifactClassMinor;
    else if (*resource[20] == 'T')
        traits->m_class = ArtifactClassTreasure;
    else {
#line 163
        assert(*resource[20] == 'S');
        traits->m_class = ArtifactClassSpecial;
    }

    static TAutoStrPtr descriptions[kNumArtifacts];
    descriptions[id].set(new char[strlen(resource[21]) + 1]);
    strcpy(descriptions[id].get(), resource[21]);
    traits->m_description = descriptions[id].get();
}
