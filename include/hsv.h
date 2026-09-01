#ifndef HOMM3_HSV_H
#define HOMM3_HSV_H

// The six integer sectors selected by HSVToRGB after scaling hue by 6.
// Palette and 24-bit bitmap conversion helpers share this exact domain.
enum THueSector {
    HSV_RED_SECTOR,
    HSV_YELLOW_SECTOR,
    HSV_GREEN_SECTOR,
    HSV_CYAN_SECTOR,
    HSV_BLUE_SECTOR,
    HSV_MAGENTA_SECTOR
};

#endif  // HOMM3_HSV_H
