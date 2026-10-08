// 3 functions in link order.
#include "va.h"

#include <string.h>

#include "resource.h"

#include "terrain.h"

VA(0x00558720, 0x4E)
DC_ADDRESS(0x120934, 0x68)
MAC_ADDRESS(0x151400, 0x7c)
resource::resource(const char* newName, EResourceType newType)
{
    if (newName) {
        strncpy(Name, newName, 12);
        Name[12] = 0;
        resType = newType;
        ReferenceCount = 0;
    } else {
        Name[0] = 0;
        resType = RESOURCE_TYPE_NONE;
        ReferenceCount = -1;
    }
}

VA_COMPGEN(0x00558770, 0x23, SCALAR_DELETING_DTOR, resource)

VA(0x005587a0, 0x7)
DC_ADDRESS(0x12099c, 0x24)
MAC_ADDRESS(0x15147c, 0x48)
resource::~resource()
{
}
