#include <stdlib.h>

#include "app.h"
#include "assets.h"
#include "game.h"

App* createApp() {
    App* app = calloc(1, sizeof(App));
    app->quit = false;
    app->drawingMenu = true; 

    app->game = createGame();
    app->fade = createAnimation((Vector2){0, 0}, true, TRANSISTION_SPEED);
    return app;
}

void cleanupApp(App* app) {
    cleanupGame(app->game);
    free(app);
}

const bool mouseInside(Rectangle r) {
    Vector2 p = GetMousePosition();
    return p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y &&
           p.y <= r.y + r.height;
}

// toggle the window fullscreen, note that we can only toggle fullscreen on desktop
void updateWindowSize(App* app) {
#if defined(PLATFORM_DESKTOP)
    bool alreadyFullscreen = IsWindowFullscreen();
    bool fullscreen = app->game->assets->data.fullscreen;

    if (!alreadyFullscreen && fullscreen) {
        app->windowSize = (Vector2){
            GetMonitorWidth(GetCurrentMonitor()),
            GetMonitorHeight(GetCurrentMonitor()),
        };
        ToggleFullscreen();
    } else if (alreadyFullscreen && !fullscreen) {
        ToggleFullscreen();
        app->windowSize = (Vector2){ GetScreenWidth(), GetScreenHeight() };
    }
#endif
}

// Draw a fullscreen overlay that gradually fades out over time
void drawFadeAnimation(App* app) {
    if (!app->fade.active) return;

    updateAnimation(&app->fade, GetFrameTime());
    float alpha = 255.0 - (255.0 * app->fade.scalar.value);

    DrawRectangle(0, 0, app->windowSize.x, app->windowSize.y,
                  (Color){ 160, 210, 235, alpha });
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
    Color pop = { 242, 92, 84, 255 };
    drawText(app->game->assets, "Chickoban",
            (Vector2){ app->windowSize.x / 2, app->windowSize.y / 3.5 },
            65, pop, true);

    float cols = 10;
    int rows = NUM_LEVELS / cols;
    float amount =
        cols + ((cols + 1) / 2); // The columns and the space in between them
    float w = app->windowSize.x > 1000 ? 1000 : app->windowSize.x;
    float boxSize = w / amount;

    float startX = ((app->windowSize.x - boxSize * amount) / 2) + (boxSize / 2);
    Vector2 pos = { startX, app->windowSize.y / 2 - boxSize * 2.5 };
    bool hovering = false;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int level = row * 10 + col;
            Rectangle r = {pos.x, pos.y, boxSize, boxSize};
            Color c = alreadySolved(app->game->assets, level)
                ? (Color){ 98, 156, 111, 255 }
                : (Color){ 58, 180, 172, 255 };

            if (mouseInside(r)) {
                hovering = true;
                c = brightenColor(c, 0.2);
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    app->drawingMenu = false;
                    changeLevel(app->game, level, false);
                    startAnimation(&app->fade, (Vector2){ 1, 1 }, true);
                    break;
                }
            }

            DrawRectangleRounded(r, 0.2, 0, c);

            const char* str = TextFormat("%d", level + 1);
            Vector2 p = {r.x + boxSize / 2, r.y + boxSize / 2};
            drawText(app->game->assets, str, p, 25, WHITE, true);

            pos.x += boxSize * 1.5;
        }
        pos.y += boxSize * 1.5;
        pos.x = startX;
    }

    Vector2 bottom = { app->windowSize.x / 2, app->windowSize.y - 50 };
    drawText(app->game->assets, "(C) 2025- @aabiji", bottom, 20, pop, true);
    drawFadeAnimation(app);

    if (hovering)
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void drawGameInfo(App* app) {
    Color c = (Color){ 77, 109, 129, 255 };

    // go back button
    Rectangle box =
        drawText(app->game->assets, "<< Back", (Vector2){15, 15}, 30, c, false);
    if (mouseInside(box)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app->drawingMenu = true;
            startAnimation(&app->fade, (Vector2){1, 1}, true);
        }
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    Level* level = &app->game->levels[app->game->level];
    const char* info[2] = {
        TextFormat("Level %d", app->game->level + 1),
        TextFormat("%d / %d boxes", countCompletedGoals(level), level->numGoals),
    };
    for (int i = 0; i < 2; i++) {
        Vector2 p = { 10, (app->windowSize.y / 2 - 40) + i * 30 };
        drawText(app->game->assets, info[i], p, 25, c, false);
    }

    const char* instructions[5] = {
        "Press m to toggle the background music",
        "Use arrow keys to move the player",
        "Press f to toggle fullscreen",
        "Press Esc to quit the game",
        "Press r to toggle restart"
    };
    for (int i = 0; i < 5; i++) {
        Vector2 p = { 10, app->windowSize.y - (i + 1) * 30 };
        drawText(app->game->assets, instructions[i], p, 20, c, false);
    }
}

void gameloop(App* app) {
    if (levelSolved(app->game) &&
        !alreadySolved(app->game->assets, app->game->level)) {
        markSolved(app->game->assets, app->game->level);
        changeLevel(app->game, -1, true);
        startAnimation(&app->fade, (Vector2){1, 1}, true);
        return;
    }

    BeginMode3D(app->game->camera);
    BeginShaderMode(app->game->assets->shader);
    drawGame(app->game);
    EndShaderMode();
    EndMode3D();

    drawGameInfo(app);
    drawFadeAnimation(app);
}

void handleInput(App* app) {
    if (WindowShouldClose() ||
        IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK)) {
        persistData(app->game->assets);
        app->quit = true;
        return;
    }

    if (IsKeyPressed(KEY_F)) togglefullscreen(app->game->assets);
    if (IsKeyPressed(KEY_M)) togglePlayBgMusic(app->game->assets);
    if (IsKeyPressed(KEY_RIGHT)) movePlayer(app->game, 1, 0);
    if (IsKeyPressed(KEY_LEFT)) movePlayer(app->game, -1, 0);
    if (IsKeyPressed(KEY_UP)) movePlayer(app->game, 0, -1);
    if (IsKeyPressed(KEY_DOWN)) movePlayer(app->game, 0, 1);

    if (IsKeyPressed(KEY_R)) {
        restartLevel(&app->game->levels[app->game->level]);
        changeLevel(app->game, app->game->level, false);
    }
}

void updateApp(void* data) {
    App* app = (App*)data;

    handleInput(app);
    updateWindowSize(app);

    updateSound(
        app->game->assets, BackgroundMusic,
        app->game->assets->data.playBgMusic);

    BeginDrawing();
    ClearBackground((Color){ 160, 210, 235, 255 });

    if (app->drawingMenu)
        drawLevelSelect(app);
    else
        gameloop(app);

    EndDrawing();
}
