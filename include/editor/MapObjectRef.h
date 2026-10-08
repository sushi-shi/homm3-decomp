// MapObjectRef.h - a reference to a placed object: its layer and its id
// in that layer (Loki h3maped; eight bytes, the bool first).
#ifndef HOMM3_EDITOR_MAPOBJECTREF_H
#define HOMM3_EDITOR_MAPOBJECTREF_H

class TMapObjectRef {
public:
    TMapObjectRef() : _m_bSecondLayer(false), _m_objectID(0) {}
    TMapObjectRef(bool bSecondLayer, unsigned int objectID)
        : _m_bSecondLayer(bSecondLayer), _m_objectID(objectID) {}

    bool getBSecondLayer() const { return _m_bSecondLayer; }
    unsigned int getObjectID() const { return _m_objectID; }

    friend bool operator==(const TMapObjectRef& lhs, const TMapObjectRef& rhs)
    {
        return lhs._m_bSecondLayer == rhs._m_bSecondLayer && lhs._m_objectID == rhs._m_objectID;
    }
    // The first layer's objects first, then by id (a player's town refs,
    // h3maped 0x42dd7f).
    friend bool operator<(const TMapObjectRef& lhs, const TMapObjectRef& rhs)
    {
        return lhs._m_bSecondLayer < rhs._m_bSecondLayer
               || (lhs._m_bSecondLayer == rhs._m_bSecondLayer && lhs._m_objectID < rhs._m_objectID);
    }

private:
    bool _m_bSecondLayer;
    unsigned int _m_objectID;
};

inline bool operator!=(const TMapObjectRef& lhs, const TMapObjectRef& rhs)
{
    return !(lhs == rhs);
}

#endif  /* HOMM3_EDITOR_MAPOBJECTREF_H */
