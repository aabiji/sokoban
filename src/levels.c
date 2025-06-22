#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "levels.h"

typedef struct {
    char* content;
    int width, height;
} Puzzle;

// TODO: fix the next 2 functions -- they're completely fucked
// Get the dimensions of each puzzle and allocate their buffers
void allocatePuzzles(char* fileContents, Puzzle* puzzles, int numPuzzles) {
    int x = 0, y = 0, width = 0;
    char* current = fileContents;
    int i = 0;

    while (*current != '\0' && i < numPuzzles) {
        if (current > fileContents) {
            bool newline = *current == '\n' && *(current - 1) == '\n';
            bool lineEnd = *current == '\n' && *(current - 1) != '\n';

            if (newline) {
                puzzles[i].width = width;
                puzzles[i].height = y;
                puzzles[i].content = calloc(width * y, sizeof(char));
                x = y = width = 0;
                i++;
            } else if (lineEnd) {
                width = fmax(width, x); // lines can't be of different lengths
                x = 0;
                y++;
            } else {
                x++;
            }
        }
        current++;
    }
}

// Read the contents of each puzzle and return
// the number of puzzles that have been parsed
int parsePuzzles(char* fileContents, Puzzle* puzzles, int numPuzzles) {
    int puzzle = 0, i = 0;
    char* current = fileContents;

    while (*current != '\0' && puzzle < numPuzzles) {
        if (current > fileContents) {
            bool newline = *current == '\n' && *(current - 1) == '\n';
            if (newline) {
                puzzle++;
                i = 0;
                continue;
            }
        }
        puzzles[puzzle].content[i++] = *current;
        current++;
    }

    return puzzle;
}

bool isNotBorder(Puzzle puzzle, int x, int y) {
    bool xOutside = x < 0 || x + 1 >= puzzle.width;
    bool yOutside = y < 0 || y + 1 >= puzzle.height;
    bool wall = puzzle.content[y * puzzle.width + x] == '#';
    return xOutside || yOutside || !wall;
}

void setBorder(Piece* p, bool c, bool s, int r) {
    p->border.isCorner = c;
    p->border.isSplitWall = s;
    p->border.rotation = r;
}

void getBorderOrientation(Puzzle puzzle, Piece* p, int x, int y) {
    bool l = isNotBorder(puzzle, x - 1, y);
    bool r = isNotBorder(puzzle, x + 1, y);
    bool t = isNotBorder(puzzle, x, y - 1);
    bool b = isNotBorder(puzzle, x, y + 1);

    if (!l && r && t && !b) setBorder(p, true, false, 0); // top right corner
    if (l && !r && t && !b) setBorder(p, true, false, 90); // top left corner
    if (l && !r && b && !t) setBorder(p, true, false, 180); // bottom left corner
    if (!l && r && b && !t) setBorder(p, true, false, 270); // bottom right corner

    if (!l && !r && !t && b) setBorder(p, false, true, 180); // upwards split wall
    if (!l && !r && t && !b) setBorder(p, false, true, 0); // downwards split wall
    if (l && !r && !t && !b) setBorder(p, false, true, 90); // right split wall
    if (!l && r && !t && !b) setBorder(p, false, true, 270); // left split wall

    // vertical wall/horizontal wall
    setBorder(p, false, false, l && r ? 90 : 0);
}

Piece getPiece(Puzzle puzzle, int x, int y) {
    char c = puzzle.content[y * puzzle.width + x];
    if (c == '$') return (Piece){ Box, false };
    if (c == '.') return (Piece){ Empty, true };
    if (c == '*') return (Piece){ Box, true };
    if (c == '#') {
        Piece p = { Border, false };
        getBorderOrientation(puzzle, &p, x, y);
        return p;
    }
    return (Piece){ Empty, false };
}

int parseLevels(char *source, Level levels[NUM_LEVELS]) {
    Puzzle puzzles[NUM_LEVELS] = { 0 };
    allocatePuzzles(source, puzzles, NUM_LEVELS);
    int amountParsed = parsePuzzles(source, puzzles, NUM_LEVELS);
    if (amountParsed != NUM_LEVELS)
        return -1; // not enough levels -- malformed source file

    for (int i = 0; i < NUM_LEVELS; i++) {
        levels[i].width = puzzles[i].width;
        levels[i].height = puzzles[i].height;
        levels[i].numGoals = 0;

        int size = levels[i].width * levels[i].height;
        levels[i].pieces = calloc(size, sizeof(Piece));
        levels[i].original = calloc(size, sizeof(Piece));

        for (int y = 0; y < puzzles[i].height; y++) {
            for (int x = 0; x < puzzles[i].width; x++) {
                int index = y * puzzles[i].width + x;
                Piece p = getPiece(puzzles[i], x, y);
                levels[i].original[index] = p;
                levels[i].pieces[index] = p;

                if (p.isGoal)
                    levels[i].goalIndexes[levels[i].numGoals++] = index;

                if (puzzles[i].content[index] == '@') {
                    levels[i].playerStartX = x;
                    levels[i].playerStartY = y;
                }
            }
        }

        free(puzzles[i].content);
    }

    return 0; // success!
}

// return true if all the goal positions are covered by a box
bool levelCompleted(Level* level) {
    for (int i = 0; i < level->numGoals; i++) {
        int pos = level->goalIndexes[i];
        Piece p = level->pieces[pos];
        if (p.isGoal && p.type != Box) {
            return false;
        }
    }
    return true;
}