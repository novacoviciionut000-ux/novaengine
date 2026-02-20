#ifndef GLOOP_H
#define GLOOP_H
#include "entities.h"
#include <stdio.h>
#include "projection_math.h"
#include "camera.h"
#include "rendering.h"
#include "handle_input.h"
#include "obj_parsing.h"
bool initializeGame(SDL_Window** window, SDL_Renderer** renderer);
void gameLoop();
void cleanUp(SDL_Window* window, SDL_Renderer* renderer);

#endif