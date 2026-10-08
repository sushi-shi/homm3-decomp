// GameObject.cpp - Loki h3maped object 13: TGameObject's reference-counted
// interning of object types, its type name and, with its vtable, the
// inline members of GameObject.h.
#include "editor/stdafx.h"

#include <assert.h>

#include "objnames.h"
#include "editor/GameObject.h"

TGameObject::_TObjectTypeMap TGameObject::_s_objectTypeMap;

TGameObject::TGameObject(const TGameObject& other)
    : _m_objectTypeIter(other._m_objectTypeIter)
{
    ++_m_objectTypeIter->second;
}

TGameObject::TGameObject(const TObjectType& objType)
{
    _m_objectTypeIter = _s_objectTypeMap.insert(_TObjectTypeMap::value_type(objType, 0)).first;
    ++_m_objectTypeIter->second;
}

TGameObject::~TGameObject()
{
    if (--_m_objectTypeIter->second == 0)
        _s_objectTypeMap.erase(_m_objectTypeIter);
}

string TGameObject::getTypeName() const
{
#line 59
    assert(getType() >= 0 && getType() < MAX_EVENT_TYPE);
    return string(akAdvObjectTypeTraits[getType()].m_name);
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
