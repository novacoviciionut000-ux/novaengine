#ifndef SCENES_H
#define SCENES_H
#include "entities.h"
#include <stdlib.h>
#include <stdio.h>
#include "camera.h"
#include <SDL3/SDL.h>
#include "enemy.h"
#include "alg.h"
#include "particles.h"
#include "rendering.h"
#include "defines.h"
scene_t *create_scene(void);
void destroy_scene(scene_t *scene);
void gather_collision_boxes(scene_t *scene);
bool add_object(scene_t *scene, void *object, int object_type);
void remove_entity_by_ptr(scene_t *scene, void *target, int entity_type);
void gather_meshes(scene_t *scene);
void _patch_scene_after_removal(scene_t *scene, size_t start_offset, size_t triangle_count);
void add_triangles(void *object, int object_type, scene_t *scene, size_t write_index);
#endif
