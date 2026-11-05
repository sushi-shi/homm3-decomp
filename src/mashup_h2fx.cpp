// HoMM2's procedural combat spell visuals, ported onto the HoMM3 battlefield
// for the mashup (not part of the retail game).
//
// The source is the HoMM2 reconstruction's SPELLS.cpp: combatManager::DoBolt
// with AddBolt/ResetBoltAngle/DrawBolt (the lightning), its two Lightning
// callers CastSpell (SPELL_LIGHTNING_BOLT) and ChainLightning, and
// combatManager::Armageddon. The bolt logic, constants, RNG draws and
// delays are HoMM2's; what differs is the screen, see FxScreen.
#include "va.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mashup_h2fx.h"

#include "bitmap16.h"
#include "cmbtmgr.h"
#include "misc.h"
#include "prefs.h"
#include "soundmgr.h"
#include "struct.h"
#include "winmgr.h"

namespace {

// The battlefield. HoMM3 draws it at 0,0 on its 800x600 screen, 800x556
// (combatManager::s_combatAreaLimits); HoMM2's was 640x443, and HoMM2
// clips its bolts and shake to that, as these clip to this. HoMM2 and
// HoMM3 hexes are the same size in pixels, so HoMM2's pixel distances
// (bolt widths and segment lengths, shake offsets) are used unscaled.
const int FIELD_WIDTH = 800;
const int FIELD_HEIGHT = 556;

// HoMM2's gfCombatSpeedMod, by the same three-step combat speed setting.
// HoMM2 scales every combat delay by it (COMBAT_DEADLINE, DelayMilli).
const float H2_COMBAT_SPEED_MOD[3] = { 1.0f, 0.7f, 0.35f };

long h2Delay(float milliseconds)
{
    int speed = g_config.m_combatSpeed;
    if (speed < 0)
        speed = 0;
    if (speed > 2)
        speed = 2;
    return static_cast<long>(milliseconds * H2_COMBAT_SPEED_MOD[speed]);
}

// HoMM2 delays (milliseconds before the speed setting).
const float COMBAT_DRAW_DELAY = 75.0f;           // SpellDrawingConstant
const int SPELL_FIZZLE_FRAME_DELAY = 75;         // SpellDrawingConstant
const float CHAIN_LIGHTNING_FRAME_DELAY = 100.0f;
const float SPELL_AREA_ANIMATION_DELAY = 75.0f;
const float SPELL_ARMAGEDDON_PALETTE_DELAY = 20.0f;
const float SPELL_ARMAGEDDON_SHAKE_DELAY = 15.0f;

// ---------------------------------------------------------------------------
// The screen.
//
// HoMM2 draws into an 8-bit screen shown through the VGA palette (6-bit
// levels, 0..63), and flashes by rewriting that palette. Every palette
// effect used here moves each palette component by itself, by a rule that
// depends only on the component's current and original level. So the whole
// palette is one curve per channel (original level -> shown level), and
// running each 16-bit pixel's channels through the curves shows the
// battlefield as HoMM2's palette would.
//
// FxScreen keeps HoMM2's screen as a 16-bit copy of the battlefield drawn
// with the unmodified colours (m_logical: bolts are drawn into it, the shake
// copies into it), and presents rectangles of it through the curves onto
// HoMM3's screen bitmap. m_original keeps the battlefield as it was when
// the effect began.
// ---------------------------------------------------------------------------

const int LEVEL_COUNT = 64;  // VGA palette levels
const int LEVEL_MAX = LEVEL_COUNT - 1;

enum EChannel {
    CHANNEL_RED = 0,
    CHANNEL_GREEN = 1,
    CHANNEL_BLUE = 2,
    CHANNEL_COUNT = 3
};

struct ChannelFormat {
    unsigned int m_mask;
    int m_shift;
    int m_max;  // largest channel value (31 or 63)
};

class FxScreen {
public:
    FxScreen();
    ~FxScreen();

    // Takes the battlefield from HoMM3's screen. False when there is no
    // screen to draw on.
    bool open();

    unsigned short* row(int y) { return m_logical + y * FIELD_WIDTH; }

    // A 16-bit pixel for HoMM2 palette levels.
    unsigned short pixel(int red, int green, int blue) const;

    // HoMM2's BlitBitmapToScreen: shows a rectangle (inclusive corners) of
    // HoMM2's screen through the current palette.
    void present(int minX, int minY, int maxX, int maxY);
    // HoMM2's SetPalette: the whole battlefield changes at once.
    void presentAll() { present(0, 0, FIELD_WIDTH - 1, FIELD_HEIGHT - 1); }

    // The palette curves; call curvesChanged after editing them.
    unsigned char m_curve[CHANNEL_COUNT][LEVEL_COUNT];
    void resetCurves();
    void curvesChanged();

    // HoMM2's Armageddon shake: HoMM2's screen gets the original
    // battlefield moved by (-offsetX, -offsetY). The strips it does not
    // cover keep the previous frame, as in HoMM2.
    void shake(int offsetX, int offsetY);

    // HoMM2's DrawFrame after an effect: the battlefield redrawn clean by
    // combatManager::drawFrame (or the original, if it draws nothing),
    // shown through the current palette.
    void redrawClean(combatManager* cm, int delay, bool waitForTimer);

private:
    void grab();

    unsigned short* m_logical;
    unsigned short* m_original;
    ChannelFormat m_format[CHANNEL_COUNT];
    // Per channel: channel value -> shown channel value, already shifted.
    unsigned short m_lookup[CHANNEL_COUNT][LEVEL_COUNT];
    bool m_identity;
};

int channelToLevel(int value, int max)
{
    return (value * LEVEL_MAX + max / 2) / max;
}

int levelToChannel(int level, int max)
{
    return (level * max + LEVEL_MAX / 2) / LEVEL_MAX;
}

ChannelFormat channelFormat(unsigned int mask, unsigned int fallback)
{
    // A 16-bit channel is 5 or 6 bits; anything else means the masks were
    // never set, and HoMM3's 565 is assumed.
    ChannelFormat format;
    format.m_mask = mask;
    format.m_shift = 0;
    if (mask) {
        while (!(mask & (1u << format.m_shift)))
            ++format.m_shift;
    }
    format.m_max = mask ? int(mask >> format.m_shift) : 0;
    if (format.m_max != 31 && format.m_max != 63) {
        format.m_mask = fallback;
        format.m_shift = 0;
        while (!(fallback & (1u << format.m_shift)))
            ++format.m_shift;
        format.m_max = int(fallback >> format.m_shift);
    }
    return format;
}

FxScreen::FxScreen()
{
    m_logical = 0;
    m_original = 0;
    m_identity = 1;
}

FxScreen::~FxScreen()
{
    delete[] m_logical;
    delete[] m_original;
}

bool FxScreen::open()
{
    if (!g_windowManager || !g_windowManager->m_screenBitmap)
        return 0;
    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    if (screen->getWidth() < FIELD_WIDTH || screen->getHeight() < FIELD_HEIGHT)
        return 0;

    // The screen's 16-bit layout (ResourceManager::setPixelFormat, from
    // the DirectDraw surface): 565 normally, 555 on some cards.
    m_format[CHANNEL_RED] = channelFormat(Bitmap16Bit::s_redMask, 0xf800);
    m_format[CHANNEL_GREEN] = channelFormat(Bitmap16Bit::s_greenMask, 0x07e0);
    m_format[CHANNEL_BLUE] = channelFormat(Bitmap16Bit::s_blueMask, 0x001f);

    m_logical = new unsigned short[FIELD_WIDTH * FIELD_HEIGHT];
    m_original = new unsigned short[FIELD_WIDTH * FIELD_HEIGHT];
    grab();
    memcpy(m_original, m_logical, FIELD_WIDTH * FIELD_HEIGHT * sizeof(unsigned short));
    resetCurves();
    return 1;
}

void FxScreen::grab()
{
    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    for (int y = 0; y < FIELD_HEIGHT; ++y)
        memcpy(row(y), screen->getMap(0, y), FIELD_WIDTH * sizeof(unsigned short));
}

unsigned short FxScreen::pixel(int red, int green, int blue) const
{
    const int levels[CHANNEL_COUNT] = { red, green, blue };
    unsigned short value = 0;
    for (int channel = 0; channel < CHANNEL_COUNT; ++channel) {
        const ChannelFormat& format = m_format[channel];
        value = (unsigned short)(value
            | (levelToChannel(levels[channel], format.m_max) << format.m_shift));
    }
    return value;
}

void FxScreen::resetCurves()
{
    for (int channel = 0; channel < CHANNEL_COUNT; ++channel) {
        for (int level = 0; level < LEVEL_COUNT; ++level)
            m_curve[channel][level] = (unsigned char)level;
    }
    curvesChanged();
}

void FxScreen::curvesChanged()
{
    m_identity = 1;
    for (int channel = 0; channel < CHANNEL_COUNT; ++channel) {
        const ChannelFormat& format = m_format[channel];
        for (int value = 0; value <= format.m_max; ++value) {
            int shown = m_curve[channel][channelToLevel(value, format.m_max)];
            int shownValue = levelToChannel(shown, format.m_max);
            if (shownValue != value)
                m_identity = 0;
            m_lookup[channel][value] = (unsigned short)(shownValue << format.m_shift);
        }
    }
}

void FxScreen::present(int minX, int minY, int maxX, int maxY)
{
    if (minX < 0)
        minX = 0;
    if (minY < 0)
        minY = 0;
    if (maxX > FIELD_WIDTH - 1)
        maxX = FIELD_WIDTH - 1;
    if (maxY > FIELD_HEIGHT - 1)
        maxY = FIELD_HEIGHT - 1;
    if (minX > maxX || minY > maxY)
        return;

    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    const int width = maxX - minX + 1;
    const ChannelFormat& red = m_format[CHANNEL_RED];
    const ChannelFormat& green = m_format[CHANNEL_GREEN];
    const ChannelFormat& blue = m_format[CHANNEL_BLUE];
    for (int y = minY; y <= maxY; ++y) {
        const unsigned short* source = row(y) + minX;
        unsigned short* destination = screen->getMap(minX, y);
        if (m_identity) {
            memcpy(destination, source, width * sizeof(unsigned short));
            continue;
        }
        for (int x = 0; x < width; ++x) {
            unsigned int value = source[x];
            destination[x] = (unsigned short)(
                m_lookup[CHANNEL_RED][(value & red.m_mask) >> red.m_shift]
                | m_lookup[CHANNEL_GREEN][(value & green.m_mask) >> green.m_shift]
                | m_lookup[CHANNEL_BLUE][(value & blue.m_mask) >> blue.m_shift]);
        }
    }
    g_windowManager->updateScreen(minX, minY, width, maxY - minY + 1);
}

void FxScreen::shake(int offsetX, int offsetY)
{
    // HoMM2 copies one row fewer than the moved height.
    const int copyWidth = FIELD_WIDTH - abs(offsetX);
    const int copyHeight = FIELD_HEIGHT - abs(offsetY) - 1;
    const unsigned short* source = m_original + (offsetX >= 0 ? offsetX : 0)
                                 + (offsetY >= 0 ? offsetY : 0) * FIELD_WIDTH;
    unsigned short* destination = m_logical + (offsetX > 0 ? 0 : -offsetX)
                                + (offsetY > 0 ? 0 : -offsetY) * FIELD_WIDTH;
    for (int copyRow = 0; copyRow < copyHeight; ++copyRow) {
        memcpy(destination, source, copyWidth * sizeof(unsigned short));
        source += FIELD_WIDTH;
        destination += FIELD_WIDTH;
    }
}

void FxScreen::redrawClean(combatManager* cm, int delay, bool waitForTimer)
{
    // drawFrame draws into the screen bitmap only; put the original back
    // under it first in case it draws nothing.
    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    for (int y = 0; y < FIELD_HEIGHT; ++y) {
        memcpy(screen->getMap(0, y), m_original + y * FIELD_WIDTH,
               FIELD_WIDTH * sizeof(unsigned short));
    }
    cm->drawFrame(0, 0, 0, delay, 1, waitForTimer);
    grab();
    presentAll();
}

bool canDraw(combatManager* cm)
{
    return cm && !cm->isQuickCombat() && cm->m_combatShowIt && !cm->m_battleOver;
}

// ---------------------------------------------------------------------------
// The lightning bolt: HoMM2's SBolt, AddBolt, ResetBoltAngle, DrawBolt and
// DoBolt, for the one colour mode Lightning uses (BOLT_COLOR_LIGHTNING).
// ---------------------------------------------------------------------------

// BoltDrawingConstant and the SPELLS.cpp bolt defines.
const int BOLT_RANDOM_WIDTH_LOW = 7;
const int BOLT_RANDOM_WIDTH_HIGH = 12;
const int BOLT_NEAR_TARGET_DISTANCE = 15;
const int BOLT_FINISHED_DISTANCE_MAX = 2;
const int BOLT_ANGLE_PERCENT_SCALE = 100;
const int BOLT_ANGLE_AVERAGE_DIVISOR = 200;
const int BOLT_MAX_COUNT = 25;
const int BOLT_EXTENT_SENTINEL = 9999;
const int BOLT_BRANCH_RANDOM_LOW = 50;
const int BOLT_BRANCH_RANDOM_HIGH = 80;
const int BOLT_BRANCH_PERCENT_LIMIT = 100;
const int BOLT_PALETTE_BRIGHTEN_STEP = 16;
const int BRANCH_MIN_REMAINING_DISTANCE_MULTIPLIER = 2;
const double BOLT_INITIAL_ANGLE_BIAS = 2.5;
const double BOLT_INITIAL_ANGLE_DIVISOR = 2.0;
const float BOLT_RANDOM_ANGLE_BIAS = 2.0f;
const double BOLT_RANDOM_ANGLE_DIVISOR = 1.5;
const double BOLT_ANGLE_DISTANCE_FACTOR = 1.5;
const double BOLT_BRANCH_COOLDOWN_FACTOR = 0.75;
const double BOLT_CHILD_ANGLE_SCALE = 0.66;
const double BOLT_CHILD_ANGLE_OFFSET = 20.0;
const double BOLT_CHILD_MIN_ANGLE_OFFSET = -20.0;

// Lightning Bolt's DoBolt arguments (CastSpell, SpellBoltConstant).
const int LIGHTNING_BRANCH_DISTANCE = 150;
const int LIGHTNING_BRANCH_LENGTH = 100;
const int LIGHTNING_START_WIDTH = 9;
const int LIGHTNING_END_WIDTH = 2;
const int LIGHTNING_MIN_ANGLE = -40;
const int LIGHTNING_MAX_ANGLE = 40;
const int LIGHTNING_ANGLE_DISTANCE = 30;

// Chain Lightning's (ChainLightning, BoltDrawingConstant).
const int CHAIN_LIGHTNING_MIN_BRANCH_DISTANCE = 8;
const int CHAIN_LIGHTNING_MAX_BRANCH_DISTANCE = 30;
const int CHAIN_LIGHTNING_DISTANCE_DIVISOR = 10;
const int CHAIN_LIGHTNING_BOLT_LENGTH = 80;
const int CHAIN_LIGHTNING_START_WIDTH = 9;
const int CHAIN_LIGHTNING_END_WIDTH = 2;
const int CHAIN_LIGHTNING_MIN_ANGLE = 10;
const int CHAIN_LIGHTNING_MAX_ANGLE = 80;
const int CHAIN_LIGHTNING_FIRST_MIN_ANGLE = 10;
const int CHAIN_LIGHTNING_FIRST_MAX_ANGLE = 80;

// BOLT_COLOR_LIGHTNING's colours by distance from the beam's edge (0 =
// outermost): KB.PAL entries 0x72, 0x74, 0x75, 0xcc, 0xce and 0xd6 (6-bit
// levels, read from HEROES2.AGG; about #e7ca14, #dbbe20, #d2b21c, #e7a245,
// #e3862d, #ff590c) - a yellow rim over an orange-red core.
const unsigned char LIGHTNING_SHADES[6][CHANNEL_COUNT] = {
    { 57, 50, 5 },   // 0x72
    { 54, 47, 8 },   // 0x74
    { 52, 44, 7 },   // 0x75
    { 57, 40, 17 },  // 0xcc
    { 56, 33, 11 },  // 0xce
    { 63, 22, 3 },   // 0xd6
};

int manhattanLength(int dx, int dy)
{
    return abs(dx) + abs(dy);
}

int vectorLength(int dx, int dy)
{
    return static_cast<int>(sqrt(static_cast<double>(dx * dx + dy * dy)));
}

struct H2Bolt {
    int m_startX;
    int m_startY;
    int m_endX;
    int m_endY;
    int m_branchDistance;
    int m_width;
    int m_angleDistance;
    float m_currentX;
    float m_currentY;
    int m_pixelX;
    int m_pixelY;
    bool m_drawVertically;
    float m_baseAngle;
    float m_angle;
    bool m_finished;
    bool m_nearTarget;
    int m_nearestDistance;
    int m_widthFirst;
    int m_widthLast;
    int m_lastBranchX;
    int m_lastBranchY;
    int m_startWidth;
    int m_endWidth;
    int m_totalDistance;
    int m_minAngle;
    int m_maxAngle;
    float m_distanceRatio;
    int m_forceAngle;
};

// DoBolt's arguments that matter (managePointer is the caller's business,
// and its fourteenth argument is unused).
struct BoltParams {
    int m_branchDistance;
    int m_branchLength;
    int m_startWidth;
    int m_endWidth;
    int m_minAngle;
    int m_maxAngle;
    int m_angleDistance;
    int m_forceAngle;
    int m_frameDelay;
    bool m_brightenPalette;
};

void clampToField(int& x, int& y)
{
    if (x < 0)
        x = 0;
    if (x > FIELD_WIDTH - 1)
        x = FIELD_WIDTH - 1;
    if (y < 0)
        y = 0;
    if (y > FIELD_HEIGHT - 1)
        y = FIELD_HEIGHT - 1;
}

// combatManager::ResetBoltAngle: aims the bolt's next segment at its end
// point, swerved by a random angle that shrinks as the bolt gets closer.
void resetBoltAngle(H2Bolt* bolt)
{
    if (bolt->m_nearTarget)
        return;

    int deltaX = abs(bolt->m_endX - bolt->m_pixelX);
    int deltaY = abs(bolt->m_endY - bolt->m_pixelY);
    int distance = vectorLength(deltaX, deltaY);
    // (HoMM2 divides by a zero length for a bolt that starts on its end.)
    if (distance > bolt->m_totalDistance || bolt->m_totalDistance == 0)
        bolt->m_distanceRatio = 0;
    else
        bolt->m_distanceRatio = static_cast<float>(bolt->m_totalDistance - distance)
                              / static_cast<float>(bolt->m_totalDistance);

    if (bolt->m_startWidth != bolt->m_endWidth) {
        int widthDelta = bolt->m_endWidth - bolt->m_startWidth;
        if (widthDelta > 0)
            ++widthDelta;
        else
            --widthDelta;
        int currentWidth = bolt->m_startWidth
                         + static_cast<int>(widthDelta * bolt->m_distanceRatio);
        if (currentWidth < 1)
            currentWidth = 1;
        bolt->m_width = currentWidth;
    }
    bolt->m_widthFirst = -(bolt->m_width >> 1);
    bolt->m_widthLast = bolt->m_widthFirst + bolt->m_width - 1;

    bolt->m_baseAngle = static_cast<float>(atan2(
        static_cast<double>(bolt->m_endX - bolt->m_pixelX),
        static_cast<double>(bolt->m_endY - bolt->m_pixelY)));
    // An integer division in HoMM2: zero for every angle range it uses.
    float averageAngle = static_cast<float>(
        (bolt->m_minAngle + bolt->m_maxAngle) / BOLT_ANGLE_AVERAGE_DIVISOR);
    averageAngle = static_cast<float>(
        averageAngle * ((BOLT_INITIAL_ANGLE_BIAS - bolt->m_distanceRatio)
                        / BOLT_INITIAL_ANGLE_DIVISOR));
    bolt->m_angle = averageAngle + bolt->m_baseAngle;

    if (bolt->m_minAngle == 0 && bolt->m_maxAngle == 0)
        return;
    if (distance > bolt->m_angleDistance * BOLT_ANGLE_DISTANCE_FACTOR || bolt->m_forceAngle) {
        float randomAngle;
        if (bolt->m_minAngle == bolt->m_maxAngle)
            randomAngle = static_cast<float>(bolt->m_minAngle) / BOLT_ANGLE_PERCENT_SCALE;
        else
            randomAngle = static_cast<float>(random(bolt->m_minAngle, bolt->m_maxAngle))
                        / BOLT_ANGLE_PERCENT_SCALE;
        randomAngle = static_cast<float>(
            randomAngle * ((BOLT_RANDOM_ANGLE_BIAS - bolt->m_distanceRatio)
                           / BOLT_RANDOM_ANGLE_DIVISOR));
        bolt->m_baseAngle = randomAngle + bolt->m_baseAngle;
    }
}

// combatManager::AddBolt.
void addBolt(H2Bolt* bolt, int startX, int startY, int endX, int endY,
             int branchDistance, int startWidth, int endWidth,
             int minAngle, int maxAngle, int angleDistance, int forceAngle)
{
    clampToField(startX, startY);
    clampToField(endX, endY);

    bolt->m_startX = startX;
    bolt->m_startY = startY;
    bolt->m_endX = endX;
    bolt->m_endY = endY;
    bolt->m_branchDistance = branchDistance;
    bolt->m_width = startWidth;
    bolt->m_startWidth = startWidth;
    bolt->m_endWidth = endWidth;
    bolt->m_minAngle = minAngle;
    bolt->m_maxAngle = maxAngle;
    bolt->m_angleDistance = angleDistance;
    bolt->m_currentX = static_cast<float>(startX);
    bolt->m_currentY = static_cast<float>(startY);
    bolt->m_pixelX = startX;
    bolt->m_pixelY = startY;
    bolt->m_finished = 0;
    bolt->m_nearTarget = 0;
    bolt->m_nearestDistance = 0;
    bolt->m_lastBranchX = startX;
    bolt->m_lastBranchY = startY;
    bolt->m_distanceRatio = 0;
    bolt->m_forceAngle = forceAngle;
    bolt->m_baseAngle = 0;
    bolt->m_angle = 0;
    // A mostly horizontal bolt is widened vertically, and vice versa.
    bolt->m_drawVertically = abs(endX - startX) > abs(endY - startY);
    bolt->m_totalDistance = vectorLength(abs(endX - startX), abs(endY - startY));
    resetBoltAngle(bolt);
}

// combatManager::DrawBolt: advances the bolt stepCount pixels along its
// current angle, painting a beam across it at each new pixel.
void drawBolt(FxScreen& screen, const unsigned short* shades, H2Bolt* bolt, int stepCount)
{
    int previousX = static_cast<int>(bolt->m_currentX);
    int previousY = static_cast<int>(bolt->m_currentY);
    const int widthFirst = bolt->m_widthFirst;
    const int widthLast = bolt->m_widthLast;
    // HoMM2 rolls a width here and never uses it; the roll is kept so the
    // bolt draws the same random numbers.
    random(BOLT_RANDOM_WIDTH_LOW, BOLT_RANDOM_WIDTH_HIGH);

    for (int step = 0; step < stepCount; ++step) {
        bolt->m_currentX = static_cast<float>(bolt->m_currentX + sin(bolt->m_baseAngle));
        bolt->m_currentY = static_cast<float>(bolt->m_currentY + cos(bolt->m_baseAngle));
        bolt->m_pixelX = static_cast<int>(bolt->m_currentX);
        bolt->m_pixelY = static_cast<int>(bolt->m_currentY);
        if (bolt->m_pixelX < 0) {
            bolt->m_pixelX = 0;
            bolt->m_currentX = 0;
        }
        if (bolt->m_pixelX > FIELD_WIDTH - 1) {
            bolt->m_pixelX = FIELD_WIDTH - 1;
            bolt->m_currentX = static_cast<float>(FIELD_WIDTH - 1);
        }
        if (bolt->m_pixelY < 0) {
            bolt->m_pixelY = 0;
            bolt->m_currentY = 0;
        }
        if (bolt->m_pixelY > FIELD_HEIGHT - 1) {
            bolt->m_pixelY = FIELD_HEIGHT - 1;
            bolt->m_currentY = static_cast<float>(FIELD_HEIGHT - 1);
        }

        int drawX = bolt->m_pixelX;
        int drawY = bolt->m_pixelY;
        if (bolt->m_pixelX == previousX && bolt->m_pixelY == previousY)
            continue;
        previousX = bolt->m_pixelX;
        previousY = bolt->m_pixelY;

        for (int beamOffset = widthFirst; beamOffset <= widthLast; ++beamOffset) {
            if (bolt->m_drawVertically)
                drawY = bolt->m_pixelY + beamOffset;
            else
                drawX = bolt->m_pixelX + beamOffset;
            if (drawX < 0 || drawX >= FIELD_WIDTH || drawY < 0 || drawY >= FIELD_HEIGHT)
                continue;
            int edgeShade = beamOffset < 0 ? beamOffset - widthFirst : widthLast - beamOffset;
            if (edgeShade > 5)
                edgeShade = 5;
            screen.row(drawY)[drawX] = shades[edgeShade];
        }

        int distance = manhattanLength(bolt->m_endX - bolt->m_pixelX,
                                       bolt->m_endY - bolt->m_pixelY);
        if (bolt->m_nearTarget) {
            if (distance > bolt->m_nearestDistance + 1
                    || distance <= BOLT_FINISHED_DISTANCE_MAX) {
                bolt->m_finished = 1;
                return;
            }
            if (distance < bolt->m_nearestDistance)
                bolt->m_nearestDistance = distance;
        } else if (distance < BOLT_NEAR_TARGET_DISTANCE) {
            bolt->m_nearTarget = 1;
            bolt->m_nearestDistance = distance;
        }
    }
}

// The body of combatManager::DoBolt: grows the bolt (and its forks)
// segment by segment, presenting each segment. With m_brightenPalette the
// palette is brightened first; restoring it is left to the caller, which
// HoMM2 does at a different point for each spell.
void runBolt(FxScreen& screen, int startX, int startY, int endX, int endY,
             const BoltParams& params)
{
    unsigned short shades[6];
    int index;
    for (index = 0; index < 6; ++index) {
        shades[index] = screen.pixel(LIGHTNING_SHADES[index][CHANNEL_RED],
                                     LIGHTNING_SHADES[index][CHANNEL_GREEN],
                                     LIGHTNING_SHADES[index][CHANNEL_BLUE]);
    }

    const int drawDistance = params.m_angleDistance;
    const int drawPassCount = (params.m_angleDistance - 1) / drawDistance + 1;
    const int branchChance = params.m_branchDistance * BOLT_ANGLE_PERCENT_SCALE
                           / params.m_angleDistance;
    unsigned long deadline = GameTime::get();

    if (params.m_brightenPalette) {
        // Every palette component up by 16 levels, capped at full.
        for (int channel = 0; channel < CHANNEL_COUNT; ++channel) {
            for (int level = 0; level < LEVEL_COUNT; ++level) {
                int brightened = level + BOLT_PALETTE_BRIGHTEN_STEP;
                screen.m_curve[channel][level] =
                    (unsigned char)(brightened > LEVEL_MAX ? LEVEL_MAX : brightened);
            }
        }
        screen.curvesChanged();
        screen.presentAll();
    }

    // The swerve range is mirrored for a bolt going left.
    int minAngle = params.m_minAngle;
    int maxAngle = params.m_maxAngle;
    if (startX > endX) {
        minAngle = -minAngle;
        maxAngle = -maxAngle;
    }
    if (minAngle > maxAngle) {
        int swap = maxAngle;
        maxAngle = minAngle;
        minAngle = swap;
    }

    H2Bolt bolts[BOLT_MAX_COUNT];
    const int extentPadding = (params.m_startWidth > params.m_endWidth
                               ? params.m_startWidth : params.m_endWidth) >> 1;
    addBolt(&bolts[0], startX, startY, endX, endY, params.m_branchDistance,
            params.m_startWidth, params.m_endWidth, minAngle, maxAngle,
            params.m_angleDistance, params.m_forceAngle);
    int boltCount = 1;

    for (;;) {
        for (int drawPass = 0; drawPass < drawPassCount; ++drawPass) {
            int minX = BOLT_EXTENT_SENTINEL;
            int minY = BOLT_EXTENT_SENTINEL;
            int maxX = -1;
            int maxY = -1;
            for (index = 0; index < boltCount; ++index) {
                H2Bolt& bolt = bolts[index];
                if (bolt.m_finished)
                    continue;
                if (bolt.m_pixelX > maxX)
                    maxX = bolt.m_pixelX;
                if (bolt.m_pixelX < minX)
                    minX = bolt.m_pixelX;
                if (bolt.m_pixelY > maxY)
                    maxY = bolt.m_pixelY;
                if (bolt.m_pixelY < minY)
                    minY = bolt.m_pixelY;
                drawBolt(screen, shades, &bolt, drawDistance);
                if (bolt.m_pixelX > maxX)
                    maxX = bolt.m_pixelX;
                if (bolt.m_pixelX < minX)
                    minX = bolt.m_pixelX;
                if (bolt.m_pixelY > maxY)
                    maxY = bolt.m_pixelY;
                if (bolt.m_pixelY < minY)
                    minY = bolt.m_pixelY;
            }

            GameTime::delayTil(deadline);
            deadline = GameTime::get() + h2Delay(static_cast<float>(params.m_frameDelay));
            screen.present(minX - extentPadding, minY - extentPadding,
                           maxX + extentPadding, maxY + extentPadding);
            pollSound();

            bool allFinished = 1;
            for (index = 0; index < boltCount; ++index) {
                if (!bolts[index].m_finished)
                    allFinished = 0;
            }
            if (allFinished)
                return;

            if (!params.m_branchDistance)
                continue;
            // Forks: each live bolt far enough from its end, and from its
            // last fork, may split off a thinner bolt aimed 0.5..0.8 rad
            // to either side.
            const int oldBoltCount = boltCount;
            for (index = 0; index < oldBoltCount; ++index) {
                H2Bolt& bolt = bolts[index];
                if (bolt.m_finished)
                    continue;
                int remainingDistance = manhattanLength(bolt.m_endX - bolt.m_pixelX,
                                                        bolt.m_endY - bolt.m_pixelY);
                if (boltCount >= BOLT_MAX_COUNT
                        || remainingDistance
                               <= params.m_angleDistance * BRANCH_MIN_REMAINING_DISTANCE_MULTIPLIER
                        || random(0, branchChance) >= BOLT_BRANCH_PERCENT_LIMIT)
                    continue;
                if (bolt.m_lastBranchX != 0) {
                    int branchSeparation = manhattanLength(bolt.m_lastBranchX - bolt.m_pixelX,
                                                           bolt.m_lastBranchY - bolt.m_pixelY);
                    if (branchSeparation < params.m_branchDistance * BOLT_BRANCH_COOLDOWN_FACTOR)
                        continue;
                }
                bolt.m_lastBranchX = bolt.m_pixelX;
                bolt.m_lastBranchY = bolt.m_pixelY;
                float branchAngle = static_cast<float>(
                    random(BOLT_BRANCH_RANDOM_LOW, BOLT_BRANCH_RANDOM_HIGH))
                                  / BOLT_ANGLE_PERCENT_SCALE;
                if (random(0, 1))
                    branchAngle = -branchAngle;
                float currentAngle = bolt.m_angle + branchAngle;
                int childDistance = random(params.m_branchLength >> 1, params.m_branchLength);
                if (childDistance > (remainingDistance >> 1))
                    childDistance = remainingDistance >> 1;
                int childX = static_cast<int>(bolt.m_pixelX + childDistance * sin(currentAngle));
                int childY = static_cast<int>(bolt.m_pixelY + childDistance * cos(currentAngle));
                int childWidth = bolt.m_endWidth < bolt.m_startWidth ? bolt.m_width - 1
                                                                     : bolt.m_width;
                addBolt(&bolts[boltCount], bolt.m_pixelX, bolt.m_pixelY, childX, childY,
                        params.m_branchDistance, childWidth, 1,
                        static_cast<int>(minAngle * BOLT_CHILD_ANGLE_SCALE
                                         + BOLT_CHILD_MIN_ANGLE_OFFSET),
                        static_cast<int>(maxAngle * BOLT_CHILD_ANGLE_SCALE
                                         + BOLT_CHILD_ANGLE_OFFSET),
                        params.m_angleDistance, bolt.m_forceAngle);
                ++boltCount;
            }
        }

        for (index = 0; index < boltCount; ++index) {
            if (!bolts[index].m_finished)
                resetBoltAngle(&bolts[index]);
        }
    }
}

// ---------------------------------------------------------------------------
// Armageddon's constants (SpellDrawingConstant).
// ---------------------------------------------------------------------------

const int SPELL_ARMAGEDDON_BRIGHTEN_PASS_COUNT = 16;
const int SPELL_ARMAGEDDON_SHAKE_PASS_COUNT = 4;
const int SPELL_ARMAGEDDON_SHAKE_FRAME_COUNT = 15;
const int SPELL_ARMAGEDDON_RESTORE_PASS_COUNT = 16;
const int SPELL_ARMAGEDDON_CHANNEL_STEP = 3;
const float SPELL_ARMAGEDDON_SHAKE_SCALE = 0.75f;

const int ARMAGEDDON_SHAKE_OFFSETS[SPELL_ARMAGEDDON_SHAKE_FRAME_COUNT][2] = {
    { 2, 2 }, { 4, 1 }, { 3, -2 }, { 0, -6 }, { 2, -2 },
    { -1, 3 }, { -5, 4 }, { -8, 6 }, { -4, 2 }, { -1, 1 },
    { -3, -3 }, { -7, -5 }, { -5, -7 }, { -2, -3 }, { 0, 0 },
};

}  // namespace

void mashupH2Bolt(combatManager* cm, int startX, int startY, int endX, int endY,
                  int chainLink)
{
    if (!canDraw(cm))
        return;
    FxScreen screen;
    if (!screen.open())
        return;

    BoltParams params;
    if (chainLink < 0) {
        // CastSpell, SPELL_LIGHTNING_BOLT: DoBolt(1, castX, castY,
        // target->MidX(), target->MidY(), 150, 100, 9, 2,
        // BOLT_COLOR_LIGHTNING, -40, 40, 30, 1, 0, 0, 1).
        params.m_branchDistance = LIGHTNING_BRANCH_DISTANCE;
        params.m_branchLength = LIGHTNING_BRANCH_LENGTH;
        params.m_startWidth = LIGHTNING_START_WIDTH;
        params.m_endWidth = LIGHTNING_END_WIDTH;
        params.m_minAngle = LIGHTNING_MIN_ANGLE;
        params.m_maxAngle = LIGHTNING_MAX_ANGLE;
        params.m_angleDistance = LIGHTNING_ANGLE_DISTANCE;
        params.m_forceAngle = 0;
        params.m_frameDelay = 0;
        params.m_brightenPalette = 1;
        runBolt(screen, startX, startY, endX, endY, params);

        // DoBolt's managePointer ending: DrawFrame(1, 0, 0, 0,
        // SPELL_FIZZLE_FRAME_DELAY, 1, 1) - which first waits out the
        // frame timer, so the bolt stays up until then - still under the
        // bright palette, then the palette back.
        screen.redrawClean(cm, SPELL_FIZZLE_FRAME_DELAY, 1);
        screen.resetCurves();
        screen.presentAll();
        return;
    }

    // ChainLightning, strike chainLink: DoBolt(0, from, to, 0, 80, 9, 2,
    // BOLT_COLOR_LIGHTNING, 10, 80, angleDistance, <unused>, 0, 0,
    // strike == 0). No forks; the segment length follows the jump's length;
    // only the first strike flashes the palette.
    int distance = vectorLength(abs(endX - startX), abs(endY - startY));
    int angleDistance = distance / CHAIN_LIGHTNING_DISTANCE_DIVISOR;
    if (angleDistance > CHAIN_LIGHTNING_MAX_BRANCH_DISTANCE)
        angleDistance = CHAIN_LIGHTNING_MAX_BRANCH_DISTANCE;
    if (angleDistance < CHAIN_LIGHTNING_MIN_BRANCH_DISTANCE)
        angleDistance = CHAIN_LIGHTNING_MIN_BRANCH_DISTANCE;
    params.m_branchDistance = 0;
    params.m_branchLength = CHAIN_LIGHTNING_BOLT_LENGTH;
    params.m_startWidth = CHAIN_LIGHTNING_START_WIDTH;
    params.m_endWidth = CHAIN_LIGHTNING_END_WIDTH;
    params.m_minAngle = chainLink == 0 ? CHAIN_LIGHTNING_FIRST_MIN_ANGLE : CHAIN_LIGHTNING_MIN_ANGLE;
    params.m_maxAngle = chainLink == 0 ? CHAIN_LIGHTNING_FIRST_MAX_ANGLE : CHAIN_LIGHTNING_MAX_ANGLE;
    params.m_angleDistance = angleDistance;
    params.m_forceAngle = 0;
    params.m_frameDelay = 0;
    params.m_brightenPalette = chainLink == 0;
    runBolt(screen, startX, startY, endX, endY, params);

    // DoBolt restores the palette as soon as the bolt is drawn; the bolt
    // then stays up for CHAIN_LIGHTNING_FRAME_DELAY before ChainLightning's
    // DrawFrame(1, 0, 0, 0, 0, 1, 1) clears it.
    if (params.m_brightenPalette) {
        screen.resetCurves();
        screen.presentAll();
    }
    GameTime::delay(h2Delay(CHAIN_LIGHTNING_FRAME_DELAY));
    screen.redrawClean(cm, 0, 1);
}

void mashupH2Armageddon(combatManager* cm)
{
    if (!canDraw(cm))
        return;
    FxScreen screen;
    if (!screen.open())
        return;

    int pass;
    int channel;
    int level;

    // Sixteen passes raise every palette component one level (capped at
    // full), 75 ms before the first and 20 ms apart: the field fades to
    // white.
    unsigned long timer = GameTime::get() + h2Delay(SPELL_AREA_ANIMATION_DELAY);
    for (pass = 0; pass < SPELL_ARMAGEDDON_BRIGHTEN_PASS_COUNT; ++pass) {
        for (channel = 0; channel < CHANNEL_COUNT; ++channel) {
            for (level = 0; level < LEVEL_COUNT; ++level) {
                if (screen.m_curve[channel][level] < LEVEL_MAX)
                    ++screen.m_curve[channel][level];
            }
        }
        screen.curvesChanged();
        GameTime::delayTil(timer);
        screen.presentAll();
        timer = GameTime::get() + h2Delay(SPELL_ARMAGEDDON_PALETTE_DELAY);
    }

    // Four shakes of fifteen frames, each smaller than the last (scale 3,
    // 2.25, 1.5, 0.75 of the offset table), 15 ms a frame. Through the
    // first shake green and blue drop three levels a frame: the white
    // field turns red.
    for (pass = 0; pass < SPELL_ARMAGEDDON_SHAKE_PASS_COUNT; ++pass) {
        float scale = static_cast<float>(
            (SPELL_ARMAGEDDON_SHAKE_PASS_COUNT - pass) * SPELL_ARMAGEDDON_SHAKE_SCALE);
        for (int frame = 0; frame < SPELL_ARMAGEDDON_SHAKE_FRAME_COUNT; ++frame) {
            if (pass == 0) {
                for (level = 0; level < LEVEL_COUNT; ++level) {
                    if (screen.m_curve[CHANNEL_GREEN][level] > SPELL_ARMAGEDDON_CHANNEL_STEP)
                        screen.m_curve[CHANNEL_GREEN][level] -= SPELL_ARMAGEDDON_CHANNEL_STEP;
                    if (screen.m_curve[CHANNEL_BLUE][level] > SPELL_ARMAGEDDON_CHANNEL_STEP)
                        screen.m_curve[CHANNEL_BLUE][level] -= SPELL_ARMAGEDDON_CHANNEL_STEP;
                }
                screen.curvesChanged();
            }
            int offsetX = static_cast<int>(ARMAGEDDON_SHAKE_OFFSETS[frame][0] * scale);
            int offsetY = static_cast<int>(ARMAGEDDON_SHAKE_OFFSETS[frame][1] * scale);
            unsigned long frameTimer = GameTime::get() + h2Delay(SPELL_ARMAGEDDON_SHAKE_DELAY);
            pollSound();
            screen.shake(offsetX, offsetY);
            screen.presentAll();
            GameTime::delayTil(frameTimer);
        }
    }

    // DrawFrame(1, 0, 0, 0, COMBAT_DRAW_DELAY, 1, 1): the clean field,
    // still red. (HoMM2 plays the stacks' damage here, under the red; the
    // caller shows that afterwards.)
    screen.redrawClean(cm, static_cast<int>(COMBAT_DRAW_DELAY), 1);

    // Sixteen passes move every component back three levels toward its
    // own colour, 20 ms apart; then the true palette.
    for (pass = 0; pass < SPELL_ARMAGEDDON_RESTORE_PASS_COUNT; ++pass) {
        for (channel = 0; channel < CHANNEL_COUNT; ++channel) {
            for (level = 0; level < LEVEL_COUNT; ++level) {
                int shown = screen.m_curve[channel][level];
                if (shown == level)
                    continue;
                if (shown > level)
                    shown = shown - SPELL_ARMAGEDDON_CHANNEL_STEP > level
                          ? shown - SPELL_ARMAGEDDON_CHANNEL_STEP : level;
                else
                    shown = shown + SPELL_ARMAGEDDON_CHANNEL_STEP < level
                          ? shown + SPELL_ARMAGEDDON_CHANNEL_STEP : level;
                screen.m_curve[channel][level] = (unsigned char)shown;
            }
        }
        screen.curvesChanged();
        screen.presentAll();
        GameTime::delay(h2Delay(SPELL_ARMAGEDDON_PALETTE_DELAY));
    }
    screen.resetCurves();
    screen.presentAll();
}
