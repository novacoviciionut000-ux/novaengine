#ifndef ENEMY_H
#define ENEMY_H
#include "defines.h"
#include "scenes.h"
#include "obj_parsing.h"
#include "particles.h"
#include "alg.h"
void update_enemy(enemy_t *enemy, real dt, camera_t *camera, scene_t *scene);
void initialize_enemies(enemy_manager_t *manager);
void behaviour_drone(enemy_t *enemy, real dt, camera_t *camera, scene_t *scene);
void remove_enemy(enemy_t *enemy, scene_t *scene);
void add_enemy(int type_enemy, scene_t *scene, vec4_t position);
void transform_enemy_verts(enemy_t *enemy);
void update_enemies(scene_t *scene, real dt, camera_t *camera);
#endif
