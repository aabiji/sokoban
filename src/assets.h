#ifndef ASSETS_H
#define ASSETS_H

#include <raylib.h>

typedef enum {
    Wall, SplitWall, Corner,
    Floor, Goal, Crate, Guy,
    NumAssets,
} AssetType;

typedef struct {
    Model model;
    Vector3 scaleFactor;
} Asset;

typedef struct {
    Font font;
    Texture textures[3];
    Asset assets[NumAssets];

    Vector3 tileSize;
    Vector3 playerScale;
} AssetManager;

AssetManager loadAssets();
void drawAsset(AssetManager* am, AssetType type, Vector2 position, float rotation);
void cleanupAssets(AssetManager* am);

#endif