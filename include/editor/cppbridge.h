// cppbridge.h - the helpers cppbridge.cpp (Loki h3maped object 51) exports
// to the dialogs: glade list, toggle and sensitivity helpers and the
// all-space tests. Like _widget (stdafx.h) the C helpers are extern "C"
// inside the anonymous namespace that holds the rest of <gtk/gtk.h>
// ("void {anonymous}::setCurrentSelection({anonymous}::GtkList *, int)");
// _isspace is an ordinary C++ function (mangled
// _isspace__FGt12basic_string...). The header name is not proven.
#ifndef HOMM3_EDITOR_CPPBRIDGE_H
#define HOMM3_EDITOR_CPPBRIDGE_H

#include "editor/stdafx.h"

#include <string>

namespace {
#include <gtk/gtk.h>

extern "C" {
bool isAllSpace(const char* text);
int getCurrentSelection(GtkList* list);
void setCurrentSelection(GtkList* l, int index);
void emptyList(GtkList* list);
bool isChecked(char* name);
void enableWidget(char* name, gboolean bEnable);
void handleQueuedEvents();
bool askYesNoQuestion(char* question);
}
}

bool _isspace(string text);

#endif  /* HOMM3_EDITOR_CPPBRIDGE_H */
