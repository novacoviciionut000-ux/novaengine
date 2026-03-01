#ifndef DEF_H
#define DEF_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define ANGULAR_VELOCITY 0.01f
#define SENSITIVITY 0.01f
#define NUM_STARS 500
#define WORLD_SCALE_FACTOR 50.0f
#define ENTITY_TYPE 0
#define PARTICLE_TYPE 1
#define ENEMY_TYPE 2
#define JUMP_POWER 2.0f
#define MAX_PARTICLES 5000
#define NUM_PARTICLE_TYPES 3
#define MAX_ENEMIES 5000
#define PARTICLE_BLOOD 0
#define PARTICLE_SPARK 1
#define PARTICLE_BULLET 2
#define EXPECTED_PARTICLES 5000
#define BLOOD_PART 0
#define SPARK_PART 1
#define BULLET_PART 2
#define BLOOD_PATHNAME "blood.obj"
#define SPARK_PATHNAME "spark.obj"
#define BULLET_PATHNAME "bullet.obj"
#define DRONE_PATHNAME "drone.obj"
#define NUM_ENEMY_TYPES 1
#define DRONE_TYPE 0
typedef float real;

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
    bool hit;
} collisionbox_t;

// --- 2. Mesh & Entity ---
typedef struct {
    vec4_t *local_verts;
    vec4_t *world_verts;
    vec4_t *camera_verts;
    SDL_FColor *triangle_colors;
    int vertex_count;
    int *triangle_map;
    int triangle_count;
} mesh_t;
typedef struct entity_s entity_t;
struct entity_s {
    vec4_t pos;
    vec4_t velocity;
    vec4_t angular_velocity;
    bool dirty;
    mesh_t *mesh;
    size_t pool_offset;
    size_t vert_pool_offset;
    collisionbox_t *collision_box;
    real speed;
    real angular_speed;
    vec4_t force_accumulator;
    bool collidable;
    vec4_t acceleration;
    real mass;
    bool HUD;
    eulerangles_t angles;
    SDL_FColor color;
    void (*on_hit)(entity_t *entity, real dt);
};

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
    vec4_t velocity;
    bool sprinting;
    real focal_length;
    real speed;
    real mass;
    real angular_speed;
    real radius;
    uint8_t grounded;
    int health;
} camera_t;
typedef struct{
    camera_t* camera;
    mesh_t **meshes;
    size_t allocated_meshes;
    size_t num_meshes;

}camera_manager_t;
//particles

typedef struct{
    mesh_t *mesh;
    int type_particle;
    vec4_t velocity;
    real collision_radius;//we will need to check in our functions if this is set to 0.
    vec4_t position;
    real gravity;
    SDL_FColor color;
    real visible_radius;
    size_t pool_offset;
    size_t vert_pool_offset;
    real lifetime;
}particle_t;
typedef struct{
    particle_t **particles;
    size_t allocated_particles;
    size_t free_particles;
}free_particles_t;
typedef struct{
    particle_t **particles;
    size_t allocated_particles;
    free_particles_t *free_particles;// an array of free_particles_t data type (size particle_type_count)
    size_t particle_type_count;
    size_t numparticles;
}particle_manager_t;
//end particles

// --- 5. Scene Container ---
typedef struct{
    entity_t** entities;
    size_t numentities;
    size_t allocated_entities;

}entity_manager_t;
typedef struct render_manager_s render_manager_t;

//multithreading helpers
typedef struct {
    camera_t *cam;
    float margin_x;
    float margin_y;
    size_t start_triangle;
    size_t end_triangle;
    render_manager_t *manager;
    // Each thread writes to its own slice
    triangle_t *output;           // pointer into render_usage at thread's offset
    vec4_t *transient;            // thread's own transient buffer
    size_t transient_count;
    size_t visible_count;
    camera_manager_t *cam_manager;
    size_t start_mesh;
    size_t end_mesh;
    mat4_t rotation;
    vec4_t cam_pos;
    SDL_Semaphore   *vert_wake;
    SDL_Semaphore   *vert_done;
    SDL_Semaphore   *wake;
    SDL_Semaphore   *done;
    SDL_Semaphore   *fill_wake;
    SDL_Semaphore   *fill_done;
    size_t           fill_index_start;
    size_t           fill_index_end;
    size_t           fill_vert_start;
    float            fill_focal_length;
    bool quit;

} worker_t;


typedef struct{
    bool recoiling;
    real recoil_timer;
    real cooldown_timer;
}gun_state_t;


struct render_manager_s {
    triangle_t *triangles;
    triangle_t *render_usage;
    size_t numtriangles;
    size_t transient_count;
    size_t allocated_triangles;
    uint32_t *indices;
    uint32_t *temp_indices;
    worker_t *workers;
    int num_workers;
    SDL_Vertex *verts;
    vec4_t *star_field;
    vec4_t *transient_buffer;
};
typedef struct{
    collisionbox_t **collision_boxes;
    size_t num_boxes;
    size_t allocated_boxes;
}collision_manager_t;
typedef struct scene_s scene_t;
typedef struct enemy_s enemy_t;
struct enemy_s{
    mesh_t *mesh;
    collisionbox_t *collision_box;
    eulerangles_t angles;
    size_t pool_offset;
    real speed;
    int enemy_type;
    vec4_t position;
    vec4_t velocity;
    vec4_t acceleration;
    real time_accumulator;
    SDL_FColor color;
    real health;
    void (*behaviour)(enemy_t *enemy, real dt, camera_t *camera, scene_t *scene);
    void (*on_hit)(enemy_t *enemy, real dt);
    bool dirty;
};
typedef struct{
    enemy_t **enemies;
    size_t num_enemies;
    enemy_t **free_enemies;
    size_t free_enemies_count;
    size_t allocated_enemies;
}enemy_manager_t;
struct scene_s{
    particle_manager_t *particle_manager;
    entity_manager_t *entity_manager;
    render_manager_t *render_manager;
    camera_manager_t *camera_manager;
    enemy_manager_t *enemy_manager;
    collision_manager_t *collision_manager;
};



typedef struct{
    uint64_t start;
    uint64_t freq;
}delta_timer;

typedef struct{
    delta_timer* timer;
    double curr_time;
}real_timer;

#endif
