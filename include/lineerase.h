// The map editor's line erase operations (h3maped RTTI TRiverEraseOp and
// TRoadEraseOp, vtables 0x54187c/0x541a14). Only the editor builds them;
// the game compiles their bodies with the river, road and line placement
// sources, links them unreferenced and /OPT:REF drops them. The game's
// other sources never see these declarations.
#ifndef HOMM3_LINEERASE_H
#define HOMM3_LINEERASE_H

#include "lineplacement.h"

// Loki's TLineEraseOp without its client: clears the lines of a rectangle
// through the painter (the walker's rectangle clear).
class TLineEraseOp {
public:
    TLineEraseOp(TMapLineFilter* newPainter);
    void operator()(unsigned int left, unsigned int top, unsigned int width,
                    unsigned int height);

    TMapLineFilter* m_painter;
};

class TRiverEraseOp : public TRiverOp {
public:
    // Erases the river of a cell whose terrain became water or rock.
    static void onTerrainTypeChanged(TRiverOp::TAbstractMap* map, const TTilePoint& point);

    TRiverEraseOp(TRiverOp::TAbstractMap* newAdapter);
    virtual ~TRiverEraseOp() {}

    void operator()(unsigned int left, unsigned int top, unsigned int width,
                    unsigned int height)
    {
        m_eraser(left, top, width, height);
    }

    TLineEraseOp m_eraser;
};

class TRoadEraseOp : public TRoadOp {
public:
    // Erases the road of a cell whose terrain became water or rock.
    static void onTerrainTypeChanged(TRoadOp::TAbstractMap* map, const TTilePoint& point);

    TRoadEraseOp(TRoadOp::TAbstractMap* newAdapter);
    virtual ~TRoadEraseOp() {}

    void operator()(unsigned int left, unsigned int top, unsigned int width,
                    unsigned int height)
    {
        m_eraser(left, top, width, height);
    }

    TLineEraseOp m_eraser;
};

#endif  // HOMM3_LINEERASE_H
