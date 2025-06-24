#include <math.h>
#include "assets.h"

Asset loadAsset(const char *path, Vector3 targetSize) {
    Asset asset;
    asset.model = LoadModel(path);

    BoundingBox bounds = GetMeshBoundingBox(asset.model.meshes[0]);
    Vector3 size = {
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z
    };

    // scale with correct aspect ratio
    float maxDim = fmax(fmax(size.x, size.y), size.z); // biggest side
    Vector3 f = {
        targetSize.x / maxDim,
        targetSize.y / maxDim,
        targetSize.z / maxDim
    };
    asset.scaleFactor = targetSize.x > 0 ? f : (Vector3){ 1, 1, 1 };
    return asset;
}

AssetManager loadAssets() {
    AssetManager am;
    am.playerScale = (Vector3){3.5, 3.5, 3.5};
    am.tileSize = (Vector3){2.5, 2.5, 2.5};

    am.font = LoadFont("assets/fonts/Grobold.ttf");

    char* paths[] = {
        "assets/Main/Green/tree1/tree.vox",
        "assets/Main/Green/grass1/grass1.vox",
        "assets/Main/Green/walktile/walktile.vox",
        "assets/Main/Green/box1/box1.vox",
        "assets/Animals/Bunny/bunny.vox"
    };

    for (int i = 0; i < NumAssets; i++) {
        Vector3 size = i == Guy ? am.playerScale : am.tileSize;
        am.assets[i] = loadAsset(TextFormat("%s.obj", paths[i]), size);
        am.textures[i] = LoadTexture(TextFormat("%s.png", paths[i]));
    }

    return am;
}

void drawAsset(AssetManager* am, AssetType type, Vector2 position, float rotation) {
    Asset asset = am->assets[type];
    Vector3 axis = { 0, 1, 0 };
    Vector3 realPos = {
        position.x * am->tileSize.x, 0,
        am->tileSize.y / 2 + position.y * am->tileSize.z
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
