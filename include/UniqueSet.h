// UniqueSet.h - a set that numbers its items in the order they were first
// added. Loki h3maped's TUniqueSet<T> (UniqueSet.h): add returns the item's
// number (h3maped 0x43438c for TObjectType, 0x491eed for std::string);
// numItems and get read the numbering. The items keep their map order; the
// numbers keep the insertion order.
#ifndef HOMM3_UNIQUESET_H
#define HOMM3_UNIQUESET_H

#include <map>
#include <stddef.h>
#include <vector>

#include "va.h"

template <class T>
class TUniqueSet {
public:
    MAC_ADDRESS(0x2268d0, 0xc4)
    size_t add(const T& item)
    {
        std::map<T, size_t>::iterator pItem = _m_map.find(item);
        if (pItem == _m_map.end()) {
            // Loki asserts insResult.second here.
            std::pair<std::map<T, size_t>::iterator, bool> insResult =
                _m_map.insert(std::map<T, size_t>::value_type(item, _m_apItem.size()));
            pItem = insResult.first;
            _m_apItem.push_back(pItem);
        }
        return pItem->second;
    }
    size_t numItems() const { return _m_apItem.size(); }
    const T& get(size_t num) const { return _m_apItem[num]->first; }

private:
    std::map<T, size_t> _m_map;
    std::vector<std::map<T, size_t>::const_iterator> _m_apItem;
};

#endif  /* HOMM3_UNIQUESET_H */
