// genericresource.cpp - E:\gamedcs\genericresource.cpp (compiland genericresource.obj)
// 3 functions in link order.
#include <va.h>
// #include "genericresource.h"

// Retail-dropped surface: Dreamcast's sole constructor caller is
// ResourceManager::GetResource. In retail that getter and this class's vtable
// are absent; after gametypewindow the complete carve runs through cinit0415..
// cinit0429 directly into global.obj's TGzInflateBuf family. The authoritative
// resource::resource0x558720 has 23 direct callers, all assigned to the typed
// bitmap, sprite, font, palette, sample and text/spreadsheet classes. This
// accounts for the reachable binary family, not unused original PC source.
// Exact constructor/destructor/getter exclusions are recorded in dc_only.tsv.

// E:\gamedcs\genericresource.cpp:26
DC_ONLY(0xc9788, 0x68)
void TGenericResource::TGenericResource(const char* name, int s, const void* d)
{
    // @stub
}

// E:\gamedcs\genericresource.cpp:34
DC_ONLY(0xc97f0, 0x4C)
void TGenericResource::~TGenericResource()
{
    // @stub
}

// E:\gamedcs\genericresource.cpp:30
DC_ONLY(0xc983c, 0x34)
void* TGenericResource::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}
