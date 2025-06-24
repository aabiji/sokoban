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

void cleanupGame(Game* game) {
    cleanupAssets(&game->assetManager);
    for (int i = 0; i < NUM_LEVELS; i++) {
        free(game->levels[i].pieces);
        free(game->levels[i].original);
    }
}

void centerTopdownCamera(Game* game) {
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

typedef struct {
    Vector2 current;
    Vector2 target;
} BoxMove;

// TODO: this is a shitty implementation, that still buggy
//       when we try to move a move that's currently animating, it breaks
//       completely rewrite this
void updateBoxAnimation(Game* game) {
    Level level = game->levels[game->level];

    BoxMove moves[100];
    int numMoves = 0;

    // first pass, update the box animations
    for (int y = 0; y < level.height; y++) {
        for (int x = 0; x < level.width; x++) {
            int index = y * level.width + x;
            Piece p = level.pieces[index];
            if (p.type != Box || !p.boxSlide.active) continue;

            updateAnimation(&level.pieces[index].boxSlide, GetFrameTime());

            // Record a box move when it's finished its animation
            if (!level.pieces[index].boxSlide.active) {
                Vector2 pos = level.pieces[index].boxSlide.vector.value;
                moves[numMoves++] = (BoxMove){ .current = { x, y }, .target = pos };
            }
        }
    }

    // second pass, move the boxes that are done with their animation
    for (int i = numMoves - 1; i >= 0; i--) {
        BoxMove m = moves[i];

        int targetIndex = m.target.y * level.width + m.target.x;
        level.pieces[targetIndex].type = Box;
        level.pieces[targetIndex].boxSlide = createAnimation(m.target, false, 0.5);

        int currentIndex = m.current.y * level.width + m.current.x;
        level.pieces[currentIndex].type = Empty;
        level.pieces[currentIndex].boxSlide = createAnimation(m.current, false, 0.5);
    }
}

void drawLevel(Game* game) {
    updateBoxAnimation(game);

    Level level = game->levels[game->level];

    // Draw the level tiles
    for (int y = 0; y < level.height; y++) {
        int first, last;
        getFirstAndLastWalls(&level, y, &first, &last);

        for (int x = 0; x < level.width; x++) {
            if (x < first || x > last) continue; // Not inside the bordering walls

            Piece p = level.pieces[y * level.width + x];
            AssetType type = getAssetType(p);
            float angle = p.type == Border ? p.border.rotation : 0;
            Vector2 pos = p.type == Box ? p.boxSlide.vector.value : (Vector2){ x, y };

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
    // get the position of the last box that'll be pushed
    Vector2 end = next;
    while (true) {
        Piece p = level->pieces[(int)(end.y * level->width + end.x)];
        if (p.type != Box) {
            if (p.type == Border || (p.type == Box && p.boxSlide.active))
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
        startAnimation(&level->pieces[current].boxSlide, after, false);
        pos.x -= x;
        pos.y -= y;
    }

    return true;
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
