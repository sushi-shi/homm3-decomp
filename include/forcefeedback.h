// forcefeedback.h - the Immersion iFeel surface ForceFeedback.obj links
// against, and the two force-feedback objects that compiland defines.
//
// COMPILAND NAME IS RETAIL RTTI, NOT A GUESS. The two throw records in
// this block publish their type descriptors: 0x6778c0 reads
// `.?AVt_initialize_failure@t_initializer@?%C:\Dev\Heroes 3 Exp 2\Game\
// ForceFeedback.cpp210603558@@` and 0x65f2b0 reads
// `.?AVt_create_failure@t_enclosure@force_feedback@@`. So the source file
// is ForceFeedback.cpp, `t_initializer` lives in an unnamed namespace
// (that is what the `?%<path><number>` scope spells) and the enclosure
// wrapper is `force_feedback::t_enclosure`. Everything else here is
// role-derived and provisional - the Dreamcast build carries no Immersion
// layer, so no CodeView row attests any of it.
//
// The Immersion classes themselves are NOT ours: IFC20.dll exports them
// and the import table publishes their decorated names verbatim (IAT
// 0x63a03c..0x63a0a0). Their declarations below are transcribed from
// those manglings - return types, parameter lists and access all read
// straight off the decoration - and the virtual ROSTER ORDER is fixed by
// the two client-side vftables retail generates for them, 0x63e618
// (CImmMouse) and 0x63e640 (CImmEnclosure), whose slots are `jmp
// dword ptr [__imp_...]` thunks in vtable order.
#ifndef HOMM3_FORCEFEEDBACK_H
#define HOMM3_FORCEFEEDBACK_H

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <windows.h>

#include <va.h>

// Opaque Immersion types: named only by the import decorations that
// mention them (`PAUIFeelit@@`, `PAUIFeelitDevice@@`, `ABUFEELIT_EFFECT@@`),
// so they stay incomplete here.
struct IFeelit;
struct IFeelitDevice;
struct FEELIT_EFFECT;

class CImmProject;

// The error-policy singleton. `?m_dwErrHandlingFlags@CIFCErrors@@0KA` is
// the ONLY member the image touches, and the trailing `0KA` types it as a
// PRIVATE static unsigned long - which is why the initializer below is a
// friend rather than the member being spelled public.
class CIFCErrors {
private:
    __declspec(dllimport) static unsigned long m_dwErrHandlingFlags;
    friend class t_ifc_errors_writer;
};

// The device handle CImmMouse is passed as. No import mentions a member
// of it, and every conversion in the image is a no-op (the same pointer
// value reaches `?Initialize@CImmEnclosure@@...PAVCImmDevice@@...` and
// `?CreateEffect@CImmProject@@...`), so it is modelled as the empty first
// base it has to be for that to hold.
class CImmDevice {
};

// Client-side vftable 0x63e618, slot for slot:
//   +0x00 `??_G` (0x4b6700, generated here - the dtor is imported)
//   +0x04 GetAPI          +0x08 GetDevice
//   +0x0c ChangeScreenResolution                +0x10 SwitchToAbsoluteMode
//   +0x14 prepare_device  +0x18 reset
// The last two are `MAE` in the import table - PROTECTED virtuals - and
// they sit after the public ones, which is declaration order.
class __declspec(dllimport) CImmMouse : public CImmDevice {
public:
    CImmMouse();
    virtual ~CImmMouse();
    virtual IFeelit* GetAPI();
    virtual IFeelitDevice* GetDevice();
    virtual int ChangeScreenResolution(int mode, unsigned long width,
                                       unsigned long height);
    virtual int SwitchToAbsoluteMode(int absolute);
    int Initialize(void* hInstance, void* hwnd, unsigned long flags);

protected:
    virtual int prepare_device();
    virtual void reset();
};

// The compound effect a project hands back. Non-virtual throughout
// (`?Start@CImmCompoundEffect@@QAEHKK@Z` is `QAE`).
class __declspec(dllimport) CImmCompoundEffect {
public:
    int Start(unsigned long iterations, unsigned long flags);
};

// The effect base CImmEnclosure overrides into. Its own two virtuals sit
// at 0x63e640+0x0c and +0x10 UNREPLACED, which is what proves the split:
// a flat class would have put CImmEnclosure's own Stop/Start there.
class __declspec(dllimport) CImmEffect {
public:
    virtual ~CImmEffect();
    virtual int GetIsCompatibleGUID(GUID& guid);
    virtual int Initialize(CImmDevice* device, const FEELIT_EFFECT& effect,
                           unsigned long flags);
    virtual int InitializeFromProject(CImmProject& project, const char* name,
                                      CImmDevice* device, unsigned long flags);
    virtual int Start(unsigned long iterations, unsigned long flags,
                      int priority);
};

// Client-side vftable 0x63e640: `??_G` (0x4b6c30), then the two overrides
// of CImmEffect above, then CImmEffect's own two, then the two new
// virtuals. `?Initialize@CImmEnclosure@@QAEH...` - the thirteen-argument
// rectangle form - is a separate NON-virtual overload; the decoration
// (`QAE` against the virtual `UAE`) is what separates them.
class __declspec(dllimport) CImmEnclosure : public CImmEffect {
public:
    CImmEnclosure();
    virtual ~CImmEnclosure();
    virtual int GetIsCompatibleGUID(GUID& guid);
    virtual int Initialize(CImmDevice* device, const FEELIT_EFFECT& effect,
                           unsigned long flags);
    virtual int Stop();
    virtual int Start(unsigned long iterations);
    int Initialize(CImmDevice* device, const RECT* rect, long a, long b,
                   unsigned long c, unsigned long d, unsigned long e,
                   unsigned long f, unsigned long g, unsigned long h,
                   CImmEffect* effect, long i, unsigned long j);
    int SetRect(const RECT* rect);
};

// The project file. Its constructor is NOT in the import table and
// retail inlines it as four zero stores over the sixteen bytes `new`
// buys, so it is an inline in the vendor header - modelled here as the
// four pointer-width members that zeroing writes.
class CImmProject {
public:
    CImmProject() : m_field0(0), m_field4(0), m_field8(0), m_fieldC(0) {}
    __declspec(dllimport) ~CImmProject();
    __declspec(dllimport) int LoadProjectFromMemory(void* data,
                                                    CImmDevice* device);
    __declspec(dllimport) void Close();
    __declspec(dllimport) CImmCompoundEffect* CreateEffect(
        const char* name, CImmDevice* device, unsigned long flags);
    __declspec(dllimport) void DestroyEffect(CImmCompoundEffect* effect);

private:
    void* m_field0;
    void* m_field4;
    void* m_field8;
    void* m_fieldC;
};

// --- ForceFeedback.obj's own objects ---

// The window origin the enclosure rectangles are kept relative to, and
// the enclosure->rectangle map ImmMouseWindowMoved walks. Retail loads
// the map's `_Head` at 0x696d64; VC6's Dinkumware map places that field
// at object +4, which fixes the object base at 0x696d60.
DATA(0x00696d60)
extern std::map<CImmEnclosure*, RECT> gImmEffectEntries;
DATA(0x00696d70) extern long gImmWindowX;
DATA(0x00696d74) extern long gImmWindowY;
DATA(0x00696d7c) extern HWND gImmWindow;

// The three singletons the initializer publishes: the mouse (handed out
// as the device everywhere), the loaded project, and the effect currently
// playing. PlayImmEffect destroys the previous effect before creating the
// next, so the last is a single slot rather than a set.
DATA(0x00696d80) extern CImmDevice* gImmDevice;
DATA(0x00696d84) extern CImmProject* gImmProject;
DATA(0x00696d88) extern CImmCompoundEffect* gImmEffect;

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

// The combat-spell rumble. A /Gr free function - name in ECX, iteration
// count in EDX - whose one decoded caller is combatManager::PowEffect
// (0x468990), which hands it akSpellEffectTraits[effect].m_immName and 1
// and then discards the result. The RETURN is a byte: the first early
// exit is `xor al,al` against `mov eax,1` on the success path, which no
// int-returning body can emit.
unsigned char PlayImmEffect(const char* effectName, int count);  // 0x4b69f0

#endif  /* HOMM3_FORCEFEEDBACK_H */
