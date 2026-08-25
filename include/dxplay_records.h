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

#endif  /* HOMM3_DXPLAY_RECORDS_H */
