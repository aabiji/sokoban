#ifndef LEVELS_H
#define LEVELS_H

#include <raymath.h>
#include <stdbool.h>

#define NUM_LEVELS 50

typedef enum {
    Floor,
    Pusher,
    Box,
    Wall,
    SplitWall,
    Goal,
    Corner,
    ObjectEnumSize
} Object;

typedef struct {
    Object obj;
    bool isGoal; // Is this the tile where boxes should be put?
} Tile;

typedef struct {
    Vector2 size;
    int numBytes;
    Tile *tiles;
    Tile *original;
} Level;

// Parse all the levels from the file and return the number of levels parsed
// NOTE: the file must end in a newline
int parseLevels(char *source, Level *buffer, int bufferLength);
void cleanupLevels(Level *levels, int amount);

#endif
