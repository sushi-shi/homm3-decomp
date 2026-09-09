// Actual coordinate, bounds, packed-zone and accessor definitions imported.
// The reference filters a flat map allocation by rectangle membership rather
// than copying the candidate's nested loops. Small fixtures avoid overflow.
#include <vector>
#include <cstdio>
#include <cstring>
// @TYPES@
struct TRmgMapItem { unsigned m_guard; TRmgZoneCellState m_zoneState; };
struct type_random_map {
    int m_mapWidth, m_mapHeight;
    TRmgMapItem* m_mapItems;
    // @ACCESSOR@
};
struct TRmgTownSlot { int m_zoneIndex; };
struct TRmgZone {
    TRmgTownSlot* m_slot;
    TRmgZoneBounds m_bounds;
    TRmgMapPosition m_levelPosition;
    TRmgMapPosition getLevelPosition() const;
    void setLevelPosition(TRmgMapPosition);
};
// @HELPERS@
struct Root { type_random_map m_map; };
template<class T> static bool check() {
    for (int scenario = 0; scenario < 3000; ++scenario) {
        int width = 1 + scenario % 9, height = 1 + scenario / 9 % 7;
        int zones[] = {-128, -1, 0, 1, 7, 127, 128, 255};
        int index = zones[scenario / 7 % 8];
        std::vector<TRmgMapItem> items(width * height * 2 + 2);
        for (unsigned i = 0; i < items.size(); ++i) {
            items[i].m_guard = 0x13570000 + i;
            unsigned bits = (i + 1) * 0x1234567U;
            std::memcpy(&items[i].m_zoneState, &bits, 4);
            // C++ signed bit-field truncation is the same observed input
            // representation the reference later decodes explicitly.
            items[i].m_zoneState.m_zone = (i + scenario) % 3 ? index : 2;
        }
        std::vector<TRmgMapItem> before = items;
        TRmgTownSlot slot; slot.m_zoneIndex = index;
        TRmgZone zone; zone.m_slot = &slot;
        zone.m_bounds.m_minimumX = scenario / 11 % (width + 1);
        zone.m_bounds.m_minimumY = scenario / 13 % (height + 1);
        zone.m_bounds.m_maximumX = width - scenario / 17 % (width + 1);
        zone.m_bounds.m_maximumY = height - scenario / 19 % (height + 1);
        zone.m_levelPosition = TRmgMapPosition(-23 + scenario % 13, 30 - scenario % 17, scenario % 2);
        TRmgZone saved = zone;
        int count = 0, sumX = 0, sumY = 0;
        for (int i = 0; i < width * height * 2; ++i) {
            int z = i / (width * height), y = (i / width) % height, x = i % width;
            if (z != saved.m_levelPosition.m_z || x < zone.m_bounds.m_minimumX || x >= zone.m_bounds.m_maximumX
                || y < zone.m_bounds.m_minimumY || y >= zone.m_bounds.m_maximumY) continue;
            unsigned bits; std::memcpy(&bits, &items[i + 1].m_zoneState, 4);
            int value = (bits >> 16) & 255;
            if (value >= 128) value -= 256;
            if (value == index) { ++count; sumX += x; sumY += y; }
        }
        T candidate;
        candidate.m_map.m_mapWidth = width; candidate.m_map.m_mapHeight = height;
        candidate.m_map.m_mapItems = &items[1];
        candidate.recenterZone(&zone);
        TRmgMapPosition expected = saved.m_levelPosition;
        if (count) { expected.m_x = sumX / count; expected.m_y = sumY / count; }
        if (std::memcmp(&zone.m_levelPosition, &expected, sizeof(expected)) || zone.m_slot != saved.m_slot
            || std::memcmp(&zone.m_bounds, &saved.m_bounds, sizeof(zone.m_bounds))
            || std::memcmp(&items[0], &before[0], items.size() * sizeof(items[0]))) return false;
    }
    return true;
}
// @CANDIDATES@
int main() {
    // @CHECKS@
    return 0;
}
