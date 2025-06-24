#ifndef GAME_H
#define GAME_H

#include "assets.h"
#include "levels.h"

typedef struct {
    Animation position;
    Animation rotation;

    int numMoves;
    bool solvedLevels[NUM_LEVELS];
} Player;

typedef struct {
    AssetManager assetManager;
    Camera3D camera;

    Player player;
    const char *saveFile;

    int level;
    Level levels[NUM_LEVELS];

    int numMoves;
    int boxMoves[25];
} Game;

Game createGame();
void cleanupGame(Game *game);

void drawLevel(Game *game);
void changeLevel(Game *game, int levelIndex, bool advance);
void restartLevel(Game *game);

int savePlayerData(Game *game);
bool movePlayer(Game *game, int deltaX, int deltaY); // returns true if level is done

#endif
