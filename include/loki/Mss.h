/* Mss.h - Miles Sound System handles as the Loki port sees them (Loki mixed
 * through SDL_mixer; work/loki-game only). */
#ifndef HOMM3_LOKI_MSS_H
#define HOMM3_LOKI_MSS_H

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;
typedef signed char S8;
typedef short S16;
typedef long S32;
typedef struct _DIG_DRIVER* HDIGDRIVER;
typedef struct _MDI_DRIVER* HMDIDRIVER;
class ds_memsample;
typedef ds_memsample* HSAMPLE;
typedef struct _SEQUENCE* HSEQUENCE;
typedef struct _STREAM* HSTREAM;
typedef S32 HTIMER;
typedef U32 HPROVIDER;
typedef U32 H3DPOBJECT;

#endif
