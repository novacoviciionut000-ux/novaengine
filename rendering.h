#ifndef RENDERING_H
#define RENDERING_H
#include "projection_math.h"
#include <SDL3/SDL.h>
#include "scenes.h"
#include "alg.h"
#include <math.h>
#include "camera.h"
#include <stdbool.h>
#include "defines.h"
#include <stdlib.h>
bool render_scene(scene_t *scene, SDL_Renderer *renderer, bool wireframe, camera_t *cam);
void fill_verts(scene_t *scene, size_t visible);
void render_triangle_wireframe(SDL_Renderer* renderer, SDL_Vertex v1, SDL_Vertex v2);
void draw_stars(SDL_Renderer *renderer, const camera_t *cam, scene_t *scene);
void init_starfield(scene_t *scene);
SDL_Vertex vec4tovert(vec4_t *vec, SDL_FColor color);
bool is_triangle_outside(vec4_t *v1, vec4_t *v2, vec4_t *v3, float mx, float my);
size_t sync_scene(scene_t *scene);
real backfacecull(vec4_t *v1, vec4_t *v2, vec4_t *v3);
int cmp(const void* ptrA, const void* ptrb);
#endif