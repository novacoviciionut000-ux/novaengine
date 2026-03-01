#ifndef PARTICLES_H
#define PARTICLES_H
#include "defines.h"
#include "scenes.h"
#include "obj_parsing.h"
#include "collisions.h"
#include "alg.h"
#include <stdlib.h>
#include <string.h>
void create_sparks(vec4_t position, scene_t *scene);
particle_t* emit_particle(scene_t *scene, int type, vec4_t position, vec4_t base_velocity);
void create_muzzle_sparks(vec4_t position, scene_t *scene);
void update_bullets(scene_t *scene);
void update_particles(scene_t *scene, real dt, camera_t *cam);
real get_particle_distance_from_camera(particle_t *part, camera_t *cam);
void release_bullet(scene_t *scene);
bool get_particles(particle_manager_t *manager);
void free_particle_deep(particle_t *part);
bool create_particles_list(particle_manager_t *manager);
particle_t *get_spark_particle(char *pathname);
float random_float(float min, float max);
particle_t *get_blood_particle(char *pathname);
void destroy_particle_manager(particle_manager_t *manager);
void create_blood(vec4_t position, scene_t *scene);
#endif
