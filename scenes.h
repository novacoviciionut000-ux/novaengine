#ifndef SCENES_H
#define SCENES_H
#include "entities.h"
#include <stdlib.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include "alg.h"
#include "defines.h"
void remove_entity_by_ptr(scene_t *scene, entity_t *target);
void _patch_scene_after_removal(scene_t *scene, size_t start_offset, size_t triangle_count);
bool add_entity(scene_t *scene, entity_t *entity);
void add_triangles(entity_t *entity, scene_t *scene, size_t write_index);
void destroy_scene(scene_t *scene);
scene_t *create_scene();
#endif