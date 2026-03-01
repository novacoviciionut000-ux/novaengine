#include "scenes.h"

#define CHUNK             16
#define EXPECTED_TRIANGLES 4096
#define EXPECTED_MESHES    1000
#define EXPECTED_COLLISION_BOXES 1000

// ---------------------------------------------------------------------------
// create_scene
// ---------------------------------------------------------------------------

scene_t *create_scene(void) {
    scene_t *scene = calloc(1, sizeof(scene_t));
    if (!scene) return NULL;

    /* --- Managers --- */
    scene->entity_manager   = calloc(1, sizeof(entity_manager_t));
    scene->particle_manager = calloc(1, sizeof(particle_manager_t));
    scene->render_manager   = calloc(1, sizeof(render_manager_t));
    scene->enemy_manager    = calloc(1, sizeof(enemy_manager_t));
    scene->camera_manager   = calloc(1, sizeof(camera_manager_t));
    scene->collision_manager = calloc(1, sizeof(collision_manager_t));
    if (!scene->entity_manager   || !scene->particle_manager ||
        !scene->render_manager   || !scene->enemy_manager    ||
        !scene->camera_manager   || !scene->collision_manager)
        goto CLEANUP;

    /* --- Entity manager --- */
    scene->entity_manager->entities           = calloc(CHUNK, sizeof(entity_t*));
    scene->entity_manager->allocated_entities = CHUNK;
    scene->entity_manager->numentities        = 0;
    if (!scene->entity_manager->entities) goto CLEANUP;

    /* --- Particle manager --- */
    scene->particle_manager->particle_type_count = NUM_PARTICLE_TYPES;
    scene->particle_manager->numparticles        = 0;
    if (!get_particles(scene->particle_manager)) goto CLEANUP;

    /* --- Camera manager --- */
    scene->camera_manager->camera  = create_camera(
        (vec4_t){{{0,0,0,0}}}, (eulerangles_t){0,0,0}, 0.01f, 0);
    scene->camera_manager->meshes  = calloc(EXPECTED_MESHES, sizeof(mesh_t*));
    scene->camera_manager->allocated_meshes = EXPECTED_MESHES;
    scene->camera_manager->num_meshes       = 0;
    if (!scene->camera_manager->camera || !scene->camera_manager->meshes) goto CLEANUP;

    /* --- Render manager --- */
    scene->render_manager->triangles        = calloc(EXPECTED_TRIANGLES,         sizeof(triangle_t));
    scene->render_manager->render_usage     = calloc(EXPECTED_TRIANGLES * 7,     sizeof(triangle_t));
    scene->render_manager->transient_buffer = calloc(EXPECTED_TRIANGLES * 2 * 3, sizeof(vec4_t));
    scene->render_manager->indices          = calloc(EXPECTED_TRIANGLES * 2,     sizeof(uint32_t));
    scene->render_manager->temp_indices     = calloc(EXPECTED_TRIANGLES * 2,     sizeof(uint32_t));
    scene->render_manager->verts            = calloc(EXPECTED_TRIANGLES * 2 * 3, sizeof(SDL_Vertex));
    scene->render_manager->star_field       = calloc(NUM_STARS,                  sizeof(vec4_t));
    scene->render_manager->allocated_triangles = EXPECTED_TRIANGLES;
    scene->render_manager->numtriangles        = 0;
    scene->render_manager->transient_count     = 0;

    if (!scene->render_manager->triangles    || !scene->render_manager->render_usage  ||
        !scene->render_manager->transient_buffer || !scene->render_manager->indices   ||
        !scene->render_manager->temp_indices || !scene->render_manager->verts         ||
        !scene->render_manager->star_field)
        goto CLEANUP;

    /* --- Enemy manager ---
     * enemies[]      : active enemy pointers  (num_enemies entries)
     * free_enemies[] : inactive enemy pointers (free_enemies_count entries)
     * Both arrays are MAX_ENEMIES in size; the pool is filled by initialize_enemies().
     */
    scene->enemy_manager->enemies      = calloc(MAX_ENEMIES, sizeof(enemy_t*));
    scene->enemy_manager->free_enemies = calloc(MAX_ENEMIES, sizeof(enemy_t*));
    if(!scene->enemy_manager->enemies || !scene->enemy_manager->free_enemies)
        goto CLEANUP;
    scene->enemy_manager->num_enemies       = 0;
    scene->enemy_manager->free_enemies_count = 0;      /* filled by initialize_enemies */
    scene->enemy_manager->allocated_enemies  = MAX_ENEMIES;
    initialize_enemies(scene->enemy_manager);
    if (!scene->enemy_manager->enemies || !scene->enemy_manager->free_enemies) goto CLEANUP;


    //COLLISION MANAGER
    scene->collision_manager->collision_boxes = calloc(EXPECTED_COLLISION_BOXES, sizeof(collisionbox_t*));
    if(!scene->collision_manager->collision_boxes)
        goto CLEANUP;
    scene->collision_manager->num_boxes = 0;
    scene->collision_manager->allocated_boxes = EXPECTED_COLLISION_BOXES;






    /* --- Thread pool --- */
    int num_workers = SDL_GetNumLogicalCPUCores() - 1;
    if (num_workers < 1) num_workers = 1;
    init_thread_pool(scene->render_manager, num_workers,
                     scene->render_manager->allocated_triangles / num_workers);

    init_starfield(scene->render_manager);
    return scene;

CLEANUP:
    destroy_scene(scene);
    return NULL;
}

// ---------------------------------------------------------------------------
// add_triangles  (internal helper)
// ---------------------------------------------------------------------------

void add_triangles(void *object, int object_type,
                           scene_t *scene, size_t write_index)
{
    mesh_t     *mesh  = NULL;
    SDL_FColor  color = {1,1,1,1};

    switch (object_type) {
        case ENTITY_TYPE: {
            entity_t *e = (entity_t*)object;
            mesh  = e->mesh;
            color = e->color;
            break;
        }
        case PARTICLE_TYPE: {
            particle_t *p = (particle_t*)object;
            mesh  = p->mesh;
            color = p->color;
            break;
        }
        case ENEMY_TYPE: {
            enemy_t *en = (enemy_t*)object;
            mesh  = en->mesh;
            color = en->color;
            break;
        }
        default: return;
    }
    if (!mesh) return;

    for (int i = 0; i < mesh->triangle_count * 3; i += 3) {
        triangle_t *curr = &scene->render_manager->triangles[write_index++];
        curr->camera_positions[0] = &mesh->camera_verts[mesh->triangle_map[i]];
        curr->camera_positions[1] = &mesh->camera_verts[mesh->triangle_map[i+1]];
        curr->camera_positions[2] = &mesh->camera_verts[mesh->triangle_map[i+2]];
        curr->color = mesh->triangle_colors
                      ? mesh->triangle_colors[i / 3]
                      : color;
        curr->avg_depth = (curr->camera_positions[0]->z +
                           curr->camera_positions[1]->z +
                           curr->camera_positions[2]->z) / 3.0f;
    }
}

// ---------------------------------------------------------------------------
// add_object
// ---------------------------------------------------------------------------

bool add_object(scene_t *scene, void *object, int object_type) {
    if (!scene || !object) return false;

    size_t tri_count = 0;

    /* 1. Add to type-specific manager array */
    switch (object_type) {
        case ENTITY_TYPE: {
            entity_t *ent = (entity_t*)object;
            tri_count     = ent->mesh->triangle_count;

            if (scene->entity_manager->numentities >=
                scene->entity_manager->allocated_entities) {
                scene->entity_manager->allocated_entities += CHUNK;
                entity_t **tmp = realloc(
                    scene->entity_manager->entities,
                    scene->entity_manager->allocated_entities * sizeof(entity_t*));
                if (!tmp) return false;
                scene->entity_manager->entities = tmp;
            }
            scene->entity_manager->entities[scene->entity_manager->numentities++] = ent;
            break;
        }
        case PARTICLE_TYPE: {
            particle_t *part = (particle_t*)object;
            tri_count        = part->mesh->triangle_count;

            if (scene->particle_manager->numparticles >=
                scene->particle_manager->allocated_particles) {
                scene->particle_manager->allocated_particles += EXPECTED_PARTICLES;
                particle_t **tmp = realloc(
                    scene->particle_manager->particles,
                    scene->particle_manager->allocated_particles * sizeof(particle_t*));
                if (!tmp) return false;
                scene->particle_manager->particles = tmp;
            }
            scene->particle_manager->particles[scene->particle_manager->numparticles++] = part;
            break;
        }
        case ENEMY_TYPE: {
            enemy_t *enemy = (enemy_t*)object;
            tri_count      = enemy->mesh->triangle_count;
            /*
             * NOTE: enemy is already in manager->enemies[] — add_enemy() put it
             * there before calling add_object().  We don't push it again here.
             * We only need to link its triangles into the render pool below.
             */
            break;
        }
        default: return false;
    }

    /* 2. Grow render pool if needed */
    size_t new_total = scene->render_manager->numtriangles + tri_count;
    if (new_total >= scene->render_manager->allocated_triangles) {
        size_t new_alloc = scene->render_manager->allocated_triangles;
        while (new_alloc <= new_total) new_alloc += EXPECTED_TRIANGLES;

        triangle_t *t1 = realloc(scene->render_manager->triangles,
                                  new_alloc * sizeof(triangle_t));
        triangle_t *t2 = realloc(scene->render_manager->render_usage,
                                  new_alloc * 7 * sizeof(triangle_t));
        SDL_Vertex *t3 = realloc(scene->render_manager->verts,
                                  new_alloc * 2 * 3 * sizeof(SDL_Vertex));
        vec4_t     *t4 = realloc(scene->render_manager->transient_buffer,
                                  new_alloc * 2 * 3 * sizeof(vec4_t));
        uint32_t   *t5 = realloc(scene->render_manager->indices,
                                  new_alloc * 2 * sizeof(uint32_t));
        uint32_t   *t6 = realloc(scene->render_manager->temp_indices,
                                  new_alloc * 2 * sizeof(uint32_t));

        if (!t1 || !t2 || !t3 || !t4 || !t5 || !t6) return false;

        scene->render_manager->triangles          = t1;
        scene->render_manager->render_usage       = t2;
        scene->render_manager->verts              = t3;
        scene->render_manager->transient_buffer   = t4;
        scene->render_manager->indices            = t5;
        scene->render_manager->temp_indices       = t6;
        scene->render_manager->allocated_triangles = new_alloc;

        /* Resize each worker's transient buffer */
        if (scene->render_manager->num_workers > 0) {
            size_t per_thread =
                (new_alloc / scene->render_manager->num_workers) * 7;
            for (int i = 0; i < scene->render_manager->num_workers; i++) {
                worker_t *w = &scene->render_manager->workers[i];
                vec4_t *nt  = SDL_realloc(w->transient,
                                           sizeof(vec4_t) * per_thread);
                if (!nt) return false;
                w->transient = nt;
            }
        }
    }

    /* 3. Record pool offset and link triangles */
    size_t offset = scene->render_manager->numtriangles;

    switch (object_type) {
        case ENTITY_TYPE:
            ((entity_t*)object)->pool_offset  = offset; break;
        case PARTICLE_TYPE:
            ((particle_t*)object)->pool_offset = offset; break;
        case ENEMY_TYPE:
            ((enemy_t*)object)->pool_offset    = offset; break;
    }
    if(object_type != PARTICLE_TYPE){
        printf("pool_offset = %zu\n", offset);
        printf("new total : %zu\n", new_total);
        printf("Buffer size: %zu\n", scene->render_manager->allocated_triangles);
    }

    add_triangles(object, object_type, scene, offset);
    scene->render_manager->numtriangles = new_total;
    return true;
}

// ---------------------------------------------------------------------------
// _patch_scene_after_removal
// ---------------------------------------------------------------------------

void _patch_scene_after_removal(scene_t *scene,
                                 size_t start_offset,
                                 size_t triangle_count)
{
    printf("start=%zu count=%zu total=%zu\n",
        start_offset, triangle_count,
        scene->render_manager->numtriangles);
    fflush(stdout);
    render_manager_t *manager = scene->render_manager;
    if (start_offset + triangle_count > manager->numtriangles) {
            printf("ERROR: Attempted to remove an object that exceeds the render pool bounds!\n");
            return;
    }
    size_t move_count =
        manager->numtriangles - (start_offset + triangle_count);

    if (move_count > 0) {
        memmove(&manager->triangles[start_offset],
                &manager->triangles[start_offset + triangle_count],
                move_count * sizeof(triangle_t));
    }
    manager->numtriangles -= triangle_count;

    /* Decrement pool offsets of everything that lived AFTER the removed block */
    for (size_t i = 0; i < scene->entity_manager->numentities; i++) {
        entity_t *e = scene->entity_manager->entities[i];
        if(e == NULL)printf("Trouble in entities\n");
        if (e->pool_offset > start_offset)

            e->pool_offset -= triangle_count;
    }
    for (size_t i = 0; i < scene->particle_manager->numparticles; i++) {

        particle_t *p = scene->particle_manager->particles[i];
        if(p == NULL)printf("Trouble in particles\n");
        if (p->pool_offset > start_offset)
            p->pool_offset -= triangle_count;
    }
    for (size_t i = 0; i < scene->enemy_manager->num_enemies; i++) {
        enemy_t *en = scene->enemy_manager->enemies[i];
        if(en == NULL)printf("Trouble in enemies\n");
        if (en->pool_offset > start_offset)
            en->pool_offset -= triangle_count;
    }
}

// ---------------------------------------------------------------------------
// remove_entity_by_ptr
// ---------------------------------------------------------------------------

void remove_entity_by_ptr(scene_t *scene, void *target, int entity_type) {
    if (!scene || !target) return;

    size_t offset    = 0;
    size_t tri_count = 0;

    switch (entity_type) {
        case ENTITY_TYPE: {
            entity_t *ent = (entity_t*)target;
            offset        = ent->pool_offset;
            tri_count     = ent->mesh->triangle_count;

            _patch_scene_after_removal(scene, offset, tri_count);

            /* Remove from active entity array */
            for (size_t i = 0; i < scene->entity_manager->numentities; i++) {
                if (scene->entity_manager->entities[i] == ent) {
                    scene->entity_manager->entities[i] =
                        scene->entity_manager->entities[
                            --scene->entity_manager->numentities];
                    break;
                }
            }
            /* Entities are heap-allocated and owned by the scene */
            free(ent->mesh->world_verts);
            free(ent->mesh->local_verts);
            free(ent->mesh->camera_verts);
            free(ent->mesh->triangle_map);
            free(ent->mesh->triangle_colors);
            free(ent->mesh);
            free(ent);
            break;
        }
        case PARTICLE_TYPE: {
            particle_t *part = (particle_t*)target;
            offset           = part->pool_offset;
            tri_count        = part->mesh->triangle_count;

            _patch_scene_after_removal(scene, offset, tri_count);

            /* Remove from active particle array — particle memory goes back
             * to the free pool in update_particles, NOT freed here */
            for (size_t i = 0; i < scene->particle_manager->numparticles; i++) {
                if (scene->particle_manager->particles[i] == part) {
                    scene->particle_manager->particles[i] =
                        scene->particle_manager->particles[
                            --scene->particle_manager->numparticles];
                    break;
                }
            }
            break;
        }
        case ENEMY_TYPE: {
            enemy_t *enemy = (enemy_t*)target;
            offset         = enemy->pool_offset;
            tri_count      = enemy->mesh->triangle_count;

            _patch_scene_after_removal(scene, offset, tri_count);

            /* Remove from active enemy array — memory goes back to
             * free_enemies[] */
            for (size_t i = 0; i < scene->enemy_manager->num_enemies; i++) {
                if (scene->enemy_manager->enemies[i] == enemy) {
                    scene->enemy_manager->enemies[i] =
                        scene->enemy_manager->enemies[
                            --scene->enemy_manager->num_enemies];
                    scene->enemy_manager->free_enemies[scene->enemy_manager->free_enemies_count++] = enemy;
                    break;
                }
            }
            break;
        }
        default: return;
    }
}

// ---------------------------------------------------------------------------
// gather_meshes  — called every frame before camera transform
// ---------------------------------------------------------------------------

void gather_meshes(scene_t *scene) {
    size_t n = 0;

    /* Helper macro to grow the mesh array if needed */
    #define PUSH_MESH(m)                                                          \
        do {                                                                      \
            if (n >= scene->camera_manager->allocated_meshes) {                  \
                scene->camera_manager->allocated_meshes += EXPECTED_MESHES;      \
                mesh_t **_tmp = realloc(scene->camera_manager->meshes,           \
                    scene->camera_manager->allocated_meshes * sizeof(mesh_t*));  \
                if (!_tmp) { destroy_scene(scene); return; }                     \
                scene->camera_manager->meshes = _tmp;                            \
            }                                                                    \
            scene->camera_manager->meshes[n++] = (m);                           \
        } while (0)

    for (size_t i = 0; i < scene->entity_manager->numentities; i++)
        PUSH_MESH(scene->entity_manager->entities[i]->mesh);

    for (size_t i = 0; i < scene->particle_manager->numparticles; i++)
        PUSH_MESH(scene->particle_manager->particles[i]->mesh);

    for (size_t i = 0; i < scene->enemy_manager->num_enemies; i++)
        PUSH_MESH(scene->enemy_manager->enemies[i]->mesh);

    #undef PUSH_MESH

    scene->camera_manager->num_meshes = n;
}
void gather_collision_boxes(scene_t *scene){

    entity_manager_t* em = scene->entity_manager;
    enemy_manager_t * enm = scene->enemy_manager;
    collision_manager_t *cm = scene->collision_manager;
    cm->num_boxes = 0;
    for(size_t i = 0; i < em->numentities; i++){
        if(em->entities[i]->collision_box == NULL) continue;
        cm->collision_boxes[cm->num_boxes++] = em->entities[i]->collision_box;
        if(cm->num_boxes >= cm -> allocated_boxes){
            cm->allocated_boxes *=2;
            collisionbox_t **tmp = realloc(cm->collision_boxes, cm->allocated_boxes * sizeof(collisionbox_t*));
            if(!tmp) destroy_scene(scene);
            cm->collision_boxes = tmp;
        }
    }
    for(size_t i = 0; i < enm->num_enemies; i++){
        if(enm->enemies[i]->collision_box == NULL) continue;
        cm->collision_boxes[cm->num_boxes++] = enm->enemies[i]->collision_box;
        if(cm->num_boxes >= cm -> allocated_boxes){
            cm->allocated_boxes *=2;
            collisionbox_t **tmp = realloc(cm->collision_boxes, cm->allocated_boxes * sizeof(collisionbox_t*));
            if(!tmp) destroy_scene(scene);
            cm->collision_boxes = tmp;
        }
    }

}
// ---------------------------------------------------------------------------
// destroy_scene
// ---------------------------------------------------------------------------

void destroy_scene(scene_t *scene) {
    if (!scene) return;

    /* Thread pool must die first — workers reference render_manager data */
    if (scene->render_manager)
        shutdown_thread_pool(scene->render_manager);

    /* Entities — owned by scene, deep free */
    if (scene->entity_manager) {
        for (size_t i = 0; i < scene->entity_manager->numentities; i++) {
            entity_t *e = scene->entity_manager->entities[i];
            if (!e) continue;
            if (e->mesh) {
                free(e->mesh->world_verts);
                free(e->mesh->local_verts);
                free(e->mesh->camera_verts);
                free(e->mesh->triangle_map);
                free(e->mesh->triangle_colors);
                free(e->mesh);
            }
            free(e);
        }
        free(scene->entity_manager->entities);
        free(scene->entity_manager);
    }

    /* Render buffers */
    if (scene->render_manager) {
        free(scene->render_manager->triangles);
        free(scene->render_manager->render_usage);
        free(scene->render_manager->verts);
        free(scene->render_manager->transient_buffer);
        free(scene->render_manager->indices);
        free(scene->render_manager->temp_indices);
        free(scene->render_manager->star_field);
        free(scene->render_manager);
    }

    /* Particles — particle manager owns all particle memory */
    if (scene->particle_manager) {
        destroy_particle_manager(scene->particle_manager);
        free(scene->particle_manager);
    }

    /* Enemies — enemy manager owns all enemy memory */
    if (scene->enemy_manager) {
        /* free_enemies[] holds ALL enemy allocations (active ones are just
         * pointers into the same pool — don't double free) */
        if (scene->enemy_manager->free_enemies) {
            for (size_t i = 0; i < scene->enemy_manager->free_enemies_count; i++) {
                enemy_t *en = scene->enemy_manager->free_enemies[i];
                if (!en) continue;
                if (en->mesh) {
                    free(en->mesh->world_verts);
                    free(en->mesh->local_verts);
                    free(en->mesh->camera_verts);
                    free(en->mesh->triangle_map);
                    free(en->mesh->triangle_colors);
                    free(en->mesh);
                }
                free(en);
            }
            free(scene->enemy_manager->free_enemies);
        }
        /* Active enemies that were never returned to pool (e.g. mid-game exit) */
        if (scene->enemy_manager->enemies) {
            for (size_t i = 0; i < scene->enemy_manager->num_enemies; i++) {
                enemy_t *en = scene->enemy_manager->enemies[i];
                if (!en) continue;
                if (en->mesh) {
                    free(en->mesh->world_verts);
                    free(en->mesh->local_verts);
                    free(en->mesh->camera_verts);
                    free(en->mesh->triangle_map);
                    free(en->mesh->triangle_colors);
                    free(en->mesh);
                }
                free(en);
            }
            free(scene->enemy_manager->enemies);
        }
        free(scene->enemy_manager);
    }

    /* Camera */
    if (scene->camera_manager) {
        free(scene->camera_manager->meshes);
        free(scene->camera_manager->camera);
        free(scene->camera_manager);
    }
    free(scene->collision_manager->collision_boxes);
    free(scene->collision_manager);

    free(scene);
}
