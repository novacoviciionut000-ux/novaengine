#ifndef RENDERING_H
#define RENDERING_H
#include "projection_math.h"
#include <SDL3/SDL.h>
#include "entities.h"
#include "alg.h"
#include "camera.h"
#include <stdlib.h>
bool fill_entity(entity_t *entity, SDL_Renderer *renderer,camera_t *cam);
bool render_entities(entity_t **entity, SDL_Renderer *renderer, int entity_count,camera_t *cam);
bool render_entity(entity_t *entity, SDL_Renderer *renderer,camera_t *cam);
vec4_t get_forward(camera_t *cam);
typedef struct{
    int idx0, idx1, idx2;
    real avg_depth;

}triangle_t;

#endif