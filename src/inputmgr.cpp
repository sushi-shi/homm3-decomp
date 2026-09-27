#include "va.h"

#include <string.h>

#include "inputmgr.h"

#include "advmgr.h"
#include "remote.h"
#include "kbwin.h"
#include "message.h"
#include "mousemgr.h"
#include "soundmgr.h"
#include "textntry.h"
#include "winmgr.h"

VA(0x004ec0e0, 0x1AB)  // dc 0xdc894
int keyboardMessageHandler(void* hwnd, unsigned winMsg, unsigned wordParam, long longParam)
{
    message* e;

    if (g_inputManager == 0)
        return 1;
    if (g_inputManager->m_status != 1)
        return 1;
    e = &g_inputManager->m_buffer[g_inputManager->m_tail];
    e->m_id = e->m_codeX = e->m_codeY = e->m_mouseX = e->m_mouseY = e->m_qualifier = 0;
    switch (winMsg) {
    case WM_KEYDOWN:
        e->m_id = MESSAGE_KEY_DOWN;
        e->m_codeX = HIWORD(longParam) & 0xff;
        break;
    case WM_KEYUP:
        e->m_id = MESSAGE_KEY_UP;
        e->m_codeX = HIWORD(longParam) & 0xff;
        break;
    }
    if (e->m_id != 0) {
        int quals = g_inputManager->getCurrQuals();
        e->m_qualifier = quals;
        g_inputManager->m_tail++;
        g_inputManager->m_tail %= 64;
        if (g_inputManager->m_head == g_inputManager->m_tail) {
            g_inputManager->m_head++;
            g_inputManager->m_head %= 64;
        }
        g_inputManager->m_extendFlag = 0;
        if (g_windowManager->m_status == 1) {
            if (g_advManager == 0 || g_advManager->m_advWindow == 0
                || g_advManager->m_advWindow->m_chatEdit->m_hasFocus == 0) {
                if (e->m_id == MESSAGE_KEY_DOWN && e->m_codeX == KEYCODE_F1)
                    appCommand(g_hwndApp, WM_COMMAND, KBWIN_MENU_HELP, 0);
                if (e->m_id == MESSAGE_KEY_DOWN && e->m_codeX == KEYCODE_F4)
                    appCommand(g_hwndApp, WM_COMMAND, KBWIN_MENU_FULLSCREEN, 0);
            }
        }
    }
    return e->m_id == 0;
}

VA(0x004ec290, 0x1CC)  // dc 0xdcaa0
int mouseMessageHandler(void* hwnd, unsigned winMsg, unsigned wordParam, long longParam)
{
    message* e;
    int x;
    int y;

    if (g_inputManager == 0)
        return 1;
    if (g_inputManager->m_status != 1)
        return 1;
    if (g_inputManager->m_bufferBusy != 0)
        return 1;
    g_inputManager->m_bufferBusy = 1;
    e = &g_inputManager->m_buffer[g_inputManager->m_tail];
    e->m_id = e->m_codeX = e->m_codeY = e->m_mouseX = e->m_mouseY = e->m_qualifier = 0;
    bool hasPosition = true;
    switch (winMsg) {
    case WM_MOUSEMOVE:
        e->m_id = MESSAGE_MOUSE_MOVE;
        break;
    case WM_LBUTTONDBLCLK:
        e->m_id = MESSAGE_LEFT_BUTTON_DOWN;
        break;
    case WM_LBUTTONDOWN:
        e->m_id = MESSAGE_LEFT_BUTTON_DOWN;
        SetCapture(g_hwndApp);
        break;
    case WM_RBUTTONDOWN:
        e->m_id = MESSAGE_RIGHT_BUTTON_DOWN;
        SetCapture(g_hwndApp);
        break;
    case WM_RBUTTONDBLCLK:
        e->m_id = MESSAGE_RIGHT_BUTTON_DOWN;
        break;
    case WM_LBUTTONUP:
        e->m_id = MESSAGE_LEFT_BUTTON_UP;
        ReleaseCapture();
        break;
    case WM_RBUTTONUP:
        e->m_id = MESSAGE_RIGHT_BUTTON_UP;
        ReleaseCapture();
        break;
    default:
        hasPosition = false;
        break;
    }
    if (hasPosition) {
        x = static_cast<short>(longParam);
        y = static_cast<short>(static_cast<unsigned long>(longParam) >> 16);
        e->m_codeX = x;
        e->m_codeY = y;
        e->m_mouseX = x;
        e->m_mouseY = y;
    }
    if (e->m_id != 0) {
        int quals = g_inputManager->getCurrQuals();
        e->m_qualifier = quals;
        g_inputManager->m_tail++;
        g_inputManager->m_tail %= 64;
        if (g_inputManager->m_head == g_inputManager->m_tail) {
            g_inputManager->m_head++;
            g_inputManager->m_head %= 64;
        }
    }
    g_inputManager->m_bufferBusy = 0;
    return e->m_id == 0;
}

// construction rather than this constructor body. The message array uses
// its canonical default constructor; repeating its stores in an additional
// derived wrapper would introduce a second clear pass absent from retail.
VA(0x004ec460, 0x6F) MAC_ADDRESS(0x10e068, 0x7c)  // dc 0xdd97c
inputManager::inputManager()
{
    m_keyboardFilter = 1;
    m_keyCodeType = 1;
    m_status = 0;
    m_bufferBusy = 0;
    m_keyboardInstalled = 0;
    m_mouseInstalled = 0;
    m_currWidgetId = -1;
    m_prevDialog = 0;
}

VA(0x004ec4d0, 0x6D) MAC_ADDRESS(0x10e10c, 0x78)  // dc 0xdd9e4
int inputManager::open(int keyboardFilter)
{
    memset(m_buffer, 0, sizeof(m_buffer));
    m_tail = 0;
    m_head = 0;
    m_keyboardFilter = keyboardFilter;
    makeScanCodeTable();
    m_id = 4;
    m_priority = -1;
    m_status = 1;
    strcpy(m_mgrName, DATA_COMPGEN(0x0067f544, inputManagerName, "inputManager"));
    return 0;
}

VA(0x004ec540, 0x1E) MAC_ADDRESS(0x10e184, 0x24)  // dc 0xdda30
void inputManager::close()
{
    if (m_status != 1)
        return;
    m_tail = 0;
    m_head = 0;
    m_keyboardFilter = 0;
    m_status = 0;
}

VA(0x004ec560, 0x5) MAC_ADDRESS(0x10e1a8, 0x8)  // dc 0xdda50
int inputManager::main(message& msg)
{
    return 0;
}

VA(0x004ec570, 0x18) MAC_ADDRESS(0x10e1b0, 0x38)  // dc 0xdda54
void inputManager::flush()
{
    process1WindowsMessage();
    m_tail = 0;
    m_head = 0;
}

// canonical message clear, and both call the already-claimed

VA(0x004ec590, 0xAE) MAC_ADDRESS(0x10e1e8, 0x15c)  // dc 0xdda74
message inputManager::getEvent()
{
    message msg;

    pollSound();
    if (m_status == STATUS_ACTIVE && m_head != m_tail) {
        msg = m_buffer[m_head];
        m_head = (m_head + 1) % 64;
        if (msg.m_id == MESSAGE_KEY_DOWN && m_keyCodeType == 0)
            asciiConvert(&msg);
    } else {
        msg.m_id = 0;
        msg.m_codeY = 0;
        msg.m_codeX = 0;
        msg.m_qualifier = 0;
    }
    return msg;
}

VA(0x004ec640, 0xAD) MAC_ADDRESS(0x10e344, 0x154)  // dc 0xddc14
message inputManager::peekEvent()
{
    message msg;

    pollSound();
    if (m_status == STATUS_ACTIVE && m_head != m_tail) {
        msg = m_buffer[m_head];
        m_head = m_head % 64;
        if (msg.m_id == MESSAGE_KEY_DOWN && m_keyCodeType == 0)
            asciiConvert(&msg);
    } else {
        msg.m_id = 0;
        msg.m_codeY = 0;
        msg.m_codeX = 0;
        msg.m_qualifier = 0;
    }
    return msg;
}

// Original: inputManager::GetCurrQuals; inputmgr.cpp:970, dc 0xddd08.
// DC973..978 and the retained keyboard/mouse bridges use the same three
// GetKeyState queries in control/alt/shift order. Complete expands this
// ordinary helper in keyboardMessageHandler, mouseMessageHandler and
// forceMouseMove; the member does not read its receiver.
// Mac retains it at 0:0x10e498 using GetKeys and the adjacent key-bit helper.
MAC_ADDRESS(0x10e498, 0x80)
int inputManager::getCurrQuals()
{
    int quals = 0;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        quals |= MESSAGE_MODIFIER_CONTROL;
    if (GetKeyState(VK_MENU) & 0x8000)
        quals |= MESSAGE_MODIFIER_ALT;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        quals |= MESSAGE_MODIFIER_SHIFT;
    return quals;
}

// Original: inputManager::SetKeyCodeType; inputmgr.cpp:984, dc 0xddd4c.
// DC985 stores the mode and calls Flush. Complete retains m_keyCodeType
// at +0x950 and tests it in GetEvent/PeekEvent. No retained setter VA or
// new call site is asserted for this ordinary source API.
void inputManager::setKeyCodeType(int newType)
{
    m_keyCodeType = newType;
    flush();
}

// Mac 0:0x10e55c copies the pretranslated message codeY field to codeX.
// Complete's Windows body decodes scan codes and shift state instead.
VA(0x004ec6f0, 0x1C6) MAC_ADDRESS(0x10e55c, 0xc)  // dc 0xddd60
void inputManager::asciiConvert(message* msg)
{
    if ((msg->m_codeX >= KEYCODE_F1 && msg->m_codeX <= KEYCODE_F10)
        || msg->m_codeX == KEYCODE_F11 || msg->m_codeX == KEYCODE_F12)
        msg->m_codeX = m_scanCodeTable[msg->m_codeX];
    else
        msg->m_codeX = m_scanCodeTable[msg->m_codeX] & 0xff;
    if ((msg->m_qualifier & MESSAGE_MODIFIER_SHIFT_KEYS) == 0) {
        if (msg->m_codeX > '@' && msg->m_codeX < '[')
            msg->m_codeX += 'a' - 'A';
    }
    if ((msg->m_qualifier & MESSAGE_MODIFIER_SHIFT_KEYS) == 0)
        return;
    switch (msg->m_codeX) {
    case '1': msg->m_codeX = '!'; break;
    case '2': msg->m_codeX = '@'; break;
    case '3': msg->m_codeX = '#'; break;
    case '4': msg->m_codeX = '$'; break;
    case '5': msg->m_codeX = '%'; break;
    case '6': msg->m_codeX = '^'; break;
    case '7': msg->m_codeX = '&'; break;
    case '8': msg->m_codeX = '*'; break;
    case '9': msg->m_codeX = '('; break;
    case '0': msg->m_codeX = ')'; break;
    case '-': msg->m_codeX = '_'; break;
    case '=': msg->m_codeX = '+'; break;
    case '\\': msg->m_codeX = '|'; break;
    case ';': msg->m_codeX = ':'; break;
    case '\'': msg->m_codeX = '"'; break;
    case ',': msg->m_codeX = '<'; break;
    case '.': msg->m_codeX = '>'; break;
    case '/': msg->m_codeX = '?'; break;
    }
}

VA(0x004ec8c0, 0x340)  // dc 0xdde74
void inputManager::makeScanCodeTable()
{
    for (unsigned index = 0; index < 128; index++)
        m_scanCodeTable[index] = static_cast<short>(index << 8);
    m_scanCodeTable[0] = 0x00;
    m_scanCodeTable[1] = 0x1b;
    m_scanCodeTable[2] = '1';
    m_scanCodeTable[3] = '2';
    m_scanCodeTable[4] = '3';
    m_scanCodeTable[5] = '4';
    m_scanCodeTable[6] = '5';
    m_scanCodeTable[7] = '6';
    m_scanCodeTable[8] = '7';
    m_scanCodeTable[9] = '8';
    m_scanCodeTable[10] = '9';
    m_scanCodeTable[11] = '0';
    m_scanCodeTable[12] = '-';
    m_scanCodeTable[13] = '=';
    m_scanCodeTable[14] = 0x7f;
    m_scanCodeTable[15] = 0x09;
    m_scanCodeTable[16] = 'Q';
    m_scanCodeTable[17] = 'W';
    m_scanCodeTable[18] = 'E';
    m_scanCodeTable[19] = 'R';
    m_scanCodeTable[20] = 'T';
    m_scanCodeTable[21] = 'Y';
    m_scanCodeTable[22] = 'U';
    m_scanCodeTable[23] = 'I';
    m_scanCodeTable[24] = 'O';
    m_scanCodeTable[25] = 'P';
    m_scanCodeTable[26] = '[';
    m_scanCodeTable[27] = ']';
    m_scanCodeTable[28] = 0x0a;
    m_scanCodeTable[29] = 0x1d00;
    m_scanCodeTable[30] = 'A';
    m_scanCodeTable[31] = 'S';
    m_scanCodeTable[32] = 'D';
    m_scanCodeTable[33] = 'F';
    m_scanCodeTable[34] = 'G';
    m_scanCodeTable[35] = 'H';
    m_scanCodeTable[36] = 'J';
    m_scanCodeTable[37] = 'K';
    m_scanCodeTable[38] = 'L';
    m_scanCodeTable[39] = ';';
    m_scanCodeTable[40] = '\'';
    m_scanCodeTable[41] = 0x2900;
    m_scanCodeTable[42] = 0x2a00;
    m_scanCodeTable[43] = '\\';
    m_scanCodeTable[44] = 'Z';
    m_scanCodeTable[45] = 'X';
    m_scanCodeTable[46] = 'C';
    m_scanCodeTable[47] = 'V';
    m_scanCodeTable[48] = 'B';
    m_scanCodeTable[49] = 'N';
    m_scanCodeTable[50] = 'M';
    m_scanCodeTable[51] = ',';
    m_scanCodeTable[52] = '.';
    m_scanCodeTable[53] = '/';
    m_scanCodeTable[54] = 0x3600;
    m_scanCodeTable[55] = '*';
    m_scanCodeTable[56] = 0x3800;
    m_scanCodeTable[57] = ' ';
    m_scanCodeTable[58] = 0x3a00;
    m_scanCodeTable[59] = 0x3b00;
    m_scanCodeTable[60] = 0x3c00;
    m_scanCodeTable[61] = 0x3d00;
    m_scanCodeTable[62] = 0x3e00;
    m_scanCodeTable[63] = 0x3f00;
    m_scanCodeTable[64] = 0x4000;
    m_scanCodeTable[65] = 0x4100;
    m_scanCodeTable[66] = 0x4200;
    m_scanCodeTable[67] = 0x4300;
    m_scanCodeTable[68] = 0x4400;
    m_scanCodeTable[69] = 0x4500;
    m_scanCodeTable[70] = 0x4600;
    m_scanCodeTable[71] = 0x4700;
    m_scanCodeTable[72] = 0x4800;
    m_scanCodeTable[73] = 0x4900;
    m_scanCodeTable[74] = '-';
    m_scanCodeTable[75] = 0x4b00;
    m_scanCodeTable[76] = 0x4c00;
    m_scanCodeTable[77] = 0x4d00;
    m_scanCodeTable[78] = '+';
    m_scanCodeTable[79] = 0x4f00;
    m_scanCodeTable[80] = 0x5000;
    m_scanCodeTable[81] = 0x5100;
    m_scanCodeTable[82] = 0x5200;
    m_scanCodeTable[83] = 0x5300;
    m_scanCodeTable[84] = 0x5400;
    m_scanCodeTable[85] = 0x5500;
    m_scanCodeTable[86] = 0x5600;
    m_scanCodeTable[87] = 0x5700;
    m_scanCodeTable[88] = 0x5800;
}

VA(0x004ecc00, 0xCA) MAC_ADDRESS(0x10e568, 0xe8)  // dc 0xde044
void inputManager::forceMouseMove()
{
    message* e;
    int quals;

    if (m_bufferBusy)
        return;
    m_bufferBusy = 1;
    e = &m_buffer[m_tail];
    e->m_id = MESSAGE_MOUSE_MOVE;
    g_mouseManager->mouseCoords(e->m_codeX, e->m_codeY);
    e->m_mouseX = e->m_codeX;
    e->m_mouseY = e->m_codeY;
    quals = getCurrQuals();
    e->m_qualifier = quals;
    m_tail = (m_tail + 1) % 64;
    if (m_head == m_tail)
        m_head = (m_head + 1) % 64;
    m_bufferBusy = 0;
}
