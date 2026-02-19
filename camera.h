#ifndef CAMERA_H
#define CAMERA_H
#include "entities.h"
#include "alg.h"
#include "defines.h"
#include <SDL3/SDL.h>
#include <stdio.h>
// Notice the 'camera_t' tag added right after the word 'struct'
typedef struct camera_t {
    vec4_t pos;
    eulerangles_t angles;
    real speed;
    real angular_speed;
    uint8_t grounded; // on which axes has the camera hit an object ? bit 0 -> x axis, bit 1 -> y axis, bit 2 -> z axis
    //we can update this bitmask in the frustum-clipping logic in rendering, using the camera coordinates.
} camera_t;

void handle_camera_rotation(camera_t *cam, const SDL_Event *event);
void move_world_to_camera_space(const camera_t *cam, entity_t **entities, int entity_count);
void handle_camera_translation(camera_t *cam, const uint8_t *keyboard_state);
camera_t* create_camera(vec4_t pos, eulerangles_t angles, real speed, real angular_speed);
real get_distance_to_closest_vertex(entity_t **entities, int num_entities);// this will need to be changed into two functions, one that calculated for x axis, and z-axis respectively.
void update_grounded(camera_t *cam, entity_t **entities, int numentities);

#endif