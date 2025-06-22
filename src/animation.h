#ifndef ANIMATION_H
#define ANIMATION_H

#include <math.h>

static float smoothstep(float t) { return t * t * (3 - 2 * t); }

static float easeInOutQuad(float t) { return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t; }

static float lerp(float start, float end, float t) { return start + (end - start) * t; }

typedef struct {
    float t;
    union {
        struct {
            Vector2 start, end;
        } vector;
        struct {
            float start, end;
        } scalar;
    };
} Animation;

static Vector2 interpolateVector(Animation a) {
    float t = smoothstep(a.t);
    return (Vector2){
        lerp(a.vector.start.x, a.vector.end.x, t),
        lerp(a.vector.start.y, a.vector.end.y, t),
    };
}

static float interpolateScalar(Animation a) {
    return lerp(a.scalar.start, a.scalar.end, smoothstep(a.t));
}

static void updateAnimation(Animation* a, float deltaTime, float duration) {
    if (a->t >= 0 && a->t < 1.0)
        a->t += deltaTime / duration;
}

static void startRotationAnimation(Animation *a, float nextAngle) {
    a->t = 0;
    a->scalar.start = a->scalar.end;
    a->scalar.end = nextAngle;
}

static void startMovementAnimation(Animation *a, Vector2 next) {
    a->t = 0;
    a->vector.start = a->vector.end;
    a->vector.end = next;
}

#endif