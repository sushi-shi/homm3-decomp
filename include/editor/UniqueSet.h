// UniqueSet.h - a set that numbers its items in the order they were first
// added (the map's writer numbers the object types it writes; h3maped's
// add 0x43438c returns the item's number). The items keep their map
// order; the numbers keep the insertion order.
#ifndef HOMM3_EDITOR_UNIQUESET_H
#define HOMM3_EDITOR_UNIQUESET_H

#include <map>
#include <vector>

template <class T>
class TUniqueSet {
public:
    unsigned int add(const T& item)
    {
        std::map<T, unsigned int>::iterator pItem = _m_map.find(item);
        if (pItem == _m_map.end()) {
            pItem = _m_map.insert(std::map<T, unsigned int>::value_type(item, _m_apItem.size())).first;
            _m_apItem.push_back(pItem);
        }
        return pItem->second;
    }
    unsigned int numItems() const { return _m_apItem.size(); }
    const T& get(unsigned int num) const { return _m_apItem[num]->first; }

private:
    std::map<T, unsigned int> _m_map;
    std::vector<std::map<T, unsigned int>::iterator> _m_apItem;
};

#endif  /* HOMM3_EDITOR_UNIQUESET_H */
