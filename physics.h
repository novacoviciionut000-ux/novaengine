#ifndef PHYS_H
#define PHYS_H
#include "defines.h"
#include "alg.h"
void apply_impulse(entity_t *entity, vec4_t impulse) ;
void apply_force(entity_t *entity, vec4_t force);
void apply_camera_gravity(camera_t *cam, real gravity_pull, real dt);
void apply_impulse_to_camera(camera_t *cam, vec4_t impulse);
void camera_jump(camera_t *cam, real jump_power);
void apply_camera_gravity(camera_t *cam, real gravity_pull, real dt);
void update_entity_physics(entity_t *entity, float dt);
void update_scene_physics(scene_t *scene, float dt,camera_t *cam);
#endif