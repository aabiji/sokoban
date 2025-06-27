#ifndef GAME_H
#define GAME_H

#include "assets.h"

typedef struct {
    Animation position;
    Animation rotation;
    int numMoves;
} Player;

typedef struct {
    AssetManager assetManager;
    Camera3D camera;
    Shader shader;

    Player player;

    int level;
    Level* levels;
    Vector3 drawOffset;
    int numBoxMoves;
    int boxMoves[25];
} Game;

Game* createGame();
void cleanupGame(Game* game);
void drawGame(Game* game);

void changeLevel(Game* game, int levelIndex, bool advance);

bool levelSolved(Game* game);
void movePlayer(Game* game, int deltaX, int deltaY);

#endif
