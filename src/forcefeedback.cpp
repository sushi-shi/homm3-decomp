// forcefeedback.cpp - the Immersion iFeel integration, retail-only.
//
// COMPILAND PROVEN BY RTTI, not by link order: the two throw records in
// this block name their own source file. 0x64cc18's type descriptor
// (0x6778c0) reads `.?AVt_initialize_failure@t_initializer@?%C:\Dev\
// Heroes 3 Exp 2\Game\ForceFeedback.cpp210603558@@` and 0x64cde8's
// (0x65f2b0) reads `.?AVt_create_failure@t_enclosure@force_feedback@@`.
// That is the whole span between font.obj's tail and game.obj's
// InitializeRandomTavernText - the `font..game` admission bracket - and
// every import it uses comes from IFC20.dll (IAT 0x63a03c..0x63a0a0).
//
// The Dreamcast build has no Immersion layer at all, so NOTHING here is
// attested by a CodeView row: the two class names above are retail's own,
// everything else is role-derived and provisional.
//
// game.obj holds three more bodies of this compiland (InitImmMouse
// 0x4b6890, ImmMouseWindowMoved 0x4b6950, ~TImmMouseEffect 0x4b6e40).
// They were claimed there before the compiland was identified and are
// left where they are: their claims are banked and moving them would buy
// nothing but a rename.
#include <va.h>

#include <fstream>
#include <string>

#include "forcefeedback.h"
#include "imm_mouse.h"
#include "resourcemanager.h"

// .bss 0x696d60..0x696d90. The map is constructed by retail's own cinit
// at 0x4b61b0 (excluded class, never claimed as source); its `_Nil` and
// `_Nilrefs` statics live at 0x696d8c/0x696d90.
DATA(0x00696d60)
std::map<CImmEnclosure*, RECT> g_immEffectEntries;
DATA(0x00696d70) long g_immWindowX;
DATA(0x00696d74) long g_immWindowY;
DATA(0x00696d7c) HWND g_immWindow;
DATA(0x00696d80) CImmDevice* g_immDevice;
DATA(0x00696d84) CImmProject* g_immProject;
DATA(0x00696d88) CImmCompoundEffect* g_immEffect;

// COMDAT pairing: `??_Gt_create_failure`, the scalar deleting destructor
// slot 0 of the throw vftable at 0x63e634 points at. It sits far ahead of
// the rest of this compiland, in the advmgr..advspells gap, which is
// ordinary COMDAT placement; the vftable reference is what owns it.
VA_COMPGEN(0x0041bed0, 0x21, SCALAR_DELETING_DTOR, t_create_failure)

// The Immersion layer's whole bring-up, and ONE function: the carve split
// it into three entries plus a two-byte hole, which config/
// retail-functions.tsv now merges (see its 2026-09-05 note). Retail's
// `catch (...)` funclet sits INSIDE the emitted body, between the try's
// `jmp` past it and the continuation the funclet returns the address of.
//
// Retail's own class name is `t_initializer`, in ForceFeedback.cpp's
// unnamed namespace - see imm_mouse.h, where the role name and the nested
// throw type are documented.
// Before normalization (locals): hInst.
VA(0x004b6260, 0x462)  // anchor-import (CImmMouse::Initialize) + anchor-rtti, retail-only
TImmMouseRuntime::TImmMouseRuntime(void* instance, void* hwnd)
{
    g_immWindow = static_cast<HWND>(hwnd);
    g_immWindowX = 0;
    g_immWindowY = 0;
    ClientToScreen(static_cast<HWND>(hwnd),
                   static_cast<POINT*>(static_cast<void*>(&g_immWindowX)));
    CIFCErrors::m_dwErrHandlingFlags = 1;

    std::auto_ptr<CImmMouse> mouse(new CImmMouse);
    if (!mouse->Initialize(instance, hwnd, 4))
        throw t_initialize_failure();

    std::auto_ptr<char> project;
    try {
        std::filebuf file;
        if (file.open((std::string("data\\") + "H3Shad.ifr").c_str(),
                      std::ios_base::in | std::ios_base::binary) == 0)
            throw t_initialize_failure();
        int size = file.pubseekoff(0, std::ios_base::end);
        file.pubseekoff(0, std::ios_base::beg);
        project = std::auto_ptr<char>(new char[size]);
        file.sgetn(project.get(), size);
    } catch (t_initialize_failure) {
        LODFile* resource = ResourceManager::pointToBitmapResource("H3Shad.ifr");
        if (resource == 0)
            throw t_initialize_failure();
        int size = ResourceManager::getBitmapResourceSize("H3Shad.ifr");
        project = std::auto_ptr<char>(new char[size]);
        ResourceManager::readFromBitmapResource(resource, project.get(), size);
    }

    std::auto_ptr<CImmProject> immProject(new CImmProject);
    if (!immProject->LoadProjectFromMemory(project.get(), mouse.get()))
        throw t_initialize_failure();
    g_immDevice = mouse.release();
    g_immProject = immProject.release();
}

// COMDAT pairings the constructor above forces out: the throw type's own
// scalar deleting destructor and the copy constructor `throw` needs
// (CatchableType 0x64cc68 names the latter at exactly this address), and
// the client-side `??_G` for the dllimported CImmMouse - slot 0 of the
// vftable at 0x63e618, the same shape as CImmEnclosure's at 0x4b6c30.
VA_COMPGEN(0x004b66d0, 0x21, SCALAR_DELETING_DTOR, t_initialize_failure)
VA_COMPGEN(0x004b6700, 0x22, SCALAR_DELETING_DTOR, CImmMouse)
VA_COMPGEN(0x004b6730, 0x157, IMPLICIT_COPY_CTOR, t_initialize_failure)

// The combat-spell rumble. Destroys whatever effect is still loaded,
// creates the named one against the default device and starts it for
// `count` iterations. The first guard returns a BYTE (`xor al,al`) while
// the two later exits clear the WHOLE register and materialize `1` as a
// dword - that split is the tell for a trailing `&&`: VC6 gives the
// short-circuit its own int-width `mov eax,1` / `xor eax,eax` pair and
// narrows for free at the return, where three separate `return`s put a
// byte zero at every exit (73.16%) and an if/return-1 pair merges all
// three (87.50%).
VA(0x004b69f0, 0x5B)  // anchor-import (CImmProject::CreateEffect), retail-only
unsigned char PlayImmEffect(const char* effectName, int count)
{
    if (g_immProject == 0)
        return 0;
    if (g_immEffect != 0)
        g_immProject->DestroyEffect(g_immEffect);
    g_immEffect = g_immProject->CreateEffect(effectName, 0, 0);
    return g_immEffect != 0 && g_immEffect->Start(count, 0) != 0;
}

// One tracked enclosure. `new CImmEnclosure` lands in the auto_ptr member
// straight from the new-expression, which is what puts the vftable store
// (client-side 0x63e640) and auto_ptr's `_Owns(_P != 0), _Ptr(_P)` pair
// back to back; the rectangle is copied into a local and offset by the
// window origin BEFORE Initialize sees it, and the same offset copy is
// what goes into the map.
VA(0x004b6a50, 0x185)  // anchor-import (CImmEnclosure::Initialize), retail-only
force_feedback::t_enclosure::t_enclosure(const RECT* rect, long a,
                                         unsigned long b, unsigned long c,
                                         unsigned char d, unsigned char e)
    : m_enclosure(new CImmEnclosure)
{
    RECT bounds = *rect;
    OffsetRect(&bounds, g_immWindowX, g_immWindowY);
    if (!m_enclosure->Initialize(g_immDevice, &bounds, a, a, b, b, c, c,
                                 (d ? 0x66 : 0) | (e ? 0x99 : 0), 0, 0, 0, 0))
        throw t_create_failure();
    // `make_pair`, not `value_type(...)`: the deduced pair's copy is what
    // orders the five stores right/top/left/bottom behind `first`
    // (99.99% against 97.98% for either explicit pair spelling).
    g_immEffectEntries.insert(std::make_pair(m_enclosure.get(), bounds));
}

// COMDAT pairing: the client-side scalar deleting destructor for the
// dllimported CImmEnclosure - slot 0 of the vftable at 0x63e640 - and the
// compiler-generated copy constructor the `throw t_create_failure()`
// above forces out (CatchableType 0x64cde8 names it at exactly this
// address, with sizeOrOffset 0x1c).
VA_COMPGEN(0x004b6c30, 0x22, SCALAR_DELETING_DTOR, CImmEnclosure)
VA_COMPGEN(0x004b6c60, 0x157, IMPLICIT_COPY_CTOR, t_create_failure)

// The holder TAdventureMapWindow owns at +0x9c. `operator new(8)` for the
// enclosure wrapper, then auto_ptr's own two stores - the EH frame is the
// new-expression's, not the constructor body's.
VA(0x004b6dc0, 0x74)  // anchor-callee (0x401400), retail-only
TImmMouseEffect::TImmMouseEffect(const RECT* rect, long a, unsigned long b,
                                 unsigned long c, unsigned char d,
                                 unsigned char e)
    : m_impl(new force_feedback::t_enclosure(rect, a, b, c, d, e))
{
}

// Two forwarders through both auto_ptrs to the enclosure's own virtuals -
// slot +0x18 (`?Start@CImmEnclosure@@UAEHK@Z`) and slot +0x14
// (`?Stop@CImmEnclosure@@UAEHXZ`) of the client vftable at 0x63e640.
VA(0x004b6f30, 0x13)  // anchor-vtable (0x63e640+0x18), retail-only
unsigned char TImmMouseEffect::Start()
{
    unsigned char started = m_impl->m_enclosure->Start(0) != 0;
    return started;
}

VA(0x004b6f50, 0xB)  // anchor-vtable (0x63e640+0x14), retail-only
void TImmMouseEffect::Stop()
{
    m_impl->m_enclosure->Stop();
}

// COMDAT pairings for the enclosure map. The constructor is the one
// retail's own cinit at 0x4b61b0 calls on 0x696d60; the rest are the
// Dinkumware red-black-tree members the insert above and game.obj's
// erase reach, and all three sizes are exactly the ones the
// map<int, type_map_hero_info> instantiation in game.obj carries
// (0x115 / 0x2F9 / 0xB3), which is the cross-check that they are the
// same members of a different instantiation. `_Inc`, `_Erase`,
// `_Lbound`, `_Ubound` and both `erase` overloads of this same tree are
// already claimed in game.cpp.
VA_COMPGEN(0x004b6f60, 0xBE, CLASS_CTOR, map)
// This TU emits four `~auto_ptr<T>` COMDATs - CImmEnclosure and
// CImmMouse at 19 bytes each, char at 16 and CImmProject at 33 - and
// all four key to one `auto_ptr_auto_ptr@dtor` group, so the owner
// names the INSTANTIATION `<Element>_<template>` the way the shared
// vector-constructor group's `hero_vector` claims do. Retail ICF
// folded the two 19-byte twins, which is why the length fallback
// alone cannot decide this one.
VA_COMPGEN(0x004b7020, 0x13, IMPLICIT_DTOR, CImmEnclosure_auto_ptr)
// ...and the other two of the four, both proven by the callee each one
// reaches: 0x4b7040 falls straight through to the free `operator delete`
// (the char instantiation - no element destructor to run), while 0x4b7050
// calls the dllimported `??1CImmProject` first and only then frees. The
// element oracle binds each by the instantiation its own owner names;
// their 16- and 33-byte extents match the two base COMDATs exactly, which
// is the same answer the length fallback would give.
VA_COMPGEN(0x004b7040, 0x10, IMPLICIT_DTOR, char_auto_ptr)
VA_COMPGEN(0x004b7050, 0x21, IMPLICIT_DTOR, CImmProject_auto_ptr)
// COMDAT pairing: basic_filebuf<char>::close, the one <fstream> member of
// this TU with no claim key until now. TImmMouseRuntime's constructor reads
// H3Shad.ifr through an ifstream, and 0x4b7400 is the `close` that ends it:
// `fclose` on the FILE* at +0x50, then basic_streambuf::_Init()'s six
// self-referential pointer stores (+0xc->+4, +0x1c->+0x14, +0x20->+0x18,
// +0x10->+8, +0x2c->+0x24, +0x30->+0x28) zeroed through, then the two
// locale words from the shared _Stinit. Agreement 0.625 against this
// object's own `?close@?$basic_filebuf@D...` COMDAT, which nothing else in
// the unit resembles.
VA_COMPGEN(0x004b7400, 0x9C, FILEBUF_CLOSE, char)
VA_COMPGEN(0x004b70e0, 0x115, TREE_INSERT, CImmEnclosure)
VA_COMPGEN(0x004b7a50, 0x2F9, TREE_NODE_INSERT, CImmEnclosure)
VA_COMPGEN(0x004b7db0, 0xB3, TREE_CONST_ITERATOR_DEC, CImmEnclosure)
// COMDAT pairing: std::_Construct for the map's 20-byte
// pair<CImmEnclosure* const, tagRECT>. `_Tree::_Insert` (0x4b7a50) is its
// only caller, the five-dword `rep movsd` fixes the element width, and the
// leading null test is placement new's. This TU emits exactly one
// `?_Construct@std@@...` COMDAT, so the pairing is unambiguous.
VA_COMPGEN(0x004b7fe0, 0x14, STD_CONSTRUCT, CImmEnclosure_pair)
