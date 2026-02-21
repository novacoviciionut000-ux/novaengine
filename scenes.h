#ifndef SCENES_H
#define SCENES_H
#include "entities.h"
#include <stdlib.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include "alg.h"
typedef struct{
    //WORLD LOGIC
    entity_t** entities;
    size_t numentities;
    size_t allocated_mem;

    //RENDERING LOGIC(This is better than my previous approach. The renderer DOES NOT need to know what an entity is. I am going to pass into my renderer only numtriangles and triangles.)
    size_t numtriangles;
    triangle_t *triangles;
    triangle_t *render_usage;//same, triangle array so that i do not allocate memory each frame at runtime.
    vec4_t *transient_buffer; //This is for clipping triangles( we need something for the new triangles to point to.)
    size_t transient_count;
    uint32_t *indices;// To avoid moving massive chunks of data during radix sort.
    uint32_t *temp_indices;
    SDL_Vertex* verts;//this is not used anywhere else but the rendering
    size_t allocated_triangles;
}scene_t;
void remove_entity_by_ptr(scene_t *scene, entity_t *target);
void _patch_scene_after_removal(scene_t *scene, size_t start_offset, size_t triangle_count);
bool add_entity(scene_t *scene, entity_t *entity);
void add_triangles(entity_t *entity, scene_t *scene, size_t write_index);
void destroy_scene(scene_t *scene);
scene_t *create_scene();
#endif