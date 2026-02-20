#ifndef OBJPARSE
#define OBJPARSE
#include "alg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "entities.h"
#include "defines.h"
#include <SDL3/SDL.h>
entity_t *get_obj(char *pathname, vec4_t position, SDL_FColor color);
#endif