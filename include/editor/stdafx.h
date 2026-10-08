// stdafx.h - the editor's common header (Loki h3maped), included first by
// every editor object. Only the part the model objects prove is here: each
// object's static initializer builds the ten terrain masks of terrain.h
// (bitset<10>(1) << K, K = 0..9) before its own statics. The MFC-shaped
// declarations the Loki port also puts here (CWnd::MoveWindow's assert
// text is an orphan string in every object) belong with the GTK shim.
#ifndef HOMM3_EDITOR_STDAFX_H
#define HOMM3_EDITOR_STDAFX_H

#include "terrain.h"

#endif  /* HOMM3_EDITOR_STDAFX_H */
