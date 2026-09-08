// Differential-test adapter around the independently reconstructed C++
// CSpriteFrame renderers. This is not linked into the Rust libraries.
#include <cstring>

#define HOMM3_CSPRITEFRAME_DRAW_METHODS
#include "cspriteframe.h"
#include "palette.h"

// Pull the current in-tree implementation into this one test-only object.
#include "src/cspriteframe.cpp"

TBlendMask CSpriteFrame::s_div2mask = {0};
unsigned short CSpriteFrame::s_div4mask = 0;

resource::resource(const char* newName, EResourceType newType)
    : m_resType(newType), m_referenceCount(0) {
    std::strncpy(m_name, newName ? newName : "", 12);
    m_name[12] = '\0';
}

resource::~resource() = default;
void resource::dispose() {}

TPalette16::TPalette16(const unsigned short* source)
    : resource("", RESOURCE_TYPE_NONE) {
    std::memcpy(m_data, source, sizeof(m_data));
}

TPalette16::~TPalette16() = default;
unsigned int TPalette16::getSize() const { return sizeof(*this); }

extern "C" int homm3_cxx_draw_frame(
    const unsigned char* stream, int streamLength,
    int encoding,
    int width, int height, int croppedWidth, int croppedHeight,
    int croppedX, int croppedY, unsigned short* destination,
    int destinationWidth, int destinationHeight, int destinationPitch,
    const unsigned short* palette, int sourceX, int sourceY,
    int sourceWidth, int sourceHeight, int destinationX, int destinationY,
    unsigned char mirrored, unsigned char transparentFills) {
    try {
        g_rleLiteralRunCode = 255;
        CSpriteFrame frame("oracle", width, height,
                           const_cast<unsigned char*>(stream), streamLength,
                           static_cast<TEncodingMethod>(encoding),
                           croppedWidth, croppedHeight,
                           croppedX, croppedY);
        TPalette16 palette16(palette);
        frame.draw(sourceX, sourceY, sourceWidth, sourceHeight, destination,
                   destinationX, destinationY, destinationWidth,
                   destinationHeight, destinationPitch, palette16, mirrored,
                   transparentFills);
        return 1;
    } catch (...) {
        return 0;
    }
}

extern "C" int homm3_cxx_draw_general(
    const unsigned char* stream, int streamLength,
    int width, int height, int croppedWidth, int croppedHeight,
    int croppedX, int croppedY, unsigned short* destination,
    int destinationWidth, int destinationHeight, int destinationPitch,
    const unsigned short* palette, int sourceX, int sourceY,
    int sourceWidth, int sourceHeight, int destinationX, int destinationY,
    unsigned char mirrored, unsigned char transparentFills) {
    return homm3_cxx_draw_frame(
        stream, streamLength, eEncodeGeneralRLE, width, height,
        croppedWidth, croppedHeight, croppedX, croppedY, destination,
        destinationWidth, destinationHeight, destinationPitch, palette,
        sourceX, sourceY, sourceWidth, sourceHeight, destinationX,
        destinationY, mirrored, transparentFills);
}
