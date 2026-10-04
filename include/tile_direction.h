#ifndef HOMM3_TILE_DIRECTION_H
#define HOMM3_TILE_DIRECTION_H

// The eight neighbour directions, clockwise from north. The order is fixed by
// the retail .bss table at 0x6a80a8, whose sixteen dwords read
// (0,-1) (1,-1) (1,0) (1,1) (0,1) (-1,1) (-1,0) (-1,-1) - and by the mask
// builder below, which clears exactly the three entries whose dy is -1 on the
// top row, the three whose dy is +1 on the bottom row, the three whose dx is
// -1 in column zero and the three whose dx is +1 in the last column.
enum ETileDirection {
    TILE_DIR_NORTH = 0,
    TILE_DIR_NORTHEAST = 1,
    TILE_DIR_EAST = 2,
    TILE_DIR_SOUTHEAST = 3,
    TILE_DIR_SOUTH = 4,
    TILE_DIR_SOUTHWEST = 5,
    TILE_DIR_WEST = 6,
    TILE_DIR_NORTHWEST = 7,
    TILE_DIR_COUNT = 8
};

#endif
