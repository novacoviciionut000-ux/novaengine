#include "projection_math.h"
SDL_Point vec4_to_screen_point(const vec4_t *vec){

    int y_offset = SCREEN_HEIGHT / 2;//we need to add these so that the cube is actually visible on the screen
    int x_offset = SCREEN_WIDTH / 2;
    SDL_Point point;
    point.x = (int)((vec->x / vec->z) * FOCAL + x_offset);
    point.y = (int)((vec->y / vec->z) * FOCAL + y_offset);
    return point;

}
SDL_FPoint vec4_to_screen_fpoint(const vec4_t *vec){
    int y_offset = SCREEN_HEIGHT / 2;//we need to add these so that the cube is actually visible on the screen
    int x_offset = SCREEN_WIDTH / 2;
    SDL_FPoint point;
    point.x = (float)((vec->x / vec->z) * FOCAL + x_offset);
    point.y = (float)((vec->y / vec->z) * FOCAL + y_offset);
    return point;

}