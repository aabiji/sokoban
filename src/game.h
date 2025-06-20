#ifndef GAME_H
#define GAME_H

#include <raylib.h>

#include "levels.h"

typedef struct {
    Model model;
    Vector3 scaleFactor;
} Asset;

typedef struct {
    float t;
    union {
        struct {
            Vector2 start, end;
        } vector;
        struct {
            float start, end;
        } scalar;
    };
} Animation;

typedef struct {
    Animation position;
    Animation rotation;

    int numMoves;
    bool solvedLevels[NUM_LEVELS];
} Player;

typedef struct {
    Font font;
    Texture textures[3];
    Asset assets[ObjectEnumSize];

    Vector3 tileSize;
    Camera3D camera;

    Player player;
    const char *saveFile;

    int level;
    Level *levels;
    int goalPositions[100];
} Game;

Game createGame();
void restartGame(Game *game);
void cleanupGame(Game *game);

void drawLevel(Game *game);
void changeLevel(Game *game, int levelIndex);
void advanceLevel(Game *game);

void drawPlayer(Game *game);
int savePlayerData(Game *game);
bool movePlayer(Game *game, int deltaX,
                int deltaY); // returns true if level is done

#endif
