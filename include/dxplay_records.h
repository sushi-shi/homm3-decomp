#ifndef HOMM3_DXPLAY_RECORDS_H
#define HOMM3_DXPLAY_RECORDS_H

#include <string.h>
#include "dxplay.h"

class CDPlayGroup {
public:
    CDPlayGroup(char* name, unsigned long id)
    {
        strcpy(m_name, name);
        m_id = id;
    }

    char m_name[0x100];
    unsigned long m_id;
};
SIZE(CDPlayGroup, 0x104);

class CDPlayPlayer {
public:
    CDPlayPlayer(char* name, unsigned long id)
    {
        strcpy(m_name, name);
        m_id = id;
    }

    char m_name[0x100];
    unsigned long m_id;
};
SIZE(CDPlayPlayer, 0x104);

class CDPlayAddressElement {
public:
    CDPlayAddressElement(const GUID* type, const void* data,
        unsigned long dataSize)
    {
        m_guid = *type;
        m_dataSize = dataSize;
        m_pData = new char[dataSize];
        memcpy(m_pData, data, m_dataSize);
    }

    ~CDPlayAddressElement()
    {
        delete [] m_pData;
    }

    GUID m_guid;
    char* m_pData;
    unsigned long m_dataSize;
};
SIZE(CDPlayAddressElement, 0x18);

#endif  /* HOMM3_DXPLAY_RECORDS_H */
