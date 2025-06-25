#include "game.h"
#include "levels.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

typedef struct {
    Game game;
    Animation fade;
    bool drawingMenu;
    bool quit;
} App;

App createApp() {
    App app;
    app.game = createGame();
    app.fade = createAnimation((Vector2){0, 0}, true, TRANSISTION_SPEED);
    app.quit = false;
    app.drawingMenu = true;
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

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
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

void drawMenu(App* app) {
    centerText(app->game.assetManager.font, "Sokoban",
               (Vector2){(float)GetScreenWidth() / 2, 50}, 45, 0, WHITE);

    float cols = 10;
    int rows = NUM_LEVELS / cols;
    float amount =
        cols + ((cols + 1) / 2); // The columns and the space in between them
    float w = GetScreenWidth() > 1000 ? 900 : GetScreenWidth();
    float boxSize = w / amount;

    float startX = ((GetScreenWidth() - boxSize * amount) / 2) + (boxSize / 2);
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
                    app->drawingMenu = false;
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

    Vector2 bottom = {(float)GetScreenWidth() / 2, GetScreenHeight() - 50};
    centerText(app->game.assetManager.font, "(C) 2025- @aabiji", bottom, 15, 0, WHITE);
    drawFadeAnimation(app);

    if (hovering)
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void drawGameInfo(App* app) {
    Color c = (Color){210, 125, 45, 255};

    // Draw the sidebar of text
    Rectangle box =
        centerText(app->game.assetManager.font, "<", (Vector2){20, 25}, 50, 0, WHITE);
    if (mouseInside(box)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app->drawingMenu = true;
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

    if (WindowShouldClose()) {
        savePlayerData(&app->game);
        app->quit = true;
        return;
    }

    BeginDrawing();
    ClearBackground((Color){ 118, 199, 249, 255 });

    if (app->drawingMenu)
        drawMenu(app);
    else
        gameloop(app);

    EndDrawing();
}

/*
TODO:
- Zoom the camera in when the level is large
- Find a royalty free, chill, fun and calming soundtrack
  Also find sound effects for clicking buttons, moving the player,
  pushing boxes and finishing a level
- Add nicer transitions. Instead of just a fade in, add text for what
  screen you're transitioning to and have a more developped background animation
- Add more puzzles
  Improve the level selection screen
- Add a help screen
- Use Emscripten to port to web
- Release on hackernews (June 27)
*/

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED | FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "Sokoban");

    App app = createApp();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(updateGameWrapper, &app, 60, 1);
#else
    SetTargetFPS(60);
    while (!app.quit)
        updateApp(&app);
#endif

    cleanupApp(&app);
    CloseWindow();
}
