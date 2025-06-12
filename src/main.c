#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "raylib.h"
#include "raymath.h"

typedef enum {
    Floor, Wall, Pusher, Box,
    Goal, Corner, PieceEnumSize
} Piece;

Piece getPiece(char c) {
    if (c == '#') return Wall;
    if (c == '@' || c == 'p') return Pusher;
    if (c == 'b' || c == '$') return Box;
    if (c == 'B' || c == '*') return Goal;
    return Floor;
}

typedef struct {
    Vector2 size;
    Piece* pieces;
    bool loaded;
} Board;

Board* loadBoards(const char* textFile, int amount) {
    char* source = LoadFileText(textFile);

    char* prev = NULL;
    char* current = source;
    char* boardStart = source;

    Vector3 pos = { 0, 0 };
    bool foundBoardSize = false;

    Board* boards = calloc(amount, sizeof(Board));
    int i = 0;

    while (*current != '\0') {
	prev = current;
	// make sure we're not skipping the first character
	if (!(current == source && pos.x == 0)) current++;

	if (*current == '\n') {
	    // Each puzzle is seperated by an empty line
	    bool newline = *prev == *current;
 
	    if (newline && !foundBoardSize) {
		// backtrace, since
		// we do a first pass to get the size of the boards
		// then another pass to get the contents of the board
		current = boardStart;
		boards[i].size.y = pos.y;
		pos.x = pos.y = 0;
		foundBoardSize = true;

		int total = boards[i].size.x * boards[i].size.y;
		boards[i].pieces = calloc(total, sizeof(Piece));
		continue;
	    } else if (newline && foundBoardSize) {
		// Read the board size and the board contents, move on to the next one
		if (++i >= amount) break;
		boards[i].loaded = true;
		pos.x = pos.y = 0;
		foundBoardSize = false;
		boardStart = current;
		continue;
	    }

	    // new row
	    boards[i].size.x = fmax(boards[i].size.x, pos.x + 1); // linex can be different lengths
	    pos.x = 0;
	    pos.y++;
	    continue;
	}

	// Read the board contents
	pos.x++;
	if (foundBoardSize) {
	    int index = pos.y * boards[i].size.x + pos.x;
	    boards[i].pieces[index] = getPiece(*current);
	}
    }

    UnloadFileText(source);
    return boards;
}

void cleanupBoards(Board* boards, int amount) {
    for (int i = 0; i < amount; i++) {
	free(boards[i].pieces);
    }
    free(boards);
}

typedef struct {
    Model model;
    Vector3 size;
    Vector3 scaleFactor;
} Asset;

Asset loadAsset(const char* path, Vector3 targetSize) {
    Asset asset;
    asset.model = LoadModel(path);

    BoundingBox bounds = GetMeshBoundingBox(asset.model.meshes[0]);
    asset.size = Vector3Subtract(bounds.max, bounds.min);

    // scale with correct aspect ratio
    float maxDim = fmax(fmax(asset.size.x, asset.size.y), asset.size.z); // biggest side
    Vector3 f = { targetSize.x / maxDim , targetSize.y / maxDim, targetSize.z / maxDim};
    asset.scaleFactor = targetSize.x > 0 ? f : Vector3One();
    return asset;
}

typedef struct
{
    Texture texture;
    Asset assets[PieceEnumSize];
    Vector3 tileSize;

    Board* boards;
    int numBoards;
    int level;
} Game;

Game newGame() {
    Game game;

    game.tileSize = (Vector3){ 2, 2, 2 }; 
    game.assets[Box] = loadAsset("../assets/box_small.gltf", game.tileSize);
    game.assets[Wall] = loadAsset("../assets/wall.gltf", game.tileSize);
    game.assets[Corner] = loadAsset("../assets/wall_corner.gltf", game.tileSize);
    game.assets[Floor] = loadAsset("../assets/floor_wood_small.gltf", game.tileSize);
    game.assets[Goal] = loadAsset("../assets/floor_dirt_small_A.gltf", game.tileSize);
    game.assets[Pusher] = loadAsset("../assets/banner_red.gltf", (Vector3){0, 0, 0});
    game.texture = LoadTexture("../assets/dungeon_texture.png");
    for (int i = 0; i < PieceEnumSize; i++) {
	Model m = game.assets[i].model;
	m.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = game.texture;
    }

    game.level = 0;
    game.numBoards = 35;
    game.boards = loadBoards("../src/levels.txt", game.numBoards);
    return game;
}

void cleanupGame(Game* game) {
    for (int i = 0; i < PieceEnumSize; i++) {
	UnloadModel(game->assets[i].model);
    }
    UnloadTexture(game->texture);

    cleanupBoards(game->boards, game->numBoards);
}

void drawLevel(Game* game) {
    Board board = game->boards[game->level];

    Vector2 start = {
	-(floor(board.size.x / 2)) * game->tileSize.x,
	-(floor(board.size.y / 2)) * game->tileSize.y,
    };
    Vector3 pos = { start.x, 0, start.y };

    for (int i = 0; i < board.size.y; i++) {
	for (int j = 0; j < board.size.x; j++) {
	    int w = board.size.x, h = board.size.y;
	    // check if sides are empty
	    // TODO: fix this
	    bool left   = j - 1 < 0  || board.pieces[i * h + (j - 1)] == Floor;
	    bool right  = j + 1 >= w || board.pieces[i * h + (j + 1)] == Floor;
	    bool top    = i - 1 < 0  || board.pieces[(i - 1) * h + j] == Floor;
	    bool bottom = i + 1 >= h || board.pieces[(i + 1) * h + j] == Floor;
	    bool edge = (top && left && bottom) || (top && right && bottom) || (bottom && left && top) || (bottom && right && top);

	    Piece current = board.pieces[i * (int)board.size.x + j];
	    Piece p = current == Wall && edge ? Corner : current;
	    Asset a = game->assets[p];

	    Vector3 axis = { 0, 0, 0 };
	    float angle = 0;

	    // side wall -- rotate so it's vertical
	    if (p == Wall && (left || right)) {
		angle = 90;
		axis.y = 1;
	    }

	    // Rotate corners properly
	    if (p == Corner) {
		axis.y = 1;
		if (top && right) angle = 0;
		else if (top && left) angle = 90;
		else if (bottom && left) angle = 180;
		else if (bottom && right) angle = 270;
	    }

	    DrawModelEx(a.model, pos, axis, angle, a.scaleFactor, WHITE);
	    pos.x += game->tileSize.x;
	}

	pos.x = start.x;
	pos.z += game->tileSize.z;
    }
}

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
	BeginDrawing();
	ClearBackground(RAYWHITE);

	BeginMode3D(camera);
	drawLevel(&game);
	EndMode3D();

	EndDrawing();
    }

    cleanupGame(&game);
    CloseWindow();
}
