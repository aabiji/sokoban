#ifndef GAME_H
#define GAME_H

#include "assets.h"
#include "levels.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

typedef struct {
    Animation position;
    Animation rotation;

    int numMoves;
    bool solvedLevels[NUM_LEVELS];
} Player;

typedef struct {
    AssetManager assetManager;
    Camera3D camera;

    Shader shader;
    Light lights[3];

    Player player;
    const char* saveFile;

    int level;
    Level levels[NUM_LEVELS];
    Vector3 drawOffset;

    int numMoves;
    int boxMoves[25];
} Game;

Game createGame();
void cleanupGame(Game* game);
void updateLighting(Game* game);

void drawLevel(Game* game);
void changeLevel(Game* game, int levelIndex, bool advance);
void restartLevel(Game* game);

bool levelSolved(Game* game);
int savePlayerData(Game* game);
void movePlayer(Game* game, int deltaX, int deltaY);

#endif
