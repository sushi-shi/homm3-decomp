// ObjectTypeTable.cpp of the Loki port (Loki object 23): the editor's one
// object-type table, loaded from objects.txt.
#include "objecttype.h"

namespace {

TObjectTypeTable objectTypeTable;

}

const TObjectTypeTable& kObjectTypeTable = objectTypeTable;

void loadObjectTypeTable()
{
    objectTypeTable.load("objects.txt");
}
