#ifndef GAME_H
#define GAME_H

#include <raylib.h>

#include "levels.h"

typedef struct {
    Model model;
    Vector3 scaleFactor;
} Asset;

typedef struct {
    bool solvedLevels[50];
} SaveData;

typedef struct {
    Font font;
    Texture textures[3];
    Asset assets[ObjectEnumSize];

    Vector3 tileSize;
    Camera3D camera;

    Vector2 player;
    float playerRotation;

    const char* saveFile;
    SaveData data;

    Level* levels;
    int goalPositions[100];
    int numLevels;
    int level;
    bool levelSolved;
} Game;

Game createGame();
void restartGame(Game* game);
void cleanupGame(Game* game);

void drawLevel(Game* game);
void changeLevel(Game* game, int levelIndex);

void movePlayer(Game* game, int deltaX, int deltaY);
int savePlayerData(Game* game);

#endif
