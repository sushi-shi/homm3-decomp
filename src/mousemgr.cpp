// mousemgr.cpp - E:\gamedcs\mousemgr.cpp (compiland mousemgr.obj)
#include "terrain.h"
#include <va.h>
#include <windows.h>
#include <ddraw.h>
#include <string.h>
#include "mousemgr.h"
#include "kbwin.h"
#include "resourcemanager.h"
#include "bitmap16.h"
#include "wingraph.h"

// SetPointer's re-entrancy latch - the byte immediately after the
// timer latches, tested and set around the whole swap (name
// provisional, no DC row for it).
DATA(0x0069ca21) unsigned char g_mouseSetPointerBusy;
DATA(0x0069ca22) unsigned char g_mouseInUpdate;

// LoadFrame's DirectDraw targets. Its channel masks belong to wingraph's
// complete DDPIXELFORMAT object, shared through the RGBto16 helper.
// DC's MouseSurface family uses Surface4/DDSURFACEDESC2. Complete instead
// creates all three through IDirectDraw::CreateSurface in ddInitGraphics
// (0x6014f0 +0x21d/+0x2bd/+0x310), without QueryInterface. They are v1
// surfaces, matching loadFrame's retail 108-byte DDSURFACEDESC. A v4 cast
// was an incorrect cross-platform type bridge, not an SDK size exception.
DATA(0x006aacc4) IDirectDrawSurface* g_ddsMouseSurface;
DATA(0x006aacc8) IDirectDrawSurface* g_ddsMouseSaveSurface;
DATA(0x006aaccc) IDirectDrawSurface* g_ddsMouseScratchSurface;

// The pointer-set sprite table SetPointer indexes with field_4c: five
// .DEF names in EPointerSet order (.data 0x67ff38).
DATA(0x0067ff38) const char* g_pointerSetSprites[mouseManager::MAX_POINTER_SETS] = {
    DATA_COMPGEN(0x0068160c, pointerSpriteDefault, "crdeflt.DEF"),
    DATA_COMPGEN(0x006815fc, pointerSpriteAdventure, "cradvntr.DEF"),
    DATA_COMPGEN(0x006815ec, pointerSpriteCombat, "crcombat.DEF"),
    DATA_COMPGEN(0x006815e0, pointerSpriteSpell, "crspell.DEF"),
    DATA_COMPGEN(0x006815d0, pointerSpriteArtifact, "artifact.DEF")
};

// Five pointer sets, 144 frames per set, one POINT per frame. The extent
// closes exactly at the first pointer-name string at 0x6815d0.
DATA(0x0067ff50) POINT g_mouseHotSpots[mouseManager::MAX_POINTER_SETS][144];

// E:\gamedcs\mousemgr.cpp:291
// mousemgr.cpp's critical-section RAII guard (DC CodeView TCSLock; the
// original source owns both in-class bodies here at lines 291/298. Retail
// expands or calls each retained body per site; the fs:[0] frame in users
// is the unwind scaffolding).
class TCSLock {
public:
    VA(0x0050d890, 0x19)  // byte-identified out-of-line copy, dc 0xff7e0
    TCSLock(CRITICAL_SECTION* criticalSection)
        : m_section(criticalSection) {
        EnterCriticalSection(m_section);
    }
    VA(0x0050cd80, 0xA)  // anchor-import (__imp__LeaveCriticalSection@4), dc 0xff800
    ~TCSLock() { LeaveCriticalSection(m_section); }

    CRITICAL_SECTION* m_section;
};

VA(0x0050cb50, 0x6F)  // dc 0xfe9d4
mouseManager::mouseManager()
{
    m_busy = 0;
    m_set = INVALID_SET;
    m_status = 0;
    strcpy(m_mgrName, DATA_COMPGEN(0x00681618, mouseManagerName, "mouseManager"));
    m_frame = -1;
    m_sprite = 0;
    m_disableCount = 0;
    m_hideCount = 1;
    InitializeCriticalSection(&m_sectionMouse);
}

// E:\gamedcs\mousemgr.cpp:344, dc 0xfea50
// The written ordinary destructor precedes Open. Retail expands this body
// into the scalar deleting destructor at 0x50cbc0.
mouseManager::~mouseManager()
{
    DeleteCriticalSection(&m_sectionMouse);
}

// Slot 3 of vtable 0x640028: own-vptr store, DeleteCriticalSection at
// +0x78, then the flags&1 operator-delete tail. No standalone retail claim.
VA_COMPGEN(0x0050cbc0, 0x2C, SCALAR_DELETING_DTOR, mouseManager)

VA(0x0050cbf0, 0x4A)  // dc 0xfea80
int mouseManager::open(int newPriority)
{
    m_noChangePointer = 0;
    reset();
    ShowCursor(0);
    m_systemPointerIsOn = 0;
    m_id = 0x40;
    m_priority = newPriority;
    m_status = 1;
    return 0;
}

VA(0x0050cc40, 0x38)  // dc 0xfeab4
void mouseManager::close()
{
    if (m_status != 1)
        return;
    unsigned char wasHidden = m_systemPointerIsOn;
    m_status = 0;
    if (!wasHidden) {
        ShowCursor(1);
        m_systemPointerIsOn = 1;
    }
    if (m_sprite)
        m_sprite->dispose();
    m_sprite = 0;
}

VA(0x0050cc80, 0x1B)  // dc 0xfeafc
void mouseManager::reset()
{
    m_savedRect.left = 0;
    m_savedRect.top = 0;
    m_savedRect.right = 0;
    m_savedRect.bottom = 0;
    m_imageX = 0;
    m_imageY = 0;
    m_currentX = 0;
    m_currentY = 0;
}

#if 0  // @carcass

// E:\gamedcs\mousemgr.cpp:431 - slot 2. No claim and no definition:
// the retail body is the program-wide `xor eax,eax; ret 4` at
// 0x4ec560, an /OPT:ICF fold that inputmgr.cpp already claims as
// inputManager::Main. Declared in mousemgr.h so the slot is modelled.
DC_ONLY(0xfeb18, 0x4)
int mouseManager::main(message& msg)
{
    // @stub
}

#endif  // @carcass

VA(0x0050cca0, 0xE0)  // dc 0xfeb1c
void mouseManager::setPointer(int newFrame, mouseManager::EPointerSet newSet)
{
    TCSLock lock(&m_sectionMouse);
    if (m_status != 1)
        return;
    if (m_noChangePointer != 0)
        return;
    if (g_mouseSetPointerBusy)
        return;
    g_mouseSetPointerBusy = 1;
    m_busy++;
    disable();
    if (newSet != SAME_SET && newSet != m_set) {
        m_set = newSet;
        if (m_sprite)
            m_sprite->dispose();
        m_sprite = ResourceManager::getSprite(g_pointerSetSprites[m_set]);
        m_frame = -1;
    }
    if (newFrame < 0) {
        m_busy--;
        enable();
        g_mouseSetPointerBusy = 0;
        return;
    }
    if (newFrame == m_frame) {
        m_busy--;
        enable();
        g_mouseSetPointerBusy = 0;
        return;
    }
    loadFrame(m_set == SPELL_SET ? 0 : newFrame);
    enable();
    update(1);
    m_busy--;
    g_mouseSetPointerBusy = 0;
}

// E:\gamedcs\mousemgr.cpp:526
// RETAIL-RECONSTRUCTED 2026-08-09. The 72-block CFG now agrees exactly,
// including both early-return families, overlap/non-overlap selection, all
// four clipping ladders and the final cleanup. SaveAndDraw and
// RestoreUnderlying are ordinary members whose expansions retain their
// argument homes and private RECTs in retail's Update stack frame.

// DC mousemgr.cpp:639/700 and 648/708 prove separate branch-local
// window_origin POINTs and ddsd descriptors. Keep those scopes, substituting
// the retail-proven Windows v1 descriptor for DC's Surface4 descriptor.
// The 32-state helper/scope family reproduced 32 distinct objects and ten
// elites with every sibling exact. Rectangle-copy initialization and ordinary
// helpers reach 99.8420 with the old shared origin, 99.7141 with the proven
// origins retained; old forced/interleaved construction was 94.1675.
VA(0x0050cd90, 0x770)  // anchor-global, dc 0xfec54
void mouseManager::update(bool forceIt)
{
    TCSLock lock(&m_sectionMouse);
    // The new rectangle belongs to the outer procedure scope.
    RECT newRect;

    if (g_mouseInUpdate)
        return;
    g_mouseInUpdate = 1;

    if (!forceIt)
        getPointerPosition();

    if (m_hideCount > 0 && !forceIt) {
        g_mouseInUpdate = 0;
        return;
    }
    if (m_disableCount > 0) {
        g_mouseInUpdate = 0;
        return;
    }

    m_busy++;
    if (!forceIt
            && m_imageX
                == m_currentX - g_mouseHotSpots[m_set][m_frame].x
            && m_imageY
                == m_currentY - g_mouseHotSpots[m_set][m_frame].y) {
        g_mouseInUpdate = 0;
        m_busy--;
        return;
    }

    m_imageX = m_currentX - g_mouseHotSpots[m_set][m_frame].x;
    m_imageY = m_currentY - g_mouseHotSpots[m_set][m_frame].y;

    newRect.left = m_imageX;
    newRect.top = m_imageY;
    // DC 587/588 records these CSprite dimension accessors.
    newRect.right = m_imageX + m_sprite->getWidth();
    newRect.bottom = m_imageY + m_sprite->getHeight();
    if (newRect.left < 0)
        newRect.left = 0;
    if (newRect.top < 0)
        newRect.top = 0;
    if (newRect.right > 800)
        newRect.right = 800;
    if (newRect.bottom > 600)
        newRect.bottom = 600;

    if (newRect.left >= m_savedRect.right
            || newRect.right <= m_savedRect.left
            || newRect.top >= m_savedRect.bottom
            || newRect.bottom <= m_savedRect.top) {
        RECT frontRect;
        frontRect = newRect;
        POINT windowOrigin;
        windowOrigin.x = 0;
        windowOrigin.y = 0;
        ClientToScreen(g_hwndApp, &windowOrigin);
        OffsetRect(&frontRect, windowOrigin.x, windowOrigin.y);

        // Complete uses the v1 descriptor.
        DDSURFACEDESC surfaceDesc;
        memset(&surfaceDesc, 0, sizeof(surfaceDesc));
        surfaceDesc.dwSize = sizeof(surfaceDesc);
        g_ddsPrimary->GetSurfaceDesc(&surfaceDesc);
        if (frontRect.left < 0)
            frontRect.left = 0;
        if (frontRect.right > static_cast<long>(surfaceDesc.dwWidth))
            frontRect.right = surfaceDesc.dwWidth;
        if (frontRect.top < 0)
            frontRect.top = 0;
        if (frontRect.bottom > static_cast<long>(surfaceDesc.dwHeight))
            frontRect.bottom = surfaceDesc.dwHeight;

        newRect = frontRect;
        OffsetRect(&newRect, -windowOrigin.x, -windowOrigin.y);
        saveAndDraw(g_ddsPrimary,
            g_ddsMouseScratchSurface, frontRect,
            m_imageX + windowOrigin.x, m_imageY + windowOrigin.y);

        OffsetRect(&m_savedRect, windowOrigin.x, windowOrigin.y);
        if (m_savedRect.left < 0)
            m_savedRect.left = 0;
        if (m_savedRect.right > static_cast<long>(surfaceDesc.dwWidth))
            m_savedRect.right = surfaceDesc.dwWidth;
        if (m_savedRect.top < 0)
            m_savedRect.top = 0;
        if (m_savedRect.bottom > static_cast<long>(surfaceDesc.dwHeight))
            m_savedRect.bottom = surfaceDesc.dwHeight;
        restoreUnderlying(g_ddsPrimary,
            m_savedRect);

        RECT copyRect;
        copyRect.left = 0;
        copyRect.top = 0;
        copyRect.right = newRect.right - newRect.left;
        copyRect.bottom = newRect.bottom - newRect.top;
        ddBlit(g_ddsMouseSaveSurface, copyRect,
            g_ddsMouseScratchSurface, copyRect, DDBLT_WAIT);
        m_savedRect = newRect;
    } else {
        RECT frontWorkRect;
        UnionRect(&frontWorkRect, &newRect, &m_savedRect);

        POINT windowOrigin;
        windowOrigin.x = 0;
        windowOrigin.y = 0;
        ClientToScreen(g_hwndApp, &windowOrigin);
        OffsetRect(&frontWorkRect, windowOrigin.x, windowOrigin.y);

        // Complete uses the v1 descriptor.
        DDSURFACEDESC surfaceDesc;
        memset(&surfaceDesc, 0, sizeof(surfaceDesc));
        surfaceDesc.dwSize = sizeof(surfaceDesc);
        g_ddsPrimary->GetSurfaceDesc(&surfaceDesc);
        if (frontWorkRect.left < 0)
            frontWorkRect.left = 0;
        if (frontWorkRect.right > static_cast<long>(surfaceDesc.dwWidth))
            frontWorkRect.right = surfaceDesc.dwWidth;
        if (frontWorkRect.top < 0)
            frontWorkRect.top = 0;
        if (frontWorkRect.bottom > static_cast<long>(surfaceDesc.dwHeight))
            frontWorkRect.bottom = surfaceDesc.dwHeight;

        RECT workRect;
        workRect = frontWorkRect;
        OffsetRect(&workRect, -windowOrigin.x, -windowOrigin.y);

        RECT dstRect;
        dstRect.left = 0;
        dstRect.top = 0;
        dstRect.right = frontWorkRect.right - frontWorkRect.left;
        dstRect.bottom = frontWorkRect.bottom - frontWorkRect.top;
        ddBlit(g_ddsMouseScratchSurface, dstRect,
            g_ddsPrimary,
            frontWorkRect, DDBLT_WAIT);

        RECT oldRect;
        oldRect = m_savedRect;
        OffsetRect(&oldRect, -workRect.left, -workRect.top);
        restoreUnderlying(g_ddsMouseScratchSurface, oldRect);

        if (newRect.left < -windowOrigin.x)
            newRect.left = -windowOrigin.x;
        if (newRect.right
                > static_cast<long>(surfaceDesc.dwWidth) - windowOrigin.x)
            newRect.right = surfaceDesc.dwWidth - windowOrigin.x;
        if (newRect.top < -windowOrigin.y)
            newRect.top = -windowOrigin.y;
        if (newRect.bottom
                > static_cast<long>(surfaceDesc.dwHeight) - windowOrigin.y)
            newRect.bottom = surfaceDesc.dwHeight - windowOrigin.y;

        m_savedRect = newRect;
        OffsetRect(&newRect, -workRect.left, -workRect.top);
        saveAndDraw(g_ddsMouseScratchSurface, g_ddsMouseSaveSurface,
            newRect, m_imageX - workRect.left,
            m_imageY - workRect.top);

        RECT srcRect;
        srcRect.left = 0;
        srcRect.top = 0;
        srcRect.right = frontWorkRect.right - frontWorkRect.left;
        srcRect.bottom = frontWorkRect.bottom - frontWorkRect.top;
        ddBlit(
            g_ddsPrimary,
            frontWorkRect, g_ddsMouseScratchSurface, srcRect, DDBLT_WAIT);
    }

    g_mouseInUpdate = 0;
    m_busy--;
}

VA(0x0050d500, 0x3F)  // dc 0xff22c
void mouseManager::mouseCoords(int& x, int& y)
{
    POINT cursor;
    GetCursorPos(&cursor);
    cursor.x &= ~1;
    ScreenToClient(g_hwndApp, &cursor);
    x = cursor.x;
    y = cursor.y;
}

// E:\gamedcs\mousemgr.cpp:798 (dc 0xff268). The signature proves const
// RECT&, and lines 805/809 construct save_rect then copy it into src_rect
// (the SH4 copy reads save_rect.right). Retail expands this ordinary helper;
// no forced-inline keyword is needed. Independent rectangle constructions
// score below the copy in the bounded family (all callers measured).
void mouseManager::saveAndDraw(
    IDirectDrawSurface* dstSurface,
    IDirectDrawSurface* saveSurface,
    const RECT& dstRect, int x, int y)
{
    if (!IsRectEmpty(&dstRect)) {
        RECT saveRect = {0, 0, dstRect.right - dstRect.left,
                         dstRect.bottom - dstRect.top};
        RECT sourceRect = saveRect;
        OffsetRect(&sourceRect, dstRect.left - x, dstRect.top - y);
        ddBlit(saveSurface, saveRect, dstSurface, dstRect, DDBLT_WAIT);
        if (m_hideCount == 0)
            ddBlit(dstSurface, dstRect, g_ddsMouseSurface, sourceRect,
                DDBLT_WAIT | DDBLT_KEYSRC);
    }
}

// E:\gamedcs\mousemgr.cpp:844 (dc 0xff328): ordinary helper, const RECT&,
// one block-local src_rect. Retail expands both call sites in Update.
void mouseManager::restoreUnderlying(
    IDirectDrawSurface* surface, const RECT& dstRect)
{
    if (!IsRectEmpty(&dstRect)) {
        RECT sourceRect;
        sourceRect.left = 0;
        sourceRect.top = 0;
        sourceRect.right = dstRect.right - dstRect.left;
        sourceRect.bottom = dstRect.bottom - dstRect.top;
        ddBlit(surface, dstRect, g_ddsMouseSaveSurface, sourceRect,
            DDBLT_WAIT);
    }
}

VA(0x0050d540, 0x6F)  // dc 0xff3a8
void mouseManager::hidePointer()
{
    TCSLock lock(&m_sectionMouse);
    if (++m_hideCount == 1 && !IsIconic(g_hwndApp))
        update(1);
}

VA(0x0050d5b0, 0xD0)  // dc 0xff3e0
void mouseManager::showPointer(bool force)
{
    TCSLock lock(&m_sectionMouse);
    if (force)
        m_hideCount = 1;
    if (m_hideCount > 0 && --m_hideCount == 0) {
        m_busy++;
        getPointerPosition();
        if (!IsIconic(g_hwndApp))
            update(1);
        m_busy--;
    }
}

// E:\gamedcs\mousemgr.cpp:934
// DC 0xff448 proves the TCSLock, x/y locals, MouseCoords call and member
// stores in this order. Retail expands this ordinary helper in Update and
// ShowPointer, including MouseCoords and both lock boundaries.
void mouseManager::getPointerPosition()
{
    TCSLock lock(&m_sectionMouse);
    int x, y;
    mouseCoords(x, y);
    m_currentX = x;
    m_currentY = y;
}

VA(0x0050d680, 0x210)  // anchor-global, dc 0xff484
void mouseManager::checkUpdate()
{
    TCSLock lock(&m_sectionMouse);
    // DC procedure 0xff484 owns update_time and animate_time as unsigned
    // long function statics (NB11 owner record 5144). Retail's one shared
    // guard byte at 0x69ca20 tests bits 1/2 for these two initializers.
    DATA_COMPGEN_GUARD(0x0069ca20, mouseTimersGuard, updateTime)
    DATA(0x0069ca18)
    static unsigned long updateTime = GameTime::get() + 33;
    DATA(0x0069ca1c)
    static unsigned long animateTime = GameTime::get() + 100;
    if (IsIconic(g_hwndApp))
        return;
    if (GameTime::isPast(updateTime) && !isBusy()) {
        updateTime = GameTime::nextFrameTime(updateTime, 33);
        update(0);
        if (GetWindowThreadProcessId(g_hwndApp, 0) == GetCurrentThreadId()) {
            if (m_currentX >= 0 && m_currentX < 800
                && m_currentY >= 0 && m_currentY < 600) {
                if (m_systemPointerIsOn) {
                    showSystemCursor(0);
                    m_systemPointerIsOn = 0;
                }
            } else if (!m_systemPointerIsOn) {
                showSystemCursor(1);
                m_systemPointerIsOn = 1;
            }
        }
    }
    if (GameTime::isPast(animateTime) && !isBusy()) {
        animateTime = GameTime::nextFrameTime(animateTime, 100);
        if (m_set == SPELL_SET) {
            loadFrame((m_frame + 1) % m_sprite->getNumFrames(0));
            update(1);
        }
    }
}

// E:\gamedcs\mousemgr.cpp:298 - TCSLock::~TCSLock (dc 0xff800) lives
// at 0x50cd80, the ten-byte row directly after SetPointer: `mov
// eax,[ecx]; push eax; call [__imp__LeaveCriticalSection@4]; ret`.
// The retail linker places its COMDAT between SetPointer and Update.
// The canonical in-class body and VA stay above in CodeView source order;
// EH unwind funclets still use its retained callable copy.

VA(0x0050d8b0, 0x16B)  // dc 0xff610
void mouseManager::loadFrame(int newFrame)
{
    TCSLock lock(&m_sectionMouse);

    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = rgBto16(0, 255, 255);
    g_ddsMouseSurface->Blt(0, 0, 0, DDBLT_COLORFILL, &fx);

    DDSURFACEDESC surfaceDesc;
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    if (g_ddsMouseSurface->Lock(0,
            &surfaceDesc,
            DDLOCK_WAIT, 0) == DD_OK) {
        Bitmap16Bit bitmap(0, 0);
        bitmap.reference(surfaceDesc.dwWidth, surfaceDesc.dwHeight,
            surfaceDesc.lPitch,
            static_cast<unsigned short*>(surfaceDesc.lpSurface));
        CSprite* sprite = m_sprite;
        sprite->drawPointer(newFrame, bitmap.getMap(0, 0), 0, 0,
            bitmap.getWidth(), bitmap.getHeight(), bitmap.getPitch(), 0);
        g_ddsMouseSurface->Unlock(0);
        m_frame = newFrame;
    }
}

// E:\gamedcs\mousemgr.cpp:1120
// Public ?ShowSystemCursor@mouseManager@@QAAX_N@Z proves bool; the NB11
// T_UCHAR formal is its lowered storage record. Preserve the separate helper
// calls at DC1123/1124 and1128/1129 and retail's byte-tested flag.
VA(0x0050da20, 0x37)  // anchor-global, dc 0xff708
void mouseManager::showSystemCursor(bool showIt)
{
    if (showIt) {
        ShowCursor(1);
        hidePointer();
    } else {
        showPointer(0);
        ShowCursor(0);
    }
}

#if 0  // @carcass

// E:\gamedcs\mousemgr.cpp:291
// Canonical constructor and VA are on the source-local class above.

// E:\gamedcs\mousemgr.cpp:298
// Canonical destructor and VA are on the source-local class above.

// E:\gamedcs\mousemgr.cpp:332
DC_ONLY(0xff818, 0x34)
void* mouseManager::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
