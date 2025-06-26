#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "assets.h"
#include "levels.h"
#include "raylib.h"

Game createGame() {
    Game game = { .numMoves = 0 };

    // Load the player data
    game.saveFile = "assets/save.dat";
    FILE *fp = fopen(game.saveFile, "rb");
    if (fp == NULL) { // reset to defaults if we couldn't read the file
        memset(&game.player, 0, sizeof(Player));
        return game;
    }
    fread(&game.player, sizeof(Player), 1, fp);

    // Load the levels
    int value = parseLevels("assets/levels.txt", game.levels);
    if (value == -1) { // TODO: tell user!
        printf("error loading the levels");
        exit(-1);
    }

    game.assetManager = loadAssets();
    return game;
}

void cleanupGame(Game* game) {
    cleanupAssets(&game->assetManager);
    for (int i = 0; i < NUM_LEVELS; i++) {
        free(game->levels[i].pieces);
        free(game->levels[i].original);
    }
}

void orientCamera(Game* game) {
    float w = game->levels[game->level].width * game->assetManager.tileSize.x;
    float h = game->levels[game->level].height * game->assetManager.tileSize.y;
    Vector3 center = {w / 2.0, 0, h / 2.0};

    // Camera distance needed to be to be able to fully see the longest side
    float longerSide = fmax(w, h);
    float distance = (longerSide / 2.0) / tanf((45 * DEG2RAD) / 2.0);
    distance = fmax(45, fmin(distance, 80)); // shouldn't be too zoomed in or zoomed out

    // coordinates necessary to tilt the camera back (tilting the content forwards)
    float tilt = -28.0 * DEG2RAD;
    float y = distance * cosf(tilt);
    float z = distance * sinf(tilt);

    game->camera.fovy = 45;
    game->camera.target = center;
    game->camera.position = (Vector3){ center.x, y, center.z - z };
    game->camera.up = (Vector3){ 0.0f, 0.0f, -1.0f };
    game->camera.projection = CAMERA_PERSPECTIVE;

    // to ensure the level is centered on screen
    game->drawOffset = (Vector3){
        game->assetManager.tileSize.x, 0.0,
        h > 50 ? -game->assetManager.tileSize.z / 2.0 : 0
    };
}

void changeLevel(Game* game, int levelIndex, bool advance) {
    if (advance) {
        levelIndex = game->level + 1;
        playSound(&game->assetManager, SuccessSfx);
    }

    game->level = fmax(0, fmin(levelIndex, NUM_LEVELS - 1));
    orientCamera(game);

    Level* level = &game->levels[game->level];
    Vector2 pos = { level->playerStartX, level->playerStartY };
    game->player.numMoves = 0;
    game->player.position = createAnimation(pos, false, PLAYER_SPEED);
    game->player.rotation = createAnimation((Vector2){ 0, 0 }, true, PLAYER_SPEED);
}

void restartLevel(Game* game) {
    Level* l = &game->levels[game->level];
    int size = l->width * l->height * sizeof(Piece);
    memcpy(l->pieces, l->original, size);
    changeLevel(game, game->level, false);
}

// return true if all the goal positions are covered by a box
bool levelSolved(Game* game) {
    Level* level = &game->levels[game->level];
    for (int i = 0; i < level->numGoals; i++) {
        int pos = level->goalIndexes[i];
        Piece p = level->pieces[pos];
        if (p.isGoal && p.type != Box) {
            return false;
        }
    }
    return true;
}

void getFirstAndLastWalls(Level* level, int row, int* first, int* last) {
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

ModelType getModelType(Piece p) {
    if (p.type == Border) return Wall;
    if (p.type == Box) return Crate;
    return p.isGoal ? Goal : Floor;
}

void updateBoxAnimations(Game* game) {
    Level* level = &game->levels[game->level];
    bool allDone = true;

    for (int i = 0; i < game->numMoves; i++) {
        int index = game->boxMoves[i];
        updateAnimation(&level->pieces[index].boxSlide, GetFrameTime());
        if (level->pieces[index].boxSlide.active)
            allDone = false;
    }

    if (!allDone || game->numMoves == 0) return;

    // once all the boxes are done animating, actually
    // move them to their target position
    for (int i = 0; i < game->numMoves; i++) {
        int index = game->boxMoves[i];
        Animation a = level->pieces[index].boxSlide;

        int currentIndex = a.vector.start.y * level->width + a.vector.start.x;
        int targetIndex = a.vector.end.y * level->width + a.vector.end.x;

        level->pieces[targetIndex].type = Box;
        level->pieces[targetIndex].boxSlide =
            createAnimation(a.vector.end, false, PLAYER_SPEED);

        level->pieces[currentIndex].type = Empty;
        level->pieces[currentIndex].boxSlide =
            createAnimation(a.vector.start, false, PLAYER_SPEED);
    }

    game->numMoves = 0;
}

void drawGame(Game* game) {
    playSound(&game->assetManager, BackgroundMusic);
 
    updateBoxAnimations(game);
    Level level = game->levels[game->level];

    // Draw the level tiles
    for (int y = 0; y < level.height; y++) {
        int first, last;
        getFirstAndLastWalls(&level, y, &first, &last);

        for (int x = 0; x < level.width; x++) {
            if (x < first || x > last) continue; // Not inside the bordering walls

            Piece p = level.pieces[y * level.width + x];
            ModelType type = getModelType(p);
            Vector2 pos = { x, y };
            Vector2 realPos = p.type == Box ? p.boxSlide.vector.value : pos;

            // Draw the floor beneath
            if (p.type != Empty) {
                ModelType t = p.isGoal ? Goal : Floor;
                drawModel(&game->assetManager, t, game->drawOffset, pos, 0, false);
            }

            // Wall, Floor, Goal, Crate, Guy, NumAssets,
            Vector3 offset = game->drawOffset;
            if (p.type == Border) offset.y = 1.0;
            else if (p.type != Empty) offset.y = 0.5;
            drawModel(&game->assetManager, type, offset, realPos, 0, p.type != Empty);
        }
    }

    // Draw the player
    drawModel(
        &game->assetManager,
        Guy,
        game->drawOffset,
        game->player.position.vector.value,
        game->player.rotation.scalar.value,
        true
    );
    updateAnimation(&game->player.position, GetFrameTime());
    updateAnimation(&game->player.rotation, GetFrameTime());
}

bool pushBoxes(Game* game, Vector2 next, int x, int y) {
    Level* level = &game->levels[game->level];

    // get the position of the last box that'll be pushed
    Vector2 end = next;
    while (true) {
        Piece p = level->pieces[(int)(end.y * level->width + end.x)];
        if (p.type != Box) {
            if (p.type == Border)
                return false; // wall's in the way or the slide animation's running
            break;
        }
        end.x += x;
        end.y += y;
    }

    // start the box sliding animation
    Vector2 pos = { end.x - x, end.y - y };
    while (pos.x != next.x - x || pos.y != next.y - y) {
        int current = pos.y * level->width + pos.x;
        Vector2 after = { pos.x + x, pos.y + y };

        // save the index of each box that's animating in order
        game->boxMoves[game->numMoves++] = current;
        startAnimation(&level->pieces[current].boxSlide, after, false);

        pos.x -= x;
        pos.y -= y;
    }

    playSound(&game->assetManager, MoveSfx);
    return true;
}

void movePlayer(Game* game, int deltaX, int deltaY) {
    // lock the player until animations are done running
    if (game->player.rotation.active || game->player.position.active ||
        game->numMoves > 0) return;

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
    if (level.pieces[index].type == Border) return;

    if (level.pieces[index].type == Box) {
        bool canPushBoxes = pushBoxes(game, next, deltaX, deltaY);
        if (!canPushBoxes) return;
    }

    game->player.numMoves++;
    startAnimation(&game->player.position, next, false);
}

int savePlayerData(Game* game) {
    FILE *fp = fopen(game->saveFile, "wb");
    if (fp == NULL)
        return -1;
    fwrite(&game->player, sizeof(Player), 1, fp);
    fclose(fp);
    return 0;
}
