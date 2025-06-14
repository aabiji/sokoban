#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "raylib.h"
#include "raymath.h"

typedef enum {
    Floor, Pusher, Box, Goal,
    Wall, SplitWall, Corner,
    PieceEnumSize
} Piece;

typedef struct {
    Vector2 size;
    int numBytes;
    Piece* tiles;
    Piece* original;
} Level;

Level* loadLevels(const char* textFile, int amount) {
    char* source = LoadFileText(textFile);

    char* prev = NULL;
    char* current = source;
    char* levelStart = source;

    Vector3 pos = { 0, 0 };
    bool foundLevelSize = false;

    Level* levels = calloc(amount, sizeof(Level));
    int i = 0;

    while (*current != '\0') {
	prev = current;
	// make sure we're not skipping the first character
	if (!(current == source && pos.x == 0)) current++;

	if (*current == '\n') {
	    // Each puzzle is seperated by an empty line
	    bool newline = *prev == *current;
 
	    if (newline && !foundLevelSize) {
		// backtrace, since
		// we do a first pass to get the size of the levels
		// then another pass to get the contents of the level
		current = levelStart;
		levels[i].size.y = pos.y;
		pos.x = pos.y = 0;
		foundLevelSize = true;

		levels[i].numBytes = levels[i].size.x * levels[i].size.y * sizeof(Piece);
		levels[i].tiles = malloc(levels[i].numBytes);
		levels[i].original = malloc(levels[i].numBytes);
		memset(levels[i].tiles, Floor, levels[i].numBytes);
		continue;
	    } else if (newline && foundLevelSize) {
		// Read the level size and the level contents, move on to the next one
		if (++i >= amount) break;
		pos.x = pos.y = 0;
		foundLevelSize = false;
		levelStart = current;
		continue;
	    }

	    // new row
	    levels[i].size.x = fmax(levels[i].size.x, pos.x + 1); // linex can be different lengths
	    pos.x = 0;
	    pos.y++;
	    continue;
	}

	// Read the level contents
	pos.x++;
	if (foundLevelSize) {
	    int index = pos.y * levels[i].size.x + pos.x;
	    Piece p = Floor;
	    if (*current == '#') p = Wall;
	    if (*current == '@') p = Pusher;
	    if (*current == '$') p = Box;
	    if (*current == '*') p = Goal;
	    levels[i].tiles[index] = p;
	}
    }

    UnloadFileText(source);
    return levels;
}

void cleanupLevels(Level* levels, int amount) {
    for (int i = 0; i < amount; i++) {
	free(levels[i].tiles);
	free(levels[i].original);
    }
    free(levels);
}

typedef struct {
    Model model;
    Vector3 scaleFactor;
} Asset;

Asset loadAsset(const char* path, Vector3 targetSize) {
    Asset asset;
    asset.model = LoadModel(path);

    BoundingBox bounds = GetMeshBoundingBox(asset.model.meshes[0]);
    Vector3 size = Vector3Subtract(bounds.max, bounds.min);

    // scale with correct aspect ratio
    float maxDim = fmax(fmax(size.x, size.y), size.z); // biggest side
    Vector3 f = { targetSize.x / maxDim , targetSize.y / maxDim, targetSize.z / maxDim};
    asset.scaleFactor = targetSize.x > 0 ? f : Vector3One();
    return asset;
}

typedef struct {
    Texture texture;
    Asset assets[PieceEnumSize];
    Vector3 tileSize;

    Vector2 player;

    Level* levels;
    int goalPositions[14];
    int numLevels;
    int level;
    bool levelSolved;
} Game;

void changeLevel(Game* game, bool next) {
    game->level = next ? game->level + 1 : game->level - 1;
    game->level = Clamp(game->level, 0, game->numLevels - 1);
    Level level = game->levels[game->level];

    int i = 0;
    int len = sizeof(game->goalPositions) / sizeof(int);
    memset(game->goalPositions, -1, sizeof(game->goalPositions));

    // Find the player and store each goal position

    for (int y = 0; y < level.size.y; y++) {
	for (int x = 0; x < level.size.x; x++) {
	    int index = y * level.size.x + x;
	    // find the player
	    if (level.original[index] == Pusher) {
		level.tiles[index] = Floor;
		game->player.x = x;
		game->player.y = y;
	    }

	    if (level.original[index] == Goal) {
		if (i >= len) {
		    printf("no more space!\n");
		    break;
		}
		game->goalPositions[i++] = index;	
	    }
	}
    }
}

Game newGame() {
    Game game;

    // Load the assets
    game.tileSize = (Vector3){ 2, 2, 2 }; 
    game.assets[Box] = loadAsset("../assets/box_small.gltf", game.tileSize);
    game.assets[Wall] = loadAsset("../assets/wall.gltf", game.tileSize);
    game.assets[SplitWall] = loadAsset("../assets/wall_Tsplit.gltf", game.tileSize);
    game.assets[Corner] = loadAsset("../assets/wall_corner.gltf", game.tileSize);
    game.assets[Floor] = loadAsset("../assets/floor_wood_small_dark.gltf", game.tileSize);
    game.assets[Goal] = loadAsset("../assets/floor_dirt_small_A.gltf", game.tileSize);
    game.assets[Pusher] = loadAsset("../assets/banner_red.gltf", (Vector3){0, 0, 0});
    game.texture = LoadTexture("../assets/dungeon_texture.png");
    for (int i = 0; i < PieceEnumSize; i++) {
	Model m = game.assets[i].model;
	m.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = game.texture;
    }

    // Load each level and keep an original copy of them
    game.numLevels = 27;
    game.levels = loadLevels("../src/levels.txt", game.numLevels);
    for (int i = 0; i < game.numLevels; i++) {
	memcpy(game.levels[i].original, game.levels[i].tiles, game.levels[i].numBytes);
    }

    game.level = -1;
    game.levelSolved = false;
    changeLevel(&game, true);

    return game;
}

void checkProgress(Game* game) {
    game->levelSolved = true;
    int len = sizeof(game->goalPositions) / sizeof(int);

    for (int i = 0; i < len; i++) {
	int pos = game->goalPositions[i];
	if (pos == -1) break; // no more goal positions
	if (game->levels[game->level].tiles[pos] != Box) {
	    game->levelSolved = false;
	    break;
	}
    }
}

void restartGame(Game* game) {
    memcpy(
	game->levels[game->level].tiles,
	game->levels[game->level].original,
	game->levels[game->level].numBytes
    );
    game->level--;
    changeLevel(game, true);
}

void cleanupGame(Game* game) {
    for (int i = 0; i < PieceEnumSize; i++) {
	UnloadModel(game->assets[i].model);
    }
    UnloadTexture(game->texture);

    cleanupLevels(game->levels, game->numLevels);
}

typedef struct {
    bool splitWall; // something like this: |-
    bool corner; // something like this: ⌜
    float rotation;
} Border;

Border computeBorder(Level level, int x, int y) {
    int w = level.size.x, h = level.size.y;
    bool l = x - 1 < 0  || level.tiles[y * w + (x - 1)] != Wall; // left empty?
    bool r = x + 1 >= w || level.tiles[y * w + (x + 1)] != Wall; // right empty?
    bool t = y - 1 < 0  || level.tiles[(y - 1) * w + x] != Wall; // top empty?
    bool b = y + 1 >= h || level.tiles[(y + 1) * w + x] != Wall; // bottom empty?

    // corners
    if (!l && r && t && !b) return (Border){ false, true, 0 };   // top right
    if (!l && r && b && !t) return (Border){ false, true, 270 }; // bottom right
    if (l && !r && b && !t) return (Border){ false, true, 180 }; // bottom left
    if (l && !r && t && !b) return (Border){ false, true, 90 };  // top left

    // split wall
    if (!l && !r && !t && b) return (Border){ true, false, 180 }; // upwards
    if (!l && !r && t && !b) return (Border){ true, false, 0 }; // downwards
    if (l && !r && !t && !b) return (Border){ true, false, 90 }; // right facing
    if (!l && r && !t && !b) return (Border){ true, false, 270  }; // left facing

    // vertical / horizontal side
    return l && r ? (Border){ false, false, 90 } : (Border){ false, false, 0 };
}

void getFirstAndLastWalls(Level b, int row, int* indexFirst, int* indexLast) {
    *indexFirst = b.size.x;
    *indexLast = 0;
    for (int i = 0; i < b.size.x; i++) {
	int index = row * b.size.x + i;
	if (b.tiles[index] == Wall) {
	    if (i < *indexFirst) *indexFirst = i;
	    if (i > *indexLast) *indexLast = i;
	}
    }
}

void drawLevel(Game* game) {
    Level level = game->levels[game->level];

    Vector2 start = {
	-(floor(level.size.x / 2)) * game->tileSize.x,
	-(floor(level.size.y / 2)) * game->tileSize.y,
    };
    Vector3 pos = { start.x, 0, start.y };

    for (int i = 0; i < level.size.y; i++) {
	int first, last;
	getFirstAndLastWalls(level, i, &first, &last);

	for (int j = 0; j < level.size.x; j++) {
	    float angle = 0;
	    Vector3 axis = {0, 1, 0};
	    Piece p = level.tiles[i * (int)level.size.x + j];
	    Asset a = game->assets[p];

	    bool player = j == game->player.x && i == game->player.y;
	    if (player) a = game->assets[Pusher];

	    // Figure out how to draw the wall
	    if (p == Wall) {
		Border b = computeBorder(level, j, i);
		if (b.corner) a = game->assets[Corner];
		if (b.splitWall) a = game->assets[SplitWall];
		angle = b.rotation;
	    }

	    if (j >= first && j <= last) { // Don't draw outside of the bordering walls
		// Draw a floor underneath
		if ((p != Floor && p != Goal) || player) {
		    Asset tile = player ? a : game->assets[Floor];
		    DrawModelEx(tile.model, pos, axis, 0, tile.scaleFactor, WHITE);
		}
		DrawModelEx(a.model, pos, axis, angle, a.scaleFactor, WHITE);
	    }

	    pos.x += game->tileSize.x;
	}

	pos.x = start.x;
	pos.z += game->tileSize.z;
    }
}

void pushBoxes(Level b, Vector2 next, bool* canMove, int x, int y) {
    // The player can only push a chain of boxes horizontally or vertically
    // We need to figure out where the last box they'll be pushing is
    Vector2 end = next;
    while (true) {
	Piece p = b.tiles[(int)(end.y * b.size.x + end.x)];
	if (p != Box) {
	    *canMove = p != Wall;
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
	b.tiles[endIndex] = b.tiles[nextIndex];
	// preserve goal tiles:
	Piece p = b.original[nextIndex] != Box ? b.original[nextIndex] : Floor;
	b.tiles[nextIndex] = p;
    }
}

void movePlayer(Game* game, int deltaX, int deltaY) {
    Level b = game->levels[game->level];
    Vector2 next = { game->player.x + deltaX, game->player.y + deltaY };

    int index = next.y * b.size.x + next.x;
    if (b.tiles[index] == Wall) return;

    if (b.tiles[index] == Box) {
	bool canPushBoxes = false;
	pushBoxes(b, next, &canPushBoxes, deltaX, deltaY);
	if (!canPushBoxes) return;
    }

    game->player = next;
    checkProgress(game);
}

// TODO: get better 3d models
//	 extract levels again -- didn't get them properly the first time
// 	 add menu transition between levels
// 	 add basic lighting
// 	 add a basic titlescreen
// 	 add ui buttons
// 	 port to web and mobile
// 	 release

int main() {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(800, 600, "Sokoban");
    SetTargetFPS(60);

    Game game = newGame();

    Camera3D camera;
    camera.fovy = 45;
    camera.position = (Vector3){ 0.0f, 15.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
	if (IsKeyPressed(KEY_L)) changeLevel(&game, true);
	if (IsKeyPressed(KEY_H)) changeLevel(&game, false);
	if (IsKeyPressed(KEY_R)) restartGame(&game);
	if (IsKeyPressed(KEY_RIGHT)) movePlayer(&game, 1, 0);
	if (IsKeyPressed(KEY_LEFT)) movePlayer(&game, -1, 0);
	if (IsKeyPressed(KEY_UP)) movePlayer(&game, 0, -1);
	if (IsKeyPressed(KEY_DOWN)) movePlayer(&game, 0, 1);

	camera.position.y += GetMouseWheelMove();

	BeginDrawing();
	ClearBackground(RAYWHITE);
	DrawText(TextFormat("%d", game.level + 1), 20, 10, 20, BLACK);
	if (game.levelSolved)
	    DrawText("level solved!", 20, 10, 20, BLACK);

	BeginMode3D(camera);
	drawLevel(&game);
	EndMode3D();

	EndDrawing();
    }

    cleanupGame(&game);
    CloseWindow();
}
