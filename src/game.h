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
} Game;

Game createGame();
void restartGame(Game* game);
void cleanupGame(Game* game);

void drawLevel(Game* game);
void changeLevel(Game* game, int levelIndex);
void advanceLevel(Game* game);

int savePlayerData(Game* game);
// Move the player and return true if the player has completed the level
bool movePlayer(Game* game, int deltaX, int deltaY);

#endif
