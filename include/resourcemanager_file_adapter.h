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
    explicit t_stdio_file_adapter(FILE* value) : m_file(value) {}

    // Before normalization (function): ResourceManager::t_stdio_file_adapter::Read.
    virtual int read(void* data, int size);
    // Before normalization (function): ResourceManager::t_stdio_file_adapter::Write.
    virtual int write(const void*, int) { return 0; }

    // Before normalization: file.
    FILE* m_file;
};

class t_lod_file_adapter : public TAbstractFile {
public:
    explicit t_lod_file_adapter(LODFile* value) : m_lodFile(value) {}

    // Before normalization (function): ResourceManager::t_lod_file_adapter::Read.
    virtual int read(void* data, int size);
    // Before normalization (function): ResourceManager::t_lod_file_adapter::Write.
    virtual int write(const void*, int) { return 0; }

    // Before normalization: lod_file.
    LODFile* m_lodFile;
};

}

#endif  /* HOMM3_RESOURCEMANAGER_FILE_ADAPTER_H */
