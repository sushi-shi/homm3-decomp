#ifndef HOMM3_SINGLESELECTIONWINDOW_PRIV_H
#define HOMM3_SINGLESELECTIONWINDOW_PRIV_H

#include "slider.h"

class CChatSlider : public slider {
public:
    virtual void SetResolution(int num);
    virtual void SetState(int state);
};

#endif  /* HOMM3_SINGLESELECTIONWINDOW_PRIV_H */
