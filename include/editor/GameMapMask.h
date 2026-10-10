// GameMapMask.h - the obstacle tool's painted area (GOG only, h3maped
// 0x43c133..0x43d0df). Its code is an object of its own (the 28-byte
// static-init thunk at 0x43c117 that opens every object) between GameMap.obj
// and GameObject.obj in the alphabetical link order, so its file name sorts
// between those two: a GameMap* name, not ObstacleArea. "Mask" describes the
// data; the class and file names are not recorded. Each map level keeps two
// bits per tile in blocks of 12 x 12 tiles, shared copy on write between the
// mask's copies; the map edit window tints the tiles of state 1 and 2 blue.
//
// Three copy-on-write levels, each held by a TRefCountingPtr (the map
// view's inline assignment 0x486a37 and the releases 0x43cc59, 0x43cd5a and
// 0x43ce72): the levels and the width in blocks, each level's tile count
// and blocks, and each block's twelve rows of three bytes, built zeroed
// element by element (0x43d06f). The obstacle tool paints state 2 where
// obstacles go and keeps state 1 clear around them; the member and state
// names are not recorded.
#ifndef HOMM3_EDITOR_GAMEMAPMASK_H
#define HOMM3_EDITOR_GAMEMAPMASK_H

#include <vector>

#include "editor/RefCountingPtr.h"

class TGameMapMask {
public:
    enum TTileState {
        eTileNone,
        eTileClear,
        eTileObstacle
    };

    TGameMapMask(unsigned int width, unsigned int height, bool bTwoLayer);

    bool isTwoLayer() const { return _m_pImpl->m_layers.size() > 1; }
    bool hasTiles(bool bSecondLayer) const { return _m_pImpl->m_layers[bSecondLayer]->m_numTiles > 0; }
    void setTileState(unsigned int x, unsigned int y, bool bSecondLayer, int state);
    void clear();
    void addSecondLayer();
    void removeSecondLayer();
    int getTileState(unsigned int x, unsigned int y, bool bSecondLayer) const;

private:
    // Four tiles' states.
    class _TQuad {
    public:
        _TQuad() : m_bits(0) {}
        ~_TQuad() {}

        int getState(unsigned int i) const { return m_bits >> i * 2 & 3; }
        void setState(unsigned int i, int state)
        {
            unsigned int shift = i * 2;
            m_bits = m_bits & ~(3 << shift) | state << shift;
        }

        unsigned char m_bits;
    };

    struct _TBlock {
        // The state of the block's tile (x, y).
        int getState(unsigned int x, unsigned int y) const { return m_aaQuads[y][x / 4].getState(x % 4); }

        _TQuad m_aaQuads[12][3];
    };

    struct _TLayer {
        unsigned int m_numTiles;
        std::vector<TRefCountingPtr<_TBlock> > m_blocks;
    };

    struct _TImpl {
        unsigned int m_numBlockColumns;
        std::vector<TRefCountingPtr<_TLayer> > m_layers;
    };

    TRefCountingPtr<_TImpl> _m_pImpl;
};

#endif  /* HOMM3_EDITOR_GAMEMAPMASK_H */
