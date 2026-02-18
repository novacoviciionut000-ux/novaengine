#ifndef GLOOP_H
#define GLOOP_H
#include <SDL3/SDL.h>
#include "squares.h"
#include <stdio.h>
bool initializeGame(SDL_Window** window, SDL_Renderer** renderer);
void gameLoop();
void cleanUp(SDL_Window* window, SDL_Renderer* renderer);

#endif