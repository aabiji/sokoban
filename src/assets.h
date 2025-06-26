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
void cleanupAssets(AssetManager* am);

void drawModel(
    AssetManager* am, ModelType type, Vector3 offset,
    Vector2 position, float rotation, bool offsetHeight);
Rectangle drawText(
    AssetManager* am, const char* text, Vector2 position,
    int fontSize, Color color); // draw text and return its (x,y,width,height)

void updateSound(AssetManager* am, Sounds sound, bool play);

#endif
