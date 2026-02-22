#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#include "defines.h"
#include <SDL3/SDL.h>
#include "alg.h"
#include "defines.h"
#include "entities.h"
#include <math.h>
#include "timers.h"
#include <stdio.h>
#include <stdbool.h>

// Forward declaration pointing to the tagged struct

vec4_t get_velocity_from_input(const input_state_t *input, camera_t *cam);
void handle_event_and_delta(long deltaTime, bool *running,entity_t **entities, int entity_count, camera_t *cam, real dt);
input_state_t get_input(const uint8_t *keyboard_state);

#endif