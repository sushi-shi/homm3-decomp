// ObjectHelp.h - context help for an object type (Loki h3maped object 20;
// the file name is Loki's, not proven). The map edit window's "What's
// This" command shows the selected object's help (h3maped 0x48bac3).
#ifndef HOMM3_EDITOR_OBJECTHELP_H
#define HOMM3_EDITOR_OBJECTHELP_H

class TObjectType;

void displayObjectHelp(const TObjectType& objType);

#endif  /* HOMM3_EDITOR_OBJECTHELP_H */
