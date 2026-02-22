#ifndef ENTITIES_H
#define ENTITIES_H
#include <stdbool.h>
#include "defines.h"
#include <SDL3/SDL.h>
#include "collisions.h"
#include "alg.h"
#include <stdlib.h>
#include "timers.h"

vec4_t* create_cube_local_vertices(real length, real width, real height);
mesh_t* create_mesh(vec4_t *local_verts, int num_verts, int triangle_count, int* triangle_map);
entity_t* create_entity(vec4_t pos, mesh_t *mesh);
void rotate_entity(entity_t *entity);
void add_angular_velocity(eulerangles_t *angles, vec4_t angular_velocity, real dt);
void free_entity(entity_t *entity);
SDL_Vertex* vec4tovertex(entity_t *entity);
bool isZero_vec(vec4_t vecA);
int* create_cube_triangles();
void update_entity(entity_t *entity, real dt);
void update_entities(entity_t **entities, int entity_count, real dt);
entity_t* create_cube_entity(vec4_t pos, real length, real width, real height);
#endif