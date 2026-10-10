// GameMapMask.cpp - the obstacle tool's painted area (h3maped
// 0x43c117..0x43d0df; GOG only). The mask keeps two bits per tile in
// blocks of 12 x 12 tiles per map level; the levels, their blocks and the
// mask itself are copy-on-write handles, so the map view's undo history
// shares every block a step did not paint.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/GameMapMask.h"

namespace {

// The tiles of a block's side.
const unsigned int kBlockSize = 12;

}  // namespace

VA(0x0043c133, 0x12a)
TGameMapMask::TGameMapMask(unsigned int width, unsigned int height, bool bTwoLayer)
{
    unsigned int numBlockColumns = (width + kBlockSize - 1) / kBlockSize;
    unsigned int numBlockRows = (height + kBlockSize - 1) / kBlockSize;
    _TImpl& impl = *_m_pImpl;
    impl.m_layers.resize(bTwoLayer ? 2 : 1, TRefCountingPtr<_TLayer>());
    impl.m_layers[0]->m_numTiles = 0;
    impl.m_layers[0]->m_blocks.resize(numBlockColumns * numBlockRows, TRefCountingPtr<_TBlock>());
    if (bTwoLayer) {
        impl.m_layers[1]->m_numTiles = 0;
        impl.m_layers[1]->m_blocks.resize(numBlockColumns * numBlockRows, TRefCountingPtr<_TBlock>());
    }
    impl.m_numBlockColumns = numBlockColumns;
}

VA(0x0043c25d, 0x107)
void TGameMapMask::setTileState(unsigned int x, unsigned int y, bool bSecondLayer, int state)
{
    int oldState = getTileState(x, y, bSecondLayer);
    if (state == oldState)
        return;
    _TImpl& impl = *_m_pImpl;
    _TQuad& quad = impl.m_layers[bSecondLayer]->m_blocks[y / kBlockSize * impl.m_numBlockColumns
                                                         + x / kBlockSize]
                       ->m_aaQuads[y % kBlockSize][x % kBlockSize / 4];
    if (state != eTileNone) {
        if (oldState == eTileNone)
            ++impl.m_layers[bSecondLayer]->m_numTiles;
    } else
        --impl.m_layers[bSecondLayer]->m_numTiles;
    quad.setState(x % kBlockSize % 4, state);
}

VA(0x0043c364, 0x10c)
void TGameMapMask::clear()
{
    _TImpl& impl = *_m_pImpl;
    const std::vector<TRefCountingPtr<_TLayer> >& layers = impl.m_layers;
    unsigned int numBlocks = layers[0]->m_blocks.size();
    if (layers[0]->m_numTiles > 0) {
        _TLayer& layer = *impl.m_layers[0];
        layer.m_numTiles = 0;
        layer.m_blocks.clear();
        layer.m_blocks.resize(numBlocks, TRefCountingPtr<_TBlock>());
    }
    if (layers.size() > 1 && layers[1]->m_numTiles > 0) {
        _TLayer& layer = *impl.m_layers[1];
        layer.m_numTiles = 0;
        layer.m_blocks.clear();
        layer.m_blocks.resize(numBlocks, TRefCountingPtr<_TBlock>());
    }
}

VA(0x0043c470, 0x102)
void TGameMapMask::addSecondLayer()
{
    _m_pImpl->m_layers.resize(2, TRefCountingPtr<_TLayer>());
    _m_pImpl->m_layers[1]->m_numTiles = 0;
    _m_pImpl->m_layers[1]->m_blocks.resize(_m_pImpl->m_layers[0]->m_blocks.size(),
                                           TRefCountingPtr<_TBlock>());
}

VA(0x0043c572, 0x52)
void TGameMapMask::removeSecondLayer()
{
    _m_pImpl->m_layers.resize(1, TRefCountingPtr<_TLayer>());
}

VA(0x0043c5c4, 0x6a)
int TGameMapMask::getTileState(unsigned int x, unsigned int y, bool bSecondLayer) const
{
    const _TBlock& block =
        *_m_pImpl->m_layers[bSecondLayer]->m_blocks[y / kBlockSize * _m_pImpl->m_numBlockColumns + x / kBlockSize];
    return block.getState(x % kBlockSize, y % kBlockSize);
}
