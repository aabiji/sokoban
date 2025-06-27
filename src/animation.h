#ifndef ANIMATION_H
#define ANIMATION_H

#include <raymath.h>
#include <stdbool.h>

// all in seconds
#define PLAYER_SPEED 0.1
#define TRANSISTION_SPEED 0.25

typedef struct {
    float t;
    bool isScalar;
    bool active;
    float duration;
    struct { Vector2 start, end, value; } vector;
    struct { float start, end, value; } scalar;
} Animation;

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static Animation createAnimation(Vector2 value, bool isScalar, float duration) {
    return (Animation){
        .t = 0, .isScalar = isScalar, .active = false, .duration = duration,
        .vector = { .start = value, .end = value, .value = value },
        .scalar = { .start = value.x, .end = value.x, .value = value.x },
    };
}

static void startAnimation(Animation* a, Vector2 value, bool reset) {
    a->t = 0;
    a->active = true;
    if (a->isScalar) {
        a->scalar.start = reset ? 0 : a->scalar.end;
        a->scalar.end = value.x;
    } else {
        a->vector.start = reset ? (Vector2){ 0, 0 } : a->vector.end;
        a->vector.end = value;
    }
}

static void updateAnimation(Animation* a, float deltaTime) {
    a->active = a->t < 1.0;
    if (a->active) {
        a->t = fmin(fmax(a->t + deltaTime / a->duration, 0), 1.0);
        float smoothed = a->t * a->t * (3 - 2 * a->t); // smoothstep function

        if (a->isScalar) {
            a->scalar.value = lerp(a->scalar.start, a->scalar.end, smoothed);
        } else {
            a->vector.value = (Vector2){
                lerp(a->vector.start.x, a->vector.end.x, smoothed),
                lerp(a->vector.start.y, a->vector.end.y, smoothed),
            };
        }
    }
}

#endif
