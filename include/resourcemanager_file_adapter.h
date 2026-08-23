// resourcemanager_file_adapter.h - ResourceManager's two local stream adapters.
#ifndef HOMM3_RESOURCEMANAGER_FILE_ADAPTER_H
#define HOMM3_RESOURCEMANAGER_FILE_ADAPTER_H

#include <stdio.h>
#include "abstractfile.h"

class LODFile;

namespace ResourceManager {

// Complete's resource readers adapt either an ordinary FILE or a selected
// LODFile to the common three-slot stream ABI. The HD names are admitted only
// after retail proves both layouts (vptr + one pointer), vtable slots and read
// behavior at these addresses.
class t_stdio_file_adapter : public TAbstractFile {
public:
    explicit t_stdio_file_adapter(FILE* value) : file(value) {}

    virtual int Read(void* data, int size);
    virtual int Write(const void*, int) { return 0; }

    FILE* file;
};

class t_lod_file_adapter : public TAbstractFile {
public:
    explicit t_lod_file_adapter(LODFile* value) : lod_file(value) {}

    virtual int Read(void* data, int size);
    virtual int Write(const void*, int) { return 0; }

    LODFile* lod_file;
};

}

#endif  /* HOMM3_RESOURCEMANAGER_FILE_ADAPTER_H */
