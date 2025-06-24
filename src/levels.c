#include <raymath.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "levels.h"

typedef struct { int length; char* str; } Line;

Piece getPiece(Line* lines, int width, int height, int x, int y) {
    Piece p = { Empty, false };
    if (x >= lines[y].length) return p;
    char c = lines[y].str[x];

    if (c == '$' || c == '*') {
        p = (Piece){ Box, c == '*' };
        p.boxSlide = createAnimation((Vector2){x, y}, false, PLAYER_SPEED);
    }
    if (c == '.') p = (Piece){ Empty, true };
    if (c == '#') p = (Piece){ Border, false };
    return p;
}

Level parseLevel(Line* lines, int width, int height) {
    Level level = {
        .numGoals = 0,
        .width = width, .height = height,
        .pieces = calloc(width * height, sizeof(Piece)),
        .original = calloc(width * height, sizeof(Piece)),
    };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            Piece p = getPiece(lines, width, height, x, y);
            level.original[index] = p;
            level.pieces[index] = p;

            if (p.isGoal)
                level.goalIndexes[level.numGoals++] = index;

            if (lines[y].str[x] == '@') {
                level.playerStartX = x;
                level.playerStartY = y;
            }
        }
    }

    return level;
}

int parseLevels(char* filePath, Level* levels) {
    FILE* file = fopen(filePath, "r");
    if (file == NULL) return -1;

    char* line = NULL;
    size_t bufferSize = 0;
    size_t length = 0;

    int i = 0;
    int width = 0;
    int height = 0;
    Line lines[MAX_LINES];

    while ((length = getline(&line, &bufferSize, file)) != -1) {
        if (length == 1 && line[0] == '\n') { // puzzles are separated by a new line
            levels[i++] = parseLevel(lines, width, height);

            // reset
            for (int y = 0; y < height; y++) {
                free(lines[y].str);
            }
            width = height = 0;
        } else {
            width = fmax(width, length); // Lines can be of different lengths
            lines[height++] = (Line){ length, strdup(line) };
        }
    }

    // parse the last level
    levels[i++] = parseLevel(lines, width, height);
    for (int y = 0; y < height; y++) {
        free(lines[y].str);
    }

    free(line);
    fclose(file);
    return i != NUM_LEVELS ? -1 : 0;
}
