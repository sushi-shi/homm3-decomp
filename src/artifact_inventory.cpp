// Gameplay enhancement: intentionally has no retail address claims.
#include "artifact_inventory.h"
#include "advmgr_popup.h"
#include "artifact.h"
#include "border.h"
#include "button.h"
#include "game.h"
#include "iconwdgt.h"
#include "message.h"
#include "textwdgt.h"
#include <stdio.h>
#include <string.h>

namespace {
enum { firstCellId = 1900, firstSortId = 2000, previousId = 2003, nextId = 2004 };
enum { sortBySet, sortByRarity, sortByName };
int g_inventorySort = sortBySet;

int artifactSet(int id)
{
    const TArtifactTraits& traits = g_artifactTraits[id];
    if (traits.m_comboType >= 0 && traits.m_comboType < 12) return traits.m_comboType;
    return traits.m_targetCombo >= 0 && traits.m_targetCombo < 12 ? traits.m_targetCombo : 12;
}

bool before(const type_artifact& left, const type_artifact& right)
{
    const TArtifactTraits& a = g_artifactTraits[left.m_artifactId];
    const TArtifactTraits& b = g_artifactTraits[right.m_artifactId];
    if (g_inventorySort == sortBySet && artifactSet(left.m_artifactId) != artifactSet(right.m_artifactId))
        return artifactSet(left.m_artifactId) < artifactSet(right.m_artifactId);
    if (g_inventorySort == sortByRarity && a.m_artifactClass != b.m_artifactClass)
        return a.m_artifactClass > b.m_artifactClass;
    int nameOrder = _stricmp(a.m_name, b.m_name);
    if (nameOrder) return nameOrder < 0;
    return left.m_extra < right.m_extra;
}

void visible(widget* item, bool show, bool interactive = false)
{
    item->m_status &= ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    if (show) item->m_status |= widget::WIDGET_DRAWN;
    if (show && interactive) item->m_status |= widget::WIDGET_ACTIVE;
}
}

void ArtifactInventoryPanel::append(widget* item, bool interactive)
{
    m_window.m_widgets.push_back(item);
    m_window.addWidget(item, -1);
    if (!interactive) item->m_status &= ~widget::WIDGET_ACTIVE;
}

ArtifactInventoryPanel::ArtifactInventoryPanel(heroWindow& window, hero& owner,
    int x, int y, int columns, int rows, ArtifactInventoryQuote quote)
    : m_window(window), m_owner(owner), m_quote(quote), m_count(0), m_page(0),
      m_capacity(columns * rows), m_resource(-1), m_selectedSlot(-1)
{
    int width = columns * 64;
    append(new coloredBorder(x - 3, y - 3, width + 6, 34 + rows * 78 + 32, 0, 0x2104, 0));
    const char* labels[] = { "Set", "Rarity", "Name" };
    for (int sort = 0; sort < 3; ++sort) {
        int left = x + sort * (width / 3);
        m_sortFrames[sort] = new coloredBorder(left, y, width / 3 - 3, 26,
            firstSortId + sort, 0x4208, 0);
        append(m_sortFrames[sort], true);
        m_sortLabels[sort] = new textWidget(left, y + 4, width / 3 - 3, 20,
            labels[sort], "smalfont.fnt", font::PRIMARY, 0, font::CENTER_JUSTIFIED, 0, 8);
        append(m_sortLabels[sort]);
    }
    for (int i = 0; i < m_capacity; ++i) {
        int left = x + (i % columns) * 64, top = y + 32 + (i / columns) * 78;
        m_frames[i] = new coloredBorder(left, top, 62, 76, 0, 0x6b46, 0);
        append(m_frames[i]);
        append(new coloredBorder(left + 1, top + 1, 60, 74, 0, 0x1082, 0));
        // The full card is clickable, including its price badge.
        m_icons[i] = new iconWidget(left + 9, top + 2, 44, 44, 0,
            "artifact.def", 0, 0, false, 0, iconWidget::ICON_STYLE_PLAIN);
        append(m_icons[i]);
        m_resources[i] = new iconWidget(left + 1, top + 43, 32, 32, 0,
            "resource.def", 0, 0, false, 0, iconWidget::ICON_STYLE_PLAIN);
        append(m_resources[i]);
        m_amounts[i] = new textWidget(left + 27, top + 53, 34, 18, "", "tiny.fnt",
            font::PRIMARY, 0, font::CENTER_JUSTIFIED, 0, 8);
        append(m_amounts[i]);
        append(new border(left, top, 62, 76, firstCellId + i, 0), true);
    }
    int footer = y + 34 + rows * 78;
    m_previous = new coloredBorder(x, footer, 42, 26, previousId, 0x4208, 0);
    append(m_previous, true);
    append(new textWidget(x, footer + 4, 42, 20, "<", "smalfont.fnt", font::HEADING,
        0, font::CENTER_JUSTIFIED, 0, 8));
    m_next = new coloredBorder(x + width - 44, footer, 42, 26, nextId, 0x4208, 0);
    append(m_next, true);
    append(new textWidget(x + width - 44, footer + 4, 42, 20, ">", "smalfont.fnt", font::HEADING,
        0, font::CENTER_JUSTIFIED, 0, 8));
    m_pageLabel = new textWidget(x + 44, footer + 4, width - 88, 20, "", "smalfont.fnt",
        font::PRIMARY, 0, font::CENTER_JUSTIFIED, 0, 8);
    append(m_pageLabel);
    refresh();
}

void ArtifactInventoryPanel::refresh(int resource)
{
    m_resource = resource;
    m_count = 0;
    if (m_quote) {
        for (int slot = 0; slot < 18; ++slot) {
            const type_artifact& item = m_owner.getArtifact(TArtifactSlot(slot));
            if (item.m_artifactId < 0 || item.m_artifactId >= ARTIFACT_COUNT) continue;
            m_entries[m_count].artifact = item;
            m_entries[m_count++].slot = slot;
        }
    }
    for (int slot = 0; slot < 64; ++slot) {
        const type_artifact& item = m_owner.getBackpack(slot);
        if (item.m_artifactId < 0 || item.m_artifactId >= ARTIFACT_COUNT) continue;
        m_entries[m_count].artifact = item;
        m_entries[m_count++].slot = slot + (m_quote ? 18 : 0);
    }
    for (int i = 1; i < m_count; ++i) {
        Entry item = m_entries[i];
        int j = i;
        while (j > 0 && before(item.artifact, m_entries[j - 1].artifact)) {
            m_entries[j] = m_entries[j - 1];
            --j;
        }
        m_entries[j] = item;
    }
    int pages = m_count ? (m_count + m_capacity - 1) / m_capacity : 1;
    if (m_page >= pages) m_page = pages - 1;
    for (int sort = 0; sort < 3; ++sort) {
        m_sortFrames[sort]->m_color = sort == g_inventorySort ? 0x9cc8 : 0x4208;
        m_sortLabels[sort]->setColor(sort == g_inventorySort ? font::WHITE : font::PRIMARY);
    }
    for (int cell = 0; cell < m_capacity; ++cell) {
        int index = m_page * m_capacity + cell;
        bool occupied = index < m_count;
        visible(m_icons[cell], occupied);
        int price = 0;
        if (occupied) {
            const Entry& entry = m_entries[index];
            m_icons[cell]->setIconFrame(entry.artifact.m_artifactId);
            if (m_quote && resource >= 0 && resource < 7) price = m_quote(entry.slot, resource);
            m_frames[cell]->m_color = entry.slot == m_selectedSlot ? 0xffff : 0x6b46;
            char help[512];
            entry.artifact.getRolloverText(help);
            m_window.getWidget(firstCellId + cell)->setHelpText(help, "", true);
        } else {
            m_frames[cell]->m_color = 0x4208;
            m_window.getWidget(firstCellId + cell)->setHelpText("Empty slot", "", true);
        }
        visible(m_resources[cell], price > 0);
        visible(m_amounts[cell], price > 0);
        if (price > 0) {
            m_resources[cell]->setIconFrame(resource);
            char amount[32];
            sprintf(amount, "%d", price);
            m_amounts[cell]->setText(amount);
        }
    }
    char page[64];
    sprintf(page, "%d / %d  (%d items)", m_page + 1, pages, m_count);
    m_pageLabel->setText(page);
    m_previous->enable(m_page > 0);
    m_next->enable(m_page + 1 < pages);
}

ArtifactInventoryPanel::Event ArtifactInventoryPanel::handle(message& msg)
{
    if (msg.m_id != MESSAGE_WIDGET) return ignored;
    if (msg.m_codeX == widget::WIDGET_SELECT) {
        if (msg.m_codeY >= firstSortId && msg.m_codeY < firstSortId + 3) {
            g_inventorySort = msg.m_codeY - firstSortId;
            m_page = 0;
            refresh(m_resource);
            return changed;
        }
        if (msg.m_codeY == previousId || msg.m_codeY == nextId) {
            m_page += msg.m_codeY == previousId ? -1 : 1;
            if (m_page < 0) m_page = 0;
            refresh(m_resource);
            return changed;
        }
    }
    int cell = msg.m_codeY - firstCellId;
    int index = m_page * m_capacity + cell;
    if (cell < 0 || cell >= m_capacity || index >= m_count) return ignored;
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        type_artifact item = m_entries[index].artifact;
        m_owner.viewArtifact(&item, 1);
        return changed;
    }
    if (msg.m_codeX == widget::WIDGET_SELECT) {
        m_selectedSlot = m_entries[index].slot;
        return m_quote && (msg.m_qualifier & MESSAGE_MODIFIER_SHIFT_KEYS) ? quickSell : picked;
    }
    return ignored;
}

namespace {
class ArtifactInventoryWindow : public CAdvPopup {
    ArtifactInventoryPanel* m_panel;
    int m_choice;
public:
    ArtifactInventoryWindow(hero& owner) : CAdvPopup(124, 35, 552, 530, 0x12), m_choice(-1)
    {
        m_widgets.push_back(new coloredBorder(0, 0, 552, 530, 0, 0x2104, 0));
        m_widgets.push_back(new textWidget(12, 12, 528, 30, "Backpack", "bigfont.fnt",
            font::HEADING, 0, font::CENTER_JUSTIFIED, 0, 8));
        m_widgets.push_back(new textWidget(20, 465, 420, 50,
            "Click to pick up an artifact. Right-click to inspect.", "smalfont.fnt",
            font::PRIMARY, 0, font::CENTER_JUSTIFIED, 0, 8));
        button* done = new button(466, 478, 64, 32, 2010, "iOk6432.def", 0, 1, true, 1, 2);
        done->setHotkey(28);
        m_widgets.push_back(done);
        for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) addWidget(*it, -1);
        m_panel = new ArtifactInventoryPanel(*this, owner, 20, 55, 8, 4);
    }
    virtual ~ArtifactInventoryWindow() { delete m_panel; deleteWidgets(); }
    int choice() const { return m_choice; }
    virtual int windowHandler(message& msg)
    {
        int handled = CAdvPopup::windowHandler(msg);
        if (handled) return handled;
        ArtifactInventoryPanel::Event event = m_panel->handle(msg);
        if (event == ArtifactInventoryPanel::picked) {
            m_choice = m_panel->selectedSlot();
            return exitDialog(msg);
        }
        if (event != ArtifactInventoryPanel::ignored)
            drawWindow(true, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
};
}

void addArtifactInventoryButton(heroWindow& window, int x, int y)
{
    button* open = new textButton(x, y, 64, 32, ARTIFACT_INVENTORY_BUTTON_ID,
        "iOk6432.def", "Bag", "smalfont.fnt", 0, 1, false, 48, 2, font::WHITE);
    window.m_widgets.push_back(open);
    window.addWidget(open, -1);
}

int showArtifactInventory(hero& owner)
{
    if (!g_currentPlayer->isLocalHuman()) return -1;
    ArtifactInventoryWindow window(owner);
    window.doModal(false);
    return window.choice();
}
