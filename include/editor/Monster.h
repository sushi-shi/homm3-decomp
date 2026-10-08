// Monster.h - a wandering monster (Loki h3maped Monster.cpp, object 19):
// quantity (+4), disposition (+8), never-flees/never-grows flags (+0xc),
// a message (+0x10), a resource reward (+0x1c) and an artifact (+0x38).
// Its creature is the object type's subtype. TDisposition's enumerators
// are not recovered.
#ifndef HOMM3_EDITOR_MONSTER_H
#define HOMM3_EDITOR_MONSTER_H

#include "editor/GameObject.h"

#include <string>

#include "armygrp.h"
#include "artifact_type.h"
#include "editor/GameResource.h"
#include "editor/Array.h"

class istream;
class ostream;
class TRawIStream;
class TRawOStream;

class TMonster : public virtual TGameObject {
public:
    // The dispositions' names are not recorded; they follow the game's
    // order (aggressive, 2, is the default).
    enum TDisposition {
        eDispositionCompliant = 0,
        eDispositionFriendly = 1,
        eDispositionAggressive = 2,
        eDispositionHostile = 3,
        eDispositionSavage = 4
    };

    enum {
        s_kNumDispositions = 5
    };

    static const int s_kMaxQuantity = 4000;
    static const unsigned int s_kMaxMessageLen = 300;
    static const unsigned int s_kMaxResourceQuantity = 99999;

    TMonster(const TObjectType& objType);
    TMonster(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual string getTypeName() const;
    virtual bool isCustomized() const;
    virtual void exportText(ostream* pOStream) const;

    void setQuantity(unsigned int newQuantity);
    void setDisposition(TDisposition newDisposition);
    void setBNeverFlees(bool bNeverFlees) { _m_bNeverFlees = bNeverFlees; }
    void setBNeverGrows(bool bNeverGrows) { _m_bNeverGrows = bNeverGrows; }
    void setMessage(const string& newMessage);
    void setResourceQuantity(TGameResourceType type, unsigned int newQuantity);
    void setArtifact(TArtifact newArtifact);

    TCreatureType getCreatureType() const { return TCreatureType(getExtra()); }
    unsigned int getQuantity() const { return _m_quantity; }
    TDisposition getDisposition() const { return _m_disposition; }
    bool getBNeverFlees() const { return _m_bNeverFlees; }
    bool getBNeverGrows() const { return _m_bNeverGrows; }
    const string& getMessage() const { return _m_message; }
    unsigned int getResourceQuantity(TGameResourceType type) const;
    TArtifact getArtifact() const { return _m_artifact; }

    virtual bool hasText() const { return !getMessage().empty(); }

private:
    unsigned int _m_quantity;
    TDisposition _m_disposition;
    bool _m_bNeverFlees : 1;
    bool _m_bNeverGrows : 1;
    string _m_message;
    TArray<unsigned int, kNumGameResourceTypes> _m_resourceQuantities;
    TArtifact _m_artifact;
};

#endif  /* HOMM3_EDITOR_MONSTER_H */
