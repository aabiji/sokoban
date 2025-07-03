#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <stddef.h>
#include <string.h>
#include "app.h"

App* app = NULL;

#if defined(PLATFORM_WEB)
EMSCRIPTEN_KEEPALIVE
void resize(int width, int height) {
    SetWindowSize(width, height);
    app->windowSize = (Vector2){ width, height };
}

EMSCRIPTEN_KEEPALIVE
void handleSwipe(const char* direction) {
    if (strcmp(direction, "left") == 0) move(app, -1, 0);
    if (strcmp(direction, "right") == 0) move(app, 1, 0);
    if (strcmp(direction, "up") == 0) move(app, 0, -1);
    if (strcmp(direction, "down") == 0) move(app, 0, 1);
}
#endif

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(900, 700, "Chickoban");
    InitAudioDevice();

    app = createApp();
    app->windowSize = (Vector2){ GetScreenWidth(), GetScreenHeight() };

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(updateApp, app, 0, 1);

    // resize to the canvas size
    int width = 0, height = 0;
    emscripten_get_canvas_element_size("canvas", &width, &height);
    app->windowSize = (Vector2){ width, height };
    SetWindowSize(width, height);
#else
    SetTargetFPS(60);
    while (!app->quit)
        updateApp(app);
#endif

    cleanupApp(app);
    CloseAudioDevice();
    CloseWindow();
}
