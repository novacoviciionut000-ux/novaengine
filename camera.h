#ifndef CAMERA_H
#define CAMERA_H
#include "entities.h"
#include "alg.h"
#include "defines.h"
#include "timers.h"
#include <SDL3/SDL.h>
#include <stdio.h>

void handle_camera_rotation(camera_t *cam, const SDL_Event *event);
void move_world_to_camera_space(camera_manager_t *manager);
void handle_camera_translation(camera_t *cam, const uint8_t *keyboard_state, real dt);
camera_t* create_camera(vec4_t pos, eulerangles_t angles, real speed, real angular_speed);
void draw_damage_vignette(SDL_Renderer *renderer, camera_t *camera, real max_health);
void update_grounded(camera_t *cam, entity_t **entities, int numentities);
vec4_t get_camera_velocity(camera_t *cam,const uint8_t *keyboard_state, real dt);

#endif
