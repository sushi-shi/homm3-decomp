#include "va.h"

#include <string.h>

#include "textresource.h"

// Original: TTextResource::TTextResource; textresource.cpp:33, dc 0x163808.
TTextResource::TTextResource() : resource(0, RESOURCE_TYPE_NONE), m_data(0)
{
}

VA_COMPGEN(0x005bbb70, 0x21, SCALAR_DELETING_DTOR, TTextResource)

VA(0x005bbba0, 0x227) MAC_ADDRESS(0x1b0b70, 0x21c)  // dc 0x163858
TTextResource::TTextResource(const char* name, int size, const char* data)
    : resource(name, RESOURCE_TYPE_TEXT)
{
    m_data = new char[size];
    if (!m_data)
        return;
    memcpy(m_data, data, size);

    int numStrings = 0;
    int bytesLeft = size;
    char* scan = m_data;
    while (bytesLeft > 0) {
        if (*scan == '\r')
            ++numStrings;
        ++scan;
        --bytesLeft;
    }
    m_text.resize(numStrings, 0);

    char* next = m_data;
    for (TTextArray::iterator it = m_text.begin(); it != m_text.end(); ++it) {
        char* end;
        if (*next != '"') {
            *it = next;
            while (*next != '\r')
                ++next;

            end = next;
            while (end > *it && end[-1] == '\t')
                --end;
        } else {
            ++next;
            *it = next;
            while (*next != '\r')
                ++next;

            end = next;
            while (end > *it && end[-1] == '\t')
                --end;
            --end;
        }
        *end = 0;
        next += 2;

        int length = strlen(*it);
        for (int i = 0; i < length; ++i) {
            if ((*it)[i] == '"') {
                while ((*it)[i + 1] == '"' && length > i) {
                    --length;
                    memcpy((*it) + i + 1, (*it) + i + 2, length - i);
                }
            }
        }
    }
}

VA(0x005bbdd0, 0x4D) MAC_ADDRESS(0x1b0d8c, 0x8c)  // dc 0x1639a4
TTextResource::~TTextResource()
{
    // Mac retains array delete (0x268c34) for the character buffer.
    if (m_data)
        delete[] m_data;
}

VA(0x005bbe20, 0x1B) MAC_ADDRESS(0x1b0e18, 0xc)
unsigned int TTextResource::getSize() const
{
    return sizeof(*this) + m_text.size();
}

// Original: TSpreadsheetResource::TSpreadsheetResource; textresource.cpp:177, dc 0x1639ec.
TSpreadsheetResource::TSpreadsheetResource()
    : resource(0, RESOURCE_TYPE_NONE), m_data(0)
{
}

VA_COMPGEN(0x005bbe40, 0x21, SCALAR_DELETING_DTOR, TSpreadsheetResource)

VA(0x005bbe70, 0x2E6) MAC_ADDRESS(0x1b0ea8, 0x270)  // dc 0x163a70
TSpreadsheetResource::TSpreadsheetResource(const char* name, int size,
                                            const char* data)
    : resource(name, RESOURCE_TYPE_TEXT)
{
    m_dataSize = size;
    m_data = new char[size];
    if (!m_data)
        return;
    memcpy(m_data, data, size);

    int numRows = 0;
    int bytesLeft = size;
    char* scan = m_data;
    while (bytesLeft > 0) {
        if (*scan == '\r')
            ++numRows;
        ++scan;
        --bytesLeft;
    }
    m_spreadsheet.resize(numRows, 0);

    char* next = m_data;
    for (TArray::iterator rowIt = m_spreadsheet.begin();
         rowIt != m_spreadsheet.end(); ++rowIt) {
        TStringVector* row = new TStringVector;
        *rowIt = row;

        int numColumns = 0;
        scan = next;
        while (*scan != '\r') {
            if (*scan == '\t')
                ++numColumns;
            ++scan;
        }
        ++numColumns;
        row->resize(numColumns, 0);

        for (TStringVector::iterator cell = row->begin();
             cell != row->end(); ++cell) {
            if (*next != '"') {
                *cell = next;
                while (*next != '\t' && *next != '\r')
                    ++next;
            } else {
                ++next;
                *cell = next;
                while (*next != '\t' && *next != '\r')
                    ++next;
                next[-1] = 0;
            }
            *next = 0;
            ++next;

            int length = strlen(*cell);
            for (int i = 0; i < length; ++i) {
                if ((*cell)[i] == '"') {
                    while ((*cell)[i + 1] == '"' && length > i) {
                        --length;
                        memcpy((*cell) + i + 1, (*cell) + i + 2,
                               length - i);
                    }
                }
            }
        }
        ++next;
    }
}

VA(0x005bc160, 0x7) MAC_ADDRESS(0x1b1118, 0xc)
unsigned int TSpreadsheetResource::getSize() const
{
    return sizeof(*this) + m_dataSize;
}

VA(0x005bc170, 0x7F) MAC_ADDRESS(0x1b1124, 0xe4)  // dc 0x163c30
TSpreadsheetResource::~TSpreadsheetResource()
{
    for (TStringVector** it = m_spreadsheet.begin(); it != m_spreadsheet.end();
         ++it) {
        if (*it)
            delete *it;
    }
    // Mac retains array delete (0x268c34) for the character buffer.
    if (m_data)
        delete[] m_data;
}

// E:\gamedcs\textresource.cpp:298
#if 0  // @carcass -- Dreamcast STLport template tail; retail uses VC6 Dinkumware

// ..\stlport\stl_vector.h:490
VA(0x005bc1f0, 0x33)  // ctor shrink-path call + Dinkumware erase(first,last), dc STLport analog 0x164254
std::vector<char** std::vector<std::vector<char *,std::allocator<char *> > *,std::allocator<std::vector<char *,std::allocator<char *> > *> >::erase(std::vector<char** __first, std::vector<char** __last)
{
    // @stub
}

#endif
