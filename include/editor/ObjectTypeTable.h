// ObjectTypeTable.h - the map editor's object types (Loki h3maped
// objecttype.h's kObjectTypeTable): the rows of objects.txt, which
// ObjectType.cpp loads at start-up into the table at 0x5a2398 and binds
// this reference to (0x49291b). The palette lists every slot's types from
// it.
#ifndef HOMM3_EDITOR_OBJECTTYPETABLE_H
#define HOMM3_EDITOR_OBJECTTYPETABLE_H

#include "va.h"
#include "objecttype.h"

DATA(0x005a23a8) extern const TObjectTypeTable& kObjectTypeTable;

#endif  /* HOMM3_EDITOR_OBJECTTYPETABLE_H */
