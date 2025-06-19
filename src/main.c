#include "raylib.h"
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "game.h"

typedef struct {
    Game game;
    SaveData playerData;
    bool drawingMenu;
    bool quit;
    float fadeTime;
} App;

App createApp() {
    App app;
    app.game = createGame();
    app.quit = false;
    app.fadeTime = 0;
    return app;
}

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

float easeInOutQuad(float t) {
    return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) *t;
}

// Draw a fullscreen overlay that gradually fades out over time
void drawFadeAnimation(App* app) {
    // Setting fadeTime >= 0 triggers the animation
    if (app->fadeTime < 0) return;

    float alpha = 255.0 - (255.0 * easeInOutQuad(app->fadeTime));

    float duration = 0.5; // seconds 
    app->fadeTime += GetFrameTime() / duration;

    DrawRectangle(
	0, 0, GetScreenWidth(), GetScreenHeight(),
	(Color){ 0, 0, 0, alpha });

    if (app->fadeTime >= 1.0)
	app->fadeTime = -1; // stop the animation
}

Color brightenColor(Color c, float amount) {
    Color new;
    new.r = (c.r + (255 - c.r) * amount);
    new.g = (c.g + (255 - c.g) * amount);
    new.b = (c.b + (255 - c.b) * amount);
    new.a = 255;
    return new;
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

	    if (mouseInside(r)) {
		hovering = true;
		c = brightenColor(c, 0.2);
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		    app->drawingMenu = false;
		    changeLevel(&app->game, level);
		    app->fadeTime = 0;
		    break;
		}
	    }

	    DrawRectangleRounded(r, 0.2, 0, c);

	    const char* str = TextFormat("%d", level + 1);
	    Vector2 p = { r.x + boxSize / 2, r.y + boxSize / 2 };
	    centerText(app->game.font, p, str, 20, 0, WHITE);

	    pos.x += boxSize * 1.5;
	}
	pos.y += boxSize * 1.5;
	pos.x = startX;
    }

    Vector2 bottom = {(float)GetScreenWidth() / 2, GetScreenHeight() - 50 };
    centerText(app->game.font, bottom, "(C) 2025- @aabiji", 15, 0, WHITE);

    drawFadeAnimation(app);
    EndDrawing();

    if (hovering)
	SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else
	SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void drawGameInfo(App* app) {
    Color color = { 160, 82, 45, 255 };

    // Draw the sidebar of text
    Rectangle box = centerText(app->game.font, (Vector2){ 50, 25 }, "< Menu", 20, 0, WHITE);
    if (mouseInside(box) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
	app->drawingMenu = true;
	app->fadeTime = 0;
    }

    const char* str = TextFormat("Level %d", app->game.level + 1);
    centerText(app->game.font, (Vector2){ 75, 55 }, str, 30, 0, color);
    // TODO: level, box / boxes, # moves
}

void gameloop(App* app) {
    bool levelSolved = false;

    if (IsKeyPressed(KEY_RIGHT))
	levelSolved = movePlayer(&app->game, 1, 0);
    if (IsKeyPressed(KEY_LEFT))
	levelSolved = movePlayer(&app->game, -1, 0);
    if (IsKeyPressed(KEY_UP))
	levelSolved = movePlayer(&app->game, 0, -1);
    if (IsKeyPressed(KEY_DOWN))
	levelSolved = movePlayer(&app->game, 0, 1);
    if (IsKeyPressed(KEY_R))
	restartGame(&app->game);

    BeginDrawing();
    ClearBackground((Color){ 135, 206, 235, 255 });

    BeginMode3D(app->game.camera);
    drawLevel(&app->game);
    EndMode3D();

    drawGameInfo(app);
    drawFadeAnimation(app);
    EndDrawing();

    if (levelSolved) {
	advanceLevel(&app->game);
	app->fadeTime = 0;
    }
}

void updateApp(void* data) {
    App* app = (App*)data;

    if (WindowShouldClose()) {
	savePlayerData(&app->game);
	app->quit = true;
	return;
    }

    if (app->drawingMenu)
	drawMenu(app);
    else
	gameloop(app);
}

// TODO: loading levels is pretty slow...
//	 debug wall rendering
//	 why are there lines when drawing certain levels?
//	 polish (different hover animations, etc)
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

    App app = createApp();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(updateGameWrapper, &app, 60, 1);
#else
    SetTargetFPS(60);
    while (!app.quit) updateApp(&app);
#endif

    cleanupGame(&app.game);
    CloseWindow();
}
