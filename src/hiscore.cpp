#include "text.h"
#include "va.h"

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>

#include "hiscore.h"

#include "bitmap816.h"
#include "border.h"
#include "button.h"
#include "game.h"
#include "iconwdgt.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "mousemgr.h"
#include "resourcemanager.h"
#include "soundmgr.h"
#include "textntry.h"
#include "textresource.h"
#include "textwdgt.h"
#include "winmgr.h"

// Initial contents recovered from the pinned Complete image.
DATA(0x0067f1fc) short g_highScoreCreatureTable[118][2] = {
    { 4, 42 },
    { 8, 28 },
    { 12, 98 },
    { 16, 70 },
    { 20, 43 },
    { 24, 56 },
    { 28, 84 },
    { 32, 29 },
    { 36, 85 },
    { 40, 0 },
    { 44, 71 },
    { 48, 57 },
    { 52, 99 },
    { 56, 58 },
    { 60, 14 },
    { 64, 1 },
    { 68, 2 },
    { 72, 100 },
    { 76, 59 },
    { 80, 86 },
    { 84, 15 },
    { 88, 16 },
    { 92, 72 },
    { 96, 101 },
    { 100, 44 },
    { 104, 30 },
    { 108, 3 },
    { 112, 88 },
    { 116, 31 },
    { 120, 87 },
    { 124, 17 },
    { 128, 18 },
    { 132, 73 },
    { 136, 45 },
    { 140, 89 },
    { 144, 32 },
    { 148, 60 },
    { 152, 104 },
    { 156, 105 },
    { 160, 61 },
    { 164, 115 },
    { 168, 113 },
    { 172, 19 },
    { 176, 74 },
    { 180, 114 },
    { 184, 4 },
    { 187, 112 },
    { 190, 46 },
    { 193, 75 },
    { 196, 47 },
    { 199, 33 },
    { 202, 90 },
    { 205, 6 },
    { 208, 48 },
    { 211, 5 },
    { 214, 49 },
    { 217, 8 },
    { 220, 22 },
    { 223, 76 },
    { 226, 20 },
    { 229, 21 },
    { 232, 106 },
    { 235, 62 },
    { 238, 34 },
    { 241, 77 },
    { 244, 7 },
    { 247, 116 },
    { 250, 91 },
    { 253, 35 },
    { 256, 107 },
    { 259, 9 },
    { 262, 50 },
    { 265, 117 },
    { 268, 63 },
    { 271, 23 },
    { 274, 78 },
    { 277, 64 },
    { 280, 36 },
    { 283, 102 },
    { 286, 37 },
    { 289, 92 },
    { 292, 103 },
    { 295, 79 },
    { 298, 65 },
    { 301, 93 },
    { 304, 51 },
    { 307, 94 },
    { 310, 108 },
    { 313, 95 },
    { 316, 109 },
    { 319, 80 },
    { 322, 81 },
    { 325, 52 },
    { 328, 24 },
    { 331, 53 },
    { 334, 10 },
    { 337, 38 },
    { 340, 25 },
    { 343, 66 },
    { 346, 11 },
    { 349, 67 },
    { 352, 39 },
    { 355, 96 },
    { 358, 68 },
    { 361, 40 },
    { 364, 110 },
    { 367, 69 },
    { 370, 82 },
    { 373, 26 },
    { 376, 12 },
    { 379, 54 },
    { 382, 111 },
    { 385, 97 },
    { 388, 55 },
    { 391, 41 },
    { 394, 27 },
    { 397, 83 },
    { 32767, 13 }
};

// The file-name pointer and the threshold/creature pairs are both
// hiscore.obj-owned retail data.  The latter is deliberately only declared:
// this TU needs the first two signed shorts of each four-byte record, while
// admitting all 117 private table rows would add no code identity evidence.
DATA(0x0067f1f0)
static const char* g_highScoreFileName =
    DATA_COMPGEN(0x0067f4d0, highScoreFileName, "HiScore.dat");
DATA(0x0067f1f4) static int g_highScoreRanks[2];

DATA(0x006991c0) THighScoreWindow* g_highScoreWindow;
DATA(0x006993cc) highScoreManager* g_highScoreManager;
DATA(0x0069955c) int g_showHighScore;
int highScoreWindowHandler(message& msg);

// The dialog methods originate in hiscore.cpp; their declarations retain
// the default branch's hiscore.h location.

// Original: CHighScoreEdit::OnNextEdit; hiscore.cpp:225, dc 0xd8e08.
// Retail CHighScoreEdit vtable 0x63ebf4 shares slots 19/20 with CMPEdit
// (0x510850/0x510870), whose two edit links have the same offsets.
void CHighScoreEdit::onNextEdit()
{
    if (m_nextEdit && (m_nextEdit->m_status & widget::WIDGET_ACTIVE))
        m_parentWindow->setFocus(m_nextEdit->m_id);
}

// Original: CHighScoreEdit::OnPrevEdit; hiscore.cpp:239, dc 0xd8e30.
void CHighScoreEdit::onPrevEdit()
{
    if (m_prevEdit && (m_prevEdit->m_status & widget::WIDGET_ACTIVE))
        m_parentWindow->setFocus(m_prevEdit->m_id);
}

// Original: CHighScoreEdit::SetFocus; hiscore.cpp:252, dc 0xd8e58.
// Slot 14 shares CMPEdit's forwarding body at 0x510890.
void CHighScoreEdit::setFocus(bool state)
{
    textEntryWidget::setFocus(state);
}

// DC names the three CHeroWindowEx-tail pointers at +0x4c/+0x50/+0x54.
// Retail's proven CHeroWindowEx is four bytes wider, putting them at
// +0x50/+0x54/+0x58; GetRolloverWidget 0x4e97f0 directly confirms the
// last shifted offset.

inline CHSInputDlg::CHSInputDlg(int maxChars)
    : CHeroWindowEx(284, 194, 232, 212, 0x12)
{
    m_widgets.reserve(3);
    m_widgets.push_back(new bitmapBorder(
        0, 0, m_width, m_height, 500,
        DATA_COMPGEN(0x0067f4e8, highScoreNameBackground, "HighName.pcx"),
        0x800));

    m_field1 = new CHighScoreEdit(
        18, 105, 199, 23, maxChars,
        DATA_COMPGEN(0x00691210, highScoreInputEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, highScoreSmallFont, "smalfont.fnt"),
        font::WHITE, font::VERT_CENTER_JUSTIFIED, 0, 0, FIELD1_ID,
        0x100, 0, 7, 5);
    m_header1 = new textWidget(
        13, 13, 205, 100, (*g_generalText)[97],
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

// DC360/362 rejects an active empty field, then DC366 returns success.
// OnWidgetDeselect names this ordinary helper at DC338; retail expands it.
// Its retained DC public is QAA_NXZ (bool); SH4 debug types expose the
// underlying byte as unsigned char. Preserve the public return type.

bool CHSInputDlg::onOK()
{
    if (m_field1->m_status & widget::WIDGET_ACTIVE) {
        if (!strlen(m_field1->getText()))
            return 0;
    }
    return 1;
}

void memError();

VA(0x004e8fb0, 0xBD)  // dc 0xd7a08
void highScoreManager::resetHighScores()
{
    memset(m_highScores, 0, sizeof(m_highScores));
    for (int i = 0; i < 11; ++i) {
        strncpy(m_highScores[1][i].m_playerName, g_highScoreStandardDefault[i][0], 41);
        strncpy(m_highScores[1][i].m_land, g_highScoreStandardDefault[i][1], 41);
        m_highScores[1][i].m_days = atoi(g_highScoreStandardDefault[i][2]);
        m_highScores[1][i].m_score = atoi(g_highScoreStandardDefault[i][3]);

        strncpy(m_highScores[0][i].m_playerName, g_highScoreCampaignDefault[i][0], 41);
        strncpy(m_highScores[0][i].m_land, g_highScoreCampaignDefault[i][1], 41);
        m_highScores[0][i].m_days = atoi(g_highScoreCampaignDefault[i][2]);
        m_highScores[0][i].m_score = atoi(g_highScoreCampaignDefault[i][3]);
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

// Original: highScoreManager::Close; hiscore.cpp:711, dc 0xd7b88.
// Retail manager vtable 0x63eb8c shares the empty close at 0x5bc690.
void highScoreManager::close()
{
}

// Original: highScoreManager::Main; hiscore.cpp:721, dc 0xd7bcc.
// Its next vtable slot shares the zero-return body at 0x4ec560.
int highScoreManager::main(message& msg)
{
    return 0;
}

VA(0x004e9110, 0xC0)  // dc 0xd7bd0
void highScoreManager::viewHiScore()
{
    // Original local: high_score_window. DC733/734 retains this modal call.
    THighScoreWindow highScoreWindow;
    highScoreWindow.doModal();
}

// The retail build inlines this sole constructor use into
// AddScoreToHighScore.  Every widget argument below is byte-visible in that
// expansion; the three-entry reserve followed by four pushes also explains
// the one reallocating final insertion.

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

// E:\gamedcs\hiscore.cpp:772
VA(0x004e91d0, 0x4CC)  // dc 0xd7c3c
int highScoreManager::addScoreToHighScore(int score, int days,
    int difficulty, int scoreType, const char* land)
{
    unsigned char cheater = g_game->m_isCheater == 1;
    m_highScoreType = scoreType;
    HighScoreRec* scores = m_highScores[scoreType];
    int rank;

    if (!scoreType && g_game->m_campaign.m_isCheater) {
        cheater = 1;
        rank = 10;
    } else if (cheater) {
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
        CHSInputDlg hsDlg(40);
        hsDlg.m_field1->setText(g_game->getLocalPlayer()->getName());
        hsDlg.doModal(0);

        memset(&scores[rank], 0, sizeof(scores[rank]));
        strcpy(scores[rank].m_playerName, hsDlg.m_field1->getText());
    }

    strcpy(scores[rank].m_land,
        cheater ? (*g_generalText)[261] : land);
    scores[rank].m_score = score;
    scores[rank].m_days = days;
    scores[rank].m_difficulty = difficulty;
    scores[rank].m_cheated = cheater;
    g_highScoreRanks[scoreType == 1] = rank;

    writeHighScores();

    g_showHighScore = 1;
    return 0;
}

VA(0x004e96a0, 0x62)  // dc 0xd8d2c
CHighScoreEdit::CHighScoreEdit(int x, int y, int w, int h, int textSize,
    char* text, char* fontName, font::TColor color,
    font::EJustify justification, char* backgroundIcon, int backgroundFrame,
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

// DC CHSInputDlg::WindowHandler (hiscore.cpp:371, dc 0xd920c) creates
// VRKeyboard, copies its result to the edit, then synthesizes dialog close.
// Complete uses native text entry: vtable 0x63ebbc slot 9 is the inherited
// CHeroWindowEx::windowHandler (0x5ff820). There is no console override.

VA(0x004e9740, 0x4E)  // dc 0xd914c
CHSInputDlg::~CHSInputDlg()
{
    deleteWidgets();
}

VA(0x004e9790, 0x53)  // dc 0xd9190
int CHSInputDlg::onWidgetDeselect(int id, bool& exitFlag)
{
    if (id == OKAY_ID) {
        if (onOK()) {
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
    int monsterType = g_highScoreCreatureTable[i][1];
    return monsterType;
}

// Four controls precede the two 11-icon score families.  The family-one
// records are emitted first into the second pointer bank, then family zero;
// constant-folding GetMonType in each loop accounts for the one inline divide
// by five in the latter family only.
// Residual (99.9766%): all 46 CFG flows and 35 named call sites agree.
// DC884..911 assigns, hides and registers each icon; the Complete layout
// uses two eleven-row families. Computing y as 26 + 50 * i restores retail's
// retained index and its call-crossing zero/this/vector registers, including
// the complete setup/control prefix. The old independent y induction reached
// 90.7049%; its index-bound control falls to 86.4942%. Binding GetMonType's
// return value to int then restores the lookup and argument registers, leaving
// one EAX/ECX SIB base/index order in the second family's score load. The
// displayed +2 table-result offsets resolve to retail's separately named
// second column. This is no longer a whole-function C1 wall.
// The 24-state row/button/helper family emitted six distinct objects, all six
// reproduced, with all other 18 hiscore rows exact. Button declaration/reuse
// and ordinary GetMonType for/while forms do not improve the coordinate form.
// The subsequent 36-state icon/index/return family emitted 16 objects with ten
// reproduced elites. The int result reaches 99.9766% with all 18 siblings
// exact; a short result loses the exact retained helper, and per-row index
// scopes stop at 99.9578%. No slot reference or allocation-result alias is
// needed by the retained form. A further 24-state bank-access, result-site
// and prefix/postfix increment family emits one object, flat at 99.9766%.
// Earlier controls: i/y declaration order and canonical hide/show calls were
// byte-flat at 90.7049%; volatile i reached 77.7892%, reference-bound IDs
// 90.6581%. No diagnostic qualifiers or alternate helper bodies are retained.
// E:\gamedcs\hiscore.cpp:858
VA(0x004e9880, 0x506)  // vtable/global/widget/resource xrefs, dc 0xd7e3c
THighScoreWindow::THighScoreWindow()
    : heroWindow(0, 0, 800, 600, 0)
{
    int i;
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
    for (i = 0; i < 11; ++i) {
        m_creatures[1][i] = new iconWidget(
            649, 26 + 50 * i, 64, 64, 1004 + i,
            g_game->m_worldMap.newfullMapFn00505EA0(
                MONSTER, highScoreManager::getMonType(
                    g_highScoreManager->m_highScores[1][i].m_score,
                    1))->m_imageName.c_str(),
            0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_creatures[1][i]->hide();
        m_widgets.push_back(m_creatures[1][i]);
    }
    for (i = 0; i < 11; ++i) {
        m_creatures[0][i] = new iconWidget(
            649, 26 + 50 * i, 64, 64, 1015 + i,
            g_game->m_worldMap.newfullMapFn00505EA0(
                MONSTER, highScoreManager::getMonType(
                    g_highScoreManager->m_highScores[0][i].m_score,
                    0))->m_imageName.c_str(),
            0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_creatures[0][i]->hide();
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
            ->show();
    }
}

// DC941/943 owns this update/dialog pair. ViewHiScore calls the ordinary
// helper at DC734; its retail body contains the corresponding expansion.

void THighScoreWindow::doModal()
{
    update();
    g_windowManager->doDialog(this, highScoreWindowHandler, 0);
}

// DC 0xd92c8 proves destructor -> conditional operator-delete.  Retail's
// 33-byte wrapper matches all 3 CFG blocks exactly; compiler-generated /Z7
// output carries no classic source-line records.
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
        (*g_generalText)[434], g_windowManager->m_screenBitmap,
        0x58, 0xb, 0x3a, 0x1a, font::PRIMARY,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    g_mediumFont->drawBoundedString(
        (*g_generalText)[435], g_windowManager->m_screenBitmap,
        0xa3, 0xb, 0x7a, 0x1a, font::PRIMARY,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    const char* landHeading;
    if (m_isStandard)
        landHeading = (*g_generalText)[436];
    else
        landHeading = (*g_generalText)[673];
    g_mediumFont->drawBoundedString(
        landHeading, g_windowManager->m_screenBitmap,
        0x12f, 0xb, 0xd2, 0x1a, font::PRIMARY,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);

    const char* valueHeading;
    if (m_isStandard)
        valueHeading = (*g_generalText)[437];
    else
        valueHeading = (*g_generalText)[76];
    g_mediumFont->drawBoundedString(
        valueHeading, g_windowManager->m_screenBitmap,
        0x213, 0xb, m_isStandard ? 0x34 : 0x7a, 0x1a, font::PRIMARY,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    if (m_isStandard) {
        g_mediumFont->drawBoundedString(
            (*g_generalText)[76], g_windowManager->m_screenBitmap,
            0x259, 0xb, 0x34, 0x1a, font::PRIMARY,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    }

    int i = 0;
    for (int y = 0x35; y < 0x25b; ++i, y += 0x32) {
        int color = g_highScoreRanks[m_isStandard] == i
                        ? font::HEADING_HIGHLIGHT
                        : font::PRIMARY;
        highScoreManager::HighScoreRec& currentRec =
            g_highScoreManager->m_highScores[m_isStandard][i];

        sprintf(g_text, DATA_COMPGEN(0x00660a1c, highScoreDecimalFormat, "%d"),
                i + 1);
        g_mediumFont->drawBoundedString(
            g_text, g_windowManager->m_screenBitmap,
            0x58, y, 0x3a, 0x1a, font::TColor(color),
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
        g_mediumFont->drawBoundedString(
            currentRec.m_playerName, g_windowManager->m_screenBitmap,
            0xa3, y, 0x7a, 0x1a, font::TColor(color),
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
        g_mediumFont->drawBoundedString(
            currentRec.m_land, g_windowManager->m_screenBitmap,
            0x12f, y, 0xd2, 0x1a, font::TColor(color),
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);

        int value;
        if (m_isStandard)
            value = currentRec.m_days;
        else
            value = currentRec.m_score;
        sprintf(g_text, DATA_COMPGEN(0x00660a1c, highScoreDecimalFormat, "%d"),
                value);
        g_mediumFont->drawBoundedString(
            g_text, g_windowManager->m_screenBitmap,
            0x213, y, m_isStandard ? 0x34 : 0x7a, 0x1a, font::TColor(color),
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);

        if (m_isStandard) {
            sprintf(g_text,
                    DATA_COMPGEN(0x00660a1c, highScoreDecimalFormat, "%d"),
                    currentRec.m_score);
            g_mediumFont->drawBoundedString(
                g_text, g_windowManager->m_screenBitmap,
                0x259, y, 0x34, 0x1a, font::TColor(color),
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
            normalDialog((*g_generalText)[667], 2, -1, -1, -1, 0, -1,
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

// COMDAT pairing: vector<widget*>::reserve, agreement 0.993.
VA_COMPGEN(0x004ea630, 0x9F, VECTOR_RESERVE, widget)
