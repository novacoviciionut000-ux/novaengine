#ifndef DEF_H
#define DEF_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>

#define SCREEN_WIDTH 1440
#define SCREEN_HEIGHT 1080
#define ANGULAR_VELOCITY 0.01f
#define SENSITIVITY 0.01f
#define NUM_STARS 500
#define WORLD_SCALE_FACTOR 50.0f
#define FOCAL 400

typedef float real;

// --- 1. Primitive Math Types (Must be first) ---
typedef struct {
    union {
        struct { real x, y, z; };
        real vals[3];
    };
} vec3_t;

typedef struct {
    union {
        struct { real x, y, z, w; };
        real vals[4];
    };
} vec4_t;

typedef struct {
    union {
        real m[3][3];
        real v[9];
    };
} mat3_t;

typedef struct {
    union {
        real m[4][4];
        real v[16];
    };
} mat4_t;

typedef struct {
    real x, y, z; 
} eulerangles_t;

typedef struct {
    real min_x, max_x;
    real min_y, max_y;
    real min_z, max_z;
} collisionbox_t;

// --- 2. Mesh & Entity ---
typedef struct {
    vec4_t *local_verts;
    vec4_t *world_verts;
    vec4_t *camera_verts;
    int vertex_count;
    int *triangle_map;
    int triangle_count;
} mesh_t;

typedef struct {
    vec4_t pos; 
    vec4_t velocity;
    vec4_t angular_velocity;
    bool dirty;
    mesh_t *mesh;
    size_t pool_offset;
    collisionbox_t *collision_box;
    real speed;
    real angular_speed;
    eulerangles_t angles; 
    SDL_FColor color; 
} entity_t;

// --- 3. Rendering ---
typedef struct {
    vec4_t *camera_positions[3];
    SDL_FColor color;
    real avg_depth;
    real dprod;
} triangle_t;

// --- 4. Camera & Input ---
typedef struct {
    bool up, down, left, right, forward, backward;
} input_state_t;

typedef struct {
    vec4_t pos;
    eulerangles_t angles;
    real speed;
    real angular_speed;
    real radius;
    uint8_t grounded; 
} camera_t;

// --- 5. Scene Container ---
typedef struct {
    entity_t** entities;
    size_t numentities;
    size_t allocated_mem;

    size_t numtriangles;
    triangle_t *triangles;
    triangle_t *render_usage;
    vec4_t *transient_buffer; 
    size_t transient_count;
    uint32_t *indices;
    uint32_t *temp_indices;
    SDL_Vertex* verts;
    size_t allocated_triangles;
    vec4_t *star_field;
} scene_t;
typedef struct{
    uint64_t start;
    uint64_t freq;
}delta_timer;

typedef struct{
    delta_timer* timer;
    double curr_time;
}real_timer;
// --- 6. Math Functions (Must be last) ---
// Now alg.h can safely use vec4_t, mat4_t, etc.

#endif