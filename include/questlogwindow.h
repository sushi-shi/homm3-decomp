#ifndef HOMM3_QUESTLOGWINDOW_H
#define HOMM3_QUESTLOGWINDOW_H

#include <vector>
#include "advmgr_popup.h"

// The constructor writes `byte [+0x60] = <a byte loaded from a stack
// local>` and then zeroes +0x64/+0x68/+0x6c - which is VC6's vector
// copy-constructing its empty allocator from the default argument's
// temporary and then nulling _First/_Last/_End. It zeroes one more dword
// at +0x70, and that is the whole of the object.

// The destructor closes it independently: it frees the pointer at +0x64
// and clears +0x64/+0x68/+0x6c, touching neither +0x60 nor +0x70. A
// leading byte member would push every one of those four offsets up by
// four, which is exactly the 4-byte error this pair caught.
class TQuestLogWindow : public CAdvPopup {
public:
    TQuestLogWindow();
    std::vector<int> m_seerHutLogList;
    // Scroll offset of the topmost listed quest: zeroed by the constructor,
    // untouched by the destructor, and read by UpdateQuestLocator (0x52e270)
    // as the base of the row index it then uses to select out of
    // seerHutLogList, bailing when the sum reaches size(). Both lanes
    // reached that role off the same body; the spelling is a house
    // placeholder, no dump names it.
    int m_firstVisibleQuest;  // +0x70

    virtual ~TQuestLogWindow();
    virtual int windowHandler(message& msg);

    void updateQuestLocator(int i);
    void updateQuestLocators();
};
SIZE(TQuestLogWindow, 0x74);

// Defined in src/questlogwindow.cpp. Declared here for its second
// consumer: THeroScreenWindow::WindowHandler's quest-log button arm calls
// it with the current hero's owner (/Gr, the id in ecx).
void doQuestLog(int player);

#endif  /* HOMM3_QUESTLOGWINDOW_H */
