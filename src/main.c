#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "app.h"

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(900, 700, "Sokoban");
    InitAudioDevice();

    App* app = createApp();
    app->windowSize = (Vector2){ GetScreenWidth(), GetScreenHeight() };

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(updateApp, app, 0, 1);
#else
    SetTargetFPS(60);
    while (!app->quit)
        updateApp(app);
#endif

    cleanupApp(app);
    CloseAudioDevice();
    CloseWindow();
}
