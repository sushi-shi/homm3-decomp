// random_object_placer.cpp - the random object placer (h3maped
// 0x4af28f..0x4afb27; GOG only, no Loki counterpart). The obstacle tool
// runs the random map generator's decoration pass over the editor's map:
// t_random_object_placer copies the map's terrain, clear tiles and
// obstacle area into the generator's cells and adds the map's objects as
// generator objects; with bReplaceObjects it first removes the objects that
// lie wholly in the obstacle area, then decorates. Every name here except
// the RTTI class names is provisional. The generator's headers clash with
// MFC's CObject, so this object does without stdafx.h; terrain.h supplies
// the ten terrain masks every editor object initializes.
#include <algorithm>
#include <string.h>
#include <utility>

#include "va.h"
#include "objecttype.h"
#include "objnames.h"
#include "rmg.h"
#include "terrain.h"
#include "editor/random_object_placer.h"

namespace {

int getPreferredTerrain(const TObjectType& objType);

class t_random_object_placer : public t_abstract_random_generator {
public:
    t_random_object_placer(t_random_object_map* pMap, type_progress_bar* pProgress, int version);
    virtual ~t_random_object_placer();

    virtual void addObject(type_object* object, TRmgMapPosition position);

    void placeObjects(bool bReplaceObjects);

private:
    TRmgObjectPropertiesRef* _getProperties(const TObjectType& objType);
    bool _isInObstacleArea(type_object* object);

    t_random_object_map* m_pMap;
    std::vector<TObjectType*> m_apObjectType;
    std::vector<std::pair<unsigned int, type_object*> > m_aObject;
};

VA(0x004af463, 0x1df)
t_random_object_placer::t_random_object_placer(t_random_object_map* pMap, type_progress_bar* pProgress,
                                               int version)
    : t_abstract_random_generator(pMap->m_width, pMap->m_height, 1, pProgress, 261000, version),
      m_pMap(pMap)
{
    TTilePoint loc;
    for (loc.m_y = 0; loc.m_y < pMap->m_height; loc.m_y++) {
        for (loc.m_x = 0; loc.m_x < pMap->m_width; loc.m_x++) {
            TRmgMapItem* pItem = m_map.getMapItem(loc.m_x, loc.m_y, 0);
            pItem->setTerrain(pMap->getTerrain(loc.m_x, loc.m_y), 0, 0, 0);
            if (pMap->isClear(loc.m_x, loc.m_y))
                pItem->setPathClearance(0);
            if (pMap->isObstacleArea(loc.m_x, loc.m_y))
                pItem->setObstacleFill(1);
        }
    }

    unsigned int objID;
    if (pMap->getFirstObject(&objID)) {
        TRmgMapPosition position;
        position.m_z = 0;
        do {
            const TObjectType& objType = pMap->getObjectType(objID);
            TTilePoint loc = pMap->getObjectLoc(objID);
            position.m_x = loc.x();
            position.m_y = loc.y();
            TRmgObjectPropertiesRef* pProperties = _getProperties(objType);
            type_object* object = new type_object(pProperties);
            t_abstract_random_generator::addObject(object, position);
            m_aObject.push_back(std::pair<unsigned int, type_object*>(objID, object));
        } while (pMap->getNextObject(&objID));
    }
}

VA(0x004af65e, 0x7d)
t_random_object_placer::~t_random_object_placer()
{
    for (unsigned int i = 0; i < m_apObjectType.size(); i++)
        delete m_apObjectType[i];
}

// Whether every blocked cell of the object inside the map lies outside the
// paths the generator keeps clear, and there is one.
VA(0x004af6db, 0xaa)
bool t_random_object_placer::_isInObstacleArea(type_object* object)
{
    const TObjectType* prototype = object->m_properties->m_prototype;
    TRmgMapPosition position = object->m_position;
    bool bInArea = false;
    for (unsigned int row = 0; row < prototype->getHeight(); row++) {
        int y = position.m_y - row;
        if (y >= 0 && y < m_map.m_mapHeight) {
            for (unsigned int col = 0; col < prototype->getWidth(); col++) {
                int x = position.m_x - col;
                if (x >= 0 && x < m_map.m_mapWidth && !prototype->getBCellPassable(col, row)) {
                    bInArea = true;
                    if (m_map.getMapItem(x, y, 0)->hasPathClearance())
                        return false;
                }
            }
        }
    }
    return bInArea;
}

// The generator's properties for an object type: an existing prototype of
// the same base type, terrain and subtype with identical bytes, or a copy
// of the type added with the placement rule of the last such prototype.
VA(0x004af785, 0x11a)
TRmgObjectPropertiesRef* t_random_object_placer::_getProperties(const TObjectType& objType)
{
    int extra = objType.getExtra();
    int type = akAdvObjectTypeTraits[objType.getType()].m_nameRow;
    int terrain = getPreferredTerrain(objType);
    TRmgObjectPlacementRule* pRule;
    for (unsigned int i = 0; i < m_objectPrototypes[type].size(); i++) {
        TRmgObjectPropertiesRef* pProperties = m_objectPrototypes[type][i];
        if (pProperties->m_preferredTerrain == terrain && pProperties->m_prototype->getExtra() == extra) {
            pRule = pProperties->m_placementRule;
            if (memcmp(pProperties->m_prototype, &objType, sizeof(TObjectType)) == 0)
                return pProperties;
        }
    }
    TObjectType* pObjType = new TObjectType(objType);
    TRmgObjectPropertiesRef* pProperties = new TRmgObjectPropertiesRef(pObjType);
    m_apObjectType.push_back(pObjType);
    pProperties->m_preferredTerrain = terrain;
    pProperties->m_placementRule = pRule;
    m_objectPrototypes[type].push_back(pProperties);
    return m_objectPrototypes[type].back();
}

// The first terrain the object type recommends; eTerrainRock when none.
VA(0x004af89f, 0x36)
int getPreferredTerrain(const TObjectType& objType)
{
    int terrain;
    for (terrain = eTerrainDirt; terrain < eTerrainRock; terrain++) {
        if (objType.getRecommendedTerrainMask()[terrain])
            break;
    }
    return terrain;
}

VA(0x004af8d5, 0x3b)
void t_random_object_placer::addObject(type_object* object, TRmgMapPosition position)
{
    t_abstract_random_generator::addObject(object, position);
    m_pMap->placeObject(*object->m_properties->m_prototype, TTilePoint(position.m_x, position.m_y));
}

VA(0x004af910, 0x189)
void t_random_object_placer::placeObjects(bool bReplaceObjects)
{
    if (bReplaceObjects) {
        unsigned int i = m_aObject.size();
        while (i--) {
            type_object* object = m_aObject[i].second;
            const TObjectType* prototype = object->m_properties->m_prototype;
            if (prototype->m_hasTrigger || !_isInObstacleArea(object))
                continue;
            std::vector<type_object*>::iterator found = std::find(m_objects.begin(), m_objects.end(), object);
            if (found != m_objects.end())
                m_objects.erase(found);
            m_pMap->eraseObject(m_aObject[i].first);
            m_aObject.erase(&m_aObject[i]);
            TRmgMapPosition position = object->m_position;
            TTilePoint cell;
            TRmgMapPosition mapPosition;
            for (cell.m_y = 0; cell.m_y < prototype->getHeight(); ++cell.m_y) {
                mapPosition.m_y = position.m_y - cell.m_y;
                if (mapPosition.m_y < 0 || mapPosition.m_y >= m_map.m_mapHeight)
                    continue;
                for (cell.m_x = 0; cell.m_x < prototype->getWidth(); ++cell.m_x) {
                    mapPosition.m_x = position.m_x - cell.m_x;
                    if (mapPosition.m_x < 0 || mapPosition.m_x >= m_map.m_mapWidth)
                        continue;
                    if (!prototype->getBCellPassable(cell.m_x, cell.m_y) || prototype->getBCellTrigger(cell.m_x, cell.m_y))
                        m_map.getMapItem(mapPosition.m_x, mapPosition.m_y, 0)->removeObject(object);
                }
            }
            delete object;
        }
    }
    decorateMap();
}

}  // namespace

VA(0x004afa99, 0x51)
void placeRandomObjects(t_random_object_map* pMap, bool bReplaceObjects, type_progress_bar* pProgress,
                        ERmgMapVersion version)
{
    t_random_object_placer placer(pMap, pProgress, version);
    placer.placeObjects(bReplaceObjects);
}
