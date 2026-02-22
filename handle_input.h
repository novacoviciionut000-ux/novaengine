#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#include "defines.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "physics.h"

void apply_input_to_camera(const input_state_t *input, camera_t *cam);
void handle_event_and_delta(long deltaTime, bool *running,scene_t *scene, camera_t *cam, real dt);
input_state_t get_input(const uint8_t *keyboard_state);

#endif