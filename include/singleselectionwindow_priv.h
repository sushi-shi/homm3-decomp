#ifndef HOMM3_SINGLESELECTIONWINDOW_PRIV_H
#define HOMM3_SINGLESELECTIONWINDOW_PRIV_H

#include "remotedlg.h"
#include "slider.h"
#include "textntry.h"
#include "netmsg.h"
#include "winmgr.h"

class CChatSlider : public slider {
public:
    virtual void SetResolution(int num);
    virtual void SetState(int state);
};

CNetMsg* GetRemoteData(unsigned char removeFromQueue,
                       unsigned char* wasCompressed);

class CHostWaitDlg : public CAnimatedDlg {
public:
    virtual ~CHostWaitDlg();
    virtual int handle_message(message& msg);

    CNetMsg* m_pMsg;
    unsigned long m_forWho;
};

class CSingleSelectionNetMsgHandler : public CAdvMgrNetMsgHandler {
public:
    virtual CNetMsg* CheckHandleNet(unsigned char inPopup,
                                    unsigned char* msgReceived);
    virtual CNetMsg* HandleNetMsg(CNetMsg* pNetMsg);

    unsigned char m_wasCompressed;
};

class CChatWidget : public textWidget {
public:
    class CChatSave : public Bitmap16Bit {
    public:
        unsigned char bSaved;
        unsigned char IsSaved() const { return bSaved; }
    };

    virtual void Draw();
    CChatSave* m_save;
};

enum EWindowMode6989f0 {
    WINDOW_MODE_6989F0_3 = 3
};
extern int gUnnamed6989f0;
extern int bVideoPaused;
int Update(message& msg);

#endif  /* HOMM3_SINGLESELECTIONWINDOW_PRIV_H */
