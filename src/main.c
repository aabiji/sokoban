#include <stdlib.h>
#include "assets.h"
#include "game.h"
#include "levels.h"
#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

typedef enum { HELP, LEVEL_SELECT, GAME } Screens;

typedef struct {
    Game* game;
    Animation fade;
    Vector2 windowSize;
    bool quit;
    Screens currentScreen;
    Screens prevScreen;
} App;

App* createApp() {
    App* app = calloc(1, sizeof(App));
    app->quit = false;

    app->game = createGame();
    app->fade = createAnimation((Vector2){0, 0}, true, TRANSISTION_SPEED);

    app->currentScreen = LEVEL_SELECT;
    app->prevScreen = LEVEL_SELECT;
    return app;
}

void cleanupApp(App* app) {
    cleanupGame(app->game);
    UnloadShader(app->game->assetManager.shader);
    free(app);
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
    drawText(&app->game->assetManager, "Sokoban",
            (Vector2){ app->windowSize.x / 2, 50 }, 45, WHITE);

    float cols = 10;
    int rows = NUM_LEVELS / cols;
    float amount =
        cols + ((cols + 1) / 2); // The columns and the space in between them
    float w = app->windowSize.x > 1000 ? 1000 : app->windowSize.x;
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
            bool solved = app->game->assetManager.data.solvedLevels[level];
            Color c = solved ? darkGreen : lightGreen;

            if (mouseInside(r)) {
                hovering = true;
                c = brightenColor(c, 0.2);
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    app->currentScreen = GAME;
                    app->prevScreen = GAME;
                    changeLevel(app->game, level, false);
                    startAnimation(&app->fade, (Vector2){ 1, 1 }, true);
                    break;
                }
            }

            DrawRectangleRounded(r, 0.2, 0, c);

            const char* str = TextFormat("%d", level + 1);
            Vector2 p = {r.x + boxSize / 2, r.y + boxSize / 2};
            drawText(&app->game->assetManager, str, p, 40, WHITE);

            pos.x += boxSize * 1.5;
        }
        pos.y += boxSize * 1.5;
        pos.x = startX;
    }

    Vector2 bottom = { app->windowSize.x / 2, app->windowSize.y - 50 };
    drawText(&app->game->assetManager, "(C) 2025- @aabiji", bottom, 35, WHITE);
    drawFadeAnimation(app);

    if (hovering)
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void drawHelpScreen(App* app) {
    // go to the previous screen
    Rectangle box =
        drawText(&app->game->assetManager, "<", (Vector2){20, 25}, 50, WHITE);
    if (mouseInside(box)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app->currentScreen = app->prevScreen;
            startAnimation(&app->fade, (Vector2){1, 1}, true);
        }
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    const char* info[6] = {
        "This is a puzzle game where your goal",
        "is to push boxes into the target positions",
        "Press Esc to exit the game",
        "Press f to toggle fullscreen",
        "Press m to toggle the background music",
        "Use the arrow keys or W, A, S, D to move the player"
    };
    for (int i = 0; i < 6; i++) {
        drawText(
            &app->game->assetManager, info[i],
            (Vector2){ app->windowSize.x / 2, 100 + i * 65 }, 50, WHITE);
    }

    // TODO:
    // find a better sound effect for moving and pushing boxes
    // move all the game info to the left hand side
    // make the go back and help button more prominent
    // draw text in a better color
    // change background when hovering buttons
    // improve the lighting shaders
    // loading solved levels doesn't seem to work anymore -- fix that
    // improve the backround?
    // port to web
    // release
}

void drawGameInfo(App* app) {
    Color c = (Color){210, 125, 45, 255};

    // go to the level select screen
    Rectangle box =
        drawText(&app->game->assetManager, "<", (Vector2){20, 25}, 50, WHITE);
    if (mouseInside(box)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app->currentScreen = LEVEL_SELECT;
            app->prevScreen = GAME;
            startAnimation(&app->fade, (Vector2){1, 1}, true);
        }
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    // go to the help screen
    box = drawText(&app->game->assetManager, "?", (Vector2){20, 75}, 50, WHITE);
    if (mouseInside(box)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app->currentScreen = HELP;
            app->prevScreen = GAME;
            startAnimation(&app->fade, (Vector2){1, 1}, true);
        }
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    const char* str = TextFormat("%d moves", app->game->player.numMoves);
    drawText(&app->game->assetManager, str, (Vector2){ app->windowSize.x / 4, 20 }, 50, c);

    const char* str1 = TextFormat("Level %d", app->game->level + 1);
    drawText(&app->game->assetManager, str1, (Vector2){ app->windowSize.x / 2, 20 }, 50, c);

    Level* level = &app->game->levels[app->game->level];
    const char* str2 = TextFormat(
        "%d / %d boxes", countCompletedGoals(level), level->numGoals);
    drawText(&app->game->assetManager, str2, (Vector2){ app->windowSize.x / 1.25, 20 }, 50, c);
}

void gameloop(App* app) {
    Level* level = &app->game->levels[app->game->level];

    if (countCompletedGoals(level) == level->numGoals) { // solved the level
        changeLevel(app->game, -1, true);
        startAnimation(&app->fade, (Vector2){1, 1}, true);
        return;
    }

    BeginMode3D(app->game->camera);
    BeginShaderMode(app->game->assetManager.shader);
    drawGame(app->game);
    EndShaderMode();
    EndMode3D();

    drawGameInfo(app);
    drawFadeAnimation(app);
}

void handleInput(App* app) {
    if (WindowShouldClose() ||
        IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK)) {
        persistData(&app->game->assetManager);
        app->quit = true;
        return;
    }

    if (IsKeyPressed(KEY_F))
        app->game->assetManager.data.fullscreen = !app->game->assetManager.data.fullscreen;

    if (IsKeyPressed(KEY_M))
        app->game->assetManager.data.playBgMusic = !app->game->assetManager.data.playBgMusic;

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_A))
        movePlayer(app->game, 1, 0);

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_D))
        movePlayer(app->game, -1, 0);

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        movePlayer(app->game, 0, -1);

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        movePlayer(app->game, 0, 1);

    if (IsKeyPressed(KEY_R)) {
        restartLevel(&app->game->levels[app->game->level]);
        changeLevel(app->game, app->game->level, false);
    }

}

void updateApp(void* data) {
    App* app = (App*)data;

    handleInput(app);
    updateSound(
        &app->game->assetManager, BackgroundMusic,
        app->game->assetManager.data.playBgMusic);

    BeginDrawing();
    ClearBackground((Color){ 163, 208, 229, 255 });

    if (app->currentScreen == LEVEL_SELECT)
        drawLevelSelect(app);
    else if (app->currentScreen == HELP)
        drawHelpScreen(app);
    else
        gameloop(app);

    EndDrawing();
}

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(900, 700, "Sokoban");
    InitAudioDevice();

    App* app = createApp();
    // TODO: set the window size based on whether we should be fullscreen or not

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(updateApp, app, 60, 1);
#else
    SetTargetFPS(60);
    while (!app->quit)
        updateApp(app);
#endif

    cleanupApp(app);
    CloseAudioDevice();
    CloseWindow();
}
