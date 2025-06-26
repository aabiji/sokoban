#ifndef ASSETS_H
#define ASSETS_H

#include <raylib.h>

typedef enum {
    Wall, Floor, Goal, Crate, Guy, NumModels,
} ModelType;

typedef enum {
    MoveSfx, PushSfx, SuccessSfx, BackgroundMusic, NumSounds,
} Sounds;

typedef struct {
    Model model;
    Vector3 size;
    Vector3 scaleFactor;
} ModelAsset;

typedef struct {
    Font font;
    Shader shader;
    Texture textures[NumModels];
    ModelAsset assets[NumModels];
    Sound sounds[NumSounds];
    Vector3 tileSize;
} AssetManager;

AssetManager loadAssets();
void drawModel(
    AssetManager* am, ModelType type, Vector3 offset,
    Vector2 position, float rotation, bool offsetHeight
);
void playSound(AssetManager* am, Sounds sound);
void cleanupAssets(AssetManager* am);

#endif
