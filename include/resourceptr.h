// resourceptr.h - canonical CodeView owner of TResourcePtr.
#ifndef HOMM3_RESOURCEPTR_H
#define HOMM3_RESOURCEPTR_H

#include "va.h"
#include "resourcemanager.h"

class TTextResource;

template<class T>
class TResourcePtr {
public:

    DC_ADDRESS(0x05b284, 0xe)
    TResourcePtr(T* ptr = 0) : m_owns(ptr != 0), m_ptr(ptr) {}
    // auto_ptr's transfer. The campaign editor expands it in its region
    // image vector (h3ccmped 0x41b4e0, under /O2 which expands only inline
    // functions).
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x57a9
    TResourcePtr(const TResourcePtr& rhs) : m_owns(rhs.m_owns), m_ptr(rhs.release()) {}

    // CodeView ResourcePtr.h:44: ownership byte +0, pointer +4.
    // objnames' state-0 unwind calls this instance.
    // VA instance: TResourcePtr<TTextResource>::~TResourcePtr
    VA(0x0041bd90, 0x12)  // anchor-eh 0x627890 for 0x41b500
    DC_ADDRESS(0x05b294, 0x24)
    ~TResourcePtr()
    {
        if (m_owns && m_ptr)
            ResourceManager::Dispose(m_ptr);
    }

    // auto_ptr's transfer: the source gives up ownership and the pointer.
    // The map editor's town sprite cache (GUIGameObject.cpp) and its quest
    // texts (QuestLocationPropsDlg.cpp) emit it; /OPT:ICF keeps the quest
    // texts' copy (h3maped 0x499b7e).
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x57b0
    TResourcePtr& operator=(const TResourcePtr& rhs)
    {
        if (this != &rhs) {
            if (m_ptr != rhs.m_ptr) {
                if (m_owns && m_ptr)
                    ResourceManager::Dispose(m_ptr);
                m_owns = rhs.m_owns;
            } else if (rhs.m_owns)
                m_owns = true;
            m_ptr = rhs.release();
        }
        return *this;
    }

    DC_ADDRESS(0x05b2b8, 0x4)
    T* get() const { return m_ptr; }

    DC_ADDRESS(0x05b2bc, 0x18)
    T* operator->() const { return get(); }

    // auto_ptr's release: the pointer leaves with its ownership. Loki's
    // assignment calls it.
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x57b2
    T* release() const
    {
        m_owns = false;
        return m_ptr;
    }

private:
    mutable unsigned char m_owns;
    T* m_ptr;
};

#endif
