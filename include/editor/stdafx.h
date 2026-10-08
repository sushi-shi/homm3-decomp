// stdafx.h - the map editor's common header, included first by every
// editor object (C:\Dev\Heroes 3 Exp 2\Editor\). An MFC 4.2 AppWizard
// application: the static SP3 NAFXCW library supplies CWinApp, the frame,
// document and view classes, the property sheets and the OLE control
// container that InitInstance enables. Loki's port replaced these headers
// with a GTK+ shim under the same name.
//
// Every editor object's static initializer builds the ten terrain masks of
// terrain.h (h3maped GameMap.cpp 0x41e5c5: ten guarded bitset<10>(1) << K
// bodies), so the header includes it, as Loki's does.
#ifndef HOMM3_EDITOR_STDAFX_H
#define HOMM3_EDITOR_STDAFX_H

#define VC_EXTRALEAN

#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#include <afxdtctl.h>
#include <afxcmn.h>

#include <bitset>
#include <string>
#include <vector>

// The editor's source spells the library's names without std:: (Loki's
// port compiles the same text against SGI's global names).
using namespace std;

#include "terrain.h"
#include "exceptions.h"

// The sized integer names of the editor's source: Loki's
// __PRETTY_FUNCTION__ texts spell uword (Tile.cpp's drawing functions take
// a uword* buffer) and assert texts spell numeric_limits< ubyte >.
typedef unsigned char ubyte;
typedef unsigned short uword;

// A 15-bit (5:5:5) editor colour: TGUIGameObject::miniMapColor returns it.
typedef uword TColor;

#endif  /* HOMM3_EDITOR_STDAFX_H */
