// hiscore.cpp - E:\gamedcs\hiscore.cpp (compiland hiscore.obj)
#include <va.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "bitmap816.h"
#include "border.h"
#include "button.h"
#include "game.h"
#include "hiscore.h"
#include "iconwdgt.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "mousemgr.h"
#include "resourcemanager.h"
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "winmgr.h"

// The file-name pointer and the threshold/creature pairs are both
// hiscore.obj-owned retail data.  The latter is deliberately only declared:
// this TU needs the first two signed shorts of each four-byte record, while
// admitting all 117 private table rows would add no code identity evidence.
DATA(0x0067f1f0)
static const char* g_highScoreFileName =
    DATA_COMPGEN(0x0067f4d0, highScoreFileName, "HiScore.dat");
DATA(0x0067f1f4) static int g_highScoreRanks[2];
DATA(0x0067f1fc) extern short g_highScoreCreatureTable[][2];
DATA(0x006a5ecc) extern char* g_highScoreDefaults0[11][4];
DATA(0x006a7f08) extern char* g_highScoreDefaults1[11][4];
DATA(0x006991c0) THighScoreWindow* g_highScoreWindow;
DATA(0x006993cc) highScoreManager* g_highScoreManager;
DATA(0x0069955c) int g_showHighScore;
int highScoreWindowHandler(message& msg);
void unnamed4f3a60(char* filename);
void memError();

VA(0x004e8fb0, 0xBD)  // dc 0xd7a08
void highScoreManager::resetHighScores()
{
    memset(m_highScores, 0, sizeof(m_highScores));
    for (int i = 0; i < 11; ++i) {
        strncpy(m_highScores[1][i].m_playerName, g_highScoreDefaults1[i][0], 41);
        strncpy(m_highScores[1][i].m_land, g_highScoreDefaults1[i][1], 41);
        m_highScores[1][i].m_days = atoi(g_highScoreDefaults1[i][2]);
        m_highScores[1][i].m_score = atoi(g_highScoreDefaults1[i][3]);

        strncpy(m_highScores[0][i].m_playerName, g_highScoreDefaults0[i][0], 41);
        strncpy(m_highScores[0][i].m_land, g_highScoreDefaults0[i][1], 41);
        m_highScores[0][i].m_days = atoi(g_highScoreDefaults0[i][2]);
        m_highScores[0][i].m_score = atoi(g_highScoreDefaults0[i][3]);
    }
}

VA(0x004e9070, 0x1C)  // dc 0xd7acc
highScoreManager::highScoreManager()
{
    m_highScoreType = 1;
}

VA(0x004e9090, 0x07)  // dc 0xd7b08
highScoreManager::~highScoreManager()
{
}

VA(0x004e90a0, 0x66)  // dc 0xd7b28
int highScoreManager::open(int newPriority)
{
    char path[351];
    sprintf(path,
            DATA_COMPGEN(0x00660358, highScorePathFormat, "%s%s"),
            DATA_COMPGEN(0x00677d88, highScoreDataDirectory, ".\\DATA\\"),
            g_highScoreFileName);
    int file = _open(path, _O_BINARY);
    if (file != -1) {
        _read(file, m_highScores, sizeof(m_highScores));
        _close(file);
    }
    return 0;
}

VA(0x004e9110, 0xC0)  // dc 0xd7bd0
void highScoreManager::viewHiScore()
{
    THighScoreWindow window;
    window.update();
    g_windowManager->doDialog(&window, highScoreWindowHandler, 0);
}

// Dreamcast hiscore.cpp:738 names WriteHighScores and preserves its
// 351-byte cBuf local even in the VMU port. Retail's two caller expansions
// prove the PC file path, flags, error handler and full score-table write.
void writeHighScores()
{
    char path[351];
    sprintf(path,
        DATA_COMPGEN(0x00660358, highScorePathFormat, "%s%s"),
        DATA_COMPGEN(0x00677d88, highScoreDataDirectory, ".\\DATA\\"),
        g_highScoreFileName);
    int file = _open(path, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY,
                     _S_IWRITE);
    if (file == -1) {
        fileError(g_highScoreFileName);
    } else {
        _write(file, g_highScoreManager->m_highScores,
               sizeof(g_highScoreManager->m_highScores));
        _close(file);
    }
}

// The retail build inlines this sole constructor use into
// AddScoreToHighScore.  Every widget argument below is byte-visible in that
// expansion; the three-entry reserve followed by four pushes also explains
// the one reallocating final insertion.
inline CHSInputDlg::CHSInputDlg(int maxChars)
    : CHeroWindowEx(284, 194, 232, 212, 0x12)
{
    m_widgets.reserve(3);
    m_widgets.push_back(new bitmapBorder(
        0, 0, m_width, m_height, 500,
        DATA_COMPGEN(0x0067f4e8, highScoreNameBackground, "HighName.pcx"),
        0x800));

    m_field1 = new CHighScoreEdit(
        18, 105, 199, 23, 40,
        DATA_COMPGEN(0x00691210, highScoreInputEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, highScoreSmallFont, "smalfont.fnt"),
        font::WHITE, 4, 0, 0, FIELD1_ID, 0x100, 0, 7, 5);
    m_header1 = new textWidget(
        13, 13, 205, 100, g_generalText->getText(97),
        DATA_COMPGEN(0x0065f2f8, highScoreSmallFont, "smalfont.fnt"),
        font::WHITE, -1, 1, 0, 8);
    m_widgets.push_back(m_field1);
    m_widgets.push_back(m_header1);
    m_widgets.push_back(new button(
        84, 143, 64, 32, OKAY_ID,
        DATA_COMPGEN(0x0067f4dc, highScoreOkayButton, "mubchck.def"),
        0, 1, 0, 28, 2));

    m_rollover = new textWidget(
        8, 186, 216, 18, 0,
        DATA_COMPGEN(0x0065f2f8, highScoreSmallFont, "smalfont.fnt"),
        font::PRIMARY, ROLLOVER_ID, 1, 32, 8);
    m_widgets.push_back(m_rollover);

    addWidgetsToMessageStream();
    setFocus(m_field1->m_id);
    m_field1->setFocus(1);
    m_field1->setAutoDraw(1);
}

VA(0x004e91d0, 0x4CC)  // dc 0xd7c3c
int highScoreManager::addScoreToHighScore(int score, int days,
    int difficulty, int scoreType, const char* land)
{
    unsigned char cheated = g_game->m_isCheater == 1;
    m_highScoreType = scoreType;
    HighScoreRec* scores = m_highScores[scoreType];
    int rank;

    if (!scoreType && g_game->m_campaign.m_isCheater) {
        cheated = 1;
        rank = 10;
    } else if (cheated) {
        rank = 10;
    } else {
        for (rank = 0; rank < 10; ++rank) {
            if (score > scores[rank].m_score
                    || scores[rank].m_score == -1)
                break;
        }
    }

    if (rank < 10) {
        for (int source = 9; source >= rank; --source)
            scores[source + 1] = scores[source];
    }

    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
    g_mouseManager->showPointer(false);

    {
        CHSInputDlg input(3);
        input.m_field1->setText(g_game->getLocalPlayer()->getName());
        input.doModal(0);

        memset(&scores[rank], 0, sizeof(scores[rank]));
        strcpy(scores[rank].m_playerName, input.m_field1->m_text.c_str());
    }

    strcpy(scores[rank].m_land,
        cheated ? g_generalText->getText(261) : land);
    scores[rank].m_score = score;
    scores[rank].m_days = days;
    scores[rank].m_difficulty = difficulty;
    scores[rank].m_cheated = cheated;
    g_highScoreRanks[scoreType == 1] = rank;

    writeHighScores();

    g_showHighScore = 1;
    return 0;
}

VA(0x004e96a0, 0x62)  // dc 0xd8d2c
CHighScoreEdit::CHighScoreEdit(int x, int y, int w, int h, int textSize,
    const char* text, const char* fontName, font::TColor color,
    unsigned justification, const char* backgroundIcon, int backgroundFrame,
    int id, int style, int readType, int insetX, int insetY)
    : textEntryWidget(x, y, w, h, textSize, text, fontName, color,
                      justification, backgroundIcon, backgroundFrame, id,
                      style, readType, insetX, insetY)
{
    m_nextEdit = 0;
    m_prevEdit = 0;
}

VA(0x004e9710, 0x2C)  // dc 0xd8dcc
int CHighScoreEdit::onKeyPress(message* msg)
{
    if (!m_hasFocus)
        return 0;
    int shift = GetKeyState(VK_SHIFT);
    return textEntryWidget::onKeyPress(msg);
}

VA(0x004e9740, 0x4E)  // dc 0xd914c
CHSInputDlg::~CHSInputDlg()
{
    deleteWidgets();
}

VA(0x004e9790, 0x53)  // dc 0xd9190
int CHSInputDlg::onWidgetDeselect(int id, bool& exitFlag)
{
    if (id == OKAY_ID) {
        if (!(m_field1->m_status & widget::WIDGET_ACTIVE)
            || strlen(m_field1->m_text.c_str())) {
            exitFlag = 1;
            g_windowManager->m_dialogReturn = DIALOG_RETURN_SPLIT_ACCEPT;
            return 1;
        }
    }
    return 0;
}

VA(0x004e97f0, 0x04)  // dc 0xd9204
textWidget* CHSInputDlg::getRolloverWidget()
{
    return m_rollover;
}

VA_COMPGEN(0x004e9800, 0x21, SCALAR_DELETING_DTOR, CHSInputDlg)

VA(0x004e9830, 0x48)  // dc 0xd7e0c
int highScoreManager::getMonType(int score, int scoreType)
{
    if (!scoreType)
        score /= 5;

    int i = 0;
    while (score > g_highScoreCreatureTable[i][0])
        ++i;
    return g_highScoreCreatureTable[i][1];
}

// Four controls precede the two 11-icon score families.  The family-one
// records are emitted first into the second pointer bank, then family zero;
// constant-folding GetMonType in each loop accounts for the one inline divide
// by five in the latter family only.
// Residual (90.7049%, measured 2026-09-01): all 46 CFG flows and the complete
// 35-call multiset agree; only four blocks differ in size.  Retail binds zero,
// this, and the widget-vector walk to EBX/ESI/EDI, while this compile binds the
// same call-crossing pseudos to EDI/EBX/ESI and strength-reduces each icon id
// instead of retaining `i` in a stack home.  The VC6 allocator model reports
// identical definition slots but different front-end processing state (C1
// class); its `i`/`y` declaration swap is byte-flat.  Negative controls:
// `volatile int i` falls to 77.7892%, and routing only the two icon ids through
// an int-reference alias falls to 90.6581%.  The DC-proven base construction,
// selector/frame/time setup, controls, icon families, widget registration,
// captured backgrounds, and active-family reveal remain in source order.
// E:\gamedcs\hiscore.cpp:858
VA(0x004e9880, 0x506)  // vtable/global/widget/resource xrefs, dc 0xd7e3c
THighScoreWindow::THighScoreWindow()
    : heroWindow(0, 0, 800, 600, 0)
{
    int i;
    int y;
    button* okay;

    m_isStandard = g_highScoreManager->m_highScoreType == 1;
    m_creatureFrame = 0;
    m_lastServe = GameTime::get();
    g_highScoreWindow = this;

    m_widgets.reserve(18);

    okay = new button(
        726, 346, 41, 199, DIALOG_RETURN_OK,
        DATA_COMPGEN(0x0067f538, highScoreExitButton, "HiScExt.def"),
        0, 1, 0, 28, 2);
    okay->setHotkey(1);
    m_widgets.push_back(okay);
    m_widgets.push_back(new button(
        726, 114, 41, 199, 1003,
        DATA_COMPGEN(0x0067f52c, highScoreResetButton, "HiScRes.def"),
        0, 1, 0, 0, 2));
    m_widgets.push_back(new button(
        31, 114, 41, 199, 1001,
        DATA_COMPGEN(0x0067f520, highScoreCampaignButton, "HiScCam.def"),
        0, 1, 0, 46, 2));
    m_widgets.push_back(new button(
        31, 346, 41, 199, 1002,
        DATA_COMPGEN(0x0067f514, highScoreStandardButton, "HiScSta.def"),
        0, 1, 0, 31, 2));

    memset(m_creatureFrames, 0, sizeof(m_creatureFrames));
    for (i = 0, y = 26; y < 576; ++i, y += 50) {
        m_creatures[1][i] = new iconWidget(
            649, y, 64, 64, 1004 + i,
            g_game->m_worldMap.newfullMapFn00505EA0(
                MONSTER, highScoreManager::getMonType(
                    g_highScoreManager->m_highScores[1][i].m_score,
                    1))->m_imageName.c_str(),
            0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_creatures[1][i]->sendMessage(
            widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_widgets.push_back(m_creatures[1][i]);
    }
    for (i = 0, y = 26; y < 576; ++i, y += 50) {
        m_creatures[0][i] = new iconWidget(
            649, y, 64, 64, 1015 + i,
            g_game->m_worldMap.newfullMapFn00505EA0(
                MONSTER, highScoreManager::getMonType(
                    g_highScoreManager->m_highScores[0][i].m_score,
                    0))->m_imageName.c_str(),
            0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_creatures[0][i]->sendMessage(
            widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_widgets.push_back(m_creatures[0][i]);
    }

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    m_hiScoreBack[0] = ResourceManager::getBitmap816(
        DATA_COMPGEN(0x0067f504, highScoreBackground0, "hiscore2.pcx"));
    m_hiScoreBack[1] = ResourceManager::getBitmap816(
        DATA_COMPGEN(0x0067f4f8, highScoreBackground1, "hiscore.pcx"));

    for (i = 0; i < 11; ++i) {
        g_highScoreWindow->m_creatures[g_highScoreWindow->m_isStandard][i]
            ->sendMessage(widget::WIDGET_SET_STATUS,
                           widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    }
}

VA_COMPGEN(0x004e9d90, 0x21, SCALAR_DELETING_DTOR, THighScoreWindow)

VA(0x004e9dc0, 0x81)  // dc 0xd8424
THighScoreWindow::~THighScoreWindow()
{
    m_hiScoreBack[1]->dispose();
    m_hiScoreBack[0]->dispose();
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// The two numeric columns are asymmetric, which is what fixes the family
// selector's polarity: a STANDARD sheet shows days (field_58) at x=0x213 in a
// 0x34-wide box and the score (field_54) again at 0x259, while a campaign
// sheet shows the score alone at 0x213 in a 0x7a-wide box. Text indices are
// raw here, as highScoreManager::AddScoreToHighScore's own GetText(261) is.

VA(0x004e9e50, 0x372)  // dc 0xd849c
void THighScoreWindow::update()
{
    getWidget(STANDARD_ID)->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                  widget::WIDGET_HIGHLIGHTED);
    getWidget(CAMPAIGN_ID)->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                  widget::WIDGET_HIGHLIGHTED);

    if (m_isStandard)
        getWidget(STANDARD_ID)->sendMessage(widget::WIDGET_SET_STATUS,
                                             widget::WIDGET_HIGHLIGHTED);
    else
        getWidget(CAMPAIGN_ID)->sendMessage(widget::WIDGET_SET_STATUS,
                                             widget::WIDGET_HIGHLIGHTED);

    m_hiScoreBack[m_isStandard]->draw(m_x, m_y, m_width, m_height,
                                   g_windowManager->m_screenBitmap, 0, 0,
                                   false);

    g_mediumFont->drawBoundedString(
        g_generalText->getText(434), g_windowManager->m_screenBitmap,
        0x58, 0xb, 0x3a, 0x1a, font::PRIMARY,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    g_mediumFont->drawBoundedString(
        g_generalText->getText(435), g_windowManager->m_screenBitmap,
        0xa3, 0xb, 0x7a, 0x1a, font::PRIMARY,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    const char* landHeading;
    if (m_isStandard)
        landHeading = g_generalText->getText(436);
    else
        landHeading = g_generalText->getText(673);
    g_mediumFont->drawBoundedString(
        landHeading, g_windowManager->m_screenBitmap,
        0x12f, 0xb, 0xd2, 0x1a, font::PRIMARY,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);

    const char* valueHeading;
    if (m_isStandard)
        valueHeading = g_generalText->getText(437);
    else
        valueHeading = g_generalText->getText(76);
    g_mediumFont->drawBoundedString(
        valueHeading, g_windowManager->m_screenBitmap,
        0x213, 0xb, m_isStandard ? 0x34 : 0x7a, 0x1a, font::PRIMARY,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    if (m_isStandard) {
        g_mediumFont->drawBoundedString(
            g_generalText->getText(76), g_windowManager->m_screenBitmap,
            0x259, 0xb, 0x34, 0x1a, font::PRIMARY,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    }

    int i = 0;
    for (int y = 0x35; y < 0x25b; ++i, y += 0x32) {
        int color = g_highScoreRanks[m_isStandard] == i
                        ? font::HEADING_HIGHLIGHT
                        : font::PRIMARY;
        HighScoreRec* currentRec =
            &g_highScoreManager->m_highScores[m_isStandard][i];

        sprintf(g_text, DATA_COMPGEN(0x00660a1c, highScoreDecimalFormat, "%d"),
                i + 1);
        g_mediumFont->drawBoundedString(
            g_text, g_windowManager->m_screenBitmap,
            0x58, y, 0x3a, 0x1a, color,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
        g_mediumFont->drawBoundedString(
            currentRec->m_playerName, g_windowManager->m_screenBitmap,
            0xa3, y, 0x7a, 0x1a, color,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
        g_mediumFont->drawBoundedString(
            currentRec->m_land, g_windowManager->m_screenBitmap,
            0x12f, y, 0xd2, 0x1a, color,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);

        int value;
        if (m_isStandard)
            value = currentRec->m_days;
        else
            value = currentRec->m_score;
        sprintf(g_text, DATA_COMPGEN(0x00660a1c, highScoreDecimalFormat, "%d"),
                value);
        g_mediumFont->drawBoundedString(
            g_text, g_windowManager->m_screenBitmap,
            0x213, y, m_isStandard ? 0x34 : 0x7a, 0x1a, color,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);

        if (m_isStandard) {
            sprintf(g_text,
                    DATA_COMPGEN(0x00660a1c, highScoreDecimalFormat, "%d"),
                    currentRec->m_score);
            g_mediumFont->drawBoundedString(
                g_text, g_windowManager->m_screenBitmap,
                0x259, y, 0x34, 0x1a, color,
                font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
        }
    }
}

// Dreamcast hiscore.cpp:1014-1031 names UpdateCreatures and owns its
// update/draw tail. Complete interleaves hide/hide/show for each of eleven
// rows, replacing the Dreamcast port's separate hide and five-row show loops.
static void updateCreatures()
{
    for (int row = 0; row < 11; ++row) {
        g_highScoreWindow->m_creatures[g_highScoreWindow->m_isStandard][row]
            ->hide();
        g_highScoreWindow->m_creatures[!g_highScoreWindow->m_isStandard][row]
            ->hide();
        g_highScoreWindow->m_creatures[g_highScoreWindow->m_isStandard][row]
            ->show();
    }
    g_highScoreWindow->update();
    g_highScoreWindow->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                 WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x004ea1d0, 0x458)  // dc 0xd8970
int highScoreWindowHandler(message& msg)
{
    bool endDialog = false;
    pollSound();

    if (msg.m_id == MESSAGE_KEY_DOWN) {
        switch (msg.m_codeX) {
        case KEYCODE_ESCAPE:
        case KEYCODE_ENTER:
            msg.m_codeY = DIALOG_RETURN_OK;
            endDialog = true;
            break;
        default:
            break;
        }

    }
    else if (msg.m_id == MESSAGE_WIDGET) {
        if (msg.m_codeX != widget::WIDGET_DESELECT)
            return MESSAGE_DISPATCH_CONSUME;

        switch (msg.m_codeY) {
        case THighScoreWindow::STANDARD_ID:
            g_highScoreWindow->m_isStandard = 1;
            updateCreatures();
            break;

        case THighScoreWindow::CAMPAIGN_ID:
            g_highScoreWindow->m_isStandard = 0;
            updateCreatures();
            break;

        case THighScoreWindow::RESET_ID:
            {
            normalDialog(g_generalText->getText(667), 2, -1, -1, -1, 0, -1,
                         0, -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                return MESSAGE_DISPATCH_CONSUME;

            g_highScoreManager->resetHighScores();

            writeHighScores();

            for (int reset = 0; reset < 11; ++reset) {
                int monsterType = highScoreManager::getMonType(
                    g_highScoreManager->m_highScores[1][reset].m_score,
                    1);
                g_highScoreWindow->m_creatures[1][reset]->setSprite(
                    g_game->m_worldMap.newfullMapFn00505EA0(
                        MONSTER, monsterType)->m_imageName.c_str());
                g_highScoreWindow->m_creatures[1][reset]->setIconFrame(0);
                monsterType = highScoreManager::getMonType(
                    g_highScoreManager->m_highScores[0][reset].m_score,
                    0);
                g_highScoreWindow->m_creatures[0][reset]->setSprite(
                    g_game->m_worldMap.newfullMapFn00505EA0(
                        MONSTER, monsterType)->m_imageName.c_str());
                g_highScoreWindow->m_creatures[0][reset]->setIconFrame(0);
            }
            }
            break;

        case DIALOG_RETURN_OK:
            endDialog = true;
            break;

        default:
            break;
        }

    }
    else {
        unsigned long now = GameTime::get();
        if (GameTime::elapsed(now, g_highScoreWindow->m_lastServe) <= 200)
            return MESSAGE_DISPATCH_CONSUME;

        g_highScoreWindow->m_lastServe = now;
        for (int frame = 0; frame < 11; ++frame) {
            int& frameNumber = g_highScoreWindow
                ->m_creatureFrames[g_highScoreWindow->m_isStandard][frame];
            iconWidget*& creature = g_highScoreWindow
                ->m_creatures[g_highScoreWindow->m_isStandard][frame];
            ++frameNumber;
            if (frameNumber >= creature->m_sprite->getNumFrames(0))
                frameNumber = 0;
            creature->sendMessage(widget::WIDGET_SET_ICON_FRAME, frameNumber);
        }
        g_highScoreWindow->update();
        g_highScoreWindow->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                      WINDOW_ALL_WIDGETS_HIGH);
    }

    if (endDialog) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg.m_codeY;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

#if 0  // @carcass

// E:\gamedcs\hiscore.cpp:615
DC_ONLY(0xd7a08, 0xC4)
void highScoreManager::resetHighScores()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:637
DC_ONLY(0xd7acc, 0x3A)
void highScoreManager::highScoreManager()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:648
DC_ONLY(0xd7b08, 0x20)
void highScoreManager::~highScoreManager()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:658
DC_ONLY(0xd7b28, 0x5E)
int highScoreManager::Open(int newPriority)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:711
DC_ONLY(0xd7b88, 0x44)
void highScoreManager::Close()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:721
DC_ONLY(0xd7bcc, 0x4)
int highScoreManager::main(message* msg)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:732
DC_ONLY(0xd7bd0, 0x22)
void highScoreManager::viewHiScore()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:738
DC_ONLY(0xd7bf4, 0x46)
void WriteHighScores()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:772
DC_ONLY(0xd7c3c, 0x1D0)
int highScoreManager::addScoreToHighScore(int iScore, int iDays, int iDiffRating, int iHighScoreType, const char* cLand)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:837
DC_ONLY(0xd7e0c, 0x2E)
int highScoreManager::getMonType(int iScore, int iScoreType)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:858
DC_ONLY(0xd7e3c, 0x5C4)
void THighScoreWindow::THighScoreWindow()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:940
DC_ONLY(0xd8400, 0x22)
void THighScoreWindow::doModal()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:953
DC_ONLY(0xd8424, 0x76)
void THighScoreWindow::~THighScoreWindow()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:969
DC_ONLY(0xd849c, 0x3AC)
void THighScoreWindow::update()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:1014
DC_ONLY(0xd8848, 0x126)
void UpdateCreatures()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:1034
DC_ONLY(0xd8970, 0x3A4)
int highScoreWindowHandler(message* msg)
{
    // @stub
}

// E:\gamedcs\TextWdgt.h:67
DC_ONLY(0xd8d14, 0x18)
const char* textWidget::getText()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:179
DC_ONLY(0xd8d2c, 0xA0)
void CHighScoreEdit::CHighScoreEdit(int textWidgetX, int textWidgetY, int textWidgetWidth, int textWidgetHeight, int textStringSize, char* textString, char* textFontName, int colorIndex, font::EJustify justification, char* backgroundIconName, int backgroundFrame, int textWidgetId, int textWidgetStyle, int iReadType, int textInsetX, int textInsetY)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:195
DC_ONLY(0xd8dcc, 0x3C)
int CHighScoreEdit::onKeyPress(message* msg)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:225
DC_ONLY(0xd8e08, 0x28)
void CHighScoreEdit::onNextEdit()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:239
DC_ONLY(0xd8e30, 0x28)
void CHighScoreEdit::onPrevEdit()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:252
DC_ONLY(0xd8e58, 0x18)
void CHighScoreEdit::setFocus(unsigned char state)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:256
DC_ONLY(0xd8e70, 0x34)
void* CHighScoreEdit::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:256
DC_ONLY(0xd8ea4, 0x18)
void CHighScoreEdit::~CHighScoreEdit()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:293
DC_ONLY(0xd8ebc, 0x290)
void CHSInputDlg::CHSInputDlg(int maxChars1)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:329
DC_ONLY(0xd914c, 0x44)
void CHSInputDlg::~CHSInputDlg()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:334
DC_ONLY(0xd9190, 0x3C)
int CHSInputDlg::onWidgetDeselect(int id, bool& bExitFlag)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:357
DC_ONLY(0xd91cc, 0x38)
unsigned char CHSInputDlg::onOK()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:369
DC_ONLY(0xd9204, 0x6)
textWidget* CHSInputDlg::getRolloverWidget()
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:371
DC_ONLY(0xd920c, 0x88)
int CHSInputDlg::windowHandler(message* msg)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:382
DC_ONLY(0xd9294, 0x34)
void* CHSInputDlg::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\hiscore.cpp:929
DC_ONLY(0xd92c8, 0x34)
void* THighScoreWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: vector<widget*>::reserve, agreement 0.993.
VA_COMPGEN(0x004ea630, 0x9F, VECTOR_RESERVE, widget)
