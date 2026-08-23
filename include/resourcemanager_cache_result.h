#ifndef HOMM3_RESOURCEMANAGER_CACHE_RESULT_H
#define HOMM3_RESOURCEMANAGER_CACHE_RESULT_H

#include "resourcemanager.h"

namespace ResourceManager {

// Internal Dinkumware tree result underlying the public cache-map pair.
// Kept in an owner header so its one proven shape is available without
// perturbing unrelated consumers of the public ResourceManager surface.
struct TCacheTreeInsertResult {
    TCacheIterator first;
    unsigned int second;
};

}

#endif
