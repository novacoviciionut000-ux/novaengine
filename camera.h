#ifndef CAMERA_H
#define CAMERA_H
#include "entities.h"
#include "alg.h"
#include "defines.h"
#include <SDL3/SDL.h>

// Notice the 'camera_t' tag added right after the word 'struct'
typedef struct camera_t {
    vec4_t pos;
    eulerangles_t angles;
    real speed;
    real angular_speed;
} camera_t;

void handle_camera_rotation(camera_t *cam, const SDL_Event *event);
void move_world_to_camera_space(const camera_t *cam, entity_t **entities, int entity_count);
void handle_camera_translation(camera_t *cam, const uint8_t *keyboard_state);
camera_t* create_camera(vec4_t pos, eulerangles_t angles, real speed, real angular_speed);

#endif