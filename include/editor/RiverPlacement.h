// RiverPlacement.h - river placement (Loki RiverPlacement.cpp). Declared so
// far only as far as TMapDoc needs it: the client interfaces the river
// operations report to (TRiverPlacementOpClient derives from TRiverOpClient,
// as their type_info functions show; slot order from TMapDoc's thunks).
#ifndef HOMM3_EDITOR_RIVERPLACEMENT_H
#define HOMM3_EDITOR_RIVERPLACEMENT_H

class TRiverOpClient {
public:
    virtual void onRiversUpdated(bool bUnderground, unsigned int left, unsigned int top,
                                unsigned int width, unsigned int height) = 0;
};

class TRiverPlacementOpClient : public TRiverOpClient {
public:
    virtual void onPlacingRiver(bool bUnderground, unsigned int x, unsigned int y) = 0;
};

#endif  /* HOMM3_EDITOR_RIVERPLACEMENT_H */
