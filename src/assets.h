#ifndef ASSETS_H
#define ASSETS_H

#include <raylib.h>
#include "levels.h"

typedef enum {
    Wall, Floor, Goal, Crate, Guy, NumModels,
} ModelType;

typedef enum {
    MoveSfx, PushSfx, SuccessSfx, BackgroundMusic, NumSounds,
} Sounds;

typedef struct {
    bool solvedLevels[NUM_LEVELS];
    bool playBgMusic;
    bool fullscreen;
} SaveData;

typedef struct {
    Model model;
    Vector3 size;
    Vector3 scaleFactor;
} ModelAsset;

typedef struct {
    Font font;
    Shader shader;
    Sound sounds[NumSounds];

    Vector3 tileSize;
    Vector3 boxSize;
    ModelAsset assets[NumModels];
    Texture textures[NumModels];

    SaveData data;
    const char* saveFile;
} AssetManager;

AssetManager* loadAssets();
void cleanupAssets(AssetManager* am);

void drawModel(
    AssetManager* am, ModelType type, Vector3 offset,
    Vector2 position, float rotation, bool offsetHeight);
Rectangle drawText(
    AssetManager* am, const char* text, Vector2 position,
    int fontSize, Color color, bool center); // draw text and return its (x,y,width,height)

void updateSound(AssetManager* am, Sounds sound, bool play);

int persistData(AssetManager* am);
void togglefullscreen(AssetManager* am);
void togglePlayBgMusic(AssetManager* am);
bool alreadySolved(AssetManager* am, int level);
void markSolved(AssetManager* am, int level);

#endif
