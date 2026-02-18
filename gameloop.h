#ifndef GLOOP_H
#define GLOOP_H
#include "entities.h"
#include <stdio.h>
#include "projection_math.h"
#include "handle_input.h"
bool initializeGame(SDL_Window** window, SDL_Renderer** renderer);
void gameLoop();
void cleanUp(SDL_Window* window, SDL_Renderer* renderer);

#endif