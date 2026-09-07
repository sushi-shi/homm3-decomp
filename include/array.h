// array.h - E:\gamedcs\array.h, the hand-rolled auto-growing pointer array
// the DirectPlay layer and the multiplayer window both store their records
// in. HAND-OWNED; the template surface and layout come from Dreamcast
// CodeView (field list 0x2967: vfptr@0, step@4, pArray@8, allocSize@0xc,
// size@0x10, total 0x14) and retail confirms the seven-slot virtual order -
// ~/scalar-deleting, Add, Get, Put, Delete, Insert, GetCount - in four
// admitted vtables.
//
// UNIFIED 2026-09-05. This template was carried TWICE, once in dxplay.h and
// once in multiplayerwindow.h, with a per-TU macro keeping the two apart -
// and the two bodies genuinely disagreed: opposite Insert algorithms,
// opposite bounds tests in Get/Delete, `delete` against `delete []`, an
// extra SetStep, in-class against out-of-class definitions. Each copy had
// been tuned to whichever instantiation its own TU is scored on, and neither
// TU ever saw the other. Retail arbitrates member by member, because the two
// instantiations that carry claimed rows are complementary:
//   dxplay.obj      CAutoArray<CDPlayAddressElement>  ~ 0x499f00, Destroy
//                   0x499f60, Get 0x499fc0, Put 0x499fe0, GetCount 0x49a010
//   multiplayer     CAutoArray<CDPlaySession>         Destroy 0x512570,
//                   Delete 0x5125d0, Insert 0x512610
// Every one of those rows is EXACT, and the two units' Add/Delete/Insert
// instantiations ICF-fold onto each other in the retail link, so one body
// has to satisfy both - which is exactly the constraint the two-copy model
// was hiding.
#ifndef HOMM3_ARRAY_H
#define HOMM3_ARRAY_H

#include <string.h>  // memcpy, the grow copy Add inlines

#include "va.h"

template<class T>
class CAutoArray {
public:
    CAutoArray()
    {
        m_step = 25;
        m_size = 0;
        m_allocSize = 0;
        m_array = 0;
    }

    // Delegating to Destroy rather than writing the loop here is what keeps
    // retail's virtual Get(i) dispatch: in a destructor VC6 assumes the
    // exact type and devirtualizes, but inside the inlined Destroy - an
    // ordinary member - Get stays a vtable call.
    virtual ~CAutoArray()
    {
        destroy(1);
    }

    // Before normalization (function): CAutoArray::Destroy.
    void destroy(unsigned char deleteData = 1)
    {
        for (unsigned long i = 0; i < m_size; ++i) {
            T* element = get(i);
            if (deleteData)
                delete element;
        }

        if (m_array)
            delete [] m_array;
        m_array = 0;
        m_size = m_allocSize = 0;
    }

    // Retail's own shape, read off the one row the whole link carries for
    // this member (0x558410, slot 1 of both CAutoArray vftables): the
    // capacity test compares allocSize against size and not the other way
    // round, allocSize grows BEFORE the allocation so `new` takes the new
    // capacity rather than the sum, the copy is a `memcpy` of size*4 BYTES
    // (retail inlines it as rep movsd + rep movsb over a byte count, which
    // an element loop cannot produce) sharing the `if (pArray)` guard with
    // the delete, and the tail stores through the VIRTUAL Put - `call
    // [vfptr+0xc]`, slot 3 - after the count has already been bumped.
    // Before normalization (function): CAutoArray::Add.
    virtual unsigned char add(T* element)
    {
        if (m_allocSize <= m_size) {
            m_allocSize += m_step;
            T** grown = new T*[m_allocSize];
            if (m_array) {
                memcpy(grown, m_array, m_size * sizeof(T*));
                delete [] m_array;
            }
            m_array = grown;
        }
        ++m_size;
        return put(m_size - 1, element);
    }

    // Before normalization (function): CAutoArray::Get.
    virtual T* get(unsigned long elementNbr)
    {
        if (elementNbr >= m_size)
            return 0;
        return m_array[elementNbr];
    }

    // Before normalization (function): CAutoArray::Put.
    virtual unsigned char put(unsigned long elementNbr, T* element)
    {
        if (elementNbr >= m_size)
            return 0;
        m_array[elementNbr] = element;
        return 1;
    }

    // Before normalization (function): CAutoArray::Delete.
    virtual unsigned char deleteElement(unsigned long elementNbr)
    {
        if (elementNbr >= m_size)
            return 0;
        for (unsigned long i = elementNbr; i < m_size - 1; ++i)
            m_array[i] = m_array[i + 1];
        --m_size;
        return 1;
    }

    // Before normalization (function): CAutoArray::Insert.
    virtual unsigned char insert(unsigned long nextElementNbr, T* element)
    {
        if (nextElementNbr >= m_size)
            return 0;
        T* lastElement = get(m_size - 1);
        for (unsigned long i = m_size - 1; i > nextElementNbr; --i)
            m_array[i] = m_array[i - 1];
        put(nextElementNbr, element);
        add(lastElement);
        return 1;
    }

    // Before normalization (function): CAutoArray::GetCount.
    virtual unsigned long getCount() { return m_size; }

protected:
    // Before normalization: step.
    unsigned long m_step;       // +0x04
    // Before normalization: pArray.
    T** m_array;               // +0x08
    // Before normalization: allocSize.
    unsigned long m_allocSize;  // +0x0c
    // Before normalization: size.
    unsigned long m_size;       // +0x10
};

#endif  /* HOMM3_ARRAY_H */
