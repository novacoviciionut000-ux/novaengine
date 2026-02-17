#ifndef SQUARES_H
#define SQUARES_H
#include "libs.h"
#include "alg.h"
#define vel 1
typedef struct{
    vec4_t pos;
    double length, width;
}square_t;
void update_square(square_t *sq, const Uint8 *keyboard_state);
void render_square(const square_t *sq, SDL_Renderer *renderer);
void add_velocity(square_t *sq, vec4_t velocity);
#endif


