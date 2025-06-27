#ifndef LEVELS_H
#define LEVELS_H

#include "animation.h"

#define NUM_LEVELS 50

typedef struct {
    enum { Empty, Border, Box } type;
    bool isGoal; // Goal or floor?
    Animation boxSlide;
} Piece;

typedef struct {
    int width;
    int height;
    int playerStartX;
    int playerStartY;
    int numGoals;
    int goalIndexes[100];
    Piece* pieces;
    Piece* original;
} Level;

int parseLevels(char* filePath, Level* levels);
void restartLevel(Level* level);

int countCompletedGoals(Level* level);
void solveLevel(Level* level);

#endif
