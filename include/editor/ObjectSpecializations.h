// ObjectSpecializations.h - the map objects that carry properties of
// their own (ObjectSpecializations.cpp; Loki h3maped object 21). Each
// derives virtually from TGameObject: its vbptr comes first, then its own
// members.
//
// Ported so far: the declarations the props dialogs need.
#ifndef HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H
#define HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H

#include <string>

#include "editor/GameObject.h"

class TRawIStream;

// A sign: its message, at most s_kMaxTextLen characters (the sign dialog's
// OnInitDialog limits its edit control to 150).
class TSign : public virtual TGameObject {
public:
    enum { s_kMaxTextLen = 150 };

    TSign(const TObjectType& objType);
    TSign(const TObjectType& objType, TRawIStream* pIStream, int version);

    const std::string& getText() const { return _m_text; }
    void setText(const std::string& newText);

    virtual void importText(std::istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const { return !_m_text.empty(); }
    virtual bool hasText() const { return true; }
    virtual void exportText(std::ostream* pOStream) const;

private:
    std::string _m_text;
};

#endif  /* HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H */
