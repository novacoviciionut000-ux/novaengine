#ifndef GLOOP_H
#define GLOOP_H
#include "libs.h"
void initializeGame(SDL_Window** window, SDL_Renderer** renderer);
void gameLoop();
void cleanUp(SDL_Window* window, SDL_Renderer* renderer);

#endif