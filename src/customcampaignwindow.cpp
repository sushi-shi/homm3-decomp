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
static int CustomCampaignBeginHandler(message& msg);
static int CustomCampaignBackHandler(message& msg);
static void CustomCampaignSliderHandler(int state, heroWindow* window);

// Complete-only. The window's widget ids run from 200 for the frame,
// title, two buttons and selected-name text (the slider takes the next
// one), while the eighteen list rows use the fixed 100.. / 118.. ranges
// OnWidgetDeselect decodes.
VA(0x004827b0, 0x727)  // DoCampaignWindow's CUSTOM_CAMPAIGN_ID arm + CamCust.pcx, retail-only
TCustomCampaignWindow::TCustomCampaignWindow()
    : CHeroWindowEx(0, 0, 800, 600, 0)
{
    int widgetId = 200;

    bitmapBorder* border = new bitmapBorder(
        0, 0, 800, 600, widgetId++,
        DATA_COMPGEN(0x00675594, customCampaignBackground, "CamCust.pcx"),
        0x800);
    Widgets.push_back(border);
    border->image->Draw(0, 0, 800, 600, gpWindowManager->screenBitmap, 0, 0,
                        false);

    textWidget* title = new textWidget(
        25, 23, 366, 22,
        DATA_COMPGEN(0x00675580, customCampaignTitle, "Select a Campaign"),
        "medfont.fnt", font::HEADING_HIGHLIGHT, widgetId++, 1, 0, 8);
    Widgets.push_back(title);

    type_func_button* beginButton = new type_func_button(
        414, 535, 166, 40, widgetId++,
        DATA_COMPGEN(0x00675574, customCampaignBeginSprite, "scnrbeg.def"),
        CustomCampaignBeginHandler, 0, 1);
    Widgets.push_back(beginButton);

    type_func_button* backButton = new type_func_button(
        584, 535, 166, 40, widgetId++,
        DATA_COMPGEN(0x00675564, customCampaignBackSprite, "scnrback.def"),
        CustomCampaignBackHandler, 0, 1);
    backButton->set_hotkey(1);
    Widgets.push_back(backButton);

    selectedName = new textWidget(422, 46, 324, 30, "", "bigfont.fnt",
                                  font::HEADING_HIGHLIGHT, widgetId++, 0, 0,
                                  8);
    Widgets.push_back(selectedName);

    description = new type_text_scroller("", 423, 107, 323, 393,
                                         "smalfont.fnt", font::WHITE,
                                         slider::BLUE);
    Widgets.push_back(description);

    for (int i = 0; i < CAMPAIGN_LIST_ROWS; i++) {
        nameWidgets[i] = new textWidget(58, 122 + i * 25, 317, 25, "",
                                        "smalfont.fnt", font::WHITE, 100 + i,
                                        1, 0, 8);
        nameWidgets[i]->send_message(widget::WIDGET_CLEAR_STATUS, 6);
        Widgets.push_back(nameWidgets[i]);
        countWidgets[i] = new textWidget(26, 122 + i * 25, 30, 23, "",
                                         "smalfont.fnt", font::WHITE, 118 + i,
                                         2, 0, 8);
        countWidgets[i]->send_message(widget::WIDGET_CLEAR_STATUS, 6);
        Widgets.push_back(countWidgets[i]);
    }

    campaignSlider = new slider(376, 92, 16, 480, widgetId, 2,
                                CustomCampaignSliderHandler, slider::BLUE,
                                CAMPAIGN_LIST_ROWS, 0);
    campaignSlider->send_message(widget::WIDGET_CLEAR_STATUS, 6);
    Widgets.push_back(campaignSlider);

    firstVisible = 0;
    selected = 0;
    AddWidgetsToMessageStream();
    LoadCampaignList();
    lastClickTime = GameTime::Get();
}

VA_COMPGEN(0x00482ee0, 0x21, SCALAR_DELETING_DTOR, TCustomCampaignWindow)

// Complete-only. Frees every campaign header the scanner kept, then the
// widgets; the header vector's own teardown and ~heroWindow follow as
// compiler-generated member and base destruction.
VA(0x00482f10, 0xB1)  // scalar-dtor callee + derived vtable 0x63d6fc, retail-only
TCustomCampaignWindow::~TCustomCampaignWindow()
{
    for (unsigned i = 0; i < campaignHeaders.size(); i++) {
        if (campaignHeaders[i])
            delete static_cast<TCampaignBrief::CampaignHeaderStruct*>(
                campaignHeaders[i]);
    }
    delete_widgets();
}

// Complete-only. Scans Maps\\*.h3c through the CRT _find family (each step
// bracketed by a _getcwd retail kept), keeps every header that loads with
// at least one map, sorts by campaign name and sizes the slider for the
// eighteen-row list.
//
// Residual (65.03%): the std::sort expansion. Retail CALLS _Sort (its
// COMDAT is 0x483940, with _Median/_Unguarded_partition folded in) and
// _Insertion_sort_1, and EXPANDS _Unguarded_insert (the predicate is
// called straight from the unguarded loop); our /Ob2 spends the budget
// the other way round - _Sort expanded with its children called, and
// _Unguarded_insert called. Every other site agrees (the ctor, Load,
// GetNumMaps and FreeData are cross-TU calls, push_back's insert(it,n,x)
// COMDAT is called on both sides). The remaining call rows are name-only
// (_getcwd, operator new, the Load/UpdateList stubs). Pins are not used
// in this lane; the lever is the /Ob2 site budget.
VA(0x00482fd0, 0x264)  // TCustomCampaignWindow ctor callee + _findfirst("*.h3c"), retail-only
void TCustomCampaignWindow::LoadCampaignList()
{
    char currentDirectory[100];
    _finddata_t fileInfo;

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
        TCampaignBrief::CampaignHeaderStruct* header =
            new TCampaignBrief::CampaignHeaderStruct(fileInfo.name);
        _getcwd(currentDirectory, sizeof(currentDirectory));
        if (!header->Load()) {
            delete header;
        } else if (header->GetNumMaps() == 0) {
            header->FreeData();
            delete header;
        } else {
            header->FreeData();
            campaignHeaders.push_back(header);
        }
        _getcwd(currentDirectory, sizeof(currentDirectory));
    } while (_findnext(findHandle, &fileInfo) == 0);
    _findclose(findHandle);
    _getcwd(currentDirectory, sizeof(currentDirectory));

    std::sort(campaignHeaders.begin(), campaignHeaders.end(),
              CampaignHeaderPointerLess());
    if (campaignHeaders.size() > CAMPAIGN_LIST_ROWS) {
        campaignSlider->send_message(widget::WIDGET_SET_STATUS, 6);
        campaignSlider->SetResolution(
            campaignHeaders.size() - (CAMPAIGN_LIST_ROWS - 1));
    }
    UpdateList();
}

// Complete-only: the sort predicate, by campaign name. Retail evaluates
// the right operand's name first (VC6's right-to-left argument order for
// the inlined string operator<) and destroys the two temporaries in
// reverse.
VA(0x00483240, 0xEA)  // _Insertion_sort_1/_Sort predicate call sites, retail-only
bool CampaignHeaderPointerLess::operator()(void* left, void* right) const
{
    return static_cast<TCampaignBrief::CampaignHeaderStruct*>(left)
               ->GetCampaignName()
           < static_cast<TCampaignBrief::CampaignHeaderStruct*>(right)
                 ->GetCampaignName();
}

// Complete-only. Refreshes the eighteen visible rows from the scroll
// origin, highlights the selected one, hides the rows past the end of
// the list, and mirrors the selection into the name text and the
// description scroller.
VA(0x00483330, 0x281)  // LoadCampaignList's tail callee, retail-only
void TCustomCampaignWindow::UpdateList()
{
    int i;

    for (i = 0; i < CAMPAIGN_LIST_ROWS
                && i < campaignHeaders.size() + firstVisible; i++) {
        TCampaignBrief::CampaignHeaderStruct* header =
            static_cast<TCampaignBrief::CampaignHeaderStruct*>(
                campaignHeaders[firstVisible + i]);
        nameWidgets[i]->SetText(header->GetCampaignName().c_str());
        countWidgets[i]->SetText(
            format_string(DATA_COMPGEN(0x006755b4, campaignMapCountFormat,
                                       "%i"),
                          header->GetNumMaps()).c_str());
        if (i == selected) {
            nameWidgets[i]->Color = font::WHITE_HIGHLIGHT;
            countWidgets[i]->Color = font::WHITE_HIGHLIGHT;
        } else {
            nameWidgets[i]->Color = font::WHITE;
            countWidgets[i]->Color = font::WHITE_HIGHLIGHT;
        }
        nameWidgets[i]->send_message(widget::WIDGET_SET_STATUS, 6);
        countWidgets[i]->send_message(widget::WIDGET_SET_STATUS, 6);
    }
    for (; i < CAMPAIGN_LIST_ROWS; i++) {
        nameWidgets[i]->send_message(widget::WIDGET_CLEAR_STATUS, 6);
        countWidgets[i]->send_message(widget::WIDGET_CLEAR_STATUS, 6);
    }

    if (selected + firstVisible < campaignHeaders.size()) {
        TCampaignBrief::CampaignHeaderStruct* header =
            static_cast<TCampaignBrief::CampaignHeaderStruct*>(
                campaignHeaders[selected + firstVisible]);
        selectedName->SetText(header->GetCampaignName().c_str());
        description->SetText(header->GetCampaignDescription().c_str());
    } else {
        description->SetText("");
    }
}

// Complete-only. Widget ids 100..117 are the name column and 118..135
// the map-count column; either selects its row. A second click on the
// same row inside 400 ms accepts the campaign and closes the dialog.
VA(0x004835c0, 0xA4)  // anchor-vtable (slot 12 of 0x63d6fc), retail-only
int TCustomCampaignWindow::OnWidgetDeselect(int id, unsigned char* bExitFlag)
{
    if (id < 100 || id > 135)
        return 0;

    int row;
    if (id >= 118)
        row = id - 118;
    else
        row = id - 100;
    selected = row;

    if (GameTime::ElapsedSince(lastClickTime) < 400) {
        if (AcceptSelection()) {
            *bExitFlag = 1;
            gpWindowManager->dialogReturn = 1;
            return 1;
        }
    }
    lastClickTime = GameTime::Get();
    UpdateList();
    DrawWindow(1, 0xffff0001, 0xffff);
    return 1;
}

// Complete-only. The campaign ordinal 20 is the custom-campaign slot
// select_campaign reserves for a file chosen here.
VA(0x00483670, 0xCE)  // OnWidgetDeselect + Begin-button callee, retail-only
bool TCustomCampaignWindow::AcceptSelection()
{
    if (selected + firstVisible < campaignHeaders.size()) {
        TCampaignBrief::CampaignHeaderStruct* header =
            static_cast<TCampaignBrief::CampaignHeaderStruct*>(
                campaignHeaders[selected + firstVisible]);
        gpGame->campaign.select_campaign(20, header->GetFileName().c_str());
        return 1;
    }
    return 0;
}

// The header-inline by-value getter retail retained as a /Gy COMDAT in
// this object (see campaignbrief.h); the body is the string copy
// constructor.
VA(0x00483740, 0x134)  // AcceptSelection's callee, retail-only
std::string TCampaignBrief::CampaignHeaderStruct::GetFileName() const
{
    return file_name;
}

// Complete-only. The Begin button accepts the selection and closes the
// modal loop with codeY 1; the Back button closes it with codeY 0.
VA(0x00483880, 0x3C)  // ctor address-take (Begin button), retail-only
static int CustomCampaignBeginHandler(message& msg)
{
    if (msg.codeX == widget::WIDGET_DESELECT && !(msg.qualifier & 0x200)) {
        if (static_cast<TCustomCampaignWindow*>(msg.window)
                ->AcceptSelection()) {
            msg.id = MESSAGE_WIDGET;
            msg.codeY = 1;
            msg.codeX = 10;
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    return 0;
}

VA(0x004838c0, 0x2E)  // ctor address-take (Back button), retail-only
static int CustomCampaignBackHandler(message& msg)
{
    if (msg.codeX == widget::WIDGET_DESELECT && !(msg.qualifier & 0x200)) {
        msg.id = MESSAGE_WIDGET;
        msg.codeY = 0;
        msg.codeX = 10;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x004838f0, 0x25)  // ctor address-take (slider callback), retail-only
static void CustomCampaignSliderHandler(int state, heroWindow* window)
{
    TCustomCampaignWindow* campaignWindow =
        static_cast<TCustomCampaignWindow*>(window);
    campaignWindow->firstVisible = state;
    campaignWindow->UpdateList();
    campaignWindow->DrawWindow(1, 0xffff0001, 0xffff);
}


// Exact Dinkumware _Insertion_sort_1 body retained by the Complete-only
// custom-campaign list sort. Retail proves four-byte pointer elements, the
// by-value empty predicate, the extra _Ty** discriminator, and all three
// out-of-line predicate calls. The stock VC6 template is byte-identical.
VA_COMPGEN(0x00483AA0, 0xA0, INSERTION_SORT_1,
           CampaignHeaderPointerLess)
