// GameObject.cpp - TGameObject's reference-counted interning of object
// types and its type name (h3maped 0x43d2f1..0x43d3fd; Loki h3maped
// object 13). Release drops Loki's range assert.
#include "editor/stdafx.h"

#include "va.h"
#include "objnames.h"
#include "editor/GameObject.h"

DATA(0x0059e410) TGameObject::_TObjectTypeMap TGameObject::_s_objectTypeMap;

VA(0x0043d2f1, 0x18)
TGameObject::TGameObject(const TGameObject& other)
    : _m_objectTypeIter(other._m_objectTypeIter)
{
    ++_m_objectTypeIter->second;
}

VA(0x0043d309, 0x4b)
TGameObject::TGameObject(const TObjectType& objType)
{
    _m_objectTypeIter = _s_objectTypeMap.insert(_TObjectTypeMap::value_type(objType, 0)).first;
    ++_m_objectTypeIter->second;
}

VA(0x0043d354, 0x27)
TGameObject::~TGameObject()
{
    if (--_m_objectTypeIter->second == 0)
        _s_objectTypeMap.erase(_m_objectTypeIter);
}

VA(0x0043d37b, 0x46)
std::string TGameObject::getTypeName() const
{
    return std::string(akAdvObjectTypeTraits[getType()].m_name);
}

TGameObject& TGameObject::operator=(const TGameObject& other)
{
    if (this == &other)
        return *this;
    if (--_m_objectTypeIter->second == 0)
        _s_objectTypeMap.erase(_m_objectTypeIter);
    _m_objectTypeIter = other._m_objectTypeIter;
    ++_m_objectTypeIter->second;
    return *this;
}
