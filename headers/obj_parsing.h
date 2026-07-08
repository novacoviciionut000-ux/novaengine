#ifndef OBJPARSE
#define OBJPARSE
#include "alg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "collisions.h"
#include <stdlib.h>
#include "entities.h"
#include "defines.h"
#include <SDL3/SDL.h>
void scale_mesh(mesh_t *mesh, real scale);
mesh_t *parse_mesh(FILE *file, const char *pathname, SDL_FColor color, bool force_color);
void *get_obj(char *pathname, vec4_t position, SDL_FColor color, real scale,
              bool collidable, real xrot, real yrot, real zrot, bool force_color, int object_type);

#endif