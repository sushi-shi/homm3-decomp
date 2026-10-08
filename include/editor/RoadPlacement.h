// RoadPlacement.h - road placement (Loki RoadPlacement.cpp). Declared so
// far only as far as TMapDoc needs it: the client interfaces the road
// operations report to (TRoadPlacementOpClient derives from TRoadOpClient,
// as their type_info functions show; slot order from TMapDoc's thunks).
#ifndef HOMM3_EDITOR_ROADPLACEMENT_H
#define HOMM3_EDITOR_ROADPLACEMENT_H

class TRoadOpClient {
public:
    virtual void onRoadsUpdated(bool bUnderground, unsigned int left, unsigned int top,
                                unsigned int width, unsigned int height) = 0;
};

class TRoadPlacementOpClient : public TRoadOpClient {
public:
    virtual void onPlacingRoad(bool bUnderground, unsigned int x, unsigned int y) = 0;
};

#endif  /* HOMM3_EDITOR_ROADPLACEMENT_H */
