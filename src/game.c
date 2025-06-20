#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "levels.h"
#include "raylib.h"

Asset loadAsset(const char *path, Vector3 targetSize) {
    Asset asset;
    asset.model = LoadModel(path);

    BoundingBox bounds = GetMeshBoundingBox(asset.model.meshes[0]);
    Vector3 size = Vector3Subtract(bounds.max, bounds.min);

    // scale with correct aspect ratio
    float maxDim = fmax(fmax(size.x, size.y), size.z); // biggest side
    Vector3 f = {targetSize.x / maxDim, targetSize.y / maxDim,
                 targetSize.z / maxDim};
    asset.scaleFactor = targetSize.x > 0 ? f : Vector3One();
    return asset;
}

void loadAssets(Game *game, Vector3 playerScale) {
    game->font = LoadFont("assets/fonts/Grobold.ttf");

    game->assets[Box] = loadAsset("assets/models/box.obj", game->tileSize);
    game->assets[Wall] = loadAsset("assets/models/wall.gltf", game->tileSize);
    game->assets[SplitWall] =
        loadAsset("assets/models/split_wall.gltf", game->tileSize);
    game->assets[Corner] =
        loadAsset("assets/models/corner.gltf", game->tileSize);
    game->assets[Pusher] = loadAsset("assets/models/player.obj", playerScale);
    game->assets[Floor] = loadAsset("assets/models/floor.obj", game->tileSize);
    game->assets[Goal] = loadAsset("assets/models/goal.obj", game->tileSize);

    game->textures[0] = LoadTexture("assets/models/texture1.png");
    game->textures[1] = LoadTexture("assets/models/texture2.png");
    game->textures[2] = LoadTexture("assets/models/texture3.png");

    for (int i = 0; i < ObjectEnumSize; i++) {
        Model m = game->assets[i].model;
        int t = i == Floor || i == Goal || i == Pusher ? 2 : 1;
        m.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = game->textures[t];
    }
}

void centerTopdownCamera(Game *game) {
    // setup the camera
    float w = game->levels[game->level].size.x * game->tileSize.x;
    float h = game->levels[game->level].size.y * game->tileSize.y;
    Vector3 center = {w / 2.0, 0, h / 2.0};

    // How high the camera needs to be to be able to fully see the longest side
    float longerSide = fmax(w, h);
    float cameraHeight = (longerSide / 2.0) / tanf((45 * DEG2RAD) / 2.0);

    game->camera.fovy = 45;
    game->camera.target = center;
    game->camera.position = (Vector3){center.x, cameraHeight, center.z};
    game->camera.up = (Vector3){0.0f, 0.0f, -1.0f};
    game->camera.projection = CAMERA_PERSPECTIVE;
}

void loadLevels(Game *game) {
    char *source = LoadFileText("assets/levels.txt");
    game->levels = calloc(NUM_LEVELS, sizeof(Level));

    int numRead = parseLevels(source, game->levels, NUM_LEVELS);
    if (numRead != NUM_LEVELS) {
        // TODO: tell user!
        // TODO: also error if there's more than 1 player (malformed level)
        printf("Coulnd't read the levels properly");
        exit(-1);
    }
    UnloadFileText(source);
}

void resetPlayer(Game *game, float x, float y) {
    game->player.numMoves = 0;
    game->player.position.vector.start = (Vector2){x, y};
    game->player.position.vector.end = (Vector2){x, y};
    game->player.rotation.scalar.start = 0;
    game->player.rotation.scalar.end = 0;
}

void changeLevel(Game *game, int levelIndex) {
    game->level = Clamp(levelIndex, 0, NUM_LEVELS - 1);
    Level level = game->levels[game->level];

    int i = 0;
    int len = sizeof(game->goalPositions) / sizeof(int);
    memset(game->goalPositions, -1, sizeof(game->goalPositions));

    // Find the player and store each goal position
    for (int y = 0; y < level.size.y; y++) {
        for (int x = 0; x < level.size.x; x++) {
            int index = y * level.size.x + x;
            Tile t = level.original[index];

            // find the player
            if (t.obj == Pusher) {
                resetPlayer(game, x, y);
                level.tiles[index] = (Tile){Floor, false};
            }

            if (t.isGoal)
                game->goalPositions[i++] = index;
        }
    }

    centerTopdownCamera(game);
}

void advanceLevel(Game *game) {
    game->player.solvedLevels[game->level] = true;
    changeLevel(game, game->level + 1);
}

int savePlayerData(Game *game) {
    FILE *fp = fopen(game->saveFile, "wb");
    if (fp == NULL)
        return -1;
    fwrite(&game->player, sizeof(Player), 1, fp);
    fclose(fp);
    return 0;
}

int readPlayerData(Game *game) {
    FILE *fp = fopen(game->saveFile, "rb");
    if (fp == NULL)
        return -1;
    int numRead = fread(&game->player, sizeof(Player), 1, fp);
    return numRead == -1 ? -1 : 0;
}

Game createGame() {
    Game game;

    // Load the assets
    game.tileSize = (Vector3){2.5, 2.5, 2.5};
    Vector3 playerScale = (Vector3){3.5, 3.5, 3.5};
    loadAssets(&game, playerScale);

    // Load the player data
    game.saveFile = "assets/save.dat";
    int val = readPlayerData(&game);
    if (val != 0) // reset to defaults if we couldn't read the file
        memset(&game.player, 0, sizeof(Player));

    loadLevels(&game);
    return game;
}

void restartGame(Game *game) {
    memcpy(game->levels[game->level].tiles, game->levels[game->level].original,
           game->levels[game->level].numBytes);
    changeLevel(game, game->level);
}

void cleanupGame(Game *game) {
    for (int i = 0; i < ObjectEnumSize; i++) {
        UnloadModel(game->assets[i].model);
    }

    for (int i = 0; i < 3; i++) {
        UnloadTexture(game->textures[i]);
    }

    cleanupLevels(game->levels, NUM_LEVELS);
}

bool levelComplete(Game *game) {
    int len = sizeof(game->goalPositions) / sizeof(int);

    for (int i = 0; i < len; i++) {
        int pos = game->goalPositions[i];
        if (pos == -1)
            break; // no more goal positions
        Tile t = game->levels[game->level].tiles[pos];
        if (t.isGoal && t.obj != Box) {
            return false;
        }
    }

    return true;
}

typedef struct {
    bool splitWall; // something like this: |-
    bool corner;    // something like this: ⌜
    float rotation;
} Border;

Border computeBorder(Level level, int x, int y) {
    int w = level.size.x, h = level.size.y;
    bool l =
        x - 1 < 0 || level.tiles[y * w + (x - 1)].obj != Wall; // left empty?
    bool r =
        x + 1 >= w || level.tiles[y * w + (x + 1)].obj != Wall; // right empty?
    bool t =
        y - 1 < 0 || level.tiles[(y - 1) * w + x].obj != Wall; // top empty?
    bool b =
        y + 1 >= h || level.tiles[(y + 1) * w + x].obj != Wall; // bottom empty?

    // corners
    if (!l && r && t && !b)
        return (Border){false, true, 0}; // top right
    if (!l && r && b && !t)
        return (Border){false, true, 270}; // bottom right
    if (l && !r && b && !t)
        return (Border){false, true, 180}; // bottom left
    if (l && !r && t && !b)
        return (Border){false, true, 90}; // top left

    // split wall
    if (!l && !r && !t && b)
        return (Border){true, false, 180}; // upwards
    if (!l && !r && t && !b)
        return (Border){true, false, 0}; // downwards
    if (l && !r && !t && !b)
        return (Border){true, false, 90}; // right facing
    if (!l && r && !t && !b)
        return (Border){true, false, 270}; // left facing

    // vertical / horizontal side
    return l && r ? (Border){false, false, 90} : (Border){false, false, 0};
}

void getFirstAndLastWalls(Level b, int row, int *indexFirst, int *indexLast) {
    *indexFirst = b.size.x;
    *indexLast = 0;
    for (int i = 0; i < b.size.x; i++) {
        int index = row * b.size.x + i;
        if (b.tiles[index].obj == Wall) {
            if (i < *indexFirst)
                *indexFirst = i;
            if (i > *indexLast)
                *indexLast = i;
        }
    }
}

Vector2 interpolateVector(Animation a) {
    a.t = a.t * a.t * (3 - 2 * a.t); // smoothstep
    return Vector2Lerp(a.vector.start, a.vector.end, a.t);
}

float interpolateScalar(Animation a) {
    return Lerp(a.scalar.start, a.scalar.end, a.t);
}

void startRotation(Animation *a, float nextAngle) {
    a->t = 0;
    a->scalar.start = a->scalar.end;
    a->scalar.end = nextAngle;
}

void startMovement(Animation *a, Vector2 next) {
    a->t = 0;
    a->vector.start = a->vector.end;
    a->vector.end = next;
}

void drawPlayer(Game *game) {
    Asset asset = game->assets[Pusher];
    Level level = game->levels[game->level];

    Vector2 position = interpolateVector(game->player.position);
    Vector3 coordinate = {position.x * game->tileSize.x, 0,
                          game->tileSize.y / 2 + position.y * game->tileSize.z};

    Vector3 axis = {0, 1, 0};
    float angle = interpolateScalar(game->player.rotation);

    DrawModelEx(asset.model, coordinate, axis, angle, asset.scaleFactor, WHITE);

    // Update the animation
    float duration = 0.1; // seconds
    if (game->player.position.t < 1)
        game->player.position.t += GetFrameTime() / duration;
    if (game->player.rotation.t < 1)
        game->player.rotation.t += GetFrameTime() / duration;
}

void drawLevel(Game *game) {
    Level level = game->levels[game->level];
    Vector3 pos = {0, 0, game->tileSize.y / 2};

    for (int i = 0; i < level.size.y; i++) {
        int first, last;
        getFirstAndLastWalls(level, i, &first, &last);

        for (int j = 0; j < level.size.x; j++) {
            float angle = 0;
            Vector3 axis = {0, 1, 0};
            Tile t = level.tiles[i * (int)level.size.x + j];
            Asset a = game->assets[t.obj];

            // Figure out how to draw the wall
            if (t.obj == Wall) {
                Border b = computeBorder(level, j, i);
                if (b.corner)
                    a = game->assets[Corner];
                if (b.splitWall)
                    a = game->assets[SplitWall];
                angle = b.rotation;
            }

            if (j >= first && j <= last) { // If in between the 2 walls
                // Draw a floor underneath
                if (t.obj != Floor && t.obj != Goal) {
                    Asset temp =
                        t.isGoal ? game->assets[Goal] : game->assets[Floor];
                    DrawModelEx(temp.model, pos, axis, 0, temp.scaleFactor,
                                WHITE);
                }
                DrawModelEx(a.model, pos, axis, angle, a.scaleFactor, WHITE);
            }

            pos.x += game->tileSize.x;
        }

        pos.x = 0;
        pos.z += game->tileSize.z;
    }
}

void pushBoxes(Level b, Vector2 next, bool *canMove, int x, int y) {
    // The player can only push a chain of boxes horizontally or vertically
    // We need to figure out where the last box they'll be pushing is
    Vector2 end = next;
    while (true) {
        Tile t = b.tiles[(int)(end.y * b.size.x + end.x)];
        if (t.obj != Box) {
            *canMove = t.obj != Wall;
            break;
        }
        end.x += x;
        end.y += y;
    }

    // We can only push one box at a time, so we can just
    // move the box on the tile the player will move to next
    // to the end of the chain
    if (*canMove) {
        int endIndex = end.y * b.size.x + end.x;
        int nextIndex = next.y * b.size.x + next.x;
        b.tiles[endIndex].obj = b.tiles[nextIndex].obj;

        Object obj = b.original[nextIndex].isGoal ? Goal : Floor;
        b.tiles[nextIndex].obj = obj;
    }
}

bool movePlayer(Game *game, int deltaX, int deltaY) {
    if (deltaX == 1)
        startRotation(&game->player.rotation, 90);
    if (deltaX == -1)
        startRotation(&game->player.rotation, 270);
    if (deltaY == 1)
        startRotation(&game->player.rotation, 0);
    if (deltaY == -1)
        startRotation(&game->player.rotation, 180);

    Level b = game->levels[game->level];
    Vector2 current = interpolateVector(game->player.position);
    Vector2 next =
        (Vector2){round(current.x + deltaX), round(current.y + deltaY)};

    int index = next.y * b.size.x + next.x;
    if (b.tiles[index].obj == Wall)
        return false;

    if (b.tiles[index].obj == Box) {
        bool canPushBoxes = false;
        pushBoxes(b, next, &canPushBoxes, deltaX, deltaY);
        if (!canPushBoxes)
            return false;
    }

    game->player.numMoves++;
    startMovement(&game->player.position, next);
    return levelComplete(game);
}
