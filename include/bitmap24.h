#ifndef HOMM3_BITMAP24_H
#define HOMM3_BITMAP24_H

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
    unsigned int m_dataSize;
    int m_imageSize;
    int m_width;
    int m_height;
    unsigned char* m_data;

    virtual ~Bitmap24Bit();
    virtual unsigned int getSize() const;

    Bitmap24Bit();
    Bitmap24Bit(const char* name, int w, int h,
                const unsigned char* source, int size);
    Bitmap24Bit(const char* name, const char* path);

    void import(int w, int h, const unsigned char* data, int size);
    void clear();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    // Dreamcast bitmap24.h:72 (dc 0x533b0); both row advances in the raw
    // Draw body inline this exact 24-bit pitch calculation in retail.
    int getPitch() const { return m_width * 3; }
    void draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
              int dx, int dy) const;
    void draw(int sx, int sy, int sw, int sh, unsigned short* dst,
              int dx, int dy, int dw, int dh, int dpitch) const;
    void adjustHSV(int x, int y, int w, int h, float hue,
                   float hueAdjust, float saturationAdjust,
                   float valueAdjust);
    void adjustHSV(float hue, float hueAdjust, float saturationAdjust,
                   float valueAdjust)
    {
        adjustHSV(0, 0, getWidth(), getHeight(), hue, hueAdjust,
                  saturationAdjust, valueAdjust);
    }

private:
    int importPCXFile(const char* filename);
};
SIZE(Bitmap24Bit, 0x30);

#endif  /* HOMM3_BITMAP24_H */
