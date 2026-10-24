// GameObject.cpp - TGameObject's reference-counted interning of object
// types and its type name (h3maped 0x43d2f1..0x43d420; Loki h3maped
// object 13). Release drops Loki's range assert.
#include "editor/stdafx.h"

#include <assert.h>

#include "va.h"
#include "objnames.h"
#include "editor/GameObject.h"

DATA(0x0059e410) TGameObject::_TObjectTypeMap TGameObject::_s_objectTypeMap;
// The map's constructor, which its static initializer calls.
VA_COMPGEN(0x0043d3fd, 0x23, CLASS_CTOR, map)

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
    assert(getType() >= 0 && getType() < MAX_EVENT_TYPE);
    return std::string(akAdvObjectTypeTraits[getType()].m_name);
}

VA(0x0043d3c1, 0x3c)
TGameObject& TGameObject::operator=(const TGameObject& other)
{
    if (this != &other) {
        if (--_m_objectTypeIter->second == 0)
            _s_objectTypeMap.erase(_m_objectTypeIter);
        _m_objectTypeIter = other._m_objectTypeIter;
        ++_m_objectTypeIter->second;
    }
    return *this;
}
