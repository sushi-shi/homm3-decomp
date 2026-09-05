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

#include "va.h"

template<class T>
class CAutoArray {
public:
    CAutoArray()
    {
        step = 25;
        size = 0;
        allocSize = 0;
        pArray = 0;
    }

    // Delegating to Destroy rather than writing the loop here is what keeps
    // retail's virtual Get(i) dispatch: in a destructor VC6 assumes the
    // exact type and devirtualizes, but inside the inlined Destroy - an
    // ordinary member - Get stays a vtable call.
    virtual ~CAutoArray()
    {
        Destroy(1);
    }

    void Destroy(unsigned char deleteData = 1)
    {
        for (unsigned long i = 0; i < size; ++i) {
            T* element = Get(i);
            if (deleteData)
                delete element;
        }

        if (pArray)
            delete [] pArray;
        pArray = 0;
        size = allocSize = 0;
    }

    virtual unsigned char Add(T* element)
    {
        if (size >= allocSize) {
            T** grown = new T*[allocSize + step];
            for (unsigned long i = 0; i < size; ++i)
                grown[i] = pArray[i];
            if (pArray)
                delete [] pArray;
            pArray = grown;
            allocSize += step;
        }
        pArray[size] = element;
        ++size;
        return 1;
    }

    virtual T* Get(unsigned long elementNbr)
    {
        if (elementNbr >= size)
            return 0;
        return pArray[elementNbr];
    }

    virtual unsigned char Put(unsigned long elementNbr, T* element)
    {
        if (elementNbr >= size)
            return 0;
        pArray[elementNbr] = element;
        return 1;
    }

    virtual unsigned char Delete(unsigned long elementNbr)
    {
        if (elementNbr >= size)
            return 0;
        for (unsigned long i = elementNbr; i < size - 1; ++i)
            pArray[i] = pArray[i + 1];
        --size;
        return 1;
    }

    virtual unsigned char Insert(unsigned long nextElementNbr, T* element)
    {
        if (nextElementNbr >= size)
            return 0;
        T* lastElement = Get(size - 1);
        for (unsigned long i = size - 1; i > nextElementNbr; --i)
            pArray[i] = pArray[i - 1];
        Put(nextElementNbr, element);
        Add(lastElement);
        return 1;
    }

    virtual unsigned long GetCount() { return size; }

protected:
    unsigned long step;       // +0x04
    T** pArray;               // +0x08
    unsigned long allocSize;  // +0x0c
    unsigned long size;       // +0x10
};

#endif  /* HOMM3_ARRAY_H */
