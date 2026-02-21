#ifndef ENTITIES_H
#define ENTITIES_H
#include <stdbool.h>
#include "defines.h"
#include <SDL3/SDL.h>
#include "alg.h"
#include <stdlib.h>
#include "timers.h"
typedef struct{
    vec4_t *camera_positions[3];
    SDL_FColor color;
    real avg_depth;

}triangle_t;
typedef struct{
    vec4_t *local_verts;
    vec4_t *world_verts;
    vec4_t *camera_verts;//this was added because we were modifying world_verts each frame. Now we have verts for the original position(0,0,0), the physical world and for the screen-space
    int vertex_count;//obviously, since both of them are dynamically allocated, we need to know how many vertices there are
    int* triangle_map;//this is an array of integers that defines which vertices form which triangles, it is basically a list of triplets of vertex indices, for example if we have a cube with 8 vertices, and we want to form a triangle with vertices 0, 1 and 2, we would have a triangle map of {0,1,2} and so on for all the triangles of the cube
    //this is also NULLABLE
    int triangle_count;//the number of triplets in the triangle map
}mesh_t;
typedef struct{

    real x,y,z;//these are stored in radians and represent the rotation around the x, y and z axes respectively, in that order, following the convention of Euler angles (yaw, pitch, roll)


}eulerangles_t;
typedef struct{

    vec4_t pos; //the position of the center of the entity
    vec4_t velocity;
    vec4_t angular_velocity;
    bool dirty;
    mesh_t *mesh;
    size_t pool_offset;//used for rendering purposes for the scenes.h logic.
    real speed;//highly useful for changing the speed of the entity without changing the velocity vector, which is useful for things like acceleration and deceleration, as well as for things like power-ups that can change the speed of the entity temporarily
    real angular_speed;
    eulerangles_t angles; //the rotation of the entity around the x, y and
    SDL_FColor color; //the color of the entity, this is used for rendering, it is a simple RGB color with an alpha channel, it is not used for anything else, it is just a way to give the entity a color for rendering purposes, it does not affect the physics or anything else, it is just a visual thing

}entity_t;

vec4_t* create_cube_local_vertices(real length, real width, real height);
mesh_t* create_mesh(vec4_t *local_verts, int num_verts, int triangle_count, int* triangle_map);
entity_t* create_entity(vec4_t pos, mesh_t *mesh);
void rotate_entity(entity_t *entity);
void add_angular_velocity(eulerangles_t *angles, vec4_t angular_velocity, real dt);
void free_entity(entity_t *entity);
SDL_Vertex* vec4tovertex(entity_t *entity);
bool isZero_vec(vec4_t vecA);
int* create_cube_triangles();
void update_entity(entity_t *entity, real dt);
void update_entities(entity_t **entities, int entity_count, real dt);
entity_t* create_cube_entity(vec4_t pos, real length, real width, real height);
#endif