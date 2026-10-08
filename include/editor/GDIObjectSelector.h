// GDIObjectSelector.h - selects a GDI object into a DC for a scope
// (RTTI: TGDIObjectSelector<CBitmap>, <CBrush>, <CFont> and <CPen>, each
// with a nested TError deriving from TGDIObjectSelectorError). The
// constructor throws its TError when the selection fails; the destructor
// selects the previous object back. The header and member names are not
// recorded.
#ifndef HOMM3_EDITOR_GDIOBJECTSELECTOR_H
#define HOMM3_EDITOR_GDIOBJECTSELECTOR_H

#include "exceptions.h"
#include "va.h"

class TGDIObjectSelectorError : public TRuntimeError {
public:
    TGDIObjectSelectorError();
};

template <class T>
class TGDIObjectSelector {
public:
    class TError : public TGDIObjectSelectorError {
    public:
        // VA instance: TGDIObjectSelector<CBitmap>::TError::TError()
        VA(0x004520ca, 0x12)
        TError() {}
    };

    // VA instance: TGDIObjectSelector<CBitmap>::TGDIObjectSelector(CDC*, CBitmap*)
    VA(0x00451fdd, 0x45)
    TGDIObjectSelector(CDC* pDC, T* pObject)
        : _m_pDC(pDC)
    {
        _m_pOldObject = _m_pDC->SelectObject(pObject);
        if (_m_pOldObject == NULL)
            throw TError();
    }

    // VA instance: TGDIObjectSelector<CBitmap>::~TGDIObjectSelector
    VA(0x00452022, 0x16)
    ~TGDIObjectSelector() { _m_pDC->SelectObject(_m_pOldObject); }

private:
    CDC* _m_pDC;
    T* _m_pOldObject;
};

#endif  /* HOMM3_EDITOR_GDIOBJECTSELECTOR_H */
