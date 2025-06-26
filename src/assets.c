#include <math.h>
#include <string.h>
#include "assets.h"
#include "raylib.h"

ModelAsset loadModel(AssetManager* am, Texture2D texture, const char *path) {
    ModelAsset asset;
    asset.model = LoadModel(path);

    asset.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    asset.model.materials[0].shader = am->shader;

    Mesh mesh = asset.model.meshes[0];
    if (mesh.normals == NULL)
        GenMeshTangents(&mesh);

    BoundingBox bounds = GetMeshBoundingBox(asset.model.meshes[0]);
    asset.size = (Vector3){
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z
    };

    // scale with correct aspect ratio
    float biggestSide = fmax(fmax(asset.size.x, asset.size.y), asset.size.z);
    asset.scaleFactor = (Vector3){
        am->tileSize.x / biggestSide,
        am->tileSize.y / biggestSide,
        am->tileSize.z / biggestSide
    };
    return asset;
}

AssetManager loadAssets() {
    AssetManager am = {
        .tileSize = (Vector3){2.5, 2.5, 2.5},
        .font = LoadFont("assets/fonts/SuperPlayful.ttf"),
        .shader = LoadShader("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl")
    };

    am.sounds[MoveSfx] = LoadSound("assets/sounds/step.wav");
    am.sounds[PushSfx] = LoadSound("assets/sounds/pop.mp3");
    am.sounds[SuccessSfx] = LoadSound("assets/sounds/success.mp3");
    am.sounds[BackgroundMusic] = LoadSound("assets/sounds/sunshine.mp3");

    char* paths[] = {
        "assets/Main/Green/tree2/tree2.vox",
        "assets/Main/Green/grass1/grass1.vox",
        "assets/Main/Green/nograss/nograss.vox",
        "assets/Main/Green/box1/box1.vox",
        "assets/Animals/Panda/panda.vox"
    };
    for (int i = 0; i < NumModels; i++) {
        am.textures[i] = LoadTexture(TextFormat("%s.png", paths[i]));
        const char* path = TextFormat("%s.obj", paths[i]);
        am.assets[i] = loadModel(&am, am.textures[i], path);
    }

    return am;
}

void drawModel(
    AssetManager* am, ModelType type, Vector3 offset,
    Vector2 position, float rotation, bool offsetHeight) {
    ModelAsset asset = am->assets[type];
    Vector3 axis = { 0, 1, 0 };
    Vector3 realPos = {
        offset.x + position.x * am->tileSize.x,
        offsetHeight ? asset.size.y + offset.y : -offset.y,
        offset.z + position.y * am->tileSize.z
    };
    DrawModelEx(asset.model, realPos, axis, rotation, asset.scaleFactor, WHITE);
}

void updateSound(AssetManager* am, Sounds sound, bool play) {
    bool alreadyPlaying = IsSoundPlaying(am->sounds[sound]);
    if (!alreadyPlaying && play)
        PlaySound(am->sounds[sound]);
    else if (alreadyPlaying && !play)
        StopSound(am->sounds[sound]);
}

Rectangle drawText(
    AssetManager* am, const char* text, Vector2 position,
    int fontSize, Color color) {
    // center the text at the target position
    Vector2 size = MeasureTextEx(am->font, text, fontSize, 0);
    position.x -= size.x / 2;
    position.y -= size.y / 2;
    DrawTextEx(am->font, text, position, fontSize, 0, color);
    return (Rectangle){position.x, position.y, size.x, size.y};
}

void cleanupAssets(AssetManager* am) {
    UnloadFont(am->font);
    for (int i = 0; i < NumModels; i++) {
        UnloadModel(am->assets[i].model);
        UnloadTexture(am->textures[i]);
    }
    for (int i = 0; i < NumSounds; i++) {
        StopSound(am->sounds[i]);
        UnloadSound(am->sounds[i]);
    }
}
