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
// All Immersion bodies and their enclosure-tree instantiations are owned
// here, including the three formerly carried by game.cpp.
#include <va.h>

#include <fstream>
#include <string>

#include "forcefeedback.h"
#include "imm_mouse.h"
#include "resourcemanager.h"

namespace force_feedback {

// One tracked enclosure. Eight bytes - a `std::auto_ptr<CImmEnclosure>`,
// whose `{ bool _Owns; _Ty* _Ptr; }` layout is exactly what the
// constructor at 0x4b6a50 writes (`test eax,eax / setne cl / mov [esi],cl
// / mov [esi+4],eax` is auto_ptr's `_Owns(_P != 0), _Ptr(_P)` verbatim)
// and what the out-of-line auto_ptr destructor at 0x4b7020 reads back.
class t_enclosure {
public:
    // `.?AVt_create_failure@t_enclosure@force_feedback@@` (0x65f2b0), a
    // 28-byte runtime_error with no members of its own: its CatchableType
    // array 0x64ce08 lists exactly {itself, runtime_error, exception} at
    // sizes 28/28/12, and the throw at 0x4b6b8c hands the base a
    // DEFAULT-constructed string.
    class t_create_failure : public std::runtime_error {
    public:
        t_create_failure() : std::runtime_error(std::string()) {}
    };

    t_enclosure(const RECT* rect, long a, unsigned long b, unsigned long c,
                unsigned char d, unsigned char e);
    ~t_enclosure();

    std::auto_ptr<CImmEnclosure> m_enclosure;
};

}  // namespace force_feedback

// Inlined into the holder's destructor as retail's only copy: the erase
// runs unconditionally on the enclosure key, and the delete is the
// auto_ptr member's own scope exit, guarded by the flag at +0.
inline force_feedback::t_enclosure::~t_enclosure()
{
    g_immEffectEntries.erase(m_enclosure.get());
}

namespace {
class t_initializer {
public:
    // `.?AVt_initialize_failure@t_initializer@...@@` (0x6778c0), a 28-byte
    // runtime_error with no members of its own - the same shape as
    // t_enclosure::t_create_failure, and thrown the same way, with a
    // DEFAULT-constructed string handed to the base.
    class t_initialize_failure : public std::runtime_error {
    public:
        t_initialize_failure() : std::runtime_error(std::string()) {}
    };

    // Retail 0x4b6260, 1122 bytes: the window origin, the iFeel error
    // policy, the mouse device, the effect project read from H3Shad.ifr
    // (with a LOD fallback in its catch), and the three globals it
    // publishes.
    // Before normalization (locals): hInst.
    t_initializer(void* instance, void* hwnd);
    // Retail's atexit thunk at 0x4b6910 - the address InitImmMouse hands to
    // _atexit, 58 B - is this destructor EXPANDED, so it is defined inline
    // here.  It touches no member: the holder is the eight bytes at
    // 0x696d78 and the two singletons sit AFTER it at 0x696d84 and
    // 0x696d80.  `delete gImmProject` is the non-virtual imported dtor plus
    // operator delete; `delete gImmDevice` is the virtual scalar deleting
    // destructor (`push 1 / call [eax]`).  Because the body names no
    // member, the `$E<n>` thunk carries no relocation to the owned datum -
    // see canonicalize_data_symbols' owner-free static-destructor arm and
    // its control in build/test_ownerless_static_dtor.py.
    ~t_initializer()
    {
        g_immProject->Close();
        delete g_immProject;
        delete g_immDevice;
    }
};

} // unnamed namespace

// .bss 0x696d60..0x696d90. The map is constructed by retail's own cinit
// at 0x4b61b0 (excluded class, never claimed as source); its `_Nil` and
// `_Nilrefs` statics live at 0x696d8c/0x696d90.
DATA(0x00696d60)
std::map<CImmEnclosure*, RECT> g_immEffectEntries;
DATA(0x00696d70) POINT g_immWindowOrigin;
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
// unnamed namespace, restored above with its nested failure type.
// Before normalization (locals): hInst.
VA(0x004b6260, 0x462)  // anchor-import (CImmMouse::Initialize) + anchor-rtti, retail-only
t_initializer::t_initializer(void* instance, void* hwnd)
{
    g_immWindow = static_cast<HWND>(hwnd);
    g_immWindowOrigin.x = 0;
    g_immWindowOrigin.y = 0;
    ClientToScreen(static_cast<HWND>(hwnd), &g_immWindowOrigin);
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

// InitImmMouse: once-guarded `static <ImmWrapper> obj(hInst, hwnd)`
// construction (guard byte 0x696d58, atexit dtor thunk 0x4b6910);
// WinMain's post-CreateWindow callee. Construction failure is caught and
// reported as false; 0x4b68f4/0x4b68fa are the EH handler/catch funclet.
// Before normalization (locals): hInst.
VA(0x004b6890, 0x7D)  // anchor-callee + contiguous catch funclets, retail-only
unsigned char initImmMouse(void* instance, void* hwnd)
{
    try {
        DATA_COMPGEN_GUARD(0x00696d58, immMouseGuard, immMouse)
        DATA(0x00696d78)
        static t_initializer immMouse(instance, hwnd);
        return 1;
    } catch (t_initializer::t_initialize_failure) {
        return 0;
    }
}

// The exit thunk _atexit receives from InitImmMouse above: retail expanded
// the whole of `~t_initializer` into it, so the 58 bytes are the two
// singleton teardowns and nothing of the holder itself. The claim binds
// through canonicalize_data_symbols' owner-free static-destructor arm -
// nothing in the body relocates `immMouse`, because the unused `this` went
// with the expansion (build/test_ownerless_static_dtor.py is its control).
VA_COMPGEN(0x004b6910, 0x3A, STATIC_DTOR, immMouse)

// ImmMouseWindowMoved: re-derives the client origin via
// ClientToScreen and offsets every tracked effect rect (linked list
// in the map whose _Head is at 0x696d64) by the delta; AppWndProc's WM_MOVE
// callee.
VA(0x004b6950, 0x9A)  // anchor-callee, retail-only
void immMouseWindowMoved()
{
    POINT origin = { 0, 0 };
    ClientToScreen(g_immWindow, &origin);

    long dx = origin.x - g_immWindowOrigin.x;
    long dy = origin.y - g_immWindowOrigin.y;
    if (dx == 0 && dy == 0)
        return;

    g_immWindowOrigin.x = origin.x;
    g_immWindowOrigin.y = origin.y;
    for (std::map<CImmEnclosure*, RECT>::iterator it = g_immEffectEntries.begin();
         it != g_immEffectEntries.end(); ++it) {
        CImmEnclosure* enclosure = it->first;
        RECT* rect = &it->second;
        OffsetRect(rect, dx, dy);
        enclosure->SetRect(rect);
    }
}

// The loop above retains VC6's real map<CImmEnclosure*, RECT> tree-successor
// COMDAT. Retail 0x4b7330 has the same nine blocks and 0xa3 bytes; its node
// consumer independently fixes the pair at +0x0c/+0x10 and `_Nil` at
// 0x696d8c. Dreamcast's generic STLport _M_increment at dc 0x64214
// corroborates the helper boundary; the Immersion integration is retail-only.
VA_COMPGEN(0x004B7330, 0xA3, TREE_CONST_ITERATOR_INC, CImmEnclosure)

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
// Before normalization (function): PlayImmEffect.
unsigned char playImmEffect(const char* effectName, int count)
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
    OffsetRect(&bounds, g_immWindowOrigin.x, g_immWindowOrigin.y);
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

// TAdventureMapWindow's slot-2 Close override (0x4014d0) reaches this body as
// `mov ecx,edi / call 0x4b6e40 / push edi / call operator delete` on its owned
// +0x9c member - the split form of `delete`, which types 0x4b6e40 as that
// member's destructor. The implementation destructor is inlined here as
// retail's only copy, which is what puts the whole
// map<CImmEnclosure*, RECT>::erase(const key_type&) chain in this body:
// _Ubound out of line at 0x4b7e70, lower_bound at 0x4b79b0, the discarded
// _Distance walk over the claimed const_iterator::_Inc, and erase(first, last)
// at 0x4b7200. The `_Pairii(lower_bound, upper_bound)` argument pair evaluates
// right to left, so upper_bound lands first; the count _Distance returns is
// dead because the caller drops erase's return value.
//
// Residual (44.8%): retail carries a C++ EH frame here and this compile does
// not, which costs the prologue, the epilogue and the two out-of-line
// iterator helpers retail calls (the 14-byte iterator ctor at 0x4b7da0 and
// the 25-byte operator== at 0x4b73e0, both expanded here). The frame is not
// decoration - it is the whole residual, and retail's own EH data says what
// produces it. FuncInfo 0x64cec8 has one unwind state whose action (0x62b6a0)
// is `mov ecx,[ebp-0x20] / jmp 0x4b7020`, handing the IMPLEMENTATION pointer
// to a 19-byte body that is only `if (+0) { if (+4) delete +4; }`. An
// unwind that DESTROYS a sub-object is not what a delete-expression emits;
// it is what a destructor body emits while a base or member is still alive.
// So the implementation is really two levels - a sub-object holding the
// owned-flag/enclosure pair, and a derived body holding the erase - and
// 0x4b7020 is that sub-object's own destructor.
//
// 2026-09-05: the sub-object is `std::auto_ptr<CImmEnclosure>`, and the
// holder here is `std::auto_ptr<force_feedback::t_enclosure>`. Both are
// byte-proven from ForceFeedback.obj's own constructors: 0x4b6dc0 and
// 0x4b6a50 each buy their pointee and then write `(p != 0)` and `p` in
// that order, which is auto_ptr's `_Owns(_P != 0), _Ptr(_P)` verbatim,
// and 0x4b7020 / 0x4b7040 / 0x4b7050 are the three out-of-line
// `~auto_ptr` bodies (CImmEnclosure, char, CImmProject) the same
// compiland emits. The earlier "TRIED AND REJECTED: splitting the class
// in two scores 14.62" measurement was taken with a HAND-WRITTEN
// sub-object; a real std::auto_ptr member is a different inline
// candidate, so the body below is now just the implicit member teardown.
VA(0x004b6e40, 0xE3)  // anchor-callee (TAdventureMapWindow::Close), retail-only
TImmMouseEffect::~TImmMouseEffect()
{
}

// Two forwarders through both auto_ptrs to the enclosure's own virtuals -
// slot +0x18 (`?Start@CImmEnclosure@@UAEHK@Z`) and slot +0x14
// (`?Stop@CImmEnclosure@@UAEHXZ`) of the client vftable at 0x63e640.
VA(0x004b6f30, 0x13)  // anchor-vtable (0x63e640+0x18), retail-only
// Before normalization (function): TImmMouseEffect::Start.
unsigned char TImmMouseEffect::start()
{
    unsigned char started = m_impl->m_enclosure->Start(0) != 0;
    return started;
}

VA(0x004b6f50, 0xB)  // anchor-vtable (0x63e640+0x14), retail-only
// Before normalization (function): TImmMouseEffect::Stop.
void TImmMouseEffect::stop()
{
    m_impl->m_enclosure->Stop();
}

// COMDAT pairings for the enclosure map. The constructor is the one
// retail's own cinit at 0x4b61b0 calls on 0x696d60; the rest are the
// Dinkumware red-black-tree members the insert above and the holder
// teardown reach, and all three sizes are exactly the ones the
// map<int, type_map_hero_info> instantiation in game.obj carries
// (0x115 / 0x2F9 / 0xB3), which is the cross-check that they are the
// same members of a different instantiation. `_Inc`, `_Erase`,
// `_Lbound`, `_Ubound` and both `erase` overloads of this same tree are
// claimed below in this same compiland.
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
// this TU with no claim key until now. t_initializer's constructor reads
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

// COMDAT pairing: map<CImmEnclosure*, RECT>::erase, both overloads, newly
// emitted by the TImmMouseEffect destructor above. The chain is closed on
// both sides: the range overload at 0x4b7200 is reached from that destructor
// (0x4b6eea) and from the already-claimed tree destructor at 0x4b61f0
// (0x4b6204, `erase(begin(), end())`), and it is the ONLY caller of 0x4b74a0
// (0x4b72fd) - exactly `while (_F != _L) erase(_F++)`. Sizes corroborate
// independently: resourcemanager's TCacheMapKey tree, the other pointer-keyed
// map in the tree, carries this same pair at 0x50F and 0x121.
VA_COMPGEN(0x004b7200, 0x121, TREE_ERASE_RANGE, CImmEnclosure)
VA_COMPGEN(0x004b74a0, 0x50F, TREE_ERASE_ITERATOR, CImmEnclosure)

// COMDAT pairing: the rest of map<CImmEnclosure*, RECT>'s out-of-line tree
// surface, reached from the TImmMouseEffect destructor above. _Lbound and
// _Ubound are the same 73 bytes and differ only in which way round they
// compare, which is exactly how <xtree> writes them and is decisive here:
// 0x4b7d50 is `cmp [node+0xc], key / jae` = `key_compare(_Key(_X), _Kv)`,
// _Lbound's polarity, and 0x4b7e70 is `cmp key, [node+0xc] / jae` =
// `key_compare(_Kv, _Key(_X))`, _Ubound's. The call graph corroborates both
// independently: 0x4b7d50 has exactly one caller, the out-of-line lower_bound
// at 0x4b79b0, while 0x4b7e70 is called straight from the destructor, which
// is where upper_bound is expanded. _Erase is erase(iterator, iterator)'s
// whole-tree arm.
//
// NOT claimed here: 0x4b6f60, the 190-byte map<int, type_map_hero_info>
// constructor this unit also emits. The name is already proven at 0x45bf70 in
// campaignbrief, so retail carries two un-folded copies of one COMDAT and only
// the first can hold the label; a second claim is refused as a duplicate
// proven name, which is the delinker working correctly.
// COMDAT pairing: the enclosure map's nested-iterator surface, the last
// three out-of-line rows of this tree. Each is byte-identical to the COMDAT
// this object emits and each is corroborated from the call graph rather
// than from length alone, which decides nothing at 25/23/14 bytes:
//   0x4b73e0  iterator::operator==   `mov eax,[ecx] / cmp eax,[edx] / sete`
//             - the _Node* compare, and the only `??8` this tree emits;
//   0x4b79b0  lower_bound            - the out-of-line wrapper, whose ONLY
//             callee is _Lbound at 0x4b7d50 (claimed above, and 0x4b7d50's
//             only caller in turn), storing the node into the hidden return;
//   0x4b7da0  const_iterator(_Node*) - the one-argument iterator ctor,
//             `ret 4` storing its argument at +0.
VA_COMPGEN(0x004b73e0, 0x19, TREE_ITERATOR_EQUAL, CImmEnclosure)
VA_COMPGEN(0x004b79b0, 0x17, TREE_LOWER_BOUND, CImmEnclosure)
VA_COMPGEN(0x004b79d0, 0x7E, TREE_ERASE, CImmEnclosure)
VA_COMPGEN(0x004b7d50, 0x49, TREE_LBOUND, CImmEnclosure)
VA_COMPGEN(0x004b7da0, 0xE, TREE_CONST_ITERATOR_CTOR, CImmEnclosure)
VA_COMPGEN(0x004b7e70, 0x49, TREE_UBOUND, CImmEnclosure)
