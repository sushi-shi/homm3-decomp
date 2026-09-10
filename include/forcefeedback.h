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
#include "../vendor/ifc-2.0.3/orig/FeelitAPI.h"

#include <va.h>

// Binary API descriptors use the pristine vendor boundary declarations.
// FEELIT_EFFECT (0x48) and FEELIT_ENCLOSURE (0x38) match IFC 2.0.3.
class CImmProject;

// The error-policy singleton. `?m_dwErrHandlingFlags@CIFCErrors@@0KA` is
// the ONLY member the image touches, and the trailing `0KA` types it as a
// PRIVATE static unsigned long - which is why the initializer below is a
// friend rather than the member being spelled public.
class CIFCErrors {
private:
    __declspec(dllimport) static unsigned long m_dwErrHandlingFlags;
    // The initializer at 0x4b6260 writes this private static directly
    // (`mov eax,[__imp_?m_dwErrHandlingFlags@CIFCErrors@@0KA] / mov
    // [eax],1`), which only a friend can do; the vendor header must have
    // named retail's own initializer class here.
    friend class TImmMouseRuntime;
};

// IFC 2.0.3 effect-cache layout: AddEffect at DLL RVA 0x7300 allocates
// eight-byte nodes (effect +0, next +4); list destruction at 0x72e0 walks
// those next pointers. Original SDK spellings are retained below.
class CImmEffect;
class CEffectListElement {
public:
    CEffectListElement() : m_immEffect(0), m_next(0) {}
    CImmEffect* m_immEffect;       // SDK m_pImmEffect.
    CEffectListElement* m_next;   // SDK m_pNext.
};

class CEffectList {
public:
    CEffectList() : m_firstEffect(0) {}
    __declspec(dllimport) ~CEffectList();
    CEffectListElement* m_firstEffect; // SDK m_pFirstEffect.
};

// Same-address polymorphic base, proven by the game's device conversions.
// IFC20.dll 2.0.3 constructor RVA 0x3f30 and destructor 0x3f50 establish
// the cache at +4; its vector deleting destructor uses stride 0x24.
class CImmDevice {
public:
    virtual ~CImmDevice();

protected:
    CEffectList m_cache;          // +04, SDK m_Cache.
    // Initialize +0xba6d sets this; reset +0xbb8f clears it.
    int m_initialized;            // +08, SDK BOOL m_bInitialized.
    // Exported GetDeviceType +0x6be0 returns this word.
    unsigned long m_deviceType;   // +0c, SDK m_dwDeviceType.
    // enum_devices_proc +0x42d0 copies the device instance GUID and
    // sets +20; Initialize +0xb961 passes that GUID to CreateDevice.
    GUID m_device;                // +10, SDK m_guidDevice.
    int m_guidValid;              // +20, SDK BOOL m_bGuidValid.
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
    // Before normalization (locals): hInstance.
    int Initialize(void* instance, void* hwnd, unsigned long flags);

protected:
    virtual int prepare_device();
    virtual void reset();

protected:
    // IFC20.dll exported GetAPI/GetDevice at RVAs 0x4470/0x4480
    // return +24/+28. Constructor 0xb7b0 zeros both; reset 0xbb60
    // releases their COM interfaces. Total 0x2c matches retail allocation.
    IFeelit* m_api;               // SDK m_piApi.
    IFeelitDevice* m_device;      // SDK m_piDevice.

};

// The compound effect a project hands back. Non-virtual throughout
// (`?Start@CImmCompoundEffect@@QAEHKK@Z` is `QAE`).
class __declspec(dllimport) CImmCompoundEffect {
public:
    int Start(unsigned long iterations, unsigned long flags);
};

// SDK ECacheState (ImmEffectSuite.h); external enumerator spellings.
enum ECacheState {
    IMMCACHE_NOT_ON_DEVICE,
    IMMCACHE_ON_DEVICE,
    IMMCACHE_SWAPPED_OUT
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

protected:
    // IFC20.dll 2.0.3 constructor RVA 0x48d0 initializes these slots;
    // cache helpers and exported priority/device getters fix their roles.
    ECacheState m_cacheState;     // +04, SDK m_CacheState.
    int m_inCurrentSuite;         // +08, SDK m_bInCurrentSuite.
    short m_priority;            // +0c, SDK m_Priority; +0e alignment.
    unsigned long m_lastStarted; // +10, SDK m_dwLastStarted; Start 0x5206.
    unsigned long m_lastStopped; // +14, SDK m_dwLastStopped.
    unsigned long m_lastLoaded;  // +18, SDK m_dwLastLoaded.
    CImmDevice* m_immDevice;      // +1c, SDK m_pImmDevice; GetDevice 0x1470.
    // Reset at 0x6b20 zeros 0x48 bytes and points axes/directions at the
    // following arrays. The old version has no embedded m_Envelope.
    FEELIT_EFFECT m_effect;       // +20, SDK m_Effect (API field spellings).
    unsigned long m_axes[2];     // +68, SDK m_dwaAxes.
    long m_directions[2];        // +70, SDK m_laDirections.
    GUID m_effectGuid;           // +78, SDK m_guidEffect; distinct from descriptor.
    int m_isPlaying;             // +88, SDK m_bIsPlaying; Start 0x5215.
    unsigned long m_deviceType;  // +8c, SDK m_dwDeviceType; initialize 0x5a46.
    // Distinguish the API interface from the owning CImmDevice pointer.
    IFeelitDevice* m_immDeviceInterface; // +90, SDK m_piImmDevice; 0x5a00.
    IFeelitEffect* m_immEffect;   // +94, SDK m_piImmEffect; GetEffect 0x1460.
    unsigned long m_axisCount;   // +98, SDK m_cAxes; initialize 0x5a3d.
    unsigned long m_noDownload;  // +9c, SDK m_dwNoDownload; initialize 0x59f1.
    unsigned long m_iterations;  // +a0, SDK m_dwIterations.
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

protected:
    // DLL constructor 0x7a6e zeros the 0x38-byte descriptor at +a4.
    // set_parameters 0x81b0 fills its rectangle, wall properties, and
    // inside-effect API pointer, then stores its address in m_effect.
    FEELIT_ENCLOSURE m_enclosure; // +a4, SDK m_enclosure.
    // Start 0x8101 tests this before GetCursorPos and SetCenter.
    int m_useMousePosAtStart;     // +dc, SDK m_bUseMousePosAtStart.
    // Total 0xe0 matches the game's allocation. No newer m_pInsideEffect.

};

// The project file. Its constructor is NOT in the import table and
// retail inlines it as four zero stores over the sixteen bytes `new`
// buys, so it is an inline in the vendor header - modelled here as the
// four pointer-width members that zeroing writes.
// IFC20.dll 2.0.3 (SHA-256 e8c2afa0e2a19cd21d03685fd6f18a025
// 160c598ae93a9770a124f2a389e3846) confirms the four-slot constructor
// at DLL RVA 0x6ba0. The newer SDK supplies semantic spellings only;
// the old DLL's exported consumers below establish their actual offsets.
class CImmProject {
public:
    CImmProject() : m_proj(0), m_createdEffects(0), m_device(0), m_next(0) {}
    __declspec(dllimport) ~CImmProject();
    __declspec(dllimport) int LoadProjectFromMemory(void* data,
                                                    CImmDevice* device);
    __declspec(dllimport) void Close();
    __declspec(dllimport) CImmCompoundEffect* CreateEffect(
        const char* name, CImmDevice* device, unsigned long flags);
    __declspec(dllimport) void DestroyEffect(CImmCompoundEffect* effect);

private:
    // SDK m_hProj (HIFRPROJECT = LPVOID). Close +0xc0d3 passes this
    // handle to IFR release; LoadProjectObjectPointer +0xc559 stores it.
    void* m_proj;
    // SDK m_pCreatedEffects. append_effect_to_list +0xca84 reads the
    // head at +4; Close +0xc0ab walks and destroys compound effects.
    CImmCompoundEffect* m_createdEffects;
    // SDK m_pDevice. Exported GetDevice +0x6bb0 returns this +8 slot.
    CImmDevice* m_device;
    // SDK m_pNext. Exported get_next/set_next +0x6be0/+0x6bd0 use +12.
    CImmProject* m_next;
};

// --- ForceFeedback.obj's own objects ---

// The window origin the enclosure rectangles are kept relative to, and
// the enclosure->rectangle map ImmMouseWindowMoved walks. Retail loads
// the map's `_Head` at 0x696d64; VC6's Dinkumware map places that field
// at object +4, which fixes the object base at 0x696d60.
// Before normalization: gImmEffectEntries.
DATA(0x00696d60)
extern std::map<CImmEnclosure*, RECT> g_immEffectEntries;
// Retail 0x4b6260 passes 0x696d70 to ClientToScreen, which owns both
// LONG coordinates. 0x4b6950 and 0x4b6a50 consume its x/y at +0/+4.
// Former split names: gImmWindowX / gImmWindowY (g_immWindowX/Y).
DATA(0x00696d70) extern POINT g_immWindowOrigin;
// Before normalization: gImmWindow.
DATA(0x00696d7c) extern HWND g_immWindow;

// The three singletons the initializer publishes: the mouse (handed out
// as the device everywhere), the loaded project, and the effect currently
// playing. PlayImmEffect destroys the previous effect before creating the
// next, so the last is a single slot rather than a set.
// Before normalization: gImmDevice.
// Before normalization: gImmProject.
DATA(0x00696d80) extern CImmDevice* g_immDevice;
// Before normalization: gImmEffect.
DATA(0x00696d84) extern CImmProject* g_immProject;
DATA(0x00696d88) extern CImmCompoundEffect* g_immEffect;

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
