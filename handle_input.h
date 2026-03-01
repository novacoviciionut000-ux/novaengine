#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#include "defines.h"
#include <SDL3/SDL.h>
#include <stdbool.h>
#include "gun.h"
#include "camera.h"
#include "particles.h"
#include "pipeline.h"
#include "physics.h"
void apply_input_to_camera(const input_state_t *input, camera_t *cam);
void handle_event_and_delta(bool *running,scene_t *scene, real dt,entity_t *gun, real time, gun_state_t *gun_state);
input_state_t get_input(const uint8_t *keyboard_state);

#endif
