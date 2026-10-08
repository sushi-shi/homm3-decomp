// Bitmap8.cpp - Bitmap8Bit (Loki h3maped object 41).
#include <assert.h>
#include <string.h>

#include <stdexcept>
#include <limits>
#include "bitmap8.h"

Bitmap8Bit::Bitmap8Bit()
    : resource(0, RESOURCE_TYPE_NONE),
      m_dataSize(0),
      m_imageSize(0),
      m_width(0),
      m_height(0),
      m_map(0)
{
}

Bitmap8Bit::Bitmap8Bit(const char* name, int w, int h, unsigned char* data,
                       const TPalette24& palette, int size)
    : resource(name, RESOURCE_TYPE_BITMAP),
      m_imageSize(w * h),
      m_width(w),
      m_height(h),
      m_palette(palette)
{
    m_dataSize = size ? size : m_imageSize;
    m_map = new unsigned char[m_dataSize];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
}

Bitmap8Bit::Bitmap8Bit(const char* name, const char* path)
    : resource(name, RESOURCE_TYPE_BITMAP),
      m_dataSize(0),
      m_imageSize(0),
      m_width(0),
      m_height(0),
      m_map(0)
{
    char filename[4096];
    strcpy(filename, path);
    strcat(filename, name);
    importPCXFile(filename);
}

Bitmap8Bit::~Bitmap8Bit()
{
    if (m_map)
        delete[] m_map;
}

void Bitmap8Bit::import(int w, int h, unsigned char* data,
                        const TPalette24& palette, int size)
{
    clear();
    m_width = w;
    m_height = h;
    m_imageSize = w * h;
    m_dataSize = size ? size : m_imageSize;
    m_map = new unsigned char[m_dataSize];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
    m_palette = palette;
}

void Bitmap8Bit::clear()
{
    m_width = 0;
    m_height = 0;
    m_dataSize = 0;
    m_imageSize = 0;
    if (m_map) {
        delete[] m_map;
        m_map = 0;
    }
}

int Bitmap8Bit::exportPCXFile(const char* filename)
{
    if (!m_map)
        return 2;
    return 0;
}

int Bitmap8Bit::importPCXFile(const char* filename)
{
#line 377
    assert(0);
    return 2;
}
