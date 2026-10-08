// resource.cpp - Loki h3maped object 49 (file name inferred from the class):
// the named, reference-counted base of every cached resource.
#include <string.h>

#include "resource.h"

resource::resource(const char* name, EResourceType type)
{
    if (name) {
        strncpy(Name, name, 12);
        Name[12] = 0;
        resType = type;
        ReferenceCount = 0;
    } else {
        Name[0] = 0;
        resType = RESOURCE_TYPE_NONE;
        ReferenceCount = -1;
    }
}

resource::~resource()
{
}
