// TextResource.cpp - Loki h3maped object 43: tab-separated text and
// spreadsheet resources. Assert lines come from the retail immediates.
#include <assert.h>
#include <string.h>
#include <glib.h>

#include "textresource.h"

TTextResource::TTextResource()
    : resource(0, RESOURCE_TYPE_NONE), Text(0), Data(0)
{
}

TTextResource::TTextResource(const char* name, int size, const char* data)
    : resource(name, RESOURCE_TYPE_TEXT)
{
    Data = new char[size];
    if (Data == NULL)
        return;
    memcpy(Data, data, size);

    char* dd;
    int count = 0;
    dd = Data;
    for (int i = 0; i < size; ++i) {
        if (*dd == '\r')
            ++count;
        ++dd;
    }
    Text.resize(count);

    dd = Data;
    for (TTextArray::iterator it = Text.begin(); it != Text.end(); ++it) {
        char* end;
        if (*dd != '"') {
            *it = dd;
            while (*dd != '\r') {
                ++dd;
#line 87
                assert(dd - Data < size);
            }
            end = dd;
            while (end > *it && *( end - 1 ) == '\t')
                --end;
        } else {
            *it = ++dd;
            while (*dd != '\r') {
                ++dd;
#line 107
                assert(dd - Data < size);
            }
            end = dd;
            while (end > *it && *( end - 1 ) == '\t')
                --end;
            --end;
        }
        *end = 0;
        dd += 2;

        int len = strlen(*it);
        for (int i = 0; i < len; ++i) {
            if ((*it)[i] == '"') {
                while (*( *it + i + 1 ) == '"' && len > i) {
                    memcpy(*it + i + 1, *it + i + 2, --len - i);
                }
            }
        }
    }
}

TTextResource::~TTextResource()
{
    if (Data)
        delete[] Data;
}

const char* TTextResource::GetText(int r) const
{
    if (this == NULL) {
        g_warning("Attempting to access text #%d, but that text has not yet been loaded.", r);
        return "";
    }
#line 168
    assert(( r >= 0 ) && ( r < Text.size() ));
    return Text[r];
}

TSpreadsheetResource::TSpreadsheetResource()
    : resource(0, RESOURCE_TYPE_NONE), Spreadsheet(0), Data(0)
{
}

TSpreadsheetResource::TSpreadsheetResource(const char* name, int size, const char* data)
    : resource(name, RESOURCE_TYPE_TEXT)
{
    Data = new char[size];
    if (Data == NULL)
        return;
    memcpy(Data, data, size);

    char* dd;
    int count = 0;
    dd = Data;
    for (int i = 0; i < size; ++i) {
        if (*dd == '\r')
            ++count;
        ++dd;
    }
    Spreadsheet.resize(count);

    dd = Data;
    for (TArray::iterator r = Spreadsheet.begin(); r != Spreadsheet.end(); ++r) {
        TStringVector* row = new TStringVector;
        *r = row;

        int columns = 0;
        for (char* p = dd; *p != '\r'; ++p) {
            if (*p == '\t')
                ++columns;
        }
        ++columns;
        row->resize(columns);

        for (TStringVector::iterator c = row->begin(); c != row->end(); ++c) {
            if (*dd != '"') {
                *c = dd;
                while (*dd != '\t' && *dd != '\r')
                    ++dd;
            } else {
                *c = ++dd;
                while (*dd != '\t' && *dd != '\r')
                    ++dd;
                *( dd - 1 ) = 0;
            }
            *dd++ = 0;

            int len = strlen(*c);
            for (int i = 0; i < len; ++i) {
                if ((*c)[i] == '"') {
                    while (*( *c + i + 1 ) == '"' && len > i) {
                        memcpy(*c + i + 1, *c + i + 2, --len - i);
                    }
                }
            }
        }
        ++dd;
    }
}

TSpreadsheetResource::~TSpreadsheetResource()
{
    for (TArray::iterator r = Spreadsheet.begin(); r != Spreadsheet.end(); ++r)
        delete *r;
    if (Data)
        delete[] Data;
}
