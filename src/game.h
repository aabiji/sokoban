#ifndef GAME_H
#define GAME_H

#include "assets.h"

typedef struct {
    Camera3D camera;
    Shader shader;
    AssetManager* assets;

    Animation playerPosition;
    Animation playerRotation;

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
