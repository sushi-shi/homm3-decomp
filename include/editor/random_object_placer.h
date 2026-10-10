// random_object_placer.h - the random object placer (random_object_placer.cpp;
// GOG only, h3maped 0x4af28f..0x4afb27; the header's name is not recorded).
// The editor's obstacle tool runs the random map generator's decoration
// over a map it reaches through t_random_object_map (RTTI): the map's
// objects, their types and locations, its terrain, the tiles the tool
// keeps clear and the tiles it fills, and object placement and removal.
// The placer (t_random_object_placer, a t_abstract_random_generator) takes
// the map's size from the interface's data. Declared so far only as far as
// the map view needs it; the slot names are not recorded.
#ifndef HOMM3_EDITOR_RANDOM_OBJECT_PLACER_H
#define HOMM3_EDITOR_RANDOM_OBJECT_PLACER_H

#include "rmg_request.h"
#include "Point.h"

class TObjectType;
class type_progress_bar;

class t_random_object_map {
public:
    t_random_object_map(int width, int height) : m_width(width), m_height(height) {}

    virtual bool getFirstObject(unsigned int* pObjID) = 0;
    virtual bool getNextObject(unsigned int* pObjID) = 0;
    virtual const TObjectType& getObjectType(unsigned int objID) = 0;
    virtual TTilePoint getObjectLoc(unsigned int objID) = 0;
    virtual int getTerrain(unsigned int x, unsigned int y) = 0;
    virtual bool isClear(unsigned int x, unsigned int y) = 0;
    virtual bool isObstacleArea(unsigned int x, unsigned int y) = 0;
    virtual void placeObject(const TObjectType& objType, TTilePoint loc) = 0;
    virtual void eraseObject(unsigned int objID) = 0;

    int m_width;
    int m_height;
};

// Decorates the map's obstacle area (h3maped 0x4afa99); with
// bReplaceObjects the objects already in it are removed first.
void placeRandomObjects(t_random_object_map* pMap, bool bReplaceObjects, type_progress_bar* pProgress,
                        ERmgMapVersion version);

#endif  /* HOMM3_EDITOR_RANDOM_OBJECT_PLACER_H */
