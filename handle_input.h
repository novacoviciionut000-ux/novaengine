#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#include <SDL3/SDL.h>
#include "alg.h"
#include "defines.h"
#include "entities.h"
#include <math.h>
#include <stdbool.h>

// Forward declaration pointing to the tagged struct
typedef struct camera_t camera_t;

typedef struct {
    bool up, down, left, right, forward, backward;
} input_state_t;

vec4_t get_velocity_from_input(const input_state_t *input, camera_t *cam);
void handle_event_and_delta(long deltaTime, long *lastTime, bool *running, entity_t **entities, int entity_count, camera_t *cam);
input_state_t get_input(const uint8_t *keyboard_state);

#endif