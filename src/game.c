#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "levels.h"

Game createGame() {
    Game game;

    // Load the levels
    int value = parseLevels("assets/levels.txt", game.levels);
    if (value == -1) { // TODO: tell user!
        printf("error loading the levels");
        exit(-1);
    }

    // Load the player data
    game.saveFile = "assets/save.dat";
    FILE *fp = fopen(game.saveFile, "rb");
    if (fp == NULL) { // reset to defaults if we couldn't read the file
        memset(&game.player, 0, sizeof(Player));
        return game;
    }
    fread(&game.player, sizeof(Player), 1, fp);

    game.assetManager = loadAssets(&game.assetManager);
    return game;
}

void cleanupGame(Game *game) {
    cleanupAssets(&game->assetManager);
    for (int i = 0; i < NUM_LEVELS; i++) {
        free(game->levels[i].pieces);
        free(game->levels[i].original);
    }
}

void centerTopdownCamera(Game *game) {
    // setup the camera
    float w = game->levels[game->level].width * game->assetManager.tileSize.x;
    float h = game->levels[game->level].height * game->assetManager.tileSize.y;
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

void changeLevel(Game* game, int levelIndex, bool advance) {
    if (advance) levelIndex = game->level + 1;
    game->level = fmax(0, fmin(levelIndex, NUM_LEVELS - 1));

    centerTopdownCamera(game);

    Level* level = &game->levels[game->level];
    Vector2 pos = { level->playerStartX, level->playerStartY };
    game->player.numMoves = 0;
    game->player.position = createAnimation(pos, false, 0.1);
    game->player.rotation = createAnimation((Vector2){ 0, 0 }, true, 0.1);
}

void restartLevel(Game* game) {
    Level* l = &game->levels[game->level];
    int size = l->width * l->height * sizeof(Piece);
    memcpy(l->pieces, l->original, size);
    changeLevel(game, game->level, false);
}

void getFirstAndLastWalls(Level* level, int row, int *first, int *last) {
    *first = level->width;
    *last = 0;
    for (int i = 0; i < level->width; i++) {
        int index = row * level->width + i;
        if (level->pieces[index].type == Border) {
            if (i < *first) *first = i;
            if (i > *last) *last = i;
        }
    }
}

AssetType getAssetType(Piece p) {
    if (p.type == Border) {
        if (p.border.isCorner) return Corner;
        if (p.border.isSplitWall) return SplitWall;
        return Wall;
    }
    if (p.type == Box) return Crate;
    return p.isGoal ? Goal : Floor;
}

void drawLevel(Game *game) {
    Level level = game->levels[game->level];

    // Draw the level tiles
    for (int y = 0; y < level.height; y++) {
        int first, last;
        getFirstAndLastWalls(&level, y, &first, &last);

        for (int x = 0; x < level.width; x++) {
            if (x < first || x > last) continue; // Not inside the bordering walls

            Piece p = level.pieces[y * level.width + x];
            AssetType type = getAssetType(p);

            Vector2 pos = { x, y };
            float angle = p.type == Border ? p.border.rotation : 0;

            // Draw the floor beneath
            if (p.type != Empty) {
                AssetType t = p.isGoal ? Goal : Floor;
                drawAsset(&game->assetManager, t, pos, 0);
            }
            drawAsset(&game->assetManager, type, pos, angle);
        }
    }

    // Draw the player
    drawAsset(
        &game->assetManager,
        Guy,
        game->player.position.vector.value,
        game->player.rotation.scalar.value
    );
    updateAnimation(&game->player.position, GetFrameTime());
    updateAnimation(&game->player.rotation, GetFrameTime());
}

bool pushBoxes(Level* level, Vector2 next, int x, int y) {
    bool canMove = false;

    // The player can only push a chain of boxes horizontally or vertically
    // We need to figure out where the last box they'll be pushing is
    Vector2 end = next;
    while (true) {
        Piece p = level->pieces[(int)(end.y * level->width + end.x)];
        if (p.type != Box) {
            canMove = p.type != Border;
            break;
        }
        end.x += x;
        end.y += y;
    }

    // We can only push one box at a time, so we can just
    // move the box on the tile the player will move to next
    // to the end of the chain
    if (canMove) {
        int endIndex = end.y * level->width + end.x;
        int nextIndex = next.y * level->width + next.x;
        level->pieces[endIndex].type = level->pieces[nextIndex].type;

        level->pieces[nextIndex].isGoal = level->original[nextIndex].isGoal;
        level->pieces[nextIndex].type = Empty;
    }

    return canMove;
}

bool movePlayer(Game* game, int deltaX, int deltaY) {
    if (game->player.rotation.active || game->player.position.active)
        return false; // can't move while animation is being ran

    if (deltaX == 1)
        startAnimation(&game->player.rotation, (Vector2){90, 0}, false);
    if (deltaX == -1)
        startAnimation(&game->player.rotation, (Vector2){270, 0}, false);
    if (deltaY == 1)
        startAnimation(&game->player.rotation, (Vector2){0, 0}, false);
    if (deltaY == -1)
        startAnimation(&game->player.rotation, (Vector2){180, 0}, false);

    Vector2 current = game->player.position.vector.value;
    Vector2 next =
        (Vector2){round(current.x + deltaX), round(current.y + deltaY)};

    Level level = game->levels[game->level];
    int index = next.y * level.width + next.x;
    if (level.pieces[index].type == Border)
        return false;

    if (level.pieces[index].type == Box) {
        bool canPushBoxes = pushBoxes(&level, next, deltaX, deltaY);
        if (!canPushBoxes) return false;
    }

    game->player.numMoves++;
    startAnimation(&game->player.position, next, false);
    return levelCompleted(&level);
}

int savePlayerData(Game *game) {
    FILE *fp = fopen(game->saveFile, "wb");
    if (fp == NULL)
        return -1;
    fwrite(&game->player, sizeof(Player), 1, fp);
    fclose(fp);
    return 0;
}
