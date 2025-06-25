#ifndef ASSETS_H
#define ASSETS_H

#include <raylib.h>

typedef enum {
    Wall, Floor, Goal, Crate, Guy, NumAssets,
} AssetType;

typedef struct {
    Model model;
    Vector3 size;
    Vector3 scaleFactor;
} Asset;

typedef struct {
    Font font;
    Shader shader;
    Texture textures[NumAssets];
    Asset assets[NumAssets];
    Vector3 tileSize;
} AssetManager;

AssetManager loadAssets();
void drawAsset(
    AssetManager* am, AssetType type, Vector3 offset,
    Vector2 position, float rotation, bool offsetHeight
);
void cleanupAssets(AssetManager* am);

#endif
