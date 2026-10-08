// ObjectPaletteWnd.cpp - Loki h3maped object 67: the object palette, three
// columns of object frames for the current slot's object types, scrolled by
// a GTK+ adjustment. Each frame shows the object's shadow and sprite (with
// the player's flag for heroes), scaled into the frame; disabled tools are
// drawn translucent on black. Hovering names the object in the label under
// the palette. Assert lines come from the retail immediates.
//
// Not exact: OnPaint's scaled copy keeps an unused 4-byte frame slot above
// its source and destination pointers that this spelling does not
// allocate.
#include "editor/stdafx.h"

#include <functional>
#include <stdio.h>
#include <string.h>
#include <string>

#include "editor/ObjectPaletteWnd.h"
#include "editor/Colors.h"
#include "editor/Tile.h"
#include "adventureobjecttype.h"
#include "artifact.h"
#include "creaturetype.h"
#include "csprite.h"
#include "objnames.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ObjectSprites.h"
#include "editor/Town.h"

// The element count the tooltip's assert expands (its text keeps the
// spacing of the expansion); the macro's name is not proven.
#define ELEMENTS(a) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

namespace {
GdkCursor* hOpenHandCursor = NULL;
}

// The two file-static helpers after _computeRows (names not proven).
static inline int max(int a, int b)
{
    return b >= a ? b : a;
}

static inline int min(int a, int b)
{
    return b <= a ? b : a;
}

const CSize TObjectPaletteWnd::s_kObjFrameSize(66, 66);

TObjectPaletteWnd::TObjectPaletteWnd(GtkWidget* thisWidget, TObjectPaletteWndClient* pClient,
                                     GtkAdjustment* pVAdjustment)
    : _m_scrollPos(0),
      _m_vAdjust(pVAdjustment),
      _m_pClient(pClient),
      _m_firstTool(0),
      _m_endTool(0),
      _m_pBackBuffer(NULL),
      _m_slot(eSlotDirt),
      _m_player(ePlayerNone),
      _m_hotTool(-1)
{
#line 94
    assert(thisWidget != NULL);
    assert(_m_vAdjust != NULL);
    assert(pClient != NULL);
    _m_hWnd = thisWidget;
    gtk_widget_add_events(_m_hWnd, GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK
                                   | GDK_BUTTON_MOTION_MASK
                                   | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON2_MOTION_MASK
                                   | GDK_BUTTON3_MOTION_MASK | GDK_BUTTON_PRESS_MASK
                                   | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK
                                   | GDK_KEY_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK
                                   | GDK_LEAVE_NOTIFY_MASK);
    if (hOpenHandCursor == NULL)
        hOpenHandCursor = gdk_cursor_new(GDK_HAND2);
#line 108
    assert(hOpenHandCursor != NULL);
    _m_pObjNameLabel = GTK_LABEL(_widget("objnamelabel"));
    gtk_label_set_text(_m_pObjNameLabel, "");

    for (unsigned int slot = 0; slot < kNumObjectSlots; slot++) {
        // The predicate comes from the heap (__builtin_new(1), deleted once
        // bound), and the slot reaches binder2nd as the loop counter itself.
        TObjectTypeInSlotPred* pPred = new TObjectTypeInSlotPred;
        binder2nd<TObjectTypeInSlotPred> pred(*pPred, (TObjectSlot)slot);
        delete pPred;
        for (const TObjectType* it = kObjectTypeTable.begin();
             (it = find_if(it, kObjectTypeTable.end(), pred)) != kObjectTypeTable.end(); it++)
            _m_aSlotInfo[slot].m_objTypes.push_back(&*it);
    }
    for (unsigned int heroClass = 0; heroClass <= kNumHeroClasses; heroClass++) {
#line 148
        assert(objectTypeInSlot( THero::s_akClassTraits[ heroClass ].m_objType, eSlotHeroes ));
        _m_aSlotInfo[eSlotHeroes].m_objTypes.push_back(&THero::s_akClassTraits[heroClass].m_objType);
    }
}

TObjectPaletteWnd::~TObjectPaletteWnd()
{
}

void TObjectPaletteWnd::setSlot(TObjectSlot newSlot)
{
#line 171
    assert(newSlot >= 0 && newSlot < kNumObjectSlots);
    if (newSlot == _m_slot)
        return;
    _removeAllTools();
    _m_slot = newSlot;
    CRect clientRect;
    GetClientRect(&clientRect);
    _m_vAdjust->lower = 0;
    _m_vAdjust->upper = max(_computeRows() * s_kObjFrameSize.cy - 1, 0);
    _m_vAdjust->page_size = clientRect.Height();
    _m_vAdjust->value = min(_m_aSlotInfo[_m_slot].m_scrollPos,
                            max(0, (unsigned int)_m_vAdjust->upper + 1
                                       - (unsigned int)_m_vAdjust->page_size));
    gtk_adjustment_changed(_m_vAdjust);
    gtk_adjustment_value_changed(_m_vAdjust);
    _m_aSlotInfo[_m_slot].m_scrollPos = (unsigned int)_m_vAdjust->value;
    _setupTools(clientRect.Height());
    OnPaint();
}

void TObjectPaletteWnd::setPlayer(TPlayer newPlayer)
{
#line 214
    assert(newPlayer >= ePlayerNone && newPlayer < kNumPlayers);
    if (_m_player != newPlayer) {
        _m_player = newPlayer;
        for (unsigned int tool = _m_firstTool; tool < _m_endTool; tool++) {
            bool bEnabled = _m_pClient->onPaletteCanCreateObject(this,
                                                                 *_m_aSlotInfo[_m_slot].m_objTypes[tool]);
            if (bEnabled != _m_abToolEnabled[tool - _m_firstTool])
                _m_abToolEnabled[tool - _m_firstTool] = bEnabled;
        }
        OnPaint();
    }
}

CSize TObjectPaletteWnd::getMinSize() const
{
    return CSize(100, 100);
}

void TObjectPaletteWnd::_setupTools(int height)
{
#line 255
    assert(height > 0);
    unsigned int scrollPos = _m_aSlotInfo[_m_slot].m_scrollPos;
    unsigned int firstRow = scrollPos / s_kObjFrameSize.cy;
    unsigned int numRows = _computeRows();
    unsigned int lastRow = (scrollPos + height + s_kObjFrameSize.cy - 1) / s_kObjFrameSize.cy;
    if (firstRow >= numRows)
        return;
    CRect toolRect;
    unsigned int tool = _m_firstTool = firstRow * 3;
    toolRect.left = toolRect.top = 0;
    toolRect.bottom = s_kObjFrameSize.cy - scrollPos % s_kObjFrameSize.cy;
    unsigned int col;
    for (col = 0; col < 3 && tool < _m_aSlotInfo[_m_slot].m_objTypes.size(); col++) {
        toolRect.right = toolRect.left + s_kObjFrameSize.cx;
        tool++;
        toolRect.left += s_kObjFrameSize.cx;
    }
    unsigned int row;
    for (row = firstRow + 1; row < lastRow - 1 && row < numRows - 1; row++) {
        toolRect.top = toolRect.bottom;
        toolRect.bottom += s_kObjFrameSize.cy;
        toolRect.left = 0;
        for (col = 0; col < 3; col++) {
            toolRect.right = toolRect.left + s_kObjFrameSize.cx;
            tool++;
            toolRect.left += s_kObjFrameSize.cx;
        }
    }
    toolRect.top = toolRect.bottom;
    toolRect.left = 0;
    if (row == lastRow - 1) {
#line 304
        assert(height > toolRect.top && height <= toolRect.top + s_kObjFrameSize.cy);
        toolRect.bottom = height;
    } else {
        toolRect.bottom += s_kObjFrameSize.cy;
    }
    for (col = 0; col < 3 && tool < _m_aSlotInfo[_m_slot].m_objTypes.size(); col++) {
        toolRect.right = toolRect.left + s_kObjFrameSize.cx;
        tool++;
        toolRect.left += s_kObjFrameSize.cx;
    }
    _m_endTool = tool;
    _m_abToolEnabled.resize(_m_endTool - _m_firstTool, false);
    for (tool = _m_firstTool; tool < _m_endTool; tool++)
        _m_abToolEnabled[tool - _m_firstTool] =
            _m_pClient->onPaletteCanCreateObject(this, *_m_aSlotInfo[_m_slot].m_objTypes[tool]);
}

void TObjectPaletteWnd::_removeAllTools()
{
    _m_endTool = 0;
    _m_firstTool = 0;
    _m_abToolEnabled.clear();
}

void TObjectPaletteWnd::OnSize(unsigned int type, int cx, int cy)
{
    if (cx == 0 || cy == 0 || _m_hWnd == NULL || _m_hWnd->window == NULL)
        return;
    if (_m_pBackBuffer) {
        gdk_pixmap_unref(_m_pBackBuffer);
        _m_pBackBuffer = NULL;
    }
    cx = s_kObjFrameSize.cx * 3;
    if (cy < s_kObjFrameSize.cy)
        cy = s_kObjFrameSize.cy;
    _removeAllTools();
    _m_pBackBuffer = gdk_pixmap_new(_m_hWnd->window, cx, cy, -1);
    _m_vAdjust->lower = 0;
    _m_vAdjust->upper = _computeRows() * s_kObjFrameSize.cy - 1;
    _m_vAdjust->page_size = cy;
    _m_vAdjust->value = min(_m_aSlotInfo[_m_slot].m_scrollPos,
                            max(0, (unsigned int)_m_vAdjust->upper + 1
                                       - (unsigned int)_m_vAdjust->page_size));
    gtk_adjustment_changed(_m_vAdjust);
    gtk_adjustment_value_changed(_m_vAdjust);
    _m_aSlotInfo[_m_slot].m_scrollPos = (unsigned int)_m_vAdjust->value;
    _setupTools(cy);
}

void TObjectPaletteWnd::OnPaint()
{
    if (_m_hWnd && _m_pBackBuffer) {
        CRect rect(0, 0, _m_hWnd->allocation.width, _m_hWnd->allocation.height);
        OnPaint(rect);
    }
}

void TObjectPaletteWnd::OnPaint(const CRect& rect)
{
#line 441
    assert(_m_vAdjust != NULL);
    assert(_m_hWnd != NULL);
    assert(_m_hWnd->window != NULL);
    if (!_m_pBackBuffer)
        return;

    GdkGC* gc = gdk_gc_new(_m_hWnd->window);
    gdk_gc_set_foreground(gc, &_m_white);
    unsigned int scrollPos = (unsigned int)_m_vAdjust->value;
    unsigned int firstRow = (rect.top + scrollPos) / s_kObjFrameSize.cy;
    unsigned int lastRow = (rect.bottom + scrollPos + s_kObjFrameSize.cy - 1) / s_kObjFrameSize.cy;
    int width = s_kObjFrameSize.cx * 3;
    int height = _m_hWnd->allocation.height;
    if (height < s_kObjFrameSize.cy)
        height = s_kObjFrameSize.cy;
    gdk_draw_rectangle(_m_pBackBuffer, gc, TRUE, 0, 0, width, height);
    GdkImage* image = gdk_image_get(_m_pBackBuffer, 0, 0, width, height);
    TColor flagColor = akPlayerColor[_m_player + 1];

    for (unsigned int row = firstRow; row < lastRow; row++) {
        unsigned int tool = row * 3;
        CRect toolRect;
        toolRect.top = row * s_kObjFrameSize.cy - scrollPos;
        toolRect.bottom = toolRect.top + s_kObjFrameSize.cy;
        for (unsigned int col = 0; col < 3; tool++, col++) {
            toolRect.left = col * s_kObjFrameSize.cx;
            toolRect.right = toolRect.left + s_kObjFrameSize.cx;
            if (tool < _m_aSlotInfo[_m_slot].m_objTypes.size()) {
                gdk_gc_set_foreground(gc, _m_abToolEnabled[tool - _m_firstTool] ? &_m_white : &_m_black);
                gdk_draw_rectangle(_m_pBackBuffer, gc, TRUE, toolRect.left, toolRect.top,
                                   toolRect.right, toolRect.bottom);
                void (CSprite::*pfnDraw)(int, int, int, int, int, unsigned short*, int, int, int,
                                         int, int, unsigned short, bool) const;
                if (_m_abToolEnabled[tool - _m_firstTool])
                    pfnDraw = &CSprite::DrawAdvObjWithFlag;
                else
                    pfnDraw = &CSprite::DrawAdvObjWithFlagAlpha;
                int srcWidth;
                int srcHeight;
                const TObjectType& objType = *_m_aSlotInfo[_m_slot].m_objTypes[tool];
                bool bHero = _m_player != ePlayerNone
                             && (objType.getType() == HERO || objType.getType() == RANDOM_HERO);
                {
                    TObjectSpritePtr pObjSprite(objType);
#line 557
                    assert(pObjSprite.get() != NULL);
                    srcWidth = pObjSprite->GetWidth();
                    srcHeight = pObjSprite->GetHeight();
#line 563
                    assert(srcWidth > 0);
                    assert(srcHeight > 0);
                    char fill = _m_abToolEnabled[tool - _m_firstTool] ? 0xff : 0;
                    memset(image->mem, fill, image->bpp / 8 * (image->width * image->height));
                    pObjSprite->DrawAdvObjShadow(0, 0, 0, srcWidth, srcHeight, (unsigned short*)image->mem,
                                                 0, 0, image->width, image->height, getImgPitch(image),
                                                 false);
                }
                if (bHero) {
                    THeroFlagSpritePtr pFlagSprite(_m_player);
#line 594
                    assert(pFlagSprite.get() != NULL);
#line 596
                    assert(pFlagSprite->GetWidth() == srcWidth);
                    assert(pFlagSprite->GetHeight() == srcHeight);
                    pFlagSprite->DrawAdvObjShadow(0, 0, 0, srcWidth, srcHeight, (unsigned short*)image->mem,
                                                  0, 0, image->width, image->height, getImgPitch(image),
                                                  false);
                }
                {
                    TObjectSpritePtr pObjSprite(objType);
#line 616
                    assert(pObjSprite.get() != NULL);
#line 618
                    assert(pObjSprite->GetWidth() == srcWidth);
                    assert(pObjSprite->GetHeight() == srcHeight);
                    (pObjSprite.get()->*pfnDraw)(0, 0, 0, srcWidth, srcHeight, (unsigned short*)image->mem,
                                                 0, 0, image->width, image->height, getImgPitch(image),
                                                 flagColor, false);
                }
                if (bHero) {
                    THeroFlagSpritePtr pFlagSprite(_m_player);
#line 641
                    assert(pFlagSprite.get() != NULL);
#line 643
                    assert(pFlagSprite->GetWidth() == srcWidth);
                    assert(pFlagSprite->GetHeight() == srcHeight);
                    (pFlagSprite.get()->*pfnDraw)(0, 0, 0, srcWidth, srcHeight, (unsigned short*)image->mem,
                                                  0, 0, image->width, image->height, getImgPitch(image),
                                                  0, false);
                }

                CRect frameRect = toolRect;
                frameRect.DeflateRect(1, 1);
                int dstWidth = min(srcWidth, frameRect.Width());
                int dstHeight = min(srcHeight, frameRect.Height());
                if (srcWidth >= srcHeight)
                    dstHeight = srcHeight * dstWidth / srcWidth;
                else
                    dstWidth = srcWidth * dstHeight / srcHeight;
                GdkVisual* visual = gdk_visual_get_system();
                if (visual->depth == 24) {
                    int imgWidth = image->width;
                    int imgHeight = image->height;
                    GdkImage* image24 = gdk_image_new(GDK_IMAGE_FASTEST, gdk_visual_get_system(),
                                                      imgWidth, imgHeight);
                    if (image24) {
                        int x;
                        int y;
                        unsigned short* pSrc16 = (unsigned short*)image->mem;
                        guint32* pDst32 = (guint32*)image24->mem;
                        int pixel;
                        for (y = 0; y < imgHeight; y++) {
                            for (x = 0; x < imgWidth; x++) {
                                pixel = pSrc16[y * imgWidth + x];
                                pDst32[y * imgWidth + x] = ((pixel & 0xf800) << 8) + ((pixel & 0x7e0) << 5)
                                                           + ((pixel & 0x1f) << 3);
                            }
                        }
                        if (srcWidth == dstWidth && srcHeight == dstHeight) {
                            gdk_draw_image(_m_pBackBuffer, gc, image24, 0, 0,
                                           frameRect.left + (frameRect.Width() - dstWidth) / 2,
                                           frameRect.top + (frameRect.Height() - dstHeight) / 2,
                                           dstWidth, dstHeight);
                        } else {
                            // Dead local: retail keeps an unused 4-byte frame
                            // slot above pSrc. Type and name are unproven.
                            int unused;
                            guint32* pSrc;
                            guint32* pDst;
                            double xScale;
                            double yScale;
                            GdkImage* scaledImage;
                            scaledImage = gdk_image_new(GDK_IMAGE_FASTEST, gdk_visual_get_system(),
                                                        dstWidth, dstHeight);
                            xScale = (double)srcWidth / dstWidth;
                            yScale = (double)srcHeight / dstHeight;
                            pSrc = (guint32*)image24->mem;
                            pDst = (guint32*)scaledImage->mem;
                            for (y = 0; y < dstHeight; y++) {
                                for (x = 0; x < dstWidth; x++)
                                    pDst[y * dstWidth + x] =
                                        pSrc[(int)(x * xScale) + (int)(y * yScale) * imgWidth];
                            }
                            gdk_draw_image(_m_pBackBuffer, gc, scaledImage, 0, 0,
                                           frameRect.left + (frameRect.Width() - dstWidth) / 2,
                                           frameRect.top + (frameRect.Height() - dstHeight) / 2,
                                           dstWidth, dstHeight);
                        }
                        gdk_image_destroy(image24);
                    }
                } else {
                    GdkImage* scaledImage = StretchBlit(image, NULL, 0, 0, srcWidth, srcHeight, 0, 0,
                                                        dstWidth, dstHeight);
                    gdk_draw_image(_m_pBackBuffer, gc, scaledImage, 0, 0,
                                   frameRect.left + (frameRect.Width() - dstWidth) / 2,
                                   frameRect.top + (frameRect.Height() - dstHeight) / 2,
                                   dstWidth, dstHeight);
                    gdk_image_destroy(scaledImage);
                }
            } else {
                gdk_gc_set_foreground(gc, &_m_black);
                gdk_draw_rectangle(_m_pBackBuffer, gc, TRUE, toolRect.left, toolRect.top,
                                   toolRect.Width(), toolRect.Height());
            }
        }
    }

    gdk_draw_pixmap(_m_hWnd->window, gc, _m_pBackBuffer, rect.left, rect.top, rect.left, rect.top,
                    rect.Width(), rect.Height());
    gdk_gc_unref(gc);
    gdk_image_destroy(image);
}

void TObjectPaletteWnd::OnVScroll(unsigned int code)
{
#line 852
    assert(_m_vAdjust != NULL);
    unsigned int pos = (unsigned int)_m_vAdjust->value;
    unsigned int pageSize = (unsigned int)_m_vAdjust->page_size;
    int delta = 0;
    switch (code) {
    case SB_LINEUP:
        delta = pos > s_kObjFrameSize.cy ? s_kObjFrameSize.cy : pos;
        break;
    case SB_LINEDOWN: {
        unsigned int maxPos = _computeRows() * s_kObjFrameSize.cy - pageSize;
        delta = maxPos - pos > s_kObjFrameSize.cy ? -s_kObjFrameSize.cy : pos - maxPos;
        break;
    }
    case SB_PAGEUP:
        delta = pos > pageSize ? pageSize : pos;
        break;
    case SB_PAGEDOWN: {
        unsigned int maxPos = _computeRows() * s_kObjFrameSize.cy - pageSize;
        delta = maxPos - pos > pageSize ? -pageSize : pos - maxPos;
        break;
    }
    case SB_TOP:
        delta = pos;
        break;
    case SB_BOTTOM:
        delta = pos - (_computeRows() * s_kObjFrameSize.cy - pageSize);
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        delta = pos - _m_scrollPos;
        break;
    }
    if (delta != 0) {
        _removeAllTools();
        _m_vAdjust->page_size = pageSize;
        _m_vAdjust->value = pos;
        gtk_adjustment_changed(_m_vAdjust);
        gtk_adjustment_value_changed(_m_vAdjust);
        _m_aSlotInfo[_m_slot].m_scrollPos = pos;
        _setupTools(pageSize);
        OnPaint();
    }
    _m_scrollPos = pos;
}

void TObjectPaletteWnd::OnLButtonDown(unsigned int flags, CPoint point)
{
#line 940
    assert(_m_pClient != NULL);
    assert(_m_vAdjust != NULL);
    CPoint pos = point;
    pos.y += (int)_m_vAdjust->value;
    unsigned int tool = pos.y / s_kObjFrameSize.cy * 3 + pos.x / s_kObjFrameSize.cx;
    if (tool < _m_aSlotInfo[_m_slot].m_objTypes.size())
        _m_pClient->onPaletteGrabObject(this, *_m_aSlotInfo[_m_slot].m_objTypes[tool]);
}

void TObjectPaletteWnd::OnMouseLeave()
{
    gtk_label_set_text(_m_pObjNameLabel, "");
    _m_hotTool = -1;
}

BOOL TObjectPaletteWnd::OnToolTipNeedText(unsigned int id)
{
    unsigned int tool = id;
    if (tool >= _m_aSlotInfo[_m_slot].m_objTypes.size()) {
        gtk_label_set_text(_m_pObjNameLabel, "");
        return false;
    }
    const TObjectType* pObjType = _m_aSlotInfo[_m_slot].m_objTypes[tool];
#line 993
    assert(pObjType->getType() >= 0 && pObjType->getType() < ELEMENTS(akAdvObjectTypeTraits));
    string text;
    switch (pObjType->getType()) {
    case ARTIFACT:
#line 1003
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < kNumArtifacts);
        text = akArtifactTraits[pObjType->getExtra()].m_name;
        break;
    case HERO:
#line 1008
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < kNumHeroClasses);
        text = THero::s_akClassTraits[pObjType->getExtra()].m_name;
        break;
    case TOWN:
#line 1013
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < kNumTownTypes);
        text = TTown::s_akTypeTraits[pObjType->getExtra()].m_pName;
        break;
    case MONSTER:
#line 1018
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < kNumCreatureTypes);
        text = akCreatureTypeTraits[pObjType->getExtra()].m_name;
        break;
    case MINE:
#line 1023
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < TMine::s_kNumMineTypes);
        text = TMine::s_akMineTypeTraits[pObjType->getExtra()].m_name;
        break;
    case CREATURE_BANK:
#line 1028
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < kNumCreatureBankTypes);
        text = akCreatureBankTypeTraits[pObjType->getExtra()].m_name;
        break;
    case CREATURE_GENERATOR_1:
#line 1033
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < TGenerator::s_kNumGenerator1Types);
        text = TGenerator::s_akGenerator1TypeTraits[pObjType->getExtra()].m_name;
        break;
    case CREATURE_GENERATOR_4:
#line 1038
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < TGenerator::s_kNumGenerator4Types);
        text = TGenerator::s_akGenerator4TypeTraits[pObjType->getExtra()].m_name;
        break;
    case RESOURCE:
#line 1043
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < kNumGameResourceTypes);
        text = akGameResourceTypeTraits[pObjType->getExtra()].m_name;
        break;
    case GARRISON:
#line 1048
        assert(pObjType->getExtra() >= 0 && pObjType->getExtra() < TGarrison::s_kNumTypes);
        text = TGarrison::s_akTypeTraits[pObjType->getExtra()].m_name;
        break;
    default:
        text = akAdvObjectTypeTraits[pObjType->getType()].m_name;
    }
    static char szText[200];
    sprintf(szText, "%s", text.c_str());
    gtk_label_set_text(_m_pObjNameLabel, szText);
    return true;
}

void TObjectPaletteWnd::OnMouseMove(unsigned int flags, CPoint point)
{
    CPoint pos = point;
    pos.y += (int)_m_vAdjust->value;
    unsigned int tool = pos.y / s_kObjFrameSize.cy * 3 + pos.x / s_kObjFrameSize.cx;
    if (tool != _m_hotTool) {
        OnToolTipNeedText(tool);
        _m_hotTool = tool;
    }
    gdk_window_set_cursor(_m_hWnd->window,
                          tool < _m_aSlotInfo[_m_slot].m_objTypes.size() ? hOpenHandCursor : NULL);
}
