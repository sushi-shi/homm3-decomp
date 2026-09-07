// customcampaignwindow.cpp - Complete-only compiland; no Dreamcast twin.
// HAND-OWNED. Retail's two static-initializer runs bracket it: cinit0275..
// 0285 at 0x4823c0 open the object right after cursor.obj's SendMapChange
// (0x482390), and cinit0287..0296 at 0x483b40 open customcampaign.obj.
// Everything between - the "Select a Campaign" window constructor
// (0x4827b0, "CamCust.pcx"), its Maps\*.h3c scanner, the campaign-name
// sort predicate, the list refresh and the std::sort COMDATs the scanner
// instantiates - is this unit. Its calls into CampaignHeaderStruct's
// members are cross-TU, which is why retail keeps every one of them out of
// line where a single-TU spelling let /Ob2 expand them. The compiland's
// real name is unknown (alphabetically it sits between cursor and
// customcampaign); the unit name is provisional.
#include <va.h>
#include <algorithm>
#include <direct.h>
#include <io.h>
#include "game.h"
#include "campaignbrief.h"
#include "customcampaign.h"
#include "misc.h"
#include "slider.h"
#include "bitmap816.h"
#include "border.h"
#include "button.h"
#include "textwdgt.h"
#include "kbwin.h"
#include "winmgr.h"

// The three address-taken callbacks the constructor hands to its Begin
// and Back buttons and to the slider (retail 0x483880 / 0x4838c0 /
// 0x4838f0). Names provisional.
// Before normalization (function): CustomCampaignBeginHandler.
static int customCampaignBeginHandler(message& msg);
// Before normalization (function): CustomCampaignBackHandler.
static int customCampaignBackHandler(message& msg);
// Before normalization (function): CustomCampaignSliderHandler.
static void customCampaignSliderHandler(int state, heroWindow* window);

// Complete-only. The window's widget ids run from 200 for the frame,
// title, two buttons and selected-name text (the slider takes the next
// one), while the eighteen list rows use the fixed 100.. / 118.. ranges
// OnWidgetDeselect decodes.
//
// The three `send_message(WIDGET_CLEAR_STATUS, 6)` sites are
// widget::hide(): each expansion is an /Ob2 candidate site, and the site
// count is what decides the push_back whose vector::insert retail expands
// (the second one inside the loop and the slider's, never the first).
// Written as raw send_message calls the first loop push_back expands too
// and the row sits at 71.96; a dead candidate site after the loop
// measured the same lever at 97.43 before the real construct was named.
VA(0x004827b0, 0x727)  // DoCampaignWindow's CUSTOM_CAMPAIGN_ID arm + CamCust.pcx, retail-only
TCustomCampaignWindow::TCustomCampaignWindow()
    : CHeroWindowEx(0, 0, 800, 600, 0)
{
    int widgetId = 200;

    bitmapBorder* border = new bitmapBorder(
        0, 0, 800, 600, widgetId++,
        DATA_COMPGEN(0x00675594, customCampaignBackground, "CamCust.pcx"),
        0x800);
    m_widgets.push_back(border);
    border->m_image->draw(0, 0, 800, 600, g_windowManager->m_screenBitmap, 0, 0,
                        false);

    textWidget* title = new textWidget(
        25, 23, 366, 22,
        DATA_COMPGEN(0x00675580, customCampaignTitle, "Select a Campaign"),
        "medfont.fnt", font::HEADING_HIGHLIGHT, widgetId++, 1, 0, 8);
    m_widgets.push_back(title);

    type_func_button* beginButton = new type_func_button(
        414, 535, 166, 40, widgetId++,
        DATA_COMPGEN(0x00675574, customCampaignBeginSprite, "scnrbeg.def"),
        customCampaignBeginHandler, 0, 1);
    m_widgets.push_back(beginButton);

    type_func_button* backButton = new type_func_button(
        584, 535, 166, 40, widgetId++,
        DATA_COMPGEN(0x00675564, customCampaignBackSprite, "scnrback.def"),
        customCampaignBackHandler, 0, 1);
    backButton->setHotkey(1);
    m_widgets.push_back(backButton);

    m_selectedName = new textWidget(422, 46, 324, 30, "", "bigfont.fnt",
                                  font::HEADING_HIGHLIGHT, widgetId++, 0, 0,
                                  8);
    m_widgets.push_back(m_selectedName);

    m_description = new type_text_scroller("", 423, 107, 323, 393,
                                         "smalfont.fnt", font::WHITE,
                                         slider::BLUE);
    m_widgets.push_back(m_description);

    for (int i = 0; i < CAMPAIGN_LIST_ROWS; i++) {
        m_nameWidgets[i] = new textWidget(58, 122 + i * 25, 317, 25, "",
                                        "smalfont.fnt", font::WHITE, 100 + i,
                                        1, 0, 8);
        m_nameWidgets[i]->hide();
        m_widgets.push_back(m_nameWidgets[i]);
        m_countWidgets[i] = new textWidget(26, 122 + i * 25, 30, 23, "",
                                         "smalfont.fnt", font::WHITE, 118 + i,
                                         2, 0, 8);
        m_countWidgets[i]->hide();
        m_widgets.push_back(m_countWidgets[i]);
    }

    m_campaignSlider = new slider(376, 92, 16, 480, widgetId, 2,
                                customCampaignSliderHandler, slider::BLUE,
                                CAMPAIGN_LIST_ROWS, 0);
    m_campaignSlider->hide();
    m_widgets.push_back(m_campaignSlider);

    m_firstVisible = 0;
    m_selected = 0;
    addWidgetsToMessageStream();
    loadCampaignList();
    m_lastClickTime = GameTime::get();
}

VA_COMPGEN(0x00482ee0, 0x21, SCALAR_DELETING_DTOR, TCustomCampaignWindow)

// Complete-only. Frees every campaign header the scanner kept, then the
// widgets; the header vector's own teardown and ~heroWindow follow as
// compiler-generated member and base destruction.
VA(0x00482f10, 0xB1)  // scalar-dtor callee + derived vtable 0x63d6fc, retail-only
TCustomCampaignWindow::~TCustomCampaignWindow()
{
    for (unsigned i = 0; i < m_campaignHeaders.size(); i++) {
        if (m_campaignHeaders[i])
            delete m_campaignHeaders[i];
    }
    deleteWidgets();
}

// Complete-only. Scans Maps\\*.h3c through the CRT _find family (each step
// bracketed by a _getcwd retail kept), keeps every header that loads with
// at least one map, sorts by campaign name and sizes the slider for the
// eighteen-row list.
//
// The std::sort shape (retail CALLS _Sort and _Insertion_sort_1 and
// EXPANDS _Unguarded_insert) followed from the slider's `show()` site and
// the header vector's element type: with a void* element the push_back
// bound insert's const T& to a temporary copy of `header` instead of the
// local itself (65.03 -> 91.75 -> 99.92).
//
// Residual (99.92%): four frame bytes. Retail keeps the predicate copy
// _Unguarded_insert takes by value in its own slot ([ebp-0x20]) with the
// insert value at [ebp-0x1c]; our frame lets the value reuse the sort
// predicate's slot. Tried and rejected: a named predicate local and an
// explicit empty predicate constructor (both byte-flat).
VA(0x00482fd0, 0x264)  // TCustomCampaignWindow ctor callee + _findfirst("*.h3c"), retail-only
void TCustomCampaignWindow::loadCampaignList()
{
    char currentDirectory[100];
    _finddata_t fileInfo;
    TCampaignBrief::CampaignHeaderStruct* header;

    _getcwd(currentDirectory, sizeof(currentDirectory));
    _chdir(DATA_COMPGEN(0x006755ac, mapsDirectory, "Maps"));
    _getcwd(currentDirectory, sizeof(currentDirectory));
    long findHandle = _findfirst(
        DATA_COMPGEN(0x006755a4, campaignFilePattern, "*.h3c"), &fileInfo);
    _chdir(DATA_COMPGEN(0x006755a0, parentDirectory, ".."));
    _getcwd(currentDirectory, sizeof(currentDirectory));
    if (findHandle == -1)
        return;

    do {
        header = new TCampaignBrief::CampaignHeaderStruct(fileInfo.name);
        _getcwd(currentDirectory, sizeof(currentDirectory));
        if (!header->load()) {
            delete header;
        } else if (header->getNumMaps() == 0) {
            header->freeData();
            delete header;
        } else {
            header->freeData();
            m_campaignHeaders.push_back(header);
        }
        _getcwd(currentDirectory, sizeof(currentDirectory));
    } while (_findnext(findHandle, &fileInfo) == 0);
    _findclose(findHandle);
    _getcwd(currentDirectory, sizeof(currentDirectory));

    std::sort(m_campaignHeaders.begin(), m_campaignHeaders.end(),
              CampaignHeaderPointerLess());
    if (m_campaignHeaders.size() > CAMPAIGN_LIST_ROWS) {
        m_campaignSlider->show();
        m_campaignSlider->setResolution(
            m_campaignHeaders.size() - (CAMPAIGN_LIST_ROWS - 1));
    }
    updateList();
}

// Complete-only: the sort predicate, by campaign name. Retail evaluates
// the right operand's name first (VC6's right-to-left argument order for
// the inlined string operator<) and destroys the two temporaries in
// reverse.
VA(0x00483240, 0xEA)  // _Insertion_sort_1/_Sort predicate call sites, retail-only
bool CampaignHeaderPointerLess::operator()(
    TCampaignBrief::CampaignHeaderStruct* left,
    TCampaignBrief::CampaignHeaderStruct* right) const
{
    return left->getCampaignName() < right->getCampaignName();
}

// Complete-only. Refreshes the eighteen visible rows from the scroll
// origin, highlights the selected one, hides the rows past the end of
// the list, and mirrors the selection into the name text and the
// description scroller.
//
// Residual (93.80%): `this` and `i` are homed ebx/edi where retail has
// edi/ebx; every block is otherwise identical. The swap arrived with the
// header vector's retype (void* -> CampaignHeaderStruct*, worth +8 on
// LoadCampaignList) and no local spelling moves it - measured flat: the
// loop counter's declaration form and signedness, the condition order,
// pre-increment, the compare operand order, the tail's local/polarity
// (the `>=` else-first form IS retail's layout, +11), raw send_message
// against show()/hide(), and hoisting the name widget above the colour
// branch (loses 1.2). Include-set class.
VA(0x00483330, 0x281)  // LoadCampaignList's tail callee, retail-only
void TCustomCampaignWindow::updateList()
{
    int i;

    for (i = 0; i < CAMPAIGN_LIST_ROWS
                && i < m_campaignHeaders.size() + m_firstVisible; i++) {
        TCampaignBrief::CampaignHeaderStruct* header =
            m_campaignHeaders[m_firstVisible + i];
        m_nameWidgets[i]->setText(header->getCampaignName().c_str());
        m_countWidgets[i]->setText(
            formatString(DATA_COMPGEN(0x006755b4, campaignMapCountFormat,
                                       "%i"),
                          header->getNumMaps()).c_str());
        if (i == m_selected) {
            m_nameWidgets[i]->m_color = font::WHITE_HIGHLIGHT;
            m_countWidgets[i]->m_color = font::WHITE_HIGHLIGHT;
        } else {
            m_nameWidgets[i]->m_color = font::WHITE;
            m_countWidgets[i]->m_color = font::WHITE_HIGHLIGHT;
        }
        m_nameWidgets[i]->show();
        m_countWidgets[i]->show();
    }
    for (; i < CAMPAIGN_LIST_ROWS; i++) {
        m_nameWidgets[i]->hide();
        m_countWidgets[i]->hide();
    }

    int index = m_selected + m_firstVisible;
    if (index >= m_campaignHeaders.size()) {
        m_description->setText("");
    } else {
        TCampaignBrief::CampaignHeaderStruct* header = m_campaignHeaders[index];
        m_selectedName->setText(header->getCampaignName().c_str());
        m_description->setText(header->getCampaignDescription().c_str());
    }
}

// Complete-only. Widget ids 100..117 are the name column and 118..135
// the map-count column; either selects its row. A second click on the
// same row inside 400 ms accepts the campaign and closes the dialog.
// Before normalization (locals): bExitFlag.
VA(0x004835c0, 0xA4)  // anchor-vtable (slot 12 of 0x63d6fc), retail-only
int TCustomCampaignWindow::onWidgetDeselect(int id, unsigned char* exitFlag)
{
    if (id < 100 || id > 135)
        return 0;

    int row;
    if (id >= 118)
        row = id - 118;
    else
        row = id - 100;
    m_selected = row;

    if (GameTime::elapsedSince(m_lastClickTime) < 400) {
        if (acceptSelection()) {
            *exitFlag = 1;
            g_windowManager->m_dialogReturn = 1;
            return 1;
        }
    }
    m_lastClickTime = GameTime::get();
    updateList();
    drawWindow(1, 0xffff0001, 0xffff);
    return 1;
}

// Complete-only. The campaign ordinal 20 is the custom-campaign slot
// select_campaign reserves for a file chosen here.
VA(0x00483670, 0xCE)  // OnWidgetDeselect + Begin-button callee, retail-only
bool TCustomCampaignWindow::acceptSelection()
{
    int index = m_selected + m_firstVisible;
    if (index >= m_campaignHeaders.size())
        return 0;
    g_game->m_campaign.selectCampaign(
        20, m_campaignHeaders[index]->getFileName().c_str());
    return 1;
}

// The header-inline by-value getter retail retained as a /Gy COMDAT in
// this object (see campaignbrief.h); the body is the string copy
// constructor.
VA(0x00483740, 0x134)  // AcceptSelection's callee, retail-only
std::string TCampaignBrief::CampaignHeaderStruct::getFileName() const
{
    return m_fileName;
}

// Complete-only. The Begin button accepts the selection and closes the
// modal loop with codeY 1; the Back button closes it with codeY 0.
VA(0x00483880, 0x3C)  // ctor address-take (Begin button), retail-only
static int customCampaignBeginHandler(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        if (static_cast<TCustomCampaignWindow*>(msg.m_window)
                ->acceptSelection()) {
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeY = 1;
            msg.m_codeX = 10;
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    return 0;
}

VA(0x004838c0, 0x2E)  // ctor address-take (Back button), retail-only
static int customCampaignBackHandler(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = 0;
        msg.m_codeX = 10;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x004838f0, 0x25)  // ctor address-take (slider callback), retail-only
static void customCampaignSliderHandler(int state, heroWindow* window)
{
    TCustomCampaignWindow* campaignWindow =
        static_cast<TCustomCampaignWindow*>(window);
    campaignWindow->m_firstVisible = state;
    campaignWindow->updateList();
    campaignWindow->drawWindow(1, 0xffff0001, 0xffff);
}


// Exact Dinkumware _Insertion_sort_1 body retained by the Complete-only
// custom-campaign list sort. Retail proves four-byte pointer elements, the
// by-value empty predicate, the extra _Ty** discriminator, and all three
// out-of-line predicate calls. The stock VC6 template is byte-identical.
VA_COMPGEN(0x00483AA0, 0xA0, INSERTION_SORT_1,
           CampaignHeaderPointerLess)

// COMDAT pairing: std::_Sort<CampaignHeaderStruct*, CampaignHeaderPointerLess>
// - agreement 0.972 and the only object in the image emitting that
// instantiation is this one.
VA_COMPGEN(0x00483940, 0x151, STD_SORT, campaignheaderstruct_ptr)
