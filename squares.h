#ifndef SQUARES_H
#define SQUARES_H
#include <SDL3/SDL.h>
#include "alg.h"
#include <math.h>
#include <stdbool.h>
#include "defines.h"
#include <stdlib.h>
typedef struct{
    vec4_t pos;
    real length, width;
}square_t;
typedef struct{

    real x,y,z;//these are stored in radians and represent the rotation around the x, y and z axes respectively, in that order, following the convention of Euler angles (yaw, pitch, roll)


}eulerangles_t;
typedef struct{
    vec4_t pos; //the position of the center of the cube in the world
    vec4_t local_verts[8]; // Always at (0,0,0), never changes - it is the "template" for the cube, we will apply transformations to it to get the world vertices
    //this is necessary because the rotation matrix rotates the cube around (0,0,0), so if we want to rotate around the center of the cube, we need to have the local vertices defined relative to the center, and then we can apply the rotation and translation to get the world vertices
    vec4_t world_verts[8];    //so that we do not need to loop through the vertices twice(O(n^2)), we will do a "convetion":
                        //the first 4 vertices will be the "bottom" face, and the last 4 vertices will be the "top" face, in the same order as the bottom face
                        //it starts with the bottom left "front" vertex, next the bottom "front" right, then bottom "back" right, and finally the bottom "back" left, then the top "front" left, top "front" right, top "back" right, and top "back" left
                        //this way, the lines we need to draw are from:
                        //0-1, 1-2, 2-3, 3-0 for the bottom face
                        //4-5, 5-6, 6-7, 7-4 for the top face
                        //0-4, 1-5, 2-6, 3-7 for the vertical edges


    eulerangles_t angles;

}cube_t;
void update_square(square_t *sq, const Uint8 *keyboard_state);
void render_square(const square_t *sq, SDL_Renderer *renderer);
void render_cube(const cube_t *cube, SDL_Renderer *renderer);
void update_cube(cube_t *cube, const Uint8 *keyboard_state);
cube_t* create_cube(vec4_t pos, real length, real width, real height);
void add_velocity(square_t *sq, vec4_t velocity);
#endif


