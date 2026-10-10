// Monster.h - a wandering monster (Monster.cpp; Loki h3maped). On Windows
// a linkable object (RTTI TMonster <- TLinkableObject <- virtual
// TGameObject): its quantity follows the link id at +0xc, then the
// disposition, the flee and growth flags, the message, the resources and
// the artifact it guards.
#ifndef HOMM3_EDITOR_MONSTER_H
#define HOMM3_EDITOR_MONSTER_H

#include <iosfwd>
#include <string>

#include "armygrp.h"
#include "artifact_type.h"
#include "creaturetype_fwd.h"
#include "gameversion.h"
#include "editor/Array.h"
#include "editor/GameResource.h"
#include "editor/ObjectSpecializations.h"

class TRawIStream;
class TRawOStream;

class TMonster : public TLinkableObject {
public:
    enum TDisposition {
        eDispositionCompliant = 0,
        eDispositionFriendly = 1,
        eDispositionAggressive = 2,
        eDispositionHostile = 3,
        eDispositionSavage = 4
    };

    enum { s_kNumDispositions = 5 };
    enum { s_kMaxQuantity = 4000, s_kMaxMessageLen = 300, s_kMaxResourceQuantity = 99999 };

    TMonster(const TObjectType& objType);
    TMonster(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void importText(std::istream* pIStream, EGameVersion version);
    virtual void write(TRawOStream* pOStream, int version) const;
    virtual std::string getTypeName() const;
    virtual bool isCustomized() const;
    virtual bool hasText() const { return !getMessage().empty(); }
    virtual void exportText(std::ostream* pOStream, EGameVersion version) const;

    void setQuantity(unsigned int newQuantity);
    void setDisposition(TDisposition newDisposition);
    void setBNeverFlees(bool bNeverFlees) { _m_bNeverFlees = bNeverFlees; }
    void setBNeverGrows(bool bNeverGrows) { _m_bNeverGrows = bNeverGrows; }
    void setMessage(const std::string& newMessage);
    void setResourceQuantity(TGameResourceType type, unsigned int newQuantity);
    void setArtifact(TArtifact newArtifact);

    TCreatureType getCreatureType() const { return TCreatureType(getExtra()); }
    unsigned int getQuantity() const { return _m_quantity; }
    TDisposition getDisposition() const { return _m_disposition; }
    bool getBNeverFlees() const { return _m_bNeverFlees; }
    bool getBNeverGrows() const { return _m_bNeverGrows; }
    const std::string& getMessage() const { return _m_message; }
    unsigned int getResourceQuantity(TGameResourceType type) const;
    TArtifact getArtifact() const { return _m_artifact; }

private:
    unsigned int _m_quantity;
    TDisposition _m_disposition;
    bool _m_bNeverFlees : 1;
    bool _m_bNeverGrows : 1;
    std::string _m_message;
    TArray<unsigned int, kNumGameResourceTypes> _m_resourceQuantities;
    TArtifact _m_artifact;
};

// The name of a number of creatures of a type, as the game spells it
// (getArmyName, creaturetype.h): nothing past the last type.
inline const char* getArmyName(TCreatureType type, unsigned int count)
{
    if (type < 0 || type > g_creatureTypeLast)
        return "";
    return count == 1 ? akCreatureTypeTraits[type].m_name : akCreatureTypeTraits[type].m_plural_name;
}

#endif  /* HOMM3_EDITOR_MONSTER_H */
