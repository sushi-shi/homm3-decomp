// GenericResource.cpp - Loki h3maped object 52 (file name inferred from the
// class TGenericResource).
#include <string.h>

#include "genericresource.h"

TGenericResource::TGenericResource(const char* name, int size, const void* data)
    : resource(name, RESOURCE_TYPE_DATA)
{
    Size = size;
    Data = new char[size];
    if (Data)
        memcpy(Data, data, size);
}

TGenericResource::~TGenericResource()
{
    if (Data)
        delete[] Data;
}
