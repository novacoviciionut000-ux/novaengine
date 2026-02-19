#include "projection_math.h"
SDL_Point vec4_to_screen_point(const vec4_t *vec){

    int y_offset = SCREEN_HEIGHT / 2;//we need to add these so that the cube is actually visible on the screen
    int x_offset = SCREEN_WIDTH / 2;
    SDL_Point point;
    vec4_t vec_copy = add_vec4(&(vec4_t){{{0,0,0,0}}}, vec);
    if(vec_copy.z == 0) vec_copy.z = 0.1f;//we need to do this to prevent division by zero, and also to prevent the cube from disappearing when it gets too close to the camera, we can adjust this value as needed, but for now it should be fine
    point.x = (int)((vec_copy.x / vec_copy.z) * FOCAL + x_offset);
    point.y = (int)((vec_copy.y / vec_copy.z) * FOCAL + y_offset);
    return point;

}
SDL_FPoint vec4_to_screen_fpoint(const vec4_t *vec){
    int y_offset = SCREEN_HEIGHT / 2;//we need to add these so that the cube is actually visible on the screen
    int x_offset = SCREEN_WIDTH / 2;
    SDL_FPoint point;
    vec4_t vec_copy = add_vec4(&(vec4_t){{{0,0,0,0}}}, vec);
     if(vec_copy.z == 0) vec_copy.z = 0.1f;//we need to do this to prevent division by zero, and also to prevent the cube from disappearing when it gets too close to the camera, we can adjust this value as needed, but for now it should be fine
    point.x = (float)((vec_copy.x / vec_copy.z) * FOCAL + x_offset);
    point.y = (float)((vec_copy.y / vec_copy.z) * FOCAL + y_offset);
    return point;

}