// forcefeedback.cpp - the Immersion iFeel integration, retail-only.

// COMPILAND PROVEN BY RTTI, not by link order: the two throw records in
// this block name their own source file. 0x64cc18's type descriptor
// (0x6778c0) reads `.?AVt_initialize_failure@t_initializer@?%C:\Dev\
// Heroes 3 Exp 2\Game\ForceFeedback.cpp210603558@@` and 0x64cde8's
// (0x65f2b0) reads `.?AVt_create_failure@t_enclosure@force_feedback@@`.
// That is the whole span between font.obj's tail and game.obj's
// InitializeRandomTavernText - the `font..game` admission bracket - and
// every import it uses comes from IFC20.dll (IAT 0x63a03c..0x63a0a0).

// The Dreamcast build has no Immersion layer at all, so NOTHING here is
// attested by a CodeView row: the two class names above are retail's own,
// everything else is role-derived and provisional.

#include "va.h"

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

// The global's registered cleanup tail-calls this map destructor.
VA_COMPGEN(0x004b61f0, 0x6e, IMPLICIT_DTOR, map)
DATA(0x00696d70) POINT g_immWindowOrigin;
DATA(0x00696d7c) HWND g_immWindow;
DATA(0x00696d80) CImmDevice* g_immDevice;
DATA(0x00696d84) CImmProject* g_immProject;
DATA(0x00696d88) CImmCompoundEffect* g_immEffect;

// COMDAT pairing: `??_Gt_create_failure`, the scalar deleting destructor of
// the thrown failure type.
VA_COMPGEN(0x0041bed0, 0x21, SCALAR_DELETING_DTOR, t_create_failure)

// unnamed namespace, restored above with its nested failure type.
VA(0x004b6260, 0x462)
t_initializer::t_initializer(void* instance, void* hwnd)
{
    g_immWindow = static_cast<HWND>(hwnd);
    g_immWindowOrigin.x = 0;
    g_immWindowOrigin.y = 0;
    ClientToScreen(static_cast<HWND>(hwnd), &g_immWindowOrigin);
    CIFCErrors::SetErrorHandling(1);

    std::auto_ptr<CImmMouse> mouse(new CImmMouse);
    if (!mouse->Initialize(instance, hwnd, 4))
        throw t_initialize_failure();

    DATA(0x0063e600) static const char shadowProjectName[] = "H3Shad.ifr";
    std::auto_ptr<char> project;
    try {
        std::filebuf file;
        if (file.open((std::string("data\\") + shadowProjectName).c_str(),
                      std::ios_base::in | std::ios_base::binary) == 0)
            throw t_initialize_failure();
        int size = file.pubseekoff(0, std::ios_base::end);
        file.pubseekoff(0, std::ios_base::beg);
        project = std::auto_ptr<char>(new char[size]);
        file.sgetn(project.get(), size);
    } catch (t_initialize_failure) {
        LODFile* resource = ResourceManager::PointToBitmapResource(shadowProjectName);
        if (resource == 0)
            throw t_initialize_failure();
        int size = ResourceManager::GetBitmapResourceSize(shadowProjectName);
        project = std::auto_ptr<char>(new char[size]);
        ResourceManager::ReadFromBitmapResource(resource, project.get(), size);
    }

    std::auto_ptr<CImmProject> immProject(new CImmProject);
    if (!immProject->LoadProjectFromMemory(project.get(), mouse.get()))
        throw t_initialize_failure();
    g_immDevice = mouse.release();
    g_immProject = immProject.release();
}

// COMDAT pairings the constructor above forces out: the throw type's own
// scalar deleting destructor and the copy constructor `throw` needs, and
// the client-side `??_G` for the dllimported CImmMouse.
VA_COMPGEN(0x004b66d0, 0x21, SCALAR_DELETING_DTOR, t_initialize_failure)

VA_COMPGEN(0x004b6700, 0x22, SCALAR_DELETING_DTOR, CImmMouse)

VA_COMPGEN(0x004b6730, 0x157, IMPLICIT_COPY_CTOR, t_initialize_failure)

// InitImmMouse: once-guarded `static <ImmWrapper> obj(hInst, hwnd)`
// construction; WinMain's post-CreateWindow callee. Construction failure is
// caught and reported as false.
VA(0x004b6890, 0x7D)
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

// immMouse's atexit destructor thunk, with the whole of `~t_initializer`
// inlined into it.
VA_COMPGEN(0x004b6910, 0x3A, STATIC_DTOR, immMouse)

// ImmMouseWindowMoved: re-derives the client origin via ClientToScreen and
// offsets every tracked effect rect in the map by the delta; AppWndProc's
// WM_MOVE callee.
VA(0x004b6950, 0x9A)
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

VA_COMPGEN(0x004B7330, 0xA3, TREE_CONST_ITERATOR_INC, CImmEnclosure)

VA(0x004b69f0, 0x5B)
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
// straight from the new-expression; the rectangle is copied into a local
// and offset by the window origin BEFORE Initialize sees it, and the same
// offset copy is what goes into the map.
VA(0x004b6a50, 0x185)
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
    g_immEffectEntries.insert(std::make_pair(m_enclosure.get(), bounds));
}

VA_COMPGEN(0x004b6c30, 0x22, SCALAR_DELETING_DTOR, CImmEnclosure)

VA_COMPGEN(0x004b6c60, 0x157, IMPLICIT_COPY_CTOR, t_create_failure)

// The holder TAdventureMapWindow owns at +0x9c; the EH frame is the
// enclosure new-expression's.
VA(0x004b6dc0, 0x74)
TImmMouseEffect::TImmMouseEffect(const RECT* rect, long a, unsigned long b,
                                 unsigned long c, unsigned char d,
                                 unsigned char e)
    : m_impl(new force_feedback::t_enclosure(rect, a, b, c, d, e))
{
}

VA(0x004b6e40, 0xE3)
TImmMouseEffect::~TImmMouseEffect()
{
}

// Two forwarders through both auto_ptrs to the enclosure's own virtuals,
// `?Start@CImmEnclosure@@UAEHK@Z` and `?Stop@CImmEnclosure@@UAEHXZ`.
VA(0x004b6f30, 0x13)
unsigned char TImmMouseEffect::start()
{
    unsigned char started = m_impl->m_enclosure->Start(0) != 0;
    return started;
}

VA(0x004b6f50, 0xB)
void TImmMouseEffect::stop()
{
    m_impl->m_enclosure->Stop();
}

// Dinkumware red-black-tree members the insert above and the holder
// teardown reach, claimed below in this same compiland.
VA_COMPGEN(0x004b6f60, 0xBE, CLASS_CTOR, map)

VA_COMPGEN(0x004b7020, 0x13, IMPLICIT_DTOR, CImmEnclosure_auto_ptr)

// ...and the other two auto_ptr destructors: the char instantiation (no
// element destructor) and the CImmProject one, which calls the dllimported
// `??1CImmProject` before freeing.
VA_COMPGEN(0x004b7040, 0x10, IMPLICIT_DTOR, char_auto_ptr)

VA_COMPGEN(0x004b7050, 0x21, IMPLICIT_DTOR, CImmProject_auto_ptr)

// COMDAT pairing: basic_filebuf<char>::close, the one <fstream> member of
// this TU; it ends t_initializer's H3Shad.ifr ifstream.
VA_COMPGEN(0x004b7400, 0x9C, FILEBUF_CLOSE, char)

VA_COMPGEN(0x004b70e0, 0x115, TREE_INSERT, CImmEnclosure)

VA_COMPGEN(0x004b7a50, 0x2F9, TREE_NODE_INSERT, CImmEnclosure)

VA_COMPGEN(0x004b7db0, 0xB3, TREE_CONST_ITERATOR_DEC, CImmEnclosure)

// COMDAT pairing: std::_Construct for the map's 20-byte
// pair<CImmEnclosure* const, tagRECT>.
VA_COMPGEN(0x004b7fe0, 0x14, STD_CONSTRUCT, CImmEnclosure_pair)

// COMDAT pairing: map<CImmEnclosure*, RECT>::erase, both overloads, emitted
// by the TImmMouseEffect destructor above and the tree destructor.
VA_COMPGEN(0x004b7200, 0x121, TREE_ERASE_RANGE, CImmEnclosure)

VA_COMPGEN(0x004b74a0, 0x50F, TREE_ERASE_ITERATOR, CImmEnclosure)

// COMDAT pairing: the rest of map<CImmEnclosure*, RECT>'s out-of-line tree
// surface, reached from the TImmMouseEffect destructor above.
VA_COMPGEN(0x004b73e0, 0x19, TREE_ITERATOR_EQUAL, CImmEnclosure)

VA_COMPGEN(0x004b79b0, 0x17, TREE_LOWER_BOUND, CImmEnclosure)

VA_COMPGEN(0x004b79d0, 0x7E, TREE_ERASE, CImmEnclosure)

VA_COMPGEN(0x004b7d50, 0x49, TREE_LBOUND, CImmEnclosure)

VA_COMPGEN(0x004b7da0, 0xE, TREE_CONST_ITERATOR_CTOR, CImmEnclosure)

VA_COMPGEN(0x004b7e70, 0x49, TREE_UBOUND, CImmEnclosure)
