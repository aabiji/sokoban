#include "game.h"
#include "levels.h"
#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

typedef struct {
    Game game;

    bool drawingMenu;
    bool quit;
    float fadeTime;

    Color bg;
} App;

App createApp() {
    App app;
    app.game = createGame();
    app.quit = false;
    app.drawingMenu = true;
    app.fadeTime = -1;
    return app;
}

// Draw text centered at a point and return its bounding box
Rectangle centerText(Font font, const char *text, Vector2 point, int fontSize,
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

float easeInOutQuad(float t) {
    return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
}

// Draw a fullscreen overlay that gradually fades out over time
void drawFadeAnimation(App *app) {
    // Setting fadeTime >= 0 triggers the animation
    if (app->fadeTime < 0)
        return;

    float alpha = 255.0 - (255.0 * easeInOutQuad(app->fadeTime));

    float duration = 0.5; // seconds
    app->fadeTime += GetFrameTime() / duration;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  (Color){0, 0, 0, alpha});

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

void drawMenu(App *app) {
    centerText(app->game.font, "Sokoban",
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
                    changeLevel(&app->game, level);
                    app->fadeTime = 0;
                    break;
                }
            }

            DrawRectangleRounded(r, 0.2, 0, c);

            const char *str = TextFormat("%d", level + 1);
            Vector2 p = {r.x + boxSize / 2, r.y + boxSize / 2};
            centerText(app->game.font, str, p, 20, 0, WHITE);

            pos.x += boxSize * 1.5;
        }
        pos.y += boxSize * 1.5;
        pos.x = startX;
    }

    Vector2 bottom = {(float)GetScreenWidth() / 2, GetScreenHeight() - 50};
    centerText(app->game.font, "(C) 2025- @aabiji", bottom, 15, 0, WHITE);
    drawFadeAnimation(app);

    if (hovering)
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void drawGameInfo(App *app) {
    Color c = (Color){210, 125, 45, 255};

    // Draw the sidebar of text
    Rectangle box =
        centerText(app->game.font, "<", (Vector2){20, 25}, 50, 0, WHITE);
    if (mouseInside(box)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            app->drawingMenu = true;
            app->fadeTime = 0;
        }
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    const char *str = TextFormat("Level %d", app->game.level + 1);
    DrawTextEx(app->game.font, str, (Vector2){45, 12}, 30, 0, c);

    const char *str1 = TextFormat("%d moves", app->game.player.numMoves);
    DrawTextEx(app->game.font, str1, (Vector2){45, 52}, 30, 0, c);

    const char *str2 = TextFormat("%d / %d", 10, 20);
    DrawTextEx(app->game.font, str2, (Vector2){45, 92}, 30, 0, c);
}

void gameloop(App *app) {
    bool levelSolved = false;

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_A))
        levelSolved = movePlayer(&app->game, 1, 0);
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_D))
        levelSolved = movePlayer(&app->game, -1, 0);
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        levelSolved = movePlayer(&app->game, 0, -1);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        levelSolved = movePlayer(&app->game, 0, 1);
    if (IsKeyPressed(KEY_R))
        restartGame(&app->game);

    BeginMode3D(app->game.camera);
    drawLevel(&app->game);
    drawPlayer(&app->game);
    EndMode3D();

    drawGameInfo(app);
    drawFadeAnimation(app);

    if (levelSolved) {
        advanceLevel(&app->game);
        app->fadeTime = 0;
    }
}

void updateApp(void *data) {
    App *app = (App *)data;

    if (WindowShouldClose()) {
        savePlayerData(&app->game);
        app->quit = true;
        return;
    }

    BeginDrawing();
    ClearBackground((Color){135, 206, 235, 234});

    if (app->drawingMenu)
        drawMenu(app);
    else
        gameloop(app);

    EndDrawing();
}

// TODO: fix the player rotation animation (looks choppy when turning sometimes)
//	 apply animations to the boxes
//	 refactor out the animation logic and player logic from game.c
//	 improve wall rendering
//	 why are there lines when drawing certain levels?
//	 polish (different hover animations, etc)
//	 sound effects and a chill soundtrack
// 	 add basic lighting
// 	 nicer level selection screen
// 	 port to web and mobile
// 	 release

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "Sokoban");

    App app = createApp();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(updateGameWrapper, &app, 60, 1);
#else
    SetTargetFPS(60);
    while (!app.quit)
        updateApp(&app);
#endif

    cleanupGame(&app.game);
    CloseWindow();
}
