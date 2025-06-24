#include <math.h>
#include "assets.h"

Asset loadAsset(const char *path, Vector3 targetSize) {
    Asset asset;
    asset.model = LoadModel(path);

    BoundingBox bounds = GetMeshBoundingBox(asset.model.meshes[0]);
    asset.size = (Vector3){
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z
    };

    // scale with correct aspect ratio
    float biggestSide = fmax(fmax(asset.size.x, asset.size.y), asset.size.z);
    Vector3 f = {
        targetSize.x / biggestSide,
        targetSize.y / biggestSide,
        targetSize.z / biggestSide
    };
    asset.scaleFactor = targetSize.x > 0 ? f : (Vector3){ 1, 1, 1 };
    return asset;
}

AssetManager loadAssets() {
    AssetManager am = {
        .tileSize = (Vector3){2.5, 2.5, 2.5},
        .font = LoadFont("assets/fonts/Grobold.ttf")
    };

    char* paths[] = {
        "assets/Main/Green/tree2/tree2.vox",
        "assets/Main/Green/grass1/grass1.vox",
        "assets/Main/Green/nograss/nograss.vox",
        "assets/Main/Green/box1/box1.vox",
        "assets/Animals/Bunny/bunny.vox"
    };
    for (int i = 0; i < NumAssets; i++) {
        am.assets[i] = loadAsset(TextFormat("%s.obj", paths[i]), am.tileSize);
        am.textures[i] = LoadTexture(TextFormat("%s.png", paths[i]));
    }

    return am;
}

void drawAsset(
    AssetManager* am, AssetType type, Vector3 offset,
    Vector2 position, float rotation, bool offsetHeight) {
    Asset asset = am->assets[type];
    Vector3 axis = { 0, 1, 0 };
    Vector3 realPos = {
        offset.x + position.x * am->tileSize.x,
        offsetHeight ? asset.size.y + offset.y : -offset.y,
        offset.z + position.y * am->tileSize.z
    };
    DrawModelEx(asset.model, realPos, axis, rotation, asset.scaleFactor, WHITE);
}

void cleanupAssets(AssetManager* am) {
    UnloadFont(am->font);
    for (int i = 0; i < NumAssets; i++) {
        UnloadModel(am->assets[i].model);
        UnloadTexture(am->textures[i]);
    }
}
