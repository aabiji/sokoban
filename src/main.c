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

typedef struct {
    Texture texture;
    Asset assets[PieceEnumSize];
    Vector3 tileSize;

    Vector2 player;

    Board* boards;
    int numBoards;
    int level;
} Game;

void changeLevel(Game* game, bool next) {
    game->level = next ? game->level + 1 : game->level - 1;
    game->level = Clamp(game->level, 0, game->numBoards);

    // find the player
    Board board = game->boards[game->level];
    for (int y = 0; y < board.size.y; y++) {
	for (int x = 0; x < board.size.x; x++) {
	    int index = y * board.size.x + x;
	    if (board.pieces[index] == Pusher) {
		board.pieces[index] = Floor;
		game->player.x = x;
		game->player.y = y;
		break;
	    }
	}
    }
}

Game newGame() {
    Game game;

    game.tileSize = (Vector3){ 2, 2, 2 }; 
    game.assets[Box] = loadAsset("../assets/box_small.gltf", game.tileSize);
    game.assets[Wall] = loadAsset("../assets/wall.gltf", game.tileSize);
    game.assets[SplitWall] = loadAsset("../assets/wall_Tsplit.gltf", game.tileSize);
    game.assets[Corner] = loadAsset("../assets/wall_corner.gltf", game.tileSize);
    game.assets[Floor] = loadAsset("../assets/floor_wood_small.gltf", game.tileSize);
    game.assets[Goal] = loadAsset("../assets/floor_dirt_small_A.gltf", game.tileSize);
    game.assets[Pusher] = loadAsset("../assets/banner_red.gltf", (Vector3){0, 0, 0});
    game.texture = LoadTexture("../assets/dungeon_texture.png");
    for (int i = 0; i < PieceEnumSize; i++) {
	Model m = game.assets[i].model;
	m.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = game.texture;
    }

    game.numBoards = 27;
    game.boards = loadBoards("../src/levels.txt", game.numBoards);

    game.level = -1;
    changeLevel(&game, true);

    return game;
}

void cleanupGame(Game* game) {
    for (int i = 0; i < PieceEnumSize; i++) {
	UnloadModel(game->assets[i].model);
    }
    UnloadTexture(game->texture);

    cleanupBoards(game->boards, game->numBoards);
}

typedef struct {
    bool splitWall; // something like this: |-
    bool corner; // something like this: ⌜
    float rotation;
} Border;

Border computeBorder(Board board, int x, int y) {
    int w = board.size.x, h = board.size.y;
    bool l = x - 1 < 0  || board.pieces[y * w + (x - 1)] != Wall; // left empty?
    bool r = x + 1 >= w || board.pieces[y * w + (x + 1)] != Wall; // right empty?
    bool t = y - 1 < 0  || board.pieces[(y - 1) * w + x] != Wall; // top empty?
    bool b = y + 1 >= h || board.pieces[(y + 1) * w + x] != Wall; // bottom empty?

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

void getFirstAndLastWalls(Board b, int row, int* indexFirst, int* indexLast) {
    *indexFirst = b.size.x;
    *indexLast = 0;
    for (int i = 0; i < b.size.x; i++) {
	int index = row * b.size.x + i;
	if (b.pieces[index] == Wall) {
	    if (i < *indexFirst) *indexFirst = i;
	    if (i > *indexLast) *indexLast = i;
	}
    }
}

void drawLevel(Game* game) {
    Board board = game->boards[game->level];

    Vector2 start = {
	-(floor(board.size.x / 2)) * game->tileSize.x,
	-(floor(board.size.y / 2)) * game->tileSize.y,
    };
    Vector3 pos = { start.x, 0, start.y };

    for (int i = 0; i < board.size.y; i++) {
	int first, last;
	getFirstAndLastWalls(board, i, &first, &last);

	for (int j = 0; j < board.size.x; j++) {
	    float angle = 0;
	    Vector3 axis = {0, 1, 0};
	    Piece p = board.pieces[i * (int)board.size.x + j];
	    Asset a = game->assets[p];

	    bool player = j == game->player.x && i == game->player.y;
	    if (player) a = game->assets[Pusher];

	    // Figure out how to draw the wall
	    if (p == Wall) {
		Border b = computeBorder(board, j, i);
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

/*
__xx_x<
___x<

_xx_
_xx<

can only push on one line
horizontal, vertical
*/
void pushBoxes(Game* game, Vector2 nextPosition) {
}

void movePlayer(Game* game, int deltaX, int deltaY) {
    Board b = game->boards[game->level];
    Vector2 next = {
	fmax(0, fmin(game->player.x + deltaX, b.size.x)),
	fmax(0, fmin(game->player.y + deltaY, b.size.y))
    };

    int index = next.y * b.size.x + next.x;
    if (b.pieces[index] == Wall) return;

    if (b.pieces[index] == Box) pushBoxes(game, next);
    game->player = next;
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
	if (IsKeyPressed(KEY_L)) changeLevel(&game, true);
	if (IsKeyPressed(KEY_H)) changeLevel(&game, false);
	if (IsKeyPressed(KEY_RIGHT)) movePlayer(&game, 1, 0);
	if (IsKeyPressed(KEY_LEFT)) movePlayer(&game, -1, 0);
	if (IsKeyPressed(KEY_UP)) movePlayer(&game, 0, -1);
	if (IsKeyPressed(KEY_DOWN)) movePlayer(&game, 0, 1);

	camera.position.y += GetMouseWheelMove();

	BeginDrawing();
	ClearBackground(RAYWHITE);
	DrawText(TextFormat("%d", game.level), 20, 10, 20, BLACK);

	BeginMode3D(camera);
	drawLevel(&game);
	EndMode3D();

	EndDrawing();
    }

    cleanupGame(&game);
    CloseWindow();
}
