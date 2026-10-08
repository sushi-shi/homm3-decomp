// genericresource.h - TGenericResource, an owned copy of raw resource bytes.
// Loki h3maped object 52 (file name inferred from the class): the editor's
// generic resource type (RESOURCE_TYPE_DATA). The accessors are in-class
// inlines that the release compiler emits in GenericResource.cpp, the unit
// defining the vtable. Field names follow resource's (Name, resType).
#ifndef HOMM3_GENERICRESOURCE_H
#define HOMM3_GENERICRESOURCE_H

#include "resource.h"

class TGenericResource : public resource {
public:
    TGenericResource(const char* name, int size, const void* data);
    virtual ~TGenericResource();

    int get_Size() const { return Size; }
    const void* get_Data() const { return Data; }

private:
    char* Data;   // +0x1c
    int Size;     // +0x20
};

#endif  /* HOMM3_GENERICRESOURCE_H */
