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

#include "forcefeedback.h"
#include "imm_mouse.h"

// .bss 0x696d60..0x696d90. The map is constructed by retail's own cinit
// at 0x4b61b0 (excluded class, never claimed as source); its `_Nil` and
// `_Nilrefs` statics live at 0x696d8c/0x696d90.
DATA(0x00696d60)
std::map<CImmEnclosure*, RECT> gImmEffectEntries;
DATA(0x00696d70) long gImmWindowX;
DATA(0x00696d74) long gImmWindowY;
DATA(0x00696d7c) HWND gImmWindow;
DATA(0x00696d80) CImmDevice* gImmDevice;
DATA(0x00696d84) CImmProject* gImmProject;
DATA(0x00696d88) CImmCompoundEffect* gImmEffect;

// The combat-spell rumble. Destroys whatever effect is still loaded,
// creates the named one against the default device and starts it for
// `count` iterations. The first guard returns a BYTE (`xor al,al`) where
// the two later exits clear the whole register.
VA(0x004b69f0, 0x5B)  // anchor-import (CImmProject::CreateEffect), retail-only
unsigned char PlayImmEffect(const char* effectName, int count)
{
    if (gImmProject == 0)
        return 0;
    if (gImmEffect != 0)
        gImmProject->DestroyEffect(gImmEffect);
    gImmEffect = gImmProject->CreateEffect(effectName, 0, 0);
    if (gImmEffect == 0)
        return 0;
    if (!gImmEffect->Start(count, 0))
        return 0;
    return 1;
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
    OffsetRect(&bounds, gImmWindowX, gImmWindowY);
    if (!m_enclosure->Initialize(gImmDevice, &bounds, a, a, b, b, c, c,
                                 (d ? 0x66 : 0) | (e ? 0x99 : 0), 0, 0, 0, 0))
        throw t_create_failure();
    gImmEffectEntries.insert(
        std::map<CImmEnclosure*, RECT>::value_type(m_enclosure.get(), bounds));
}

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
    return m_impl->m_enclosure->Start(0) != 0;
}

VA(0x004b6f50, 0xB)  // anchor-vtable (0x63e640+0x14), retail-only
void TImmMouseEffect::Stop()
{
    m_impl->m_enclosure->Stop();
}
