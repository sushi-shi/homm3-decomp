// UniqueSet.h - a set that numbers its distinct items in insertion order
// (Loki h3maped). add() returns an item's id, inserting it on first sight;
// get() maps an id back to the item. Instantiated for the object-type
// image names (ObjectType.cpp) and the map's object types.
#ifndef HOMM3_EDITOR_UNIQUESET_H
#define HOMM3_EDITOR_UNIQUESET_H

#include <assert.h>
#include <map>
#include <vector>

template<class T>
class TUniqueSet {
public:
    size_t add(const T& item);
    size_t numItems() const { return _m_idMap.size(); }
    const T& get(unsigned int id) const;

private:
    typedef map<T, size_t> TMap;

    TMap _m_map;
    vector<TMap::const_iterator> _m_idMap;
};

template<class T>
size_t TUniqueSet<T>::add(const T& item)
{
    TMap::iterator it = _m_map.find(item);
    if (it == _m_map.end()) {
        pair<TMap::iterator, bool> insResult = _m_map.insert(TMap::value_type(item, _m_idMap.size()));
#line 59 "UniqueSet.h"
        assert(insResult.second);
        _m_idMap.push_back(it = insResult.first);
    }
    return it->second;
}

template<class T>
inline const T& TUniqueSet<T>::get(unsigned int id) const
{
#line 78 "UniqueSet.h"
    assert(id < _m_idMap.size());
    assert(_m_idMap[ id ]->second == id);
    return _m_idMap[id]->first;
}

#endif  /* HOMM3_EDITOR_UNIQUESET_H */
