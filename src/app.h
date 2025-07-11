#ifndef APP_H
#define APP_H

#include "game.h"

typedef struct {
    Game* game;
    Animation fade;
    Vector2 windowSize;
    bool quit;
    bool drawingMenu;
} App;

App* createApp();
void cleanupApp(App* app);
void updateApp(void* data);
void move(App* app, int directionX, int directionY);
void handleMouseMove(App* app);

#endif
