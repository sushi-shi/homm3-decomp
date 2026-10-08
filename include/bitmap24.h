#ifndef HOMM3_BITMAP24_H
#define HOMM3_BITMAP24_H

#include "va.h"

#include "resource.h"

class Bitmap16Bit;

// Retail's two constructors and destructor prove the resource base and the
// five-dword tail. The data constructor stores its byte-count at +0x1c,
// width/height at +0x24/+0x28 and the owned pixel pointer at +0x2c; the
// destructor releases that pointer. Slot 2 then reports DataSize plus the
// fixed 0x30-byte object extent, matching the resource-size virtual used by
// Bitmap16Bit and Bitmap816.
class Bitmap24Bit : public resource {
public:
    Bitmap24Bit();
    Bitmap24Bit(const char* name, int w, int h,
                const unsigned char* source, int size);
    Bitmap24Bit(const char* name, const char* path);
    // Loki's vtable (0x8427260) holds only the destructor.
    virtual ~Bitmap24Bit();

    void import(int w, int h, const unsigned char* data, int size);
    void clear();
    int importPCXFile(const char* filename);
    void Draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
              int dx, int dy) const;
    void Draw(int sx, int sy, int sw, int sh, unsigned short* dst,
              int dx, int dy, int dw, int dh, int dpitch) const;
    void AdjustHSV(int x, int y, int w, int h, float hue,
                   float hueAdjust, float saturationAdjust,
                   float valueAdjust);

    // DC bitmap24.h; Loki emits these after Bitmap24.cpp's own functions.
    int GetDataSize() const { return m_dataSize; }
    int GetImageSize() const { return m_imageSize; }
    DC_ADDRESS(0x122b24, 0x4)
    int GetWidth() const { return m_width; }
    DC_ADDRESS(0x122b28, 0x4)
    int GetHeight() const { return m_height; }
    DC_ADDRESS(0x0533b0, 0xa)
    int GetPitch() const { return m_width * 3; }
    unsigned char* GetMap() { return m_data; }
    DC_ADDRESS(0x122b2c, 0x60)
    void AdjustHSV(float hue, float hueAdjust, float saturationAdjust,
                   float valueAdjust)
    {
        AdjustHSV(0, 0, GetWidth(), GetHeight(), hue, hueAdjust,
                  saturationAdjust, valueAdjust);
    }

private:
    int m_dataSize;
    int m_imageSize;
    int m_width;
    int m_height;
    unsigned char* m_data;
};
SIZE(Bitmap24Bit, 0x30);

#endif  /* HOMM3_BITMAP24_H */
