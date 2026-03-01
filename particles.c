#include "particles.h"

#define bullet_x 1.5f
#define bullet_y 1.5f
#define bullet_z 1.5f

/*Basically, the particle manager has a field "free_particles", that in itself is a struct which is basically an array of arrays(like a matrix basically).
On each line, there lies "inactive" particles of each specific type. When a particle is needed, it is taken from the "free pool"(the free_pool does a quick
swap and pop to remove that particle from it, and it decreases the number of free particles) of it's specific type, then
it is sent to the renderer vid add_object(the camera is linked to it anyway because it takes meshes from both particles and entities), then it is removed
via _remove_entity_by_ptr which basically simply removes it's triangles from the rendering data, then updates the pool offsets of all objects that are AFTER it
in the triangle pool. The random effects are simply to make the particles look "natural" */


float random_float(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

particle_t *get_blood_particle(char *pathname) {
    particle_t *part = (particle_t*)get_obj(pathname, (vec4_t){{{0,0,0,0}}},
                        (SDL_FColor){1.0f, 0.0f, 0.0f, 1.0f},
                        0.03f, false, 0, -M_PIdiv2, 0, false, PARTICLE_TYPE);
    if (!part) return NULL; // was missing — crash if get_obj fails

    part->lifetime      = 2.0f;
    part->velocity      = (vec4_t){{{0,0,0,0}}};
    part->position      = (vec4_t){{{0,0,0,0}}};
    part->gravity       = -0.1f;
    part->color         = (SDL_FColor){1.0f, 0.0f, 0.0f, 1.0f};
    part->visible_radius = 25.0f;
    part->type_particle = BLOOD_PART;
    return part;
}

particle_t *get_spark_particle(char *pathname) {
    particle_t *part = (particle_t*)get_obj(pathname, (vec4_t){{{0,0,0,0}}},
                        (SDL_FColor){1.0f, 0.8f, 0.0f, 1.0f},
                        0.05f, false, 0, 0, 0, false, PARTICLE_TYPE);
    if (!part) return NULL;

    part->lifetime      = 1.0f;
    part->velocity      = (vec4_t){{{0,0,0,0}}};
    part->position      = (vec4_t){{{0, 0, 0, 0}}};
    part->gravity       = -0.05f;
    part->color         = (SDL_FColor){1.0f, 0.8f, 0.0f, 1.0f};
    part->visible_radius = 25.0f;
    part->collision_radius = 0.0f;
    part->type_particle = SPARK_PART;
    return part;
}
particle_t *get_bullet_particle(char *pathname){
    particle_t *part = (particle_t*)get_obj(pathname, (vec4_t){{{0,0,0,0}}},
                        (SDL_FColor){0.0f, 0.2f, 0.2f, 1.0f},
                        0.07f, false, 0, 0, 0, false, PARTICLE_TYPE);
    if (!part) return NULL;

    part->lifetime      = 100.0f;
    part->velocity      = (vec4_t){{{0,0,0,0}}};
    part->position      = (vec4_t){{{0, 0, 0, 0}}};
    part->gravity       = 0.0f;
    part->color         = (SDL_FColor){1.0f, 0.8f, 0.0f, 1.0f};
    part->visible_radius = 50.0f;
    part->collision_radius = 0.1f;
    part->type_particle = PARTICLE_BULLET;
    return part;
}

bool create_particles_list(particle_manager_t *manager) {
    // Allocate the outer free_particles array — must exist before indexing into it
    manager->free_particles = calloc(manager->particle_type_count, sizeof(free_particles_t));
    if (!manager->free_particles) return false;

    // Allocate the master active particles array
    size_t total_max = MAX_PARTICLES * manager->particle_type_count;
    manager->particles = calloc(total_max, sizeof(particle_t*));
    if (!manager->particles) goto CLEANUP;
    manager->allocated_particles = total_max;
    manager->numparticles = 0;

    // Allocate each type's free pool
    for (size_t i = 0; i < manager->particle_type_count; i++) {
        manager->free_particles[i].particles = calloc(MAX_PARTICLES, sizeof(particle_t*));
        if (!manager->free_particles[i].particles) goto CLEANUP;
        manager->free_particles[i].allocated_particles = MAX_PARTICLES;
        manager->free_particles[i].free_particles      = 0;
    }
    return true;

CLEANUP:
    if (manager->particles) {
        free(manager->particles);
        manager->particles = NULL;
    }
    for (size_t j = 0; j < manager->particle_type_count; j++) {
        if (manager->free_particles[j].particles) {
            free(manager->free_particles[j].particles);
            manager->free_particles[j].particles = NULL;
        }
    }
    free(manager->free_particles);
    manager->free_particles = NULL;
    return false;
}

void free_particle_deep(particle_t *part) {
    if (!part) return;
    if (part->mesh) {
        free(part->mesh->local_verts);
        free(part->mesh->world_verts);
        free(part->mesh->camera_verts);
        free(part->mesh->triangle_map);
        free(part->mesh->triangle_colors);
        free(part->mesh);
    }
    free(part);
}

bool get_particles(particle_manager_t *manager) {
    if (!create_particles_list(manager)) return false;

    // 1. Load prototypes from disk
    particle_t *prototypes[manager->particle_type_count];
    memset(prototypes, 0, sizeof(prototypes));

    for (size_t i = 0; i < manager->particle_type_count; i++) {
        switch (i) {
            case BLOOD_PART:
                prototypes[i] = get_blood_particle(BLOOD_PATHNAME);
                break;
            case SPARK_PART:
                prototypes[i] = get_spark_particle(SPARK_PATHNAME);
                break;
            case BULLET_PART:
                prototypes[i] = get_bullet_particle(BULLET_PATHNAME);
                break;
            default:
                goto CLEANUP;
        }
        if (!prototypes[i]) goto CLEANUP;
    }

    // 2. Fill pools by cloning prototypes
    // Slot 0 is reserved for the prototype — emit_particle never touches it
    // The usable pool is slots 1..allocated_particles-1
    for (size_t i = 0; i < manager->particle_type_count; i++) {
        // Store prototype at slot 0
        manager->free_particles[i].particles[0] = prototypes[i];

        size_t allocated = manager->free_particles[i].allocated_particles;

        for (size_t j = 1; j < allocated; j++) {
            particle_t *clone = calloc(1, sizeof(particle_t));
            if (!clone) goto CLEANUP;

            *clone = *prototypes[i];
            clone->type_particle = i;

            mesh_t *mesh = calloc(1, sizeof(mesh_t));
            if (!mesh) { free(clone); goto CLEANUP; }

            *mesh = *prototypes[i]->mesh;

            int vc = mesh->vertex_count;
            int tc = mesh->triangle_count;

            mesh->local_verts     = malloc(vc * sizeof(vec4_t));
            mesh->world_verts     = calloc(vc, sizeof(vec4_t));
            mesh->camera_verts    = calloc(vc, sizeof(vec4_t));
            mesh->triangle_map    = malloc(tc * 3 * sizeof(int));
            mesh->triangle_colors = malloc(tc * sizeof(SDL_FColor));

            if (!mesh->local_verts || !mesh->world_verts || !mesh->camera_verts ||
                !mesh->triangle_map || !mesh->triangle_colors) {
                free(mesh->local_verts);
                free(mesh->world_verts);
                free(mesh->camera_verts);
                free(mesh->triangle_map);
                free(mesh->triangle_colors);
                free(mesh);
                free(clone);
                goto CLEANUP;
            }

            memcpy(mesh->local_verts,     prototypes[i]->mesh->local_verts,     vc * sizeof(vec4_t));
            memcpy(mesh->triangle_map,    prototypes[i]->mesh->triangle_map,    tc * 3 * sizeof(int));
            memcpy(mesh->triangle_colors, prototypes[i]->mesh->triangle_colors, tc * sizeof(SDL_FColor));

            clone->mesh = mesh;
            manager->free_particles[i].particles[j] = clone;
        }

        // Set counter to allocated-1 so slot 0 (prototype) is never popped
        manager->free_particles[i].free_particles = allocated - 1;
    }
    return true;

CLEANUP:
    for (size_t i = 0; i < manager->particle_type_count; i++) {
        if (!manager->free_particles[i].particles) continue;
        for (size_t j = 0; j < manager->free_particles[i].allocated_particles; j++) {
            if (manager->free_particles[i].particles[j])
                free_particle_deep(manager->free_particles[i].particles[j]);
        }
    }
    return false;
}

real get_particle_distance_from_camera(particle_t *part, camera_t *cam) {
    return get_distance(&part->position, &cam->pos);
}

void transform_particle_verts(scene_t *scene) {
    particle_manager_t *pm = scene->particle_manager;

    for (size_t i = 0; i < pm->numparticles; i++) {
        particle_t *part = pm->particles[i];
        mesh_t *mesh = part->mesh;

        // local -> world: simple translation, no matrix needed
        for (int v = 0; v < mesh->vertex_count; v++) {
            mesh->world_verts[v].x = mesh->local_verts[v].x + part->position.x;
            mesh->world_verts[v].y = mesh->local_verts[v].y + part->position.y;
            mesh->world_verts[v].z = mesh->local_verts[v].z + part->position.z;
            mesh->world_verts[v].w = 1.0f;
        }

    }
}

void update_particles(scene_t *scene, real dt, camera_t *cam) {
    particle_manager_t *pm = scene->particle_manager;
    if (pm->numparticles == 0) return;

    for (size_t i = 0; i < pm->numparticles; i++) {
        particle_t *part = pm->particles[i];

        part->velocity.y -= part->gravity * dt;
        part->position.x += part->velocity.x * dt;
        part->position.y += part->velocity.y * dt;
        part->position.z += part->velocity.z * dt;
        part->lifetime   -= dt;

        bool is_dead = (part->lifetime <= 0) ||
                       (get_particle_distance_from_camera(part, cam) > part->visible_radius);

        if (is_dead) {
            int type = part->type_particle;

            // Guard against pool overflow
            if (pm->free_particles[type].free_particles < pm->free_particles[type].allocated_particles - 1) {
                size_t free_idx = pm->free_particles[type].free_particles++;
                pm->free_particles[type].particles[free_idx + 1] = part; // +1 to skip slot 0
            }

            remove_entity_by_ptr(scene, part, PARTICLE_TYPE);
            i--;
        }
    }

    transform_particle_verts(scene);
}

particle_t* emit_particle(scene_t *scene, int type, vec4_t position, vec4_t base_velocity) {
    particle_manager_t *pm = scene->particle_manager;

    if (pm->free_particles[type].free_particles == 0) return NULL;

    // Pop from top of stack — never touches slot 0 (prototype)
    size_t free_idx = pm->free_particles[type].free_particles--;
    particle_t *part  = pm->free_particles[type].particles[free_idx];
    pm->free_particles[type].particles[free_idx] = NULL; // clear slot to prevent double free
    particle_t *proto = pm->free_particles[type].particles[0];

    part->lifetime         = proto->lifetime;
    part->gravity          = proto->gravity;
    part->color            = proto->color;
    part->visible_radius   = proto->visible_radius;
    part->collision_radius = proto->collision_radius;
    part->position         = position;

    if (type == BLOOD_PART) {
        part->velocity.x = base_velocity.x + random_float(-0.05f, 0.05f);
        part->velocity.y = base_velocity.y + random_float(-0.1f, 0.1f);
        part->velocity.z = base_velocity.z + random_float(-0.05f, 0.05f);
    } else if (type == SPARK_PART) {
        real rx = random_float(-0.3f, 0.3f);
        real ry = random_float(0.05f, 0.1f);
        real rz = random_float(-0.3f, 0.3f);
        part->velocity.x = base_velocity.x + rx * 2.5f;
        part->velocity.y = base_velocity.y + ry * 2.5f;
        part->velocity.z = base_velocity.z + rz * 2.5f;
    }else {//the bullets need to follow another path as their velocity is fixed.
        part->velocity = base_velocity;
    }

    add_object(scene, part, PARTICLE_TYPE);
    return part;
}
particle_t* emit_particle_muzzle(scene_t *scene, int type, vec4_t position, vec4_t base_velocity) {
    particle_manager_t *pm = scene->particle_manager;

    if (pm->free_particles[type].free_particles == 0) return NULL;
    // This function is basically extra - it could only support sparks but i am keeping it cause maybe at some point
    // I will want to make the gun shoot another type of particle so yeah.
    size_t free_idx = pm->free_particles[type].free_particles--;
    particle_t *part  = pm->free_particles[type].particles[free_idx];
    pm->free_particles[type].particles[free_idx] = NULL; // clear slot to prevent double free
    particle_t *proto = pm->free_particles[type].particles[0];

    part->lifetime         = proto->lifetime;
    part->gravity          = proto->gravity;
    part->color            = proto->color;
    part->visible_radius   = proto->visible_radius;
    part->collision_radius = proto->collision_radius;
    part->position         = position;

    if (type == BLOOD_PART) {
        part->velocity.x = base_velocity.x + random_float(-0.05f, 0.05f);
        part->velocity.y = base_velocity.y + random_float(-0.02f, 0.02f);
        part->velocity.z = base_velocity.z + random_float(-0.05f, 0.05f);
    } else if (type == SPARK_PART) {
        real rx = random_float(-0.3f, 0.3f);
        real ry = random_float(-1.0f, 1.0f);
        real rz = random_float(-0.4f, 0.4f);
        part->velocity.x = base_velocity.x + rx * 2.5f;
        part->velocity.y = base_velocity.y + ry * 2.5f;
        part->velocity.z = base_velocity.z + rz * 2.5f;
    }else {//the bullets need to follow another path as their velocity is fixed.
        part->velocity = base_velocity;
    }

    add_object(scene, part, PARTICLE_TYPE);
    return part;
}

void create_sparks(vec4_t position, scene_t *scene) {
    for (int i = 0; i < 40; i++) {
        particle_t *p = emit_particle(scene, SPARK_PART, position, (vec4_t){{{0,0,0,0}}});
        if (!p) {
            continue;
        } else {
            //printf("SUCCESS: Particle spawned at %f, %f, %f\n", p->position.x, p->position.y, p->position.z);
            //printf("Active Particles: %zu\n", scene->particle_manager->numparticles);
        }
    }
}
void create_muzzle_sparks(vec4_t position, scene_t *scene) {
    for (int i = 0; i < 15; i++) {
        particle_t *p = emit_particle_muzzle(scene, SPARK_PART, position, (vec4_t){{{0,0,0,0}}});
        if (!p) {
            continue;
        } else {
          //  printf("SUCCESS: Particle spawned at %f, %f, %f\n", p->position.x, p->position.y, p->position.z);
           // printf("Active Particles: %zu\n", scene->particle_manager->numparticles);
        }
    }
}
void create_blood(vec4_t position, scene_t *scene) {
    for (int i = 0; i < 60; i++) {
        particle_t *p = emit_particle(scene, BLOOD_PART, position, (vec4_t){{{0,0,0,0}}});
        if (!p) {
            continue;
        } else {
            //printf("SUCCESS: Particle spawned at %f, %f, %f\n", p->position.x, p->position.y, p->position.z);
           // printf("Active Particles: %zu\n", scene->particle_manager->numparticles);
        }
    }
}

// Note: does NOT free manager itself — destroy_scene owns that allocation
void destroy_particle_manager(particle_manager_t *manager) {
    if (!manager) return;

    // Free active particles
    for (size_t i = 0; i < manager->numparticles; i++)
        free_particle_deep(manager->particles[i]);

    // Free sleeping particles and their pool arrays
    if (manager->free_particles) {
        for (size_t i = 0; i < manager->particle_type_count; i++) {
            if (!manager->free_particles[i].particles) continue;
            // slot 0 is always the prototype, free_particles counts the rest
            // free all slots including prototype at slot 0
            size_t total = manager->free_particles[i].allocated_particles;
            for (size_t j = 0; j < total; j++) {
                if (manager->free_particles[i].particles[j])
                    free_particle_deep(manager->free_particles[i].particles[j]);
            }
            free(manager->free_particles[i].particles);
        }
        free(manager->free_particles);
    }

    if (manager->particles)
        free(manager->particles);

    // Do NOT free(manager) — destroy_scene owns it
}


void release_bullet(scene_t *scene) {
    camera_t *cam = scene->camera_manager->camera;
    real pitch = cam->angles.x;
    real yaw   = -cam->angles.y;

    // Forward vector
    float dir_x = cosf(pitch) * -sinf(yaw);
    float dir_y = sinf(pitch);
    float dir_z = cosf(pitch) *  cosf(yaw);

    // Right vector
    float right_x = cosf(yaw);
    float right_y = 0.0f;
    float right_z = sinf(yaw);

    // Up vector
    float up_x = -sinf(pitch) * -sinf(yaw);
    float up_y =  cosf(pitch);
    float up_z = -sinf(pitch) *  cosf(yaw);

    float spawn_dist = cam->radius + 0.01f;

    vec4_t spawn_pos = (vec4_t){{{
        cam->pos.x + dir_x * spawn_dist,
        cam->pos.y + dir_y * spawn_dist,
        cam->pos.z + dir_z * spawn_dist,
        0
    }}};

    vec4_t muzzle_pos = (vec4_t){{{
        cam->pos.x + dir_x * 0.1f + right_x * 0.01f + up_x * (-0.06f),
        cam->pos.y + dir_y * 0.1f + right_y * 0.01f + up_y * (-0.02f),
        cam->pos.z + dir_z * 0.1f + right_z * 0.01f + up_z * ( 0.01f),
        0
    }}};

    vec4_t velocity = (vec4_t){{{
        bullet_x * dir_x,
        bullet_y * dir_y,
        bullet_z * dir_z,
        0
    }}};

    create_muzzle_sparks(muzzle_pos, scene);

    particle_t *bullet = emit_particle(scene, BULLET_PART, spawn_pos, velocity);
    if (!bullet) { printf("Bullet is NULL!\n"); return; }
}
void update_bullets(scene_t *scene){
    particle_manager_t *pm = scene->particle_manager;
    for(size_t i = 0; i < pm -> numparticles; i++){
        if(pm->particles[i]->type_particle == BULLET_PART){
            if(pm->particles[i]->lifetime <= 0) continue;
            if(check_bullet_collision(pm->particles[i], scene->collision_manager)){
                printf("Collision detected !\n");
                pm->particles[i]->lifetime = 0;
                create_blood(pm->particles[i]->position, scene);
            }
            if(get_distance(&pm->particles[i]->position, &scene->camera_manager->camera->pos) < scene->camera_manager->camera->radius){
                scene->camera_manager->camera->health -=5;
                create_blood(scene->camera_manager->camera->pos, scene);
            }
        }else continue;
    }
}
