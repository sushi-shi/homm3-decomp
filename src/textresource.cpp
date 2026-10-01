#include "va.h"

#include <string.h>

#include "textresource.h"

// Shared by flags, optional names and row delimiters. Nullable formats retain
// their own pointer guard before this non-null spreadsheet-cell operation.
bool isResourceFieldSet(const char* value)
{
    return value[0] && value[0] != ' ';
}

// Project-inferred common operation used by pooled table loaders. Preserve
// one length scan, unsigned byte count and memcpy rather than a second scan.
unsigned copyResourceString(char* destination, const char* source)
{
    unsigned length = strlen(source) + 1;
    memcpy(destination, source, length);
    return length;
}

// Project-inferred parser operations shared by text lines and spreadsheet rows.
// Count only carriage returns within the supplied byte extent.
static int countResourceRows(const char* data, int size)
{
    int rows = 0;
    int bytesLeft = size;
    const char* scan = data;
    while (bytesLeft > 0) {
        if (*scan == '\r')
            ++rows;
        ++scan;
        --bytesLeft;
    }
    return rows;
}

static void collapseResourceQuotes(char* text)
{
    int length = strlen(text);
    for (int i = 0; i < length; ++i) {
        if (text[i] == '"') {
            while (text[i + 1] == '"' && length > i) {
                --length;
                memcpy(text + i + 1, text + i + 2, length - i);
            }
        }
    }
}

// Original: TTextResource::TTextResource; textresource.cpp:33
DC_ADDRESS(0x163808, 0x50)
TTextResource::TTextResource() : resource(0, RESOURCE_TYPE_NONE), m_data(0)
{
}

VA_COMPGEN(0x005bbb70, 0x21, SCALAR_DELETING_DTOR, TTextResource)

VA(0x005bbba0, 0x227)
DC_ADDRESS(0x163858, 0x14c)
MAC_ADDRESS(0x1b0b70, 0x21c)
TTextResource::TTextResource(const char* name, int size, const char* data)
    : resource(name, RESOURCE_TYPE_TEXT)
{
    m_data = new char[size];
    if (!m_data)
        return;
    memcpy(m_data, data, size);

    int numStrings = countResourceRows(m_data, size);
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

        collapseResourceQuotes(*it);
    }
}

VA(0x005bbdd0, 0x4D)
DC_ADDRESS(0x1639a4, 0x46)
MAC_ADDRESS(0x1b0d8c, 0x8c)
TTextResource::~TTextResource()
{
    // Mac retains array delete (0x268c34) for the character buffer.
    if (m_data)
        delete[] m_data;
}

VA(0x005bbe20, 0x1B)
MAC_ADDRESS(0x1b0e18, 0xc)
unsigned int TTextResource::getSize() const
{
    return sizeof(*this) + m_text.size();
}

// Original: TSpreadsheetResource::TSpreadsheetResource; textresource.cpp:177
DC_ADDRESS(0x1639ec, 0x84)
TSpreadsheetResource::TSpreadsheetResource()
    : resource(0, RESOURCE_TYPE_NONE), m_data(0)
{
}

VA_COMPGEN(0x005bbe40, 0x21, SCALAR_DELETING_DTOR, TSpreadsheetResource)

VA(0x005bbe70, 0x2E6)
DC_ADDRESS(0x163a70, 0x1c0)
MAC_ADDRESS(0x1b0ea8, 0x270)
TSpreadsheetResource::TSpreadsheetResource(const char* name, int size,
                                            const char* data)
    : resource(name, RESOURCE_TYPE_TEXT)
{
    m_dataSize = size;
    m_data = new char[size];
    if (!m_data)
        return;
    memcpy(m_data, data, size);

    int numRows = countResourceRows(m_data, size);
    m_spreadsheet.resize(numRows, 0);

    char* next = m_data;
    for (TArray::iterator rowIt = m_spreadsheet.begin();
         rowIt != m_spreadsheet.end(); ++rowIt) {
        TStringVector* row = new TStringVector;
        *rowIt = row;

        int numColumns = 0;
        char* scan = next;
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

            collapseResourceQuotes(*cell);
        }
        ++next;
    }
}

VA(0x005bc160, 0x7)
MAC_ADDRESS(0x1b1118, 0xc)
unsigned int TSpreadsheetResource::getSize() const
{
    return sizeof(*this) + m_dataSize;
}

VA(0x005bc170, 0x7F)
DC_ADDRESS(0x163c30, 0xc8)
MAC_ADDRESS(0x1b1124, 0xe4)
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
