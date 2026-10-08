// Monster.h - a wandering monster (Loki h3maped Monster.cpp, object 19):
// quantity (+4), disposition (+8), never-flees/never-grows flags (+0xc),
// a message (+0x10), a resource reward (+0x1c) and an artifact (+0x38).
// Its creature is the object type's subtype. TDisposition's enumerators
// are not recovered.
#ifndef HOMM3_EDITOR_MONSTER_H
#define HOMM3_EDITOR_MONSTER_H

#include <string>

#include "armygrp.h"
#include "artifact_type.h"
#include "editor/GameObject.h"
#include "editor/GameResource.h"
#include "editor/ResourceQuantities.h"

class istream;
class ostream;
class TRawIStream;
class TRawOStream;

class TMonster : public virtual TGameObject {
public:
    enum TDisposition {
    };

    TMonster(const TObjectType& objType);
    TMonster(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual string getTypeName() const;
    virtual bool isCustomized() const;
    virtual bool hasText() const;
    virtual void exportText(ostream* pOStream) const;

    TCreatureType getCreatureType() const { return TCreatureType(getExtra()); }
    unsigned int getQuantity() const { return _m_quantity; }
    void setQuantity(unsigned int newQuantity);
    TDisposition getDisposition() const { return _m_disposition; }
    void setDisposition(TDisposition newDisposition);
    bool getBNeverFlees() const { return _m_bNeverFlees; }
    void setBNeverFlees(bool bNeverFlees) { _m_bNeverFlees = bNeverFlees; }
    bool getBNeverGrows() const { return _m_bNeverGrows; }
    void setBNeverGrows(bool bNeverGrows) { _m_bNeverGrows = bNeverGrows; }
    const string& getMessage() const { return _m_message; }
    void setMessage(const string& newMessage);
    unsigned int getResourceQuantity(TGameResourceType type) const;
    void setResourceQuantity(TGameResourceType type, unsigned int newQuantity);
    TArtifact getArtifact() const { return _m_artifact; }
    void setArtifact(TArtifact newArtifact);

private:
    unsigned int _m_quantity;
    TDisposition _m_disposition;
    bool _m_bNeverFlees : 1;
    bool _m_bNeverGrows : 1;
    string _m_message;
    TResourceQuantities _m_resourceQuantities;
    TArtifact _m_artifact;
};

#endif  /* HOMM3_EDITOR_MONSTER_H */
