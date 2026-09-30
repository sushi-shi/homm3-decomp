#ifndef HOMM3_ARTIFACT_INVENTORY_H
#define HOMM3_ARTIFACT_INVENTORY_H

#include "hero.h"

class heroWindow;
class message;
class widget;
class iconWidget;
class coloredBorder;
class textWidget;

enum { ARTIFACT_INVENTORY_BUTTON_ID = 1800 };
typedef int (*ArtifactInventoryQuote)(int slot, int resource);

// A shared, paged inventory. Trading slots use the merchant's numbering:
// 0..17 equipped, 18..81 backpack. Hero-screen slots are backpack indices.
// Sorting changes the view, preserving artifact identity and scroll payloads.
class ArtifactInventoryPanel {
public:
    enum Event { ignored, changed, picked, quickSell };
    ArtifactInventoryPanel(heroWindow& window, hero& owner, int x, int y,
        int columns, int rows, ArtifactInventoryQuote quote = 0);
    Event handle(message& msg);
    void refresh(int resource = -1);
    int selectedSlot() const { return m_selectedSlot; }
private:
    struct Entry { type_artifact artifact; int slot; };
    heroWindow& m_window;
    hero& m_owner;
    ArtifactInventoryQuote m_quote;
    Entry m_entries[82];
    int m_count, m_page, m_capacity, m_resource, m_selectedSlot;
    iconWidget* m_icons[64];
    iconWidget* m_resources[64];
    textWidget* m_amounts[64];
    coloredBorder* m_frames[64];
    coloredBorder* m_sortFrames[3];
    textWidget* m_sortLabels[3];
    textWidget* m_pageLabel;
    widget* m_previous;
    widget* m_next;
    void append(widget* item, bool interactive = false);
};

void addArtifactInventoryButton(heroWindow& window, int x, int y);
int showArtifactInventory(hero& owner);

#endif
