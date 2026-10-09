// GameMapMask.h - the obstacle tool's painted area (GOG only, h3maped
// 0x43c133..0x43d0df). Its code is an object of its own (the 28-byte
// static-init thunk at 0x43c117 that opens every object) between GameMap.obj
// and GameObject.obj in the alphabetical link order, so its file name sorts
// between those two: a GameMap* name, not ObstacleArea. "Mask" describes the
// data; the class and file names are not recorded. Each map level keeps two
// bits per tile in blocks of 12 x 12 tiles, shared copy on write between the
// mask's copies; the map edit window tints the tiles of state 1 and 2 blue.
// Declared so far only as far as the map windows need it: the layout below is
// the one the edit window's inline emptiness test reads (the data's layer
// pointers at +0xc, each layer's tile count at +4).
#ifndef HOMM3_EDITOR_GAMEMAPMASK_H
#define HOMM3_EDITOR_GAMEMAPMASK_H

#include <vector>

class TGameMapMask {
public:
    bool hasTiles(bool bSecondLayer) const { return _m_pData->m_layers[bSecondLayer]->m_numTiles > 0; }
    int getTileState(unsigned int x, unsigned int y, bool bSecondLayer) const;

private:
    struct _TLayer {
        int m_refCount;
        unsigned int m_numTiles;
    };

    struct _TData {
        int m_refCount;
        unsigned int m_numBlockColumns;
        std::vector<_TLayer*> m_layers;
    };

    _TData* _m_pData;
};

#endif  /* HOMM3_EDITOR_GAMEMAPMASK_H */
