// 3 functions in link order.
#include "va.h"

#include <string.h>

#include "resource.h"

#include "terrain.h"

VA(0x00558720, 0x4E)  // dc 0x120934
resource::resource(const char* newName, EResourceType newType)
{
    if (newName) {
        strncpy(m_name, newName, 12);
        m_name[12] = 0;
        m_resType = newType;
        m_referenceCount = 0;
    } else {
        m_name[0] = 0;
        m_resType = RESOURCE_TYPE_NONE;
        m_referenceCount = -1;
    }
}

VA_COMPGEN(0x00558770, 0x23, SCALAR_DELETING_DTOR, resource)

VA(0x005587a0, 0x7)  // dc 0x12099c
resource::~resource()
{
}
