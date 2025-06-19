#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "game.h"

typedef enum { MENU_SCREEN, GAME_SCREEN } AppState;

typedef struct {
    Game game;
    AppState state;
    bool quit;
    SaveData playerData;
} App;

// Draw text centered at a point and return its bounding box
Rectangle centerText(
    Font font, Vector2 point, const char* text,
    int fontSize, int spacing, Color color
) {
    Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 p = { point.x - size.x / 2, point.y - size.y / 2 };
    DrawTextEx(font, text, p, fontSize, spacing, color);
    return (Rectangle){ p.x, p.y, size.x, size.y };
}

const bool mouseInside(Rectangle r) {
    Vector2 p = GetMousePosition();
    return p.x >= r.x && p.x <= r.x + r.width &&
	   p.y >= r.y && p.y <= r.y + r.height;
}

void drawMenu(App* app) {
    BeginDrawing();
    ClearBackground(BLACK);

    centerText(app->game.font, (Vector2){ (float)GetScreenWidth() / 2, 50 }, "Sokoban", 45, 0, WHITE);

    float cols = 10;
    int rows = app->game.numLevels / cols;
    float amount = cols + ((cols + 1) / 2); // The columns and the space in between them
    float w = GetScreenWidth() > 1000 ? 900 : GetScreenWidth();
    float boxSize = w / amount;

    float startX = ((GetScreenWidth() - boxSize * amount) / 2) + (boxSize / 2);
    Vector2 pos = { startX, 100 };
    bool hovering = false;

    for (int row = 0; row < rows; row++) {
	for (int col = 0; col < cols; col++) {
	    int level = row * 10 + col;
	    Rectangle r = {pos.x, pos.y, boxSize, boxSize };
	    Color c = app->game.data.solvedLevels[level] ? GREEN : GRAY;
	    DrawRectangleRounded(r, 0.2, 0, c);

	    const char* str = TextFormat("%d", level + 1);
	    Vector2 p = { r.x + boxSize / 2, r.y + boxSize / 2 };
	    centerText(app->game.font, p, str, 20, 0, WHITE);

	    if (mouseInside(r)) {
		hovering = true;
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		    app->state = GAME_SCREEN;
		    changeLevel(&app->game, level);
		    break;
		}
	    }

	    pos.x += boxSize * 1.5;
	}
	pos.y += boxSize * 1.5;
	pos.x = startX;
    }

    Vector2 bottom = {(float)GetScreenWidth() / 2, GetScreenHeight() - 50 };
    centerText(app->game.font, bottom, "(C) 2025- @aabiji", 15, 0, WHITE);
    EndDrawing();

    if (hovering)
	SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else
	SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void gameloop(App* app) {
    Color bg = { 135, 206, 235, 255 };
    Color text = { 160, 82, 45, 255 };

    if (IsKeyPressed(KEY_R)) restartGame(&app->game);
    if (IsKeyPressed(KEY_RIGHT)) movePlayer(&app->game, 1, 0);
    if (IsKeyPressed(KEY_LEFT)) movePlayer(&app->game, -1, 0);
    if (IsKeyPressed(KEY_UP)) movePlayer(&app->game, 0, -1);
    if (IsKeyPressed(KEY_DOWN)) movePlayer(&app->game, 0, 1);

    BeginDrawing();
    ClearBackground(bg);

    BeginMode3D(app->game.camera);
    drawLevel(&app->game);
    EndMode3D();

    // Draw the sidebar of text
    Rectangle box = centerText(app->game.font, (Vector2){ 50, 25 }, "< Menu", 20, 0, WHITE);
    if (mouseInside(box) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
	// TODO: show transition
	app->state = MENU_SCREEN;
    }

    const char* str = TextFormat("Level %d", app->game.level + 1);
    centerText(app->game.font, (Vector2){ 75, 55 }, str, 30, 0, text);

    // TODO: level, box / boxes, # moves
    if (app->game.levelSolved) {
	// TODO: show transition animation and transition to the next level (smooth fade in?)
	DrawTextEx(app->game.font, "Solved!", (Vector2){ 20, 40 }, 30, 0, text);
	app->game.data.solvedLevels[app->game.level] = true;
    }

    EndDrawing();
}

void updateApp(void* data) {
    App* app = (App*)data;

    if (WindowShouldClose()) {
	savePlayerData(&app->game);
	app->quit = true;
	return;
    }

    if (app->state == MENU_SCREEN)
	drawMenu(app);
    else
	gameloop(app);
}

// TODO: loading levels is pretty slow...
//	 debug wall rendering
//	 why are there lines when drawing certain levels?
//	 draw levels we've completed in a different color
//	 store the levels we've finished
//	 polish (different hover animations, etc)
//	 add a smooth fade in transition
//	 add character animation
//	 sound effects and a chill soundtrack
// 	 add basic lighting
// 	 nicer menu screen
// 	 port to web and mobile
// 	 release

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Sokoban");

    App app = { createGame(), MENU_SCREEN, false };

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(updateGameWrapper, &app, 60, 1);
#else
    SetTargetFPS(60);
    while (!app.quit) updateApp(&app);
#endif

    cleanupGame(&app.game);
    CloseWindow();
}
