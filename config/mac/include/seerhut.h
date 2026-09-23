// Selected-body Mac view for the retail quest dialog family. The Mac
// type_skill_quest vtable and its slot-5 body fix the base/skill offsets.
#ifndef HOMM3_MAC_SEERHUT_H
#define HOMM3_MAC_SEERHUT_H

#include "va.h"

void operator delete(void* value);

namespace Metrowerks {
namespace details {
template<class Allocator, class Size, int Version> class compressed_pair_imp;
template<class Allocator, class Size>
class compressed_pair_imp<Allocator, Size, 1> : private Allocator {
    Size m_second;
public:
    explicit compressed_pair_imp(Size value) : m_second(value) {}
    Allocator& first() { return *this; }
    Size& second() { return m_second; }
};
}
template<class Allocator, class Size>
class compressed_pair
    : private details::compressed_pair_imp<Allocator, Size, 1> {
    typedef details::compressed_pair_imp<Allocator, Size, 1> base;
public:
    explicit compressed_pair(Size value) : base(value) {}
    Allocator& first() { return base::first(); }
    Size& second() { return base::second(); }
};
}

namespace std {
class string {
    struct rep {
        unsigned long m_length;
        char m_beforeData[8];
        char* m_data;
    }* m_handle;
public:
    string(const string&);
    ~string();
    const char* c_str() const { return m_handle->m_data; }
};
template<class T> class allocator {
public:
    void deallocate(T* value, unsigned long) { ::operator delete(value); }
};
// The installed MSL classifies this record through its non-POD vector path.
// Its deleter clears constructed elements before releasing the allocation.
template<class T> class __vector_deleter {
protected:
    Metrowerks::compressed_pair<allocator<T>, unsigned long> m_capacity;
    unsigned long m_size;
    T* m_data;
    __vector_deleter() : m_capacity(0), m_size(0), m_data(0) {}
    ~__vector_deleter();
    void clear();
};
template<class T> class __vector_imp : private __vector_deleter<T> {
protected:
    __vector_imp() {}
    void push_back(const T& value);
};
template<class T> class vector : private __vector_imp<T> {
public:
    vector() {}
    ~vector();
    void push_back(const T& value) { __vector_imp<T>::push_back(value); }
};
}

struct type_dialog_resource {
    int m_resource;
    unsigned long m_qualifier;
};

// CodeWarrior's four-byte string handle contracts the three base strings
// from the 0x40-byte Windows base to 0x1c; the Mac slot-5 wrapper passes
// this+0x1c as its four signed skill requirements.
class type_quest {
public:
    unsigned char m_seerHut;
    char m_paddingBeforeTexts[3];
    std::string m_proposalText;
    std::string m_progressText;
    std::string m_completionText;
    int m_textVariant;
    int m_limit;
    virtual ~type_quest();
    virtual void doProgressDialog();
    std::string getProgressDialogText();
};

class type_skill_quest : public type_quest {
public:
    signed char m_requiredSkills[4];
    void showSkillRequirementsDialog(const char* text, const signed char* skills);
    virtual void doProgressDialog();
};

void extendedDialog(const char* text,
                    std::vector<type_dialog_resource>& resources,
                    long x, long y, long timeout);

#endif
