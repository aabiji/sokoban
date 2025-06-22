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

    am.assets[Crate] = loadAsset("assets/models/box.obj", am.tileSize);
    am.assets[Wall] = loadAsset("assets/models/wall.gltf", am.tileSize);
    am.assets[SplitWall] =
        loadAsset("assets/models/split_wall.gltf", am.tileSize);
    am.assets[Corner] =
        loadAsset("assets/models/corner.gltf", am.tileSize);
    am.assets[Guy] = loadAsset("assets/models/player.obj", am.playerScale);
    am.assets[Floor] = loadAsset("assets/models/floor.obj", am.tileSize);
    am.assets[Goal] = loadAsset("assets/models/goal.obj", am.tileSize);

    am.textures[0] = LoadTexture("assets/models/texture1.png");
    am.textures[1] = LoadTexture("assets/models/texture2.png");
    am.textures[2] = LoadTexture("assets/models/texture3.png");

    for (int i = 0; i < NumAssets; i++) {
        Model m = am.assets[i].model;
        int t = i == Floor || i == Goal || i == Guy ? 2 : 1;
        m.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = am.textures[t];
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
    }
    for (int i = 0; i < 3; i++) {
        UnloadTexture(am->textures[i]);
    }
}
