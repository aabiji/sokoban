#include "game.h"
#include "levels.h"
#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

typedef struct {
    Game game;
    Animation fade;
    bool quit;
    Vector2 windowSize;
    enum { HELP, LEVEL_SELECT, GAME } screen;
} App;

App createApp(Vector2 windowSize) {
    App app;
    app.windowSize = windowSize;
    app.game = createGame();
    app.fade = createAnimation((Vector2){0, 0}, true, TRANSISTION_SPEED);
    app.quit = false;
    app.screen = LEVEL_SELECT;
    return app;
}

void cleanupApp(App* app) {
    cleanupGame(&app->game);
    UnloadShader(app->game.assetManager.shader);
}

// Draw text centered at a point and return its bounding box
Rectangle centerText(Font font, const char* text, Vector2 point, int fontSize,
                     int spacing, Color color) {
    Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 p = {point.x - size.x / 2, point.y - size.y / 2};
    DrawTextEx(font, text, p, fontSize, spacing, color);
    return (Rectangle){p.x, p.y, size.x, size.y};
}

const bool mouseInside(Rectangle r) {
    Vector2 p = GetMousePosition();
    return p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y &&
           p.y <= r.y + r.height;
}

// Draw a fullscreen overlay that gradually fades out over time
void drawFadeAnimation(App* app) {
    if (!app->fade.active) return;

    updateAnimation(&app->fade, GetFrameTime());
    float alpha = 255.0 - (255.0 * app->fade.scalar.value);

    DrawRectangle(0, 0, app->windowSize.x, app->windowSize.y,
                  (Color){0, 0, 0, alpha});
}

Color brightenColor(Color c, float amount) {
    Color new;
    new.r = (c.r + (255 - c.r) * amount);
    new.g = (c.g + (255 - c.g) * amount);
    new.b = (c.b + (255 - c.b) * amount);
    new.a = 255;
    return new;
}

void drawLevelSelect(App* app) {
    centerText(app->game.assetManager.font, "Sokoban",
               (Vector2){ app->windowSize.x / 2, 50 }, 45, 0, WHITE);

    //printf("%f\n", app->windowSize.x / 2);

    float cols = 10;
    int rows = NUM_LEVELS / cols;
    float amount =
        cols + ((cols + 1) / 2); // The columns and the space in between them
    float w = app->windowSize.x > 1000 ? 900 : app->windowSize.x;
    float boxSize = w / amount;

    float startX = ((app->windowSize.x - boxSize * amount) / 2) + (boxSize / 2);
    Vector2 pos = {startX, 100};
    bool hovering = false;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int level = row * 10 + col;
            Rectangle r = {pos.x, pos.y, boxSize, boxSize};
            Color lightGreen = (Color){71, 194, 120, 255};
            Color darkGreen = (Color){32, 137, 107, 255};
            Color c =
                app->game.player.solvedLevels[level] ? darkGreen : lightGreen;

            if (mouseInside(r)) {
                hovering = true;
                c = brightenColor(c, 0.2);
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    app->screen = GAME;
                    changeLevel(&app->game, level, false);
                    startAnimation(&app->fade, (Vector2){ 1, 1 }, true);
                    break;
                }
            }

            DrawRectangleRounded(r, 0.2, 0, c);

            const char* str = TextFormat("%d", level + 1);
            Vector2 p = {r.x + boxSize / 2, r.y + boxSize / 2};
            centerText(app->game.assetManager.font, str, p, 20, 0, WHITE);

            pos.x += boxSize * 1.5;
        }
        pos.y += boxSize * 1.5;
        pos.x = startX;
    }

    Vector2 bottom = { app->windowSize.x / 2, app->windowSize.y - 50 };
    centerText(app->game.assetManager.font, "(C) 2025- @aabiji", bottom, 15, 0, WHITE);
    drawFadeAnimation(app);

    if (hovering)
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void drawHelpScreen(App* app) {

}

void drawGameInfo(App* app) {
    Color c = (Color){210, 125, 45, 255};

    // Draw the sidebar of text
    Rectangle box =
        centerText(app->game.assetManager.font, "<", (Vector2){20, 25}, 50, 0, WHITE);
    if (mouseInside(box)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app->screen = LEVEL_SELECT;
            startAnimation(&app->fade, (Vector2){1, 1}, true);
        }
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    const char* str = TextFormat("Level %d", app->game.level + 1);
    DrawTextEx(app->game.assetManager.font, str, (Vector2){45, 12}, 30, 0, c);

    const char* str1 = TextFormat("%d moves", app->game.player.numMoves);
    DrawTextEx(app->game.assetManager.font, str1, (Vector2){45, 52}, 30, 0, c);

    const char* str2 = TextFormat("%d / %d", 10, 20);
    DrawTextEx(app->game.assetManager.font, str2, (Vector2){45, 92}, 30, 0, c);
}

void gameloop(App* app) {
    if (levelSolved(&app->game)) {
        changeLevel(&app->game, -1, true);
        startAnimation(&app->fade, (Vector2){1, 1}, true);
        return;
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_A)) movePlayer(&app->game, 1, 0);
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_D)) movePlayer(&app->game, -1, 0);
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) movePlayer(&app->game, 0, -1);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) movePlayer(&app->game, 0, 1);
    if (IsKeyPressed(KEY_R)) restartLevel(&app->game);

    BeginMode3D(app->game.camera);
    BeginShaderMode(app->game.assetManager.shader);
    drawGame(&app->game);
    EndShaderMode();
    EndMode3D();

    drawGameInfo(app);
    drawFadeAnimation(app);
}

void updateApp(void* data) {
    App* app = (App*)data;

    if (WindowShouldClose() ||
        IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK)) {
        savePlayerData(&app->game);
        app->quit = true;
        return;
    }

    BeginDrawing();
    ClearBackground((Color){ 163, 208, 229, 255 });

    if (app->screen == LEVEL_SELECT)
        drawLevelSelect(app);
    else if (app->screen == HELP)
        drawHelpScreen(app);
    else
        gameloop(app);

    EndDrawing();
}

/*
TODO:
- Find a royalty free, chill, fun and calming soundtrack
  Also find sound effects for clicking buttons, moving the player,
  pushing boxes and finishing a level
- Add nicer transitions. Instead of just a fade in, add text for what
  screen you're transitioning to and have a more developped background animation
  Improve the level selection screen
- Add a help screen
- Use Emscripten to port to web
- Release on hackernews (June 27)
Improve the buttons
Better background?
*/

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_FULLSCREEN_MODE | FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "Sokoban");
    InitAudioDevice();

    Vector2 windowSize = {
        GetMonitorWidth(GetCurrentMonitor()),
        GetMonitorHeight(GetCurrentMonitor())
    };
    App app = createApp(windowSize);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(updateGameWrapper, &app, 60, 1);
#else
    SetTargetFPS(60);
    while (!app.quit)
        updateApp(&app);
#endif

    cleanupApp(&app);
    CloseAudioDevice();
    CloseWindow();
}
