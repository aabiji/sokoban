#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "levels.h"

Tile getTile(char c) {
    Tile tile = {Floor, false};
    if (c == '#')
        tile.obj = Wall;
    if (c == '@')
        tile.obj = Pusher;
    if (c == '$')
        tile.obj = Box;
    if (c == '.') {
        tile.isGoal = true;
        tile.obj = Goal;
    }
    if (c == '*') {
        tile.isGoal = true;
        tile.obj = Box;
    }
    return tile;
}

int parseLevels(char *source, Level *buffer, int bufferLength) {
    char *prev = NULL;
    char *current = source;
    char *levelStart = source;

    Vector3 pos = {0, 0};
    bool foundLevelSize = false;

    int i = 0;

    while (*current != '\0') {
        prev = current;
        // make sure we're not skipping the first character
        if (!(current == source && pos.x == 0))
            current++;

        if (*current == '\n') {
            // Each puzzle is seperated by an empty line
            bool newline = *prev == *current;

            if (newline && !foundLevelSize) {
                // backtrace, since
                // we do a first pass to get the size of the buffer
                // then another pass to get the contents of the level
                current = levelStart;
                buffer[i].size.y = pos.y;
                pos.x = pos.y = 0;
                foundLevelSize = true;

                buffer[i].numBytes =
                    buffer[i].size.x * buffer[i].size.y * sizeof(Tile);
                buffer[i].tiles = malloc(buffer[i].numBytes);
                buffer[i].original = malloc(buffer[i].numBytes);
                memset(buffer[i].original, Floor, buffer[i].numBytes);
                continue;
            } else if (newline && foundLevelSize) {
                // Copy the original data into a mutable copy
                memcpy(buffer[i].tiles, buffer[i].original, buffer[i].numBytes);
                // Read the level size and the level contents, move on to the
                // next one
                if (++i >= bufferLength)
                    break;
                pos.x = pos.y = 0;
                foundLevelSize = false;
                levelStart = current;
                continue;
            }

            // new row
            buffer[i].size.x = fmax(
                buffer[i].size.x, pos.x + 1); // lines can be different lengths
            pos.x = 0;
            pos.y++;
            continue;
        }

        // Read the level contents
        pos.x++;
        if (foundLevelSize) {
            int index = pos.y * buffer[i].size.x + pos.x;
            buffer[i].original[index] = getTile(*current);
        }
    }

    return i;
}

void cleanupLevels(Level *levels, int count) {
    for (int i = 0; i < count; i++) {
        free(levels[i].tiles);
        free(levels[i].original);
    }
    free(levels);
}
