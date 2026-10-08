// TerrainPlacement.cpp - Loki h3maped object 31: terrain placement. Painting
// a terrain type sets the cells of a rectangle; the operation's destructor
// then grows the painted area until every cell of that type is valid (no
// one-cell-wide strips, and for water and rock no cell touching its own
// terrain in two separate spans), recomputes every transition tile of the
// layer from its eight neighbours and reports the changed rectangle. The
// per-terrain traits pick the tile for a ground shape: the normal tilesets
// from a tile property table, rock from its pre-flipped tiles. Assert lines
// come from the retail immediates. The traits' flags, the tables, the
// ground shapes other than eGS_full and the transition types are named
// from their users; their names are not proven.
#include "editor/stdafx.h"

#include <assert.h>
#include <stdlib.h>
#include <limits>
#include <set>
#include <vector>

#include "editor/TerrainPlacement.h"
#include "editor/Clamp.h"
#include "editor/TilePoint.h"

#define ARRAY_SIZE( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

namespace {

const unsigned int kNumDirs = 8;

// The tile shapes of a terrain's tileset: full tiles, then the dirt and the
// sand transitions (corner, vertical and horizontal edge, inner corner and
// their diagonal variants), the two opposite corners and the mixed
// dirt/sand transitions.
enum TGroundShape {
    eGS_full = 0,
    eGS_unused = 1,
    eGS_dirtCorner = 2,
    eGS_dirtVertEdge = 3,
    eGS_dirtHorizEdge = 4,
    eGS_dirtInnerCorner = 5,
    eGS_dirtDiagCorner = 6,
    eGS_dirtDiagInnerCorner = 7,
    eGS_sandCorner = 8,
    eGS_sandVertEdge = 9,
    eGS_sandHorizEdge = 10,
    eGS_sandInnerCorner = 11,
    eGS_sandDiagCorner = 12,
    eGS_sandDiagInnerCorner = 13,
    eGS_dirtDirtCorners = 14,
    eGS_dirtSandCorners = 15,
    eGS_sandSandCorners = 16,
    eGS_mixed17 = 17,
    eGS_mixed18 = 18,
    eGS_mixed19 = 19,
    eGS_mixed20 = 20,
    eGS_mixed21 = 21,
    eGS_mixed22 = 22,
    eGS_mixed23 = 23,
    eGS_mixed24 = 24,
    eGS_mixed25 = 25,
    eGS_mixed26 = 26,
    eGS_mixed27 = 27,
    eGS_mixed28 = 28,
    kNumGroundShapes = 29
};

// The transition a neighbour of another terrain draws on a cell.
enum TTransType {
    eTT_none = 0,
    eTT_dirt = 1,
    eTT_sand = 2
};

struct TFlippedState {
    TFlippedState() {}
    TFlippedState(bool bHFlipped, bool bVFlipped) : m_bHFlipped(bHFlipped), m_bVFlipped(bVFlipped) {}

    TFlippedState operator!() const { return TFlippedState(!m_bHFlipped, !m_bVFlipped); }

    bool m_bHFlipped;
    bool m_bVFlipped;
};

class TTerrainTypeTraits {
public:
    virtual ~TTerrainTypeTraits() = 0;

    virtual bool hasSpecialTiles() const = 0;
    virtual bool isTileSpecial(unsigned int tileNum) const = 0;
    virtual TGroundShape getTileGroundShape(unsigned int tileNum) const = 0;
    virtual unsigned int pickFullTile(unsigned int specialFrequency,
                                      unsigned int requestedTileNum = numeric_limits<unsigned>::max()) const = 0;
    virtual unsigned int pickTransitionTile(TGroundShape groundShape, TFlippedState flippedState,
                                            TFlippedState* pActualFlippedState,
                                            unsigned int requestedTileNum = numeric_limits<unsigned>::max()) const = 0;

    // Neighbours of another terrain draw dirt transitions on it (sand
    // transitions where either side lacks this).
    bool m_bDirtTransitions;
    // Its tileset draws a cell that touches its own terrain in several
    // separate spans; water and rock have no such tiles.
    bool m_bSplitSpans;

protected:
    TTerrainTypeTraits(bool bDirtTransitions, bool bSplitSpans)
        : m_bDirtTransitions(bDirtTransitions), m_bSplitSpans(bSplitSpans) {}
};

TTerrainTypeTraits::~TTerrainTypeTraits()
{
}

class TNormalTerrainTypeTraits : public TTerrainTypeTraits {
public:
    struct TTileProps {
        TGroundShape m_groundShape;
        bool m_bSpecial;
    };

    TNormalTerrainTypeTraits(bool bDirtTransitions, bool bSplitSpans, unsigned int specialPercent,
                             unsigned int numTiles, const TTileProps* akTileProps);
    virtual ~TNormalTerrainTypeTraits() {}

    virtual bool hasSpecialTiles() const { return _m_aGroundShapeProps[eGS_full][true].m_numTiles != 0; }
    virtual bool isTileSpecial(unsigned int tileNum) const;
    virtual TGroundShape getTileGroundShape(unsigned int tileNum) const;
    virtual unsigned int pickFullTile(unsigned int specialFrequency,
                                      unsigned int requestedTileNum = numeric_limits<unsigned>::max()) const;
    virtual unsigned int pickTransitionTile(TGroundShape groundShape, TFlippedState flippedState,
                                            TFlippedState* pActualFlippedState,
                                            unsigned int requestedTileNum = numeric_limits<unsigned>::max()) const;

private:
    struct _TGroundShapeProps {
        _TGroundShapeProps() : m_firstTile(0), m_numTiles(0) {}

        unsigned int m_firstTile;
        unsigned int m_numTiles;
    };

    TGroundShape _getGroundShape(unsigned int tileNum) const
    {
#line 169
        assert(tileNum < _m_numTiles);
        return _m_akTileProps[tileNum].m_groundShape;
    }

    unsigned int _m_specialPercent;
    unsigned int _m_numTiles;
    const TTileProps* _m_akTileProps;
    _TGroundShapeProps _m_aGroundShapeProps[kNumGroundShapes][2];
};

TNormalTerrainTypeTraits::TNormalTerrainTypeTraits(bool bDirtTransitions, bool bSplitSpans,
                                                   unsigned int specialPercent, unsigned int numTiles,
                                                   const TTileProps* akTileProps)
    : TTerrainTypeTraits(bDirtTransitions, bSplitSpans), _m_specialPercent(specialPercent),
      _m_numTiles(numTiles), _m_akTileProps(akTileProps)
{
#line 184
    assert(_m_specialPercent <= 100);
    assert(_m_numTiles > 0);
    assert(_m_akTileProps != NULL);
    unsigned int tileNum = 0;
    TGroundShape groundShape = _m_akTileProps[0].m_groundShape;
    bool bSpecial = _m_akTileProps[0].m_bSpecial;
    for (;;) {
#line 196
        assert(groundShape >= 0 && groundShape < kNumGroundShapes);
        ++_m_aGroundShapeProps[groundShape][bSpecial].m_numTiles;
        if (++tileNum >= _m_numTiles)
            break;
        if (_m_akTileProps[tileNum].m_groundShape != groundShape
            || _m_akTileProps[tileNum].m_bSpecial != bSpecial) {
            groundShape = _m_akTileProps[tileNum].m_groundShape;
            bSpecial = _m_akTileProps[tileNum].m_bSpecial;
            _m_aGroundShapeProps[groundShape][bSpecial].m_firstTile = tileNum;
        }
    }
}

bool TNormalTerrainTypeTraits::isTileSpecial(unsigned int tileNum) const
{
#line 216
    assert(tileNum < _m_numTiles);
    return _m_akTileProps[tileNum].m_bSpecial;
}

TGroundShape TNormalTerrainTypeTraits::getTileGroundShape(unsigned int tileNum) const
{
#line 223
    assert(tileNum < _m_numTiles);
    return _m_akTileProps[tileNum].m_groundShape;
}

unsigned int TNormalTerrainTypeTraits::pickFullTile(unsigned int specialFrequency,
                                                    unsigned int requestedTileNum) const
{
#line 230
    assert(specialFrequency <= TTerrainPlacementOp::s_kMaxSpecialTileFrequency);
    assert(requestedTileNum == std::numeric_limits< unsigned >::max() || requestedTileNum < _m_numTiles);
    unsigned int tileNum;
    if (requestedTileNum == numeric_limits<unsigned>::max()
        || _getGroundShape(requestedTileNum) != eGS_full) {
        const _TGroundShapeProps* pShapeProps;
        if (_m_aGroundShapeProps[eGS_full][true].m_numTiles != 0) {
            unsigned int specialPercent =
                _m_specialPercent * specialFrequency / TTerrainPlacementOp::s_kMaxSpecialTileFrequency;
            if (rand() % 100 < specialPercent)
                pShapeProps = &_m_aGroundShapeProps[eGS_full][true];
            else
                pShapeProps = &_m_aGroundShapeProps[eGS_full][false];
        } else
            pShapeProps = &_m_aGroundShapeProps[eGS_full][false];
#line 250
        assert(pShapeProps->m_numTiles != 0);
        tileNum = pShapeProps->m_firstTile + rand() % pShapeProps->m_numTiles;
    } else
        tileNum = requestedTileNum;
    return tileNum;
}

unsigned int TNormalTerrainTypeTraits::pickTransitionTile(TGroundShape groundShape,
                                                          TFlippedState flippedState,
                                                          TFlippedState* pActualFlippedState,
                                                          unsigned int requestedTileNum) const
{
#line 266
    assert(groundShape >= 0 && groundShape < kNumGroundShapes);
    assert(groundShape != eGS_full);
    assert(pActualFlippedState != NULL);
    assert(requestedTileNum == std::numeric_limits< unsigned >::max() || requestedTileNum < _m_numTiles);
    unsigned int tileNum;
    if (requestedTileNum == numeric_limits<unsigned>::max()
        || _getGroundShape(requestedTileNum) != groundShape) {
        const _TGroundShapeProps& shapeProps = _m_aGroundShapeProps[groundShape][false];
#line 278
        assert(shapeProps.m_numTiles != 0);
        tileNum = shapeProps.m_firstTile + rand() % shapeProps.m_numTiles;
    } else
        tileNum = requestedTileNum;
    *pActualFlippedState = flippedState;
    return tileNum;
}

// Rock's tileset holds its transitions pre-flipped: a tile is picked by
// ground shape and flipped state and is never flipped itself.
class TRockTraits : public TTerrainTypeTraits {
public:
    TRockTraits();
    virtual ~TRockTraits() {}

    virtual bool hasSpecialTiles() const { return false; }
    virtual bool isTileSpecial(unsigned int tileNum) const;
    virtual TGroundShape getTileGroundShape(unsigned int tileNum) const;
    virtual unsigned int pickFullTile(unsigned int specialFrequency,
                                      unsigned int requestedTileNum = numeric_limits<unsigned>::max()) const;
    virtual unsigned int pickTransitionTile(TGroundShape groundShape, TFlippedState flippedState,
                                            TFlippedState* pActualFlippedState,
                                            unsigned int requestedTileNum = numeric_limits<unsigned>::max()) const;

private:
    struct _TTileProps {
        TGroundShape m_groundShape;
        bool m_bHFlipped;
        bool m_bVFlipped;
    };

    struct _TTileSubsetProps {
        _TTileSubsetProps() : m_firstTile(0), m_numTiles(0) {}

        unsigned int m_firstTile;
        unsigned int m_numTiles;
    };

    class _TGroundShapeMap {
    public:
        _TGroundShapeMap();

        inline const _TTileSubsetProps& getSubsetProps(TGroundShape groundShape,
                                                       TFlippedState flippedState) const;

    private:
        _TTileSubsetProps _m_aSubsetProps[kNumGroundShapes][2][2];
    };

    static const _TTileProps _s_akTileProps[48];
    static const _TGroundShapeMap _s_kGroundShapeMap;
};

const TRockTraits::_TTileProps TRockTraits::_s_akTileProps[48] = {
    { eGS_full, false, false }, { eGS_full, false, false },
    { eGS_full, false, false }, { eGS_full, false, false },
    { eGS_full, false, false }, { eGS_full, false, false },
    { eGS_full, false, false }, { eGS_full, false, false },
    { eGS_sandCorner, false, false }, { eGS_sandCorner, false, false },
    { eGS_sandCorner, true, false }, { eGS_sandCorner, true, false },
    { eGS_sandCorner, false, true }, { eGS_sandCorner, false, true },
    { eGS_sandCorner, true, true }, { eGS_sandCorner, true, true },
    { eGS_sandVertEdge, false, false }, { eGS_sandVertEdge, false, false },
    { eGS_sandVertEdge, true, false }, { eGS_sandVertEdge, true, false },
    { eGS_sandHorizEdge, false, false }, { eGS_sandHorizEdge, false, false },
    { eGS_sandHorizEdge, false, true }, { eGS_sandHorizEdge, false, true },
    { eGS_sandInnerCorner, false, false }, { eGS_sandInnerCorner, false, false },
    { eGS_sandInnerCorner, true, false }, { eGS_sandInnerCorner, true, false },
    { eGS_sandInnerCorner, false, true }, { eGS_sandInnerCorner, false, true },
    { eGS_sandInnerCorner, true, true }, { eGS_sandInnerCorner, true, true },
    { eGS_sandDiagCorner, false, false }, { eGS_sandDiagCorner, false, false },
    { eGS_sandDiagCorner, true, false }, { eGS_sandDiagCorner, true, false },
    { eGS_sandDiagCorner, false, true }, { eGS_sandDiagCorner, false, true },
    { eGS_sandDiagCorner, true, true }, { eGS_sandDiagCorner, true, true },
    { eGS_sandDiagInnerCorner, false, false }, { eGS_sandDiagInnerCorner, false, false },
    { eGS_sandDiagInnerCorner, true, false }, { eGS_sandDiagInnerCorner, true, false },
    { eGS_sandDiagInnerCorner, false, true }, { eGS_sandDiagInnerCorner, false, true },
    { eGS_sandDiagInnerCorner, true, true }, { eGS_sandDiagInnerCorner, true, true }
};

TRockTraits::_TGroundShapeMap::_TGroundShapeMap()
{
    unsigned int tileNum = 0;
    TGroundShape groundShape = _s_akTileProps[0].m_groundShape;
    bool bHFlipped = _s_akTileProps[0].m_bHFlipped;
    bool bVFlipped = _s_akTileProps[0].m_bVFlipped;
    for (;;) {
#line 411
        assert(groundShape >= 0 && groundShape < kNumGroundShapes);
        ++_m_aSubsetProps[groundShape][bHFlipped][bVFlipped].m_numTiles;
        if (++tileNum >= ARRAY_SIZE(_s_akTileProps))
            break;
        if (_s_akTileProps[tileNum].m_groundShape != groundShape
            || _s_akTileProps[tileNum].m_bHFlipped != bHFlipped
            || _s_akTileProps[tileNum].m_bVFlipped != bVFlipped) {
            groundShape = _s_akTileProps[tileNum].m_groundShape;
            bHFlipped = _s_akTileProps[tileNum].m_bHFlipped;
            bVFlipped = _s_akTileProps[tileNum].m_bVFlipped;
            _m_aSubsetProps[groundShape][bHFlipped][bVFlipped].m_firstTile = tileNum;
        }
    }
}

inline const TRockTraits::_TTileSubsetProps&
TRockTraits::_TGroundShapeMap::getSubsetProps(TGroundShape groundShape, TFlippedState flippedState) const
{
#line 433
    assert(groundShape >= 0 && groundShape < kNumGroundShapes);
    return _m_aSubsetProps[groundShape][flippedState.m_bHFlipped][flippedState.m_bVFlipped];
}

const TRockTraits::_TGroundShapeMap TRockTraits::_s_kGroundShapeMap;

TRockTraits::TRockTraits()
    : TTerrainTypeTraits(false, false)
{
}

bool TRockTraits::isTileSpecial(unsigned int tileNum) const
{
#line 448
    assert(tileNum < ARRAY_SIZE(_s_akTileProps));
    return false;
}

TGroundShape TRockTraits::getTileGroundShape(unsigned int tileNum) const
{
#line 455
    assert(tileNum < ARRAY_SIZE(_s_akTileProps));
    return _s_akTileProps[tileNum].m_groundShape;
}

unsigned int TRockTraits::pickFullTile(unsigned int specialFrequency, unsigned int requestedTileNum) const
{
#line 462
    assert(requestedTileNum == std::numeric_limits< unsigned >::max() || requestedTileNum < ARRAY_SIZE(_s_akTileProps));
    unsigned int tileNum;
    if (requestedTileNum == numeric_limits<unsigned>::max()
        || _s_akTileProps[requestedTileNum].m_groundShape != eGS_full) {
        const _TTileSubsetProps& subsetProps =
            _s_kGroundShapeMap.getSubsetProps(eGS_full, TFlippedState(false, false));
#line 470
        assert(subsetProps.m_numTiles > 0);
        tileNum = subsetProps.m_firstTile + rand() % subsetProps.m_numTiles;
    } else
        tileNum = requestedTileNum;
    return tileNum;
}

unsigned int TRockTraits::pickTransitionTile(TGroundShape groundShape, TFlippedState flippedState,
                                             TFlippedState* pActualFlippedState,
                                             unsigned int requestedTileNum) const
{
#line 486
    assert(groundShape >= 0 && groundShape < kNumGroundShapes);
    assert(groundShape != eGS_full);
    assert(pActualFlippedState != NULL);
    assert(requestedTileNum == std::numeric_limits< unsigned >::max() || requestedTileNum < ARRAY_SIZE(_s_akTileProps));
    unsigned int tileNum;
    if (requestedTileNum == numeric_limits<unsigned>::max()
        || _s_akTileProps[requestedTileNum].m_groundShape != groundShape
        || _s_akTileProps[requestedTileNum].m_bHFlipped != flippedState.m_bHFlipped
        || _s_akTileProps[requestedTileNum].m_bVFlipped != flippedState.m_bVFlipped) {
        const _TTileSubsetProps& subsetProps = _s_kGroundShapeMap.getSubsetProps(groundShape, flippedState);
#line 499
        assert(subsetProps.m_numTiles > 0);
        tileNum = subsetProps.m_firstTile + rand() % subsetProps.m_numTiles;
    } else
        tileNum = requestedTileNum;
    *pActualFlippedState = TFlippedState(false, false);
    return tileNum;
}

// The normal tilesets: grass, snow, swamp, rough, subterranean and lava
// share one layout; dirt, sand and water have their own.
const TNormalTerrainTypeTraits::TTileProps akNormalTileProps[] = {
    { eGS_dirtCorner, false }, { eGS_dirtCorner, false }, { eGS_dirtCorner, false }, { eGS_dirtCorner, false },
    { eGS_dirtVertEdge, false }, { eGS_dirtVertEdge, false }, { eGS_dirtVertEdge, false }, { eGS_dirtVertEdge, false },
    { eGS_dirtHorizEdge, false }, { eGS_dirtHorizEdge, false }, { eGS_dirtHorizEdge, false }, { eGS_dirtHorizEdge, false },
    { eGS_dirtInnerCorner, false }, { eGS_dirtInnerCorner, false }, { eGS_dirtInnerCorner, false }, { eGS_dirtInnerCorner, false },
    { eGS_dirtDiagCorner, false }, { eGS_dirtDiagCorner, false },
    { eGS_dirtDiagInnerCorner, false }, { eGS_dirtDiagInnerCorner, false },
    { eGS_sandCorner, false }, { eGS_sandCorner, false }, { eGS_sandCorner, false }, { eGS_sandCorner, false },
    { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false },
    { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false },
    { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false },
    { eGS_sandDiagCorner, false }, { eGS_sandDiagCorner, false },
    { eGS_sandDiagInnerCorner, false }, { eGS_sandDiagInnerCorner, false },
    { eGS_dirtDirtCorners, false }, { eGS_dirtSandCorners, false }, { eGS_sandSandCorners, false },
    { eGS_mixed17, false }, { eGS_mixed18, false }, { eGS_mixed19, false }, { eGS_mixed20, false },
    { eGS_mixed21, false }, { eGS_mixed22, false },
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false },
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_mixed23, false }, { eGS_mixed24, false }, { eGS_mixed25, false }, { eGS_mixed26, false },
    { eGS_mixed28, false }, { eGS_mixed27, false }
};

const TNormalTerrainTypeTraits::TTileProps akDirtTileProps[] = {
    { eGS_sandCorner, false }, { eGS_sandCorner, false }, { eGS_sandCorner, false }, { eGS_sandCorner, false },
    { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false },
    { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false },
    { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false },
    { eGS_sandDiagCorner, false }, { eGS_sandDiagCorner, false },
    { eGS_sandDiagInnerCorner, false }, { eGS_sandDiagInnerCorner, false },
    { eGS_sandSandCorners, false },
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false },
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_mixed24, false }
};

const TNormalTerrainTypeTraits::TTileProps akSandTileProps[] = {
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false },
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true },
    { eGS_full, true }, { eGS_full, true }, { eGS_full, true }, { eGS_full, true }
};

const TNormalTerrainTypeTraits::TTileProps akWaterTileProps[] = {
    { eGS_sandCorner, false }, { eGS_sandCorner, false }, { eGS_sandCorner, false }, { eGS_sandCorner, false },
    { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false }, { eGS_sandVertEdge, false },
    { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false }, { eGS_sandHorizEdge, false },
    { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false }, { eGS_sandInnerCorner, false },
    { eGS_sandDiagCorner, false }, { eGS_sandDiagCorner, false },
    { eGS_sandDiagInnerCorner, false }, { eGS_sandDiagInnerCorner, false },
    { eGS_sandSandCorners, false },
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false },
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false },
    { eGS_full, false }, { eGS_full, false }, { eGS_full, false }, { eGS_full, false }
};

TNormalTerrainTypeTraits kDirtTraits(true, true, 50, ARRAY_SIZE(akDirtTileProps), akDirtTileProps);
TNormalTerrainTypeTraits kSandTraits(false, true, 70, ARRAY_SIZE(akSandTileProps), akSandTileProps);
TNormalTerrainTypeTraits kGrassTraits(true, true, 50, ARRAY_SIZE(akNormalTileProps), akNormalTileProps);
TNormalTerrainTypeTraits kSnowTraits(true, true, 80, ARRAY_SIZE(akNormalTileProps), akNormalTileProps);
TNormalTerrainTypeTraits kSwampTraits(true, true, 80, ARRAY_SIZE(akNormalTileProps), akNormalTileProps);
TNormalTerrainTypeTraits kRoughTraits(true, true, 80, ARRAY_SIZE(akNormalTileProps), akNormalTileProps);
TNormalTerrainTypeTraits kSubterraneanTraits(true, true, 60, ARRAY_SIZE(akNormalTileProps), akNormalTileProps);
TNormalTerrainTypeTraits kLavaTraits(true, true, 80, ARRAY_SIZE(akNormalTileProps), akNormalTileProps);
TNormalTerrainTypeTraits kWaterTraits(false, false, 0, ARRAY_SIZE(akWaterTileProps), akWaterTileProps);
TRockTraits kRockTraits;

TTerrainTypeTraits* const akTerrainTypeTraits[kNumTerrainTypes] = {
    &kDirtTraits, &kSandTraits, &kGrassTraits, &kSnowTraits, &kSwampTraits,
    &kRoughTraits, &kSubterraneanTraits, &kLavaTraits, &kWaterTraits, &kRockTraits
};

// Each neighbour direction (clockwise from north) as it lands after
// flipping a tile horizontally and/or vertically.
const unsigned int akFlippedDir[2][2][kNumDirs] = {
    { { 0, 1, 2, 3, 4, 5, 6, 7 }, { 4, 3, 2, 1, 0, 7, 6, 5 } },
    { { 0, 7, 6, 5, 4, 3, 2, 1 }, { 4, 5, 6, 7, 0, 1, 2, 3 } }
};

// A cell of terrainType at (x, y) would sit between two cells of other
// terrains horizontally (vertically).
bool wouldBeInvalidHoriz(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y,
                         TTerrainType terrainType)
{
    return x > 0 && x < pMapLayer->getWidth() - 1
        && pMapLayer->getCell(x - 1, y).getTerrainType() != terrainType
        && pMapLayer->getCell(x + 1, y).getTerrainType() != terrainType;
}

bool wouldBeInvalidVert(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y,
                        TTerrainType terrainType)
{
    return y > 0 && y < pMapLayer->getHeight() - 1
        && pMapLayer->getCell(x, y - 1).getTerrainType() != terrainType
        && pMapLayer->getCell(x, y + 1).getTerrainType() != terrainType;
}

inline bool isInvalidHoriz(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y)
{
    return wouldBeInvalidHoriz(pMapLayer, x, y, pMapLayer->getCell(x, y).getTerrainType());
}

inline bool isInvalidVert(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y)
{
    return wouldBeInvalidVert(pMapLayer, x, y, pMapLayer->getCell(x, y).getTerrainType());
}

// Which neighbours share the cell's terrain; a diagonal neighbour counts
// only when one of the two cells beside it does.
void computeAdjacentSimilar(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y,
                            bool (&abAdjSimilar)[kNumDirs])
{
#line 787
    assert(x < pMapLayer->getWidth());
    assert(y < pMapLayer->getHeight());
    TTerrainType terrainType = pMapLayer->getCell(x, y).getTerrainType();
    unsigned int top = y > 0 ? y - 1 : y;
    unsigned int bottom = y < pMapLayer->getHeight() - 1 ? y + 1 : y;
    unsigned int left = x > 0 ? x - 1 : x;
    unsigned int right = x < pMapLayer->getWidth() - 1 ? x + 1 : x;
    abAdjSimilar[0] = pMapLayer->getCell(x, top).getTerrainType() == terrainType;
    abAdjSimilar[4] = pMapLayer->getCell(x, bottom).getTerrainType() == terrainType;
    abAdjSimilar[6] = pMapLayer->getCell(left, y).getTerrainType() == terrainType;
    abAdjSimilar[2] = pMapLayer->getCell(right, y).getTerrainType() == terrainType;
    abAdjSimilar[7] = (abAdjSimilar[0] || abAdjSimilar[6])
        && pMapLayer->getCell(left, top).getTerrainType() == terrainType;
    abAdjSimilar[1] = (abAdjSimilar[0] || abAdjSimilar[2])
        && pMapLayer->getCell(right, top).getTerrainType() == terrainType;
    abAdjSimilar[5] = (abAdjSimilar[4] || abAdjSimilar[6])
        && pMapLayer->getCell(left, bottom).getTerrainType() == terrainType;
    abAdjSimilar[3] = (abAdjSimilar[4] || abAdjSimilar[2])
        && pMapLayer->getCell(right, bottom).getTerrainType() == terrainType;
}

// The cell touches its own terrain in more than one separate span.
bool hasMultipleAdjSimilarSpans(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y)
{
    bool abAdjSimilar[kNumDirs];
    computeAdjacentSimilar(pMapLayer, x, y, abAdjSimilar);
    unsigned int startDir = 0;
    while (abAdjSimilar[startDir])
        if ((startDir = (startDir + 1) % kNumDirs) == 0)
            return false;
    unsigned int dir = startDir;
    do
        if ((dir = (dir + 1) % kNumDirs) == startDir)
            return false;
    while (!abAdjSimilar[dir]);
    do
        if ((dir = (dir + 1) % kNumDirs) == startDir)
            return false;
    while (abAdjSimilar[dir]);
    do
        if (abAdjSimilar[dir])
            return true;
    while ((dir = (dir + 1) % kNumDirs) != startDir);
    return false;
}

inline bool isInvalid(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y)
{
    if (isInvalidHoriz(pMapLayer, x, y) || isInvalidVert(pMapLayer, x, y))
        return true;
    if (akTerrainTypeTraits[pMapLayer->getCell(x, y).getTerrainType()]->m_bSplitSpans)
        return false;
    return hasMultipleAdjSimilarSpans(pMapLayer, x, y);
}

inline void computeAdjacentDirs(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y,
                                bool (&abDir)[kNumDirs])
{
    ::computeAdjacentDirs(pMapLayer->getWidth(), pMapLayer->getHeight(), x, y, abDir);
}

TTransType getTransType(TTerrainType terr1, TTerrainType terr2)
{
#line 900
    assert(terr1 >= 0 && terr1 < kNumTerrainTypes);
    assert(terr2 >= 0 && terr2 < kNumTerrainTypes);
    if (terr1 == terr2 || terr1 == eTerrainSand)
        return eTT_none;
    if (!akTerrainTypeTraits[terr1]->m_bDirtTransitions || !akTerrainTypeTraits[terr2]->m_bDirtTransitions)
        return eTT_sand;
    return terr1 != eTerrainDirt ? eTT_dirt : eTT_none;
}

void getTransitions(const TGameMap::TLayer* pMapLayer, const TTilePoint& loc,
                    TTransType (&aTransType)[kNumDirs])
{
    TTerrainType terrainType = pMapLayer->getCell(loc).getTerrainType();
    unsigned int top = loc.y() > 0 ? loc.y() - 1 : loc.y();
    unsigned int bottom = loc.y() < pMapLayer->getHeight() - 1 ? loc.y() + 1 : loc.y();
    unsigned int left = loc.x() > 0 ? loc.x() - 1 : loc.x();
    unsigned int right = loc.x() < pMapLayer->getWidth() - 1 ? loc.x() + 1 : loc.x();
    aTransType[0] = getTransType(terrainType, pMapLayer->getCell(loc.x(), top).getTerrainType());
    aTransType[4] = getTransType(terrainType, pMapLayer->getCell(loc.x(), bottom).getTerrainType());
    aTransType[6] = getTransType(terrainType, pMapLayer->getCell(left, loc.y()).getTerrainType());
    aTransType[2] = getTransType(terrainType, pMapLayer->getCell(right, loc.y()).getTerrainType());
    aTransType[7] = getTransType(terrainType, pMapLayer->getCell(left, top).getTerrainType());
    aTransType[1] = getTransType(terrainType, pMapLayer->getCell(right, top).getTerrainType());
    aTransType[5] = getTransType(terrainType, pMapLayer->getCell(left, bottom).getTerrainType());
    aTransType[3] = getTransType(terrainType, pMapLayer->getCell(right, bottom).getTerrainType());
}

// The ground shape and flip that draw a cell's transitions, trying the four
// flips of each pattern in turn (directions in the unflipped tile's frame).
void computeGroundShape(const TTransType (&akTransType)[kNumDirs], TGroundShape* pGroundShape,
                        TFlippedState* pFlippedState)
{
    static const TFlippedState akFlippedState[4] = {
        TFlippedState(false, false), TFlippedState(false, true),
        TFlippedState(true, false), TFlippedState(true, true)
    };
    unsigned int flip;
    const unsigned int* aDir;
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[2]] == eTT_dirt && akTransType[aDir[4]] == eTT_dirt) {
            if (akTransType[aDir[1]] == eTT_sand && akTransType[aDir[5]] == eTT_sand) {
                *pGroundShape = eGS_mixed28;
                *pFlippedState = akFlippedState[flip];
                return;
            } else if (akTransType[aDir[3]] == eTT_sand) {
                *pGroundShape = eGS_mixed27;
                *pFlippedState = akFlippedState[flip];
                return;
            }
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[0]] == eTT_dirt && akTransType[aDir[6]] == eTT_dirt) {
            if (akTransType[aDir[3]] != eTT_none) {
                *pGroundShape = akTransType[aDir[3]] == eTT_dirt ? eGS_mixed23 : eGS_mixed25;
                *pFlippedState = akFlippedState[flip];
                return;
            }
        } else if (akTransType[aDir[0]] == eTT_sand && akTransType[aDir[6]] == eTT_sand) {
            if (akTransType[aDir[3]] != eTT_none) {
                *pGroundShape = akTransType[aDir[3]] == eTT_sand ? eGS_mixed24 : eGS_mixed26;
                *pFlippedState = akFlippedState[flip];
                return;
            }
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[2]] == eTT_sand && akTransType[aDir[4]] == eTT_dirt) {
            if (akTransType[aDir[5]] != eTT_sand) {
                *pGroundShape = eGS_mixed21;
                *pFlippedState = akFlippedState[flip];
            } else {
                *pGroundShape = eGS_sandCorner;
                *pFlippedState = !akFlippedState[flip];
            }
            return;
        } else if (akTransType[aDir[2]] == eTT_dirt && akTransType[aDir[4]] == eTT_sand) {
            if (akTransType[aDir[1]] != eTT_sand) {
                *pGroundShape = eGS_mixed22;
                *pFlippedState = akFlippedState[flip];
            } else {
                *pGroundShape = eGS_sandCorner;
                *pFlippedState = !akFlippedState[flip];
            }
            return;
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[2]] == eTT_dirt && akTransType[aDir[4]] == eTT_dirt) {
            *pFlippedState = akFlippedState[flip];
            if (akTransType[aDir[5]] == eTT_sand) {
                *pGroundShape = eGS_mixed17;
                return;
            } else if (akTransType[aDir[1]] == eTT_sand) {
                *pGroundShape = eGS_mixed18;
                return;
            }
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[0]] == eTT_dirt && akTransType[aDir[6]] == eTT_dirt) {
            *pGroundShape = eGS_dirtCorner;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[0]] == eTT_sand && akTransType[aDir[6]] == eTT_sand) {
            *pGroundShape = eGS_sandCorner;
            *pFlippedState = akFlippedState[flip];
            return;
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[2]] == eTT_dirt && akTransType[aDir[5]] == eTT_sand) {
            *pGroundShape = eGS_mixed17;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[4]] == eTT_dirt && akTransType[aDir[1]] == eTT_sand) {
            *pGroundShape = eGS_mixed18;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[2]] == eTT_sand && akTransType[aDir[5]] == eTT_dirt) {
            *pGroundShape = eGS_mixed21;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[4]] == eTT_sand && akTransType[aDir[1]] == eTT_dirt) {
            *pGroundShape = eGS_mixed22;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[6]] == eTT_dirt && akTransType[aDir[1]] == eTT_dirt
            || akTransType[aDir[0]] == eTT_dirt && akTransType[aDir[5]] == eTT_dirt) {
            *pGroundShape = eGS_dirtCorner;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[6]] == eTT_sand && akTransType[aDir[1]] == eTT_sand
            || akTransType[aDir[0]] == eTT_sand && akTransType[aDir[5]] == eTT_sand) {
            *pGroundShape = eGS_sandCorner;
            *pFlippedState = akFlippedState[flip];
            return;
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[2]] == eTT_dirt && akTransType[aDir[3]] == eTT_sand) {
            *pGroundShape = eGS_mixed19;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[4]] == eTT_dirt && akTransType[aDir[3]] == eTT_sand) {
            *pGroundShape = eGS_mixed20;
            *pFlippedState = akFlippedState[flip];
            return;
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[0]] == eTT_dirt) {
            *pGroundShape = eGS_dirtHorizEdge;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[0]] == eTT_sand) {
            *pGroundShape = eGS_sandHorizEdge;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[6]] == eTT_dirt) {
            *pGroundShape = eGS_dirtVertEdge;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[6]] == eTT_sand) {
            *pGroundShape = eGS_sandVertEdge;
            *pFlippedState = akFlippedState[flip];
            return;
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[7]] == eTT_dirt && akTransType[aDir[3]] == eTT_dirt) {
            *pGroundShape = eGS_dirtDirtCorners;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[7]] == eTT_dirt && akTransType[aDir[3]] == eTT_sand) {
            *pGroundShape = eGS_dirtSandCorners;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[7]] == eTT_sand && akTransType[aDir[3]] == eTT_sand) {
            *pGroundShape = eGS_sandSandCorners;
            *pFlippedState = akFlippedState[flip];
            return;
        }
    }
    for (flip = 0; flip < 4; ++flip) {
        aDir = akFlippedDir[akFlippedState[flip].m_bHFlipped][akFlippedState[flip].m_bVFlipped];
        if (akTransType[aDir[3]] == eTT_dirt) {
            *pGroundShape = eGS_dirtInnerCorner;
            *pFlippedState = akFlippedState[flip];
            return;
        } else if (akTransType[aDir[3]] == eTT_sand) {
            *pGroundShape = eGS_sandInnerCorner;
            *pFlippedState = akFlippedState[flip];
            return;
        }
    }
    *pGroundShape = eGS_full;
    *pFlippedState = TFlippedState(false, false);
}

// A corner tile becomes its diagonal variant when the cell continues
// diagonally past the corner (outside) or two cells into it (inside).
bool isOutsideDiag(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y,
                   TFlippedState flippedState)
{
#line 1240
    assert(pMapLayer != NULL);
    assert(x >= 0 && x < pMapLayer->getWidth());
    assert(y >= 0 && y < pMapLayer->getHeight());
    static TPoint<int> akOffset[4][2] = {
        { TPoint<int>(-1, 1), TPoint<int>(1, -1) },
        { TPoint<int>(1, 1), TPoint<int>(-1, -1) },
        { TPoint<int>(-1, -1), TPoint<int>(1, 1) },
        { TPoint<int>(1, -1), TPoint<int>(-1, 1) }
    };
    TTerrainType terrainType = pMapLayer->getCell(x, y).getTerrainType();
    unsigned int flip = flippedState.m_bVFlipped * 2 | flippedState.m_bHFlipped;
    const TPoint<int>* const aOffset = akOffset[flip];
    TTilePoint testPt(clamp<int>(0, x + aOffset[0].x(), pMapLayer->getWidth() - 1),
                      clamp<int>(0, y + aOffset[0].y(), pMapLayer->getHeight() - 1));
    if (pMapLayer->getCell(testPt).getTerrainType() == terrainType)
        return true;
    testPt.x(clamp<int>(0, x + aOffset[1].x(), pMapLayer->getWidth() - 1));
    testPt.y(clamp<int>(0, y + aOffset[1].y(), pMapLayer->getHeight() - 1));
    return pMapLayer->getCell(testPt).getTerrainType() == terrainType;
}

bool isInsideDiag(const TGameMap::TLayer* pMapLayer, unsigned int x, unsigned int y,
                  TFlippedState flippedState)
{
#line 1270
    assert(pMapLayer != NULL);
    assert(x >= 0 && x < pMapLayer->getWidth());
    assert(y >= 0 && y < pMapLayer->getHeight());
    static const TPoint<int> akOffset[4] = {
        TPoint<int>(2, 2), TPoint<int>(-2, 2), TPoint<int>(2, -2), TPoint<int>(-2, -2)
    };
    TTerrainType terrainType = pMapLayer->getCell(x, y).getTerrainType();
    unsigned int flip = flippedState.m_bVFlipped * 2 | flippedState.m_bHFlipped;
    const TPoint<int>& offset = akOffset[flip];
    TTilePoint testPt(clamp<int>(0, x + offset.x(), pMapLayer->getWidth() - 1), y);
    if (pMapLayer->getCell(testPt).getTerrainType() != terrainType)
        return true;
    testPt.x(x);
    testPt.y(clamp<int>(0, y + offset.y(), pMapLayer->getHeight() - 1));
    return pMapLayer->getCell(testPt).getTerrainType() != terrainType;
}

}  // namespace

TTerrainPlacementOp::TTerrainPlacementOp(TTerrainPlacementOpClient* pClient, TGameMap* pMap,
                                         bool bSecondLayer, TTerrainType terrainType,
                                         unsigned int specialTileFrequency)
    : _m_pClient(pClient), _m_pMap(pMap), _m_bSecondLayer(bSecondLayer), _m_terrainType(terrainType),
      _m_specialTileFrequency(specialTileFrequency)
{
#line 1574
    assert(pClient != NULL);
    assert(pMap != NULL);
    assert(!bSecondLayer || pMap->isTwoLayer());
    assert(terrainType >= 0 && terrainType < kNumTerrainTypes);
    assert(specialTileFrequency <= s_kMaxSpecialTileFrequency);
}

TTerrainPlacementOp::~TTerrainPlacementOp()
{
    TTilePoint topLeft(_getPMapLayer()->getWidth(), _getPMapLayer()->getHeight());
    TTilePoint bottomRight(0, 0);
    do {
        while (!_m_invalidSet.empty()) {
            TTilePoint invalidPt = *_m_invalidSet.begin();
#line 1592
            assert(isInvalid( _getPMapLayer(), invalidPt.x(), invalidPt.y() ));
            _validateTile(invalidPt.x(), invalidPt.y(), &topLeft, &bottomRight);
        }
        while (!_m_testSet.empty()) {
            TTilePoint testPt = *_m_testSet.begin();
            _m_testSet.erase(testPt);
#line 1602
            assert(_getPMapLayer()->getCell( testPt.x(), testPt.y() ).getTerrainType() != _m_terrainType);
            if (isInvalid(_getPMapLayer(), testPt.x(), testPt.y()))
                _placeTile(testPt.x(), testPt.y(), &topLeft, &bottomRight);
        }
    } while (!_m_invalidSet.empty());
    _processTransitions(&topLeft, &bottomRight);
    if (bottomRight.x() > topLeft.x()) {
#line 1617
        assert(bottomRight.y() > topLeft.y());
        TTilePoint size = bottomRight - topLeft;
        _m_pClient->onTerrainUpdated(_m_bSecondLayer, topLeft.x(), topLeft.y(), size.x(), size.y());
    }
}

void TTerrainPlacementOp::operator()(unsigned int left, unsigned int top, unsigned int width,
                                     unsigned int height)
{
#line 1626
    assert(left < _getPMapLayer()->getWidth());
    assert(top < _getPMapLayer()->getHeight());
    unsigned int right = left + width;
    unsigned int bottom = top + height;
#line 1632
    assert(right <= _getPMapLayer()->getWidth());
    assert(bottom <= _getPMapLayer()->getHeight());
    for (unsigned int y = top; y < bottom; ++y)
        for (unsigned int x = left; x < right; ++x)
            if (_m_terrainType != _getPMapLayer()->getCell(x, y).getTerrainType())
                _placeTile(x, y);
            else {
                TGameMap::TLayer::TCell* pCell = _getPMapLayer()->getPCell(x, y);
                unsigned int tileNum = _pickFullTile(x, y, _m_terrainType);
                pCell->setTileNum(tileNum);
                pCell->setBHFlipped(false);
                pCell->setBVFlipped(false);
            }
    _m_pClient->onTerrainUpdated(_m_bSecondLayer, left, top, width, height);
}

void TTerrainPlacementOp::_placeTile(unsigned int x, unsigned int y)
{
    TGameMap::TLayer::TCell* pCell = _getPMapLayer()->getPCell(x, y);
    unsigned int tileNum = _pickFullTile(x, y, _m_terrainType);
    TTerrainType oldTerrainType = pCell->getTerrainType();
    pCell->setTerrainType(_m_terrainType);
    pCell->setTileNum(tileNum);
    pCell->setBHFlipped(false);
    pCell->setBVFlipped(false);
    _m_pClient->onTerrainTypeChanged(_m_bSecondLayer, x, y, oldTerrainType);
    if (_m_testSet.find(TTilePoint(x, y)) != _m_testSet.end())
        _m_testSet.erase(TTilePoint(x, y));
    const TGameMap::TLayer* pMapLayer = _getPMapLayer();
    if (akTerrainTypeTraits[_m_terrainType]->m_bSplitSpans) {
        if (y > 0 && _m_invalidSet.find(TTilePoint(x, y - 1)) != _m_invalidSet.end()
            && !isInvalidHoriz(pMapLayer, x, y - 1)) {
            _m_invalidSet.erase(TTilePoint(x, y - 1));
            _validTilePlaced(x, y - 1);
        }
        if (y < pMapLayer->getHeight() - 1 && _m_invalidSet.find(TTilePoint(x, y + 1)) != _m_invalidSet.end()
            && !isInvalidHoriz(pMapLayer, x, y + 1)) {
            _m_invalidSet.erase(TTilePoint(x, y + 1));
            _validTilePlaced(x, y + 1);
        }
        if (x > 0 && _m_invalidSet.find(TTilePoint(x - 1, y)) != _m_invalidSet.end()
            && !isInvalidVert(pMapLayer, x - 1, y)) {
            _m_invalidSet.erase(TTilePoint(x - 1, y));
            _validTilePlaced(x - 1, y);
        }
        if (x < pMapLayer->getWidth() - 1 && _m_invalidSet.find(TTilePoint(x + 1, y)) != _m_invalidSet.end()
            && !isInvalidVert(pMapLayer, x + 1, y)) {
            _m_invalidSet.erase(TTilePoint(x + 1, y));
            _validTilePlaced(x + 1, y);
        }
    } else {
        bool abAdjDir[kNumDirs];
        computeAdjacentDirs(pMapLayer, x, y, abAdjDir);
        for (unsigned int dir = 0; dir < kNumDirs; ++dir) {
            if (!abAdjDir[dir])
                continue;
            TTilePoint adjPt = TPoint<int>((int)x, (int)y) + akAdjOffset[dir];
            if (pMapLayer->getCell(adjPt).getTerrainType() == _m_terrainType)
                if (_m_invalidSet.find(adjPt) != _m_invalidSet.end()) {
                    if (!isInvalid(pMapLayer, adjPt.x(), adjPt.y())) {
                        _m_invalidSet.erase(adjPt);
                        _validTilePlaced(adjPt.x(), adjPt.y());
                    }
                } else if (isInvalid(pMapLayer, adjPt.x(), adjPt.y()))
                    _m_invalidSet.insert(adjPt);
        }
    }
    if (isInvalid(pMapLayer, x, y)) {
#line 1743
        assert(_m_invalidSet.find( TTilePoint( x, y ) ) == _m_invalidSet.end());
        _m_invalidSet.insert(TTilePoint(x, y));
    } else
        _validTilePlaced(x, y);
}

void TTerrainPlacementOp::_placeTile(unsigned int x, unsigned int y, TTilePoint* pTopLeft,
                                     TTilePoint* pBottomRight)
{
    if (x < pTopLeft->x())
        pTopLeft->x(x);
    if (y < pTopLeft->y())
        pTopLeft->y(y);
    if (x >= pBottomRight->x())
        pBottomRight->x(x + 1);
    if (y >= pBottomRight->y())
        pBottomRight->y(y + 1);
    _placeTile(x, y);
}

// A valid cell of the painted terrain: its orthogonal neighbours of other
// terrains, and its diagonal ones of terrains without split-span tiles, are
// tested again.
void TTerrainPlacementOp::_validTilePlaced(unsigned int x, unsigned int y)
{
    const TGameMap::TLayer* pMapLayer = _getPMapLayer();
#line 1772
    assert(pMapLayer->getCell( x, y ).getTerrainType() == _m_terrainType);
    if (y > 0 && pMapLayer->getCell(x, y - 1).getTerrainType() != _m_terrainType)
        _m_testSet.insert(TTilePoint(x, y - 1));
    else if (y < pMapLayer->getHeight() - 1 && pMapLayer->getCell(x, y + 1).getTerrainType() != _m_terrainType)
        _m_testSet.insert(TTilePoint(x, y + 1));
    if (x > 0 && pMapLayer->getCell(x - 1, y).getTerrainType() != _m_terrainType)
        _m_testSet.insert(TTilePoint(x - 1, y));
    else if (x < pMapLayer->getWidth() - 1 && pMapLayer->getCell(x + 1, y).getTerrainType() != _m_terrainType)
        _m_testSet.insert(TTilePoint(x + 1, y));
    if (x > 0 && y > 0) {
        TTerrainType terrainType = pMapLayer->getCell(x - 1, y - 1).getTerrainType();
        if (terrainType != _m_terrainType && !akTerrainTypeTraits[terrainType]->m_bSplitSpans)
            _m_testSet.insert(TTilePoint(x - 1, y - 1));
    }
    if (x < pMapLayer->getWidth() - 1 && y > 0) {
        TTerrainType terrainType = pMapLayer->getCell(x + 1, y - 1).getTerrainType();
        if (terrainType != _m_terrainType && !akTerrainTypeTraits[terrainType]->m_bSplitSpans)
            _m_testSet.insert(TTilePoint(x + 1, y - 1));
    }
    if (x > 0 && y < pMapLayer->getHeight() - 1) {
        TTerrainType terrainType = pMapLayer->getCell(x - 1, y + 1).getTerrainType();
        if (terrainType != _m_terrainType && !akTerrainTypeTraits[terrainType]->m_bSplitSpans)
            _m_testSet.insert(TTilePoint(x - 1, y + 1));
    }
    if (x < pMapLayer->getWidth() - 1 && y < pMapLayer->getHeight() - 1) {
        TTerrainType terrainType = pMapLayer->getCell(x + 1, y + 1).getTerrainType();
        if (terrainType != _m_terrainType && !akTerrainTypeTraits[terrainType]->m_bSplitSpans)
            _m_testSet.insert(TTilePoint(x + 1, y + 1));
    }
}

// Makes an invalid cell valid by painting a neighbour: the cell beside a
// one-cell-wide strip, or the cells of every span of other terrains but the
// longest between the spans of its own.
void TTerrainPlacementOp::_validateTile(unsigned int x, unsigned int y, TTilePoint* pTopLeft,
                                        TTilePoint* pBottomRight)
{
    const TGameMap::TLayer* pMapLayer = _getPMapLayer();
#line 1825
    assert(pMapLayer->getCell( x, y ).getTerrainType() == _m_terrainType);
    if (isInvalidVert(pMapLayer, x, y)) {
#line 1829
        assert(y > 0);
        assert(y < pMapLayer->getHeight() - 1);
        if (isInvalid(pMapLayer, x, y - 1)
            || !isInvalid(pMapLayer, x, y + 1)
               && (!wouldBeInvalidHoriz(pMapLayer, x, y - 1, _m_terrainType)
                   || wouldBeInvalidHoriz(pMapLayer, x, y + 1, _m_terrainType)))
            _placeTile(x, y - 1, pTopLeft, pBottomRight);
        else
            _placeTile(x, y + 1, pTopLeft, pBottomRight);
    }
    pMapLayer = _getPMapLayer();
    if (isInvalidHoriz(pMapLayer, x, y)) {
#line 1847
        assert(x > 0);
        assert(x < pMapLayer->getWidth() - 1);
        if (isInvalid(pMapLayer, x - 1, y)
            || !isInvalid(pMapLayer, x + 1, y)
               && (!wouldBeInvalidVert(pMapLayer, x - 1, y, _m_terrainType)
                   || wouldBeInvalidVert(pMapLayer, x + 1, y, _m_terrainType)))
            _placeTile(x - 1, y, pTopLeft, pBottomRight);
        else
            _placeTile(x + 1, y, pTopLeft, pBottomRight);
    }
    if (!akTerrainTypeTraits[_m_terrainType]->m_bSplitSpans) {
        const TGameMap::TLayer* pMapLayer = _getPMapLayer();
        if (hasMultipleAdjSimilarSpans(pMapLayer, x, y)) {
            bool abAdjSimilar[kNumDirs];
            computeAdjacentSimilar(pMapLayer, x, y, abAdjSimilar);
            struct {
                unsigned int m_length;
                unsigned int m_startDir;
                unsigned int m_numDirs;
            } aAdjDifferentSpan[4];
            unsigned int numAdjDifferentSpans = 0;
            unsigned int startDir = 0;
            while (!abAdjSimilar[startDir]) {
                ++startDir;
#line 1882
                assert(startDir < kNumDirs);
            }
            unsigned int dir = startDir;
            unsigned int spanNum;
            for (;;) {
                do
                    if ((dir = (dir + 1) % kNumDirs) == startDir)
                        goto spansDone;
                while (abAdjSimilar[dir]);
                spanNum = numAdjDifferentSpans++;
                aAdjDifferentSpan[spanNum].m_length = 0;
                aAdjDifferentSpan[spanNum].m_startDir = dir;
                aAdjDifferentSpan[spanNum].m_numDirs = 0;
                do {
                    aAdjDifferentSpan[spanNum].m_length += (dir & 1) == 0 ? 2 : 1;
                    ++aAdjDifferentSpan[spanNum].m_numDirs;
                    if ((dir = (dir + 1) % kNumDirs) == startDir)
                        goto spansDone;
                } while (!abAdjSimilar[dir]);
            }
        spansDone:
#line 1908
            assert(numAdjDifferentSpans > 1 && numAdjDifferentSpans <= ARRAY_SIZE( aAdjDifferentSpan ));
            bool abAdjDir[kNumDirs];
            computeAdjacentDirs(pMapLayer, x, y, abAdjDir);
            do {
                spanNum = 0;
                unsigned int minLength = aAdjDifferentSpan[0].m_length;
                for (unsigned int i = 1; i < numAdjDifferentSpans; ++i)
                    if (aAdjDifferentSpan[i].m_length < minLength) {
                        spanNum = i;
                        minLength = aAdjDifferentSpan[i].m_length;
                    }
                for (dir = aAdjDifferentSpan[spanNum].m_startDir;
                     dir != (aAdjDifferentSpan[spanNum].m_startDir + aAdjDifferentSpan[spanNum].m_numDirs) % kNumDirs;
                     dir = (dir + 1) & (kNumDirs - 1))
                    if (abAdjDir[dir]) {
                        TTilePoint adjPt = TPoint<int>((int)x, (int)y) + akAdjOffset[dir];
                        _placeTile(adjPt.x(), adjPt.y(), pTopLeft, pBottomRight);
                    }
                --numAdjDifferentSpans;
                for (; spanNum < numAdjDifferentSpans; ++spanNum)
                    aAdjDifferentSpan[spanNum] = aAdjDifferentSpan[spanNum + 1];
            } while (numAdjDifferentSpans > 1);
        }
    }
}

// Counts each cell's neighbours of other terrains, then repicks a full tile
// for every cell without any and the transition tile for every other one.
void TTerrainPlacementOp::_processTransitions(TTilePoint* pTopLeft, TTilePoint* pBottomRight)
{
    const TGameMap::TLayer* pMapLayer = _getPMapLayer();
    unsigned int mapWidth = pMapLayer->getWidth();
    unsigned int mapHeight = pMapLayer->getHeight();
    vector<unsigned char> aNumTransitions(mapWidth * mapHeight, 0);
    TTerrainType terrainType;
    unsigned int y;
    unsigned int x;
    for (y = 0; y < mapHeight - 1; ++y) {
        terrainType = pMapLayer->getCell(0, y).getTerrainType();
        if (pMapLayer->getCell(1, y).getTerrainType() != terrainType) {
            ++aNumTransitions[y * mapWidth];
            ++aNumTransitions[y * mapWidth + 1];
        }
        if (pMapLayer->getCell(1, y + 1).getTerrainType() != terrainType) {
            ++aNumTransitions[y * mapWidth];
            ++aNumTransitions[(y + 1) * mapWidth + 1];
        }
        if (pMapLayer->getCell(0, y + 1).getTerrainType() != terrainType) {
            ++aNumTransitions[y * mapWidth];
            ++aNumTransitions[(y + 1) * mapWidth];
        }
        for (x = 1; x < mapWidth - 1; ++x) {
            terrainType = pMapLayer->getCell(x, y).getTerrainType();
            if (pMapLayer->getCell(x + 1, y).getTerrainType() != terrainType) {
                ++aNumTransitions[y * mapWidth + x];
                ++aNumTransitions[y * mapWidth + x + 1];
            }
            if (pMapLayer->getCell(x + 1, y + 1).getTerrainType() != terrainType) {
                ++aNumTransitions[y * mapWidth + x];
                ++aNumTransitions[(y + 1) * mapWidth + x + 1];
            }
            if (pMapLayer->getCell(x, y + 1).getTerrainType() != terrainType) {
                ++aNumTransitions[y * mapWidth + x];
                ++aNumTransitions[(y + 1) * mapWidth + x];
            }
            if (pMapLayer->getCell(x - 1, y + 1).getTerrainType() != terrainType) {
                ++aNumTransitions[y * mapWidth + x];
                ++aNumTransitions[(y + 1) * mapWidth + x - 1];
            }
        }
#line 2012
        assert(x == mapWidth - 1);
        terrainType = pMapLayer->getCell(x, y).getTerrainType();
        if (pMapLayer->getCell(x, y + 1).getTerrainType() != terrainType) {
            ++aNumTransitions[y * mapWidth + x];
            ++aNumTransitions[(y + 1) * mapWidth + x];
        }
        if (pMapLayer->getCell(x - 1, y + 1).getTerrainType() != terrainType) {
            ++aNumTransitions[y * mapWidth + x];
            ++aNumTransitions[(y + 1) * mapWidth + x - 1];
        }
    }
#line 2028
    assert(y == mapHeight - 1);
    for (x = 0; x < mapWidth - 1; ++x) {
        terrainType = pMapLayer->getCell(x, y).getTerrainType();
        if (pMapLayer->getCell(x + 1, y).getTerrainType() != terrainType) {
            ++aNumTransitions[y * mapWidth + x];
            ++aNumTransitions[y * mapWidth + x + 1];
        }
    }
    for (y = 0; y < mapHeight; ++y)
        for (x = 0; x < mapWidth; ++x)
            if (aNumTransitions[y * mapWidth + x] != 0) {
                pMapLayer = _getPMapLayer();
                const TGameMap::TLayer::TCell& cell = pMapLayer->getCell(x, y);
                TTransType aTransType[kNumDirs];
                getTransitions(pMapLayer, TTilePoint(x, y), aTransType);
                TGroundShape groundShape;
                TFlippedState flippedState;
                computeGroundShape(aTransType, &groundShape, &flippedState);
                if (groundShape == eGS_dirtCorner) {
                    if (isOutsideDiag(pMapLayer, x, y, flippedState))
                        groundShape = eGS_dirtDiagCorner;
                } else if (groundShape == eGS_sandCorner) {
                    if (isOutsideDiag(pMapLayer, x, y, flippedState))
                        groundShape = eGS_sandDiagCorner;
                } else if (groundShape == eGS_dirtInnerCorner) {
                    if (isInsideDiag(pMapLayer, x, y, flippedState))
                        groundShape = eGS_dirtDiagInnerCorner;
                } else if (groundShape == eGS_sandInnerCorner) {
                    if (isInsideDiag(pMapLayer, x, y, flippedState))
                        groundShape = eGS_sandDiagInnerCorner;
                }
                unsigned int tileNum;
                if (groundShape != eGS_full)
                    tileNum = akTerrainTypeTraits[cell.getTerrainType()]->pickTransitionTile(
                        groundShape, flippedState, &flippedState, cell.getTileNum());
                else
                    tileNum = _pickFullTile(x, y, cell.getTerrainType(), cell.getTileNum());
                if (cell.getTileNum() != tileNum || cell.getBHFlipped() != flippedState.m_bHFlipped
                    || cell.getBVFlipped() != flippedState.m_bVFlipped) {
                    if (x < pTopLeft->x())
                        pTopLeft->x(x);
                    if (y < pTopLeft->y())
                        pTopLeft->y(y);
                    if (x >= pBottomRight->x())
                        pBottomRight->x(x + 1);
                    if (y >= pBottomRight->y())
                        pBottomRight->y(y + 1);
                    TGameMap::TLayer::TCell* pCell = _getPMapLayer()->getPCell(x, y);
                    pCell->setTileNum(tileNum);
                    pCell->setBHFlipped(flippedState.m_bHFlipped);
                    pCell->setBVFlipped(flippedState.m_bVFlipped);
                }
            } else {
                const TGameMap::TLayer::TCell& cell = _getPMapLayer()->getCell(x, y);
                unsigned int tileNum = _pickFullTile(x, y, cell.getTerrainType(), cell.getTileNum());
                if (cell.getTileNum() != tileNum || cell.getBHFlipped() || cell.getBVFlipped()) {
                    if (x < pTopLeft->x())
                        pTopLeft->x(x);
                    if (y < pTopLeft->y())
                        pTopLeft->y(y);
                    if (x >= pBottomRight->x())
                        pBottomRight->x(x + 1);
                    if (y >= pBottomRight->y())
                        pBottomRight->y(y + 1);
                    TGameMap::TLayer::TCell* pCell = _getPMapLayer()->getPCell(x, y);
                    pCell->setTileNum(tileNum);
                    pCell->setBHFlipped(false);
                    pCell->setBVFlipped(false);
                }
            }
}

// Special tiles thin out beside other special tiles: each orthogonal
// neighbour of the same terrain on a special tile halves the frequency.
unsigned int TTerrainPlacementOp::_computeEffectiveFrequency(unsigned int specialTileFrequency,
                                                             const TGameMap::TLayer& mapLayer,
                                                             unsigned int x, unsigned int y,
                                                             TTerrainType terrainType)
{
#line 2144
    assert(terrainType >= 0 && terrainType < kNumTerrainTypes);
    assert(x < mapLayer.getWidth());
    assert(y < mapLayer.getHeight());
    const TTerrainTypeTraits* pTraits = akTerrainTypeTraits[terrainType];
    if (x > 0) {
        const TGameMap::TLayer::TCell& cell = mapLayer.getCell(x - 1, y);
        if (cell.getTerrainType() == terrainType && pTraits->isTileSpecial(cell.getTileNum()))
            specialTileFrequency >>= 1;
    }
    if (y > 0) {
        const TGameMap::TLayer::TCell& cell = mapLayer.getCell(x, y - 1);
        if (cell.getTerrainType() == terrainType && pTraits->isTileSpecial(cell.getTileNum()))
            specialTileFrequency >>= 1;
    }
    if (x < mapLayer.getWidth() - 1) {
        const TGameMap::TLayer::TCell& cell = mapLayer.getCell(x + 1, y);
        if (cell.getTerrainType() == terrainType && pTraits->isTileSpecial(cell.getTileNum()))
            specialTileFrequency >>= 1;
    }
    if (y < mapLayer.getHeight() - 1) {
        const TGameMap::TLayer::TCell& cell = mapLayer.getCell(x, y + 1);
        if (cell.getTerrainType() == terrainType && pTraits->isTileSpecial(cell.getTileNum()))
            specialTileFrequency >>= 1;
    }
    return specialTileFrequency;
}

unsigned int TTerrainPlacementOp::_pickFullTile(unsigned int x, unsigned int y, TTerrainType terrainType)
{
    unsigned int specialFrequency =
        _computeEffectiveFrequency(_m_specialTileFrequency, *_getPMapLayer(), x, y, terrainType);
    return akTerrainTypeTraits[terrainType]->pickFullTile(specialFrequency);
}

unsigned int TTerrainPlacementOp::_pickFullTile(unsigned int x, unsigned int y, TTerrainType terrainType,
                                                unsigned int requestedTileNum)
{
    unsigned int specialFrequency =
        _computeEffectiveFrequency(_m_specialTileFrequency, *_getPMapLayer(), x, y, terrainType);
    return akTerrainTypeTraits[terrainType]->pickFullTile(specialFrequency, requestedTileNum);
}

void TTerrainPlacementOp::repaintMap(TGameMap* pMap, unsigned int specialTileFrequency)
{
#line 2203
    assert(pMap != NULL);
    assert(specialTileFrequency <= s_kMaxSpecialTileFrequency);
    unsigned int numLayers = pMap->isTwoLayer() ? 2 : 1;
    for (unsigned int layer = 0; layer < numLayers; ++layer)
        _repaintMapLayer(pMap->getPLayer(layer), specialTileFrequency);
}

void TTerrainPlacementOp::_repaintMapLayer(TGameMap::TLayer* pMapLayer, unsigned int specialTileFrequency)
{
#line 2215
    assert(pMapLayer != NULL);
    unsigned int mapWidth = pMapLayer->getWidth();
    unsigned int mapHeight = pMapLayer->getHeight();
    for (unsigned int y = 0; y < mapHeight; ++y)
        for (unsigned int x = 0; x < mapWidth; ++x) {
            const TGameMap::TLayer::TCell& cell = pMapLayer->getCell(x, y);
            TTerrainType terrainType = cell.getTerrainType();
            const TTerrainTypeTraits* pTraits = akTerrainTypeTraits[terrainType];
            if (pTraits->hasSpecialTiles() && pTraits->getTileGroundShape(cell.getTileNum()) == eGS_full) {
                unsigned int specialFrequency =
                    _computeEffectiveFrequency(specialTileFrequency, *pMapLayer, x, y, terrainType);
                unsigned int tileNum = pTraits->pickFullTile(specialFrequency);
                pMapLayer->getPCell(x, y)->setTileNum(tileNum);
            }
        }
}
