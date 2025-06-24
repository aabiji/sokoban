#ifndef LEVELS_H
#define LEVELS_H

#include <stdbool.h>
#include "animation.h"

#define NUM_LEVELS 50
#define MAX_LINES 40
#define MAX_GOAL_INDEXES 100

typedef struct {
    enum { Empty, Border, Box } type;
    bool isGoal; // Goal or floor?
    struct {
        float rotation;
        bool isCorner;
        bool isSplitWall;
    } border;
    Animation boxSlide;
} Piece;

typedef struct {
    int width;
    int height;
    int playerStartX;
    int playerStartY;
    int numGoals;
    int goalIndexes[MAX_GOAL_INDEXES];
    Piece* pieces;
    Piece* original;
} Level;

int parseLevels(char* filePath, Level* levels);
bool levelCompleted(Level* level);

#endif
