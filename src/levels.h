#ifndef LEVELS_H
#define LEVELS_H

#include <raymath.h>
#include <stdbool.h>

typedef enum {
    Floor, Pusher, Box, Wall, SplitWall,
    Goal, Corner, ObjectEnumSize
} Object;

typedef struct {
    Object obj;
    bool isGoal; // Is this the tile where boxes should be put?
} Tile;

typedef struct {
    Vector2 size;
    int numBytes;
    Tile* tiles;
    Tile* original;
} Level;

// NOTE: the file must end in a newline
void loadLevels(char* source, Level* buffer, int bufferLength, int* levelsRead);
void cleanupLevels(Level* levels, int amount);

#endif
