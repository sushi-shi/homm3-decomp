#ifndef HOMM3_MAPCELL_NEWMAP_VIEW_H
#define HOMM3_MAPCELL_NEWMAP_VIEW_H

#include "game.h"

// Mapcell-private vtable view of CMapObjectData (game.h) for the broadcast
// helper NewfullMap::NewfullMapFn_00505D60 (0x505d60): it calls virtual slot
// +0x28 on every mapObjectData record, handing each a (type_point, int) pair.
// game.h's shared CMapObjectDataNewMapView declares every slot nullary
// (game.cpp only ever needs the nullary +0x38), so the two-argument +0x28
// signature the retail body proves cannot be spelled against that shared view
// without editing game.h.  This TU-local view mirrors the same "TU-local
// vtable view" device game.h documents, naming only the slots up to +0x28 and
// giving +0x28 its real signature.  No instance is ever constructed, so - as
// with the game.h view - no vtable is emitted for it.
class CMapObjectDataMapcellView : public CMapObjectData {
public:
    virtual void MapcellVFn04();
    virtual void MapcellVFn08();
    virtual void MapcellVFn0c();
    virtual void MapcellVFn10();
    virtual void MapcellVFn14();
    virtual void MapcellVFn18();
    virtual void MapcellVFn1c();
    virtual void MapcellVFn20();
    virtual void MapcellVFn24();
    virtual void BroadcastToRecord(type_point point, int player);
};

#endif  /* HOMM3_MAPCELL_NEWMAP_VIEW_H */
