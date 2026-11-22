// CampaignMapWnd.h - the campaign map window (CampaignMapWnd.cpp): the
// campaign's map image with each scenario's region drawn over it in the
// region's color, selected, with a map or without one. A click selects a
// region, a double click or the context menu's command edits its
// scenario, and a tool tip names the region under the cursor. The window
// asks its client (the view) for the campaign and tells it what was
// selected or is to be edited. Layout from the image: the client at 0x3c,
// the selected scenario at 0x40, the tool tip at 0x44, the scenario whose
// tool is registered at 0x4c, the off-screen bitmap at 0x50, the map image
// at 0x6c and the region images from 0x74.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGNMAPWND_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGNMAPWND_H

#include <memory>
#include <vector>

#include "bitmap16.h"
#include "bitmap816.h"
#include "resourceptr.h"
#include "editor/DIBSection.h"

class TCampaign;

class TCampaignMapWndClient {
public:
    virtual const TCampaign* getCampaign() = 0;
    virtual void selectScenario(int scenario) = 0;
    virtual void editScenario(int scenario) = 0;
};

class TCampaignMapWnd : public CWnd {
public:
    TCampaignMapWnd(CWnd* pParent, TCampaignMapWndClient* pClient);
    virtual ~TCampaignMapWnd();

    void initialUpdate();
    void update();
    CSize getSize() const;

protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnScenarioProperties();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnToolTipNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    // A region's images by player color.
    struct TRegionImages {
        TResourcePtr<Bitmap816> m_selected[8];
        TResourcePtr<Bitmap816> m_enabled[8];
        TResourcePtr<Bitmap816> m_conquered[8];
    };

    void _removeTool();
    void _addTool(unsigned int scenario);
    unsigned int _getScenario(const CPoint& point);

    TCampaignMapWndClient* _m_pClient;
    unsigned int _m_selectedScenario;
    std::auto_ptr<CToolTipCtrl> _m_pToolTip;
    unsigned int _m_toolScenario;
    T16bppDIBSection _m_bitmap;
    TResourcePtr<Bitmap16Bit> _m_pMapImage;
    std::vector<TRegionImages> _m_regionImages;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGNMAPWND_H */
