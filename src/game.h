#ifndef GAME_H
#define GAME_H

#include <raylib.h>

#include "levels.h"

typedef struct {
    Model model;
    Vector3 scaleFactor;
} Asset;

typedef struct {
    int numMoves;
    float rotation;
    Vector2 position;
    // solved[levelIndex] == true if the level has been solved
    bool solved[NUM_LEVELS];
} Player;

typedef struct {
    Font font;
    Texture textures[3];
    Asset assets[ObjectEnumSize];

    Vector3 tileSize;
    Camera3D camera;

    Player player;
    const char* saveFile;

    int level;
    Level* levels;
    int goalPositions[100];
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
