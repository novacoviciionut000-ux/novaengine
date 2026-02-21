#ifndef RENDERING_H
#define RENDERING_H
#include "projection_math.h"
#include <SDL3/SDL.h>
#include "entities.h"
#include "scenes.h"
#include "alg.h"
#include "camera.h"
#include <stdbool.h>
#include "defines.h"
#include <stdlib.h>
bool render_scene(scene_t *scene, SDL_Renderer *renderer);
void fill_verts(scene_t *scene, size_t visible);
SDL_Vertex vec4tovert(vec4_t *vec, SDL_FColor color);
size_t sync_scene(scene_t *scene);
real backfacecull(vec4_t *v1, vec4_t *v2, vec4_t *v3);
int cmp(const void* ptrA, const void* ptrb);
#endif