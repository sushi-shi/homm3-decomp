// DCAttributeSelector.h - sets one of a DC's attributes for a scope and
// restores the previous value when it goes out of scope. The attribute is
// a template argument: the CDC member that sets it, called through VC6's
// virtual-call thunks (h3maped 0x4bf381 for SetTextColor, 0x487731 for
// SetBkColor). The rulers keep the text and background colours with it;
// the class name and its header are not recorded.
#ifndef HOMM3_EDITOR_DCATTRIBUTESELECTOR_H
#define HOMM3_EDITOR_DCATTRIBUTESELECTOR_H

#include "va.h"

template <class T, T (CDC::*pfnSet)(T)>
class TDCAttributeSelector {
public:
    TDCAttributeSelector(CDC* pDC, T value) : _m_pDC(pDC), _m_oldValue((pDC->*pfnSet)(value)) {}

    // VA instance: TDCAttributeSelector<unsigned long, &CDC::SetTextColor>::~TDCAttributeSelector
    VA(0x004bf34f, 0xb)
    // VA instance: TDCAttributeSelector<unsigned long, &CDC::SetBkColor>::~TDCAttributeSelector
    VA(0x004bf35a, 0xb)
    // VA instance: TDCAttributeSelector<int, &CDC::SetStretchBltMode>::~TDCAttributeSelector
    VA(0x0048d271, 0xb)
    ~TDCAttributeSelector() { (_m_pDC->*pfnSet)(_m_oldValue); }

private:
    CDC* _m_pDC;
    T _m_oldValue;
};

typedef TDCAttributeSelector<COLORREF, &CDC::SetTextColor> TTextColorSelector;
typedef TDCAttributeSelector<COLORREF, &CDC::SetBkColor> TBkColorSelector;

#endif  /* HOMM3_EDITOR_DCATTRIBUTESELECTOR_H */
