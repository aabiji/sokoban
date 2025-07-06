#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include "assets.h"

#if defined(PLATFORM_WEB)
    #define GLSL_VERSION 100
#else
    #define GLSL_VERSION 330
#endif

void loadSaveData(AssetManager* am) {
#if defined(PLATFORM_WEB)
    am->saveFile = "/game-data/save.dat";
#else
    am->saveFile = "assets/save.dat";
#endif

    int result = -1;
    FILE* fp = fopen(am->saveFile, "rb");
    if (fp != NULL) result = fread(&am->data, sizeof(SaveData), 1, fp);
    if (fp == NULL || result != 0) {  // reset to defaults if we couldn't read the file
        memset(&am->data.solvedLevels, false, sizeof(am->data.solvedLevels));
        am->data.playBgMusic = true;
        am->data.fullscreen = true;
    }
}

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

    bool isBox = strcmp(path, "assets/models/box/box1.vox.obj") == 0;
    printf("%d\n", isBox);
    Vector3 targetSize = isBox ? am->boxSize : am->tileSize;

    // scale with correct aspect ratio
    float biggestSide = fmax(fmax(asset.size.x, asset.size.y), asset.size.z);
    asset.scaleFactor = (Vector3){
        targetSize.x / biggestSide,
        targetSize.y / biggestSide,
        targetSize.z / biggestSide
    };
    return asset;
}

AssetManager* loadAssets() {
    AssetManager* am = calloc(1, sizeof(AssetManager));
    am->tileSize = (Vector3){2.5, 2.5, 2.5};
    am->boxSize = (Vector3){2.0, 2.0, 2.0};
    am->font = LoadFont("assets/puffy.otf");

    const char* vertexName = TextFormat("assets/shaders/vertex-%d.glsl", GLSL_VERSION);
    const char* fragName = TextFormat("assets/shaders/fragment-%d.glsl", GLSL_VERSION);
    am->shader = LoadShader(vertexName, fragName);

    am->sounds[MoveSfx] = LoadSound("assets/sounds/step.wav");
    am->sounds[PushSfx] = LoadSound("assets/sounds/pop.mp3");
    am->sounds[SuccessSfx] = LoadSound("assets/sounds/success.mp3");
    am->sounds[BackgroundMusic] = LoadSound("assets/sounds/sunshine.mp3");

    loadSaveData(am);

    char* paths[] = {
        "assets/models/tree/tree2.vox",
        "assets/models/grass/grass1.vox",
        "assets/models/nograss/nograss.vox",
        "assets/models/box/box1.vox",
        "assets/models/chicken/chicken.vox"
    };
    for (int i = 0; i < NumModels; i++) {
        am->textures[i] = LoadTexture(TextFormat("%s.png", paths[i]));
        const char* path = TextFormat("%s.obj", paths[i]);
        am->assets[i] = loadModel(am, am->textures[i], path);
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
    int fontSize, Color color, bool center) {
    Vector2 size = MeasureTextEx(am->font, text, fontSize, 0);
    if (center) {
        position.x -= size.x / 2;
        position.y -= size.y / 2;
    }
    DrawTextEx(am->font, text, position, fontSize, 0, color);
    return (Rectangle){position.x, position.y, size.x, size.y};
}

int persistData(AssetManager* am) {
    FILE* fp = fopen(am->saveFile, "wb");
    if (fp == NULL) return -1;
    fwrite(&am->data, sizeof(SaveData), 1, fp);
    fclose(fp);
    return 0;
}

void togglefullscreen(AssetManager* am) { am->data.fullscreen = !am->data.fullscreen; }
void togglePlayBgMusic(AssetManager* am) { am->data.playBgMusic = !am->data.playBgMusic; }
bool alreadySolved(AssetManager* am, int level) { return am->data.solvedLevels[level]; }
void markSolved(AssetManager* am, int level) { am->data.solvedLevels[level] = true; }

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
    UnloadShader(am->shader);
    free(am);
}
