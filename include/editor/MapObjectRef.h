// MapObjectRef.h - a reference to a placed object: its layer and its
// object ID there (Loki h3maped; the header's file name is not proven).
// Default-constructed it names no object (first layer, ID 0). References
// order by layer, then ID, so they can fill a set.
#ifndef HOMM3_EDITOR_MAPOBJECTREF_H
#define HOMM3_EDITOR_MAPOBJECTREF_H

#include <function.h>

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

private:
    friend struct less<TMapObjectRef>;

    bool _m_bSecondLayer;
    unsigned int _m_objectID;
};

template<>
struct less<TMapObjectRef> : public binary_function<TMapObjectRef, TMapObjectRef, bool> {
    bool operator()(const TMapObjectRef& lhs, const TMapObjectRef& rhs) const
    {
        return lhs._m_bSecondLayer < rhs._m_bSecondLayer
            || (lhs._m_bSecondLayer == rhs._m_bSecondLayer && lhs._m_objectID < rhs._m_objectID);
    }
};

// Inline: the map specifications' player page keeps the linkonce copy.
inline bool operator!=(const TMapObjectRef& lhs, const TMapObjectRef& rhs)
{
    return !(lhs == rhs);
}

#endif  /* HOMM3_EDITOR_MAPOBJECTREF_H */
