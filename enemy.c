#include "enemy.h"

void on_hit_drone(enemy_t *enemy, real dt) {
    enemy->health -= 10;
}

void behaviour_drone(enemy_t *enemy, real dt, camera_t *camera, scene_t *scene) {
    if (get_distance(&enemy->position, &camera->pos) < 10.0f) {
        enemy->time_accumulator += dt;
        if (enemy->time_accumulator > 2.0f) {
            enemy->time_accumulator = 0.0f;

            vec4_t to_player = (vec4_t){{{
                camera->pos.x - enemy->position.x,
                camera->pos.y - enemy->position.y,
                camera->pos.z - enemy->position.z,
                0.0f
            }}};

            vec4_t bullet_norm = normalize_vec4(&to_player);

            /* Half extents */

            real spawn_dist = 0.2f;
            vec4_t spawn_pos = (vec4_t){{{
                enemy->position.x + bullet_norm.x * spawn_dist,
                enemy->position.y + bullet_norm.y * spawn_dist,
                enemy->position.z + bullet_norm.z * spawn_dist,
                0.0f
            }}};

            vec4_t bullet_dir = (vec4_t){{{
                bullet_norm.x * 3.0f,
                bullet_norm.y * 3.0f,
                bullet_norm.z * 3.0f,
                0.0f
            }}};
            printf("enemy: %f %f %f | cam: %f %f %f\n",
                enemy->position.x, enemy->position.y, enemy->position.z,
                camera->pos.x, camera->pos.y, camera->pos.z);
            printf("Bullet norm x: %f\nBullet norm y: %f\n Bullet norm z:%f\nSpawn pos x:%f\nSpawn pos y:%f\nSpawn pos z:%f\n", bullet_norm.x,bullet_norm.y,bullet_norm.z, spawn_pos.x, spawn_pos.y, spawn_pos.z);
            emit_particle(scene, BULLET_PART, spawn_pos, bullet_dir);
            create_sparks(enemy->position, scene);
        }
    } else {
        enemy->time_accumulator = 0.0f;
    }
}
void move_collisionbox(enemy_t *enemy, real dt){
    enemy->collision_box->min_x += enemy->velocity.x * dt;
    enemy->collision_box->min_y += enemy->velocity.y * dt;
    enemy->collision_box->min_z += enemy->velocity.z * dt;
    enemy->collision_box->max_x += enemy->velocity.x * dt;
    enemy->collision_box->max_y += enemy->velocity.y * dt;
    enemy->collision_box->max_z += enemy->velocity.z * dt;
}
void initialize_enemies(enemy_manager_t *manager) {
    int num_per_type = manager->allocated_enemies / NUM_ENEMY_TYPES;
    size_t curr = 0;

    enemy_t *drone = get_obj(DRONE_PATHNAME,
                             (vec4_t){{{0, 0, 0, 1}}},
                             (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f},
                             0.03f, true, 0, 0, 0, false, ENEMY_TYPE);
    if (!drone) return;
    drone->speed = 1.0f;

    for (int type = 0; type < NUM_ENEMY_TYPES; type++) {
        for (int i = 0; i < num_per_type; i++) {
            if (type == DRONE_TYPE) {
                enemy_t *new_drone    = calloc(1, sizeof(enemy_t));
                mesh_t  *new_mesh     = calloc(1, sizeof(mesh_t));
                vec4_t  *local_verts  = calloc(drone->mesh->vertex_count, sizeof(vec4_t));
                vec4_t  *world_verts  = calloc(drone->mesh->vertex_count, sizeof(vec4_t));
                vec4_t  *camera_verts = calloc(drone->mesh->vertex_count, sizeof(vec4_t));
                collisionbox_t *collisionbox = calloc(1, sizeof(collisionbox_t));
                int      tc           = drone->mesh->triangle_count;
                int     *tri_map      = malloc(tc * 3 * sizeof(int));
                SDL_FColor *tri_colors = malloc(tc * sizeof(SDL_FColor));

                if (!new_drone || !new_mesh || !local_verts ||
                    !world_verts || !camera_verts || !collisionbox || !tri_map || !tri_colors) {
                    free(new_drone);
                    free(new_mesh);
                    free(local_verts);
                    free(world_verts);
                    free(camera_verts);
                    free(collisionbox);
                    free(tri_map);
                    free(tri_colors);
                    goto CLEANUP;
                }

                memcpy(local_verts, drone->mesh->local_verts,
                       drone->mesh->vertex_count * sizeof(vec4_t));
                memcpy(tri_map, drone->mesh->triangle_map,
                       tc * 3 * sizeof(int));
                memcpy(tri_colors, drone->mesh->triangle_colors,
                       tc * sizeof(SDL_FColor));

                *new_mesh = *drone->mesh;
                new_mesh->local_verts     = local_verts;
                new_mesh->world_verts     = world_verts;
                new_mesh->camera_verts    = camera_verts;
                new_mesh->triangle_map    = tri_map;
                new_mesh->triangle_colors = tri_colors;

                *new_drone             = *drone;
                new_drone->mesh        = new_mesh;
                new_drone->enemy_type  = type;
                new_drone->behaviour   = behaviour_drone;
                new_drone->speed       = drone->speed;
                new_drone->health      = drone->health;
                new_drone->mesh->triangle_count = drone->mesh->triangle_count;
                new_drone->collision_box = collisionbox;
                new_drone -> time_accumulator = 0.0f;
                new_drone->on_hit = on_hit_drone;
                manager->free_enemies[curr++] = new_drone;
            }
        }
    }
CLEANUP:
    free(drone->mesh->world_verts);
    free(drone->mesh->local_verts);
    free(drone->mesh->camera_verts);
    free(drone->mesh->triangle_map);
    free(drone->mesh->triangle_colors);
    free(drone->mesh);
    free(drone);
    manager->free_enemies_count = curr;
}

void remove_enemy(enemy_t *enemy, scene_t *scene) {
    remove_entity_by_ptr(scene, (void*)enemy, ENEMY_TYPE);
}
void transform_enemy_verts(enemy_t *enemy) {
    /*
     * TODO: refactor into a generic transform system.
     * See comment below -- a transform_t struct with a mesh pointer
     * would let this be parallelized the same way as the camera pipeline.
     */
    mat4_t id          = mat4_identity();
    mat4_t m_rot_z     = rot_z(&id,      enemy->angles.z, true);
    mat4_t m_rot_y     = rot_y(&m_rot_z, enemy->angles.y, true);
    mat4_t final_matrix = rot_x(&m_rot_y, enemy->angles.x, true);

    for (int i = 0; i < enemy->mesh->vertex_count; i++) {
        vec4_t rotated   = apply_transform(&final_matrix, &enemy->mesh->local_verts[i]);
        vec4_t world_pos = add_vec4(&rotated, &enemy->position);
        enemy->mesh->world_verts[i] = world_pos;
    }

    enemy->dirty = false;
}
void center_collisionbox(enemy_t* enemy, vec4_t pos){
    collisionbox_t *col = enemy->collision_box;
    col -> min_x += pos.x;
    col -> max_x += pos.x;
    col -> min_y += pos.y;
    col -> max_y += pos.y;
    col -> min_z += pos.z;
    col -> max_z += pos.z;
}
void add_enemy(int type_enemy, scene_t *scene, vec4_t position) {
    enemy_manager_t *manager = scene->enemy_manager;

    if (manager->num_enemies >= manager->allocated_enemies) return;
    if (manager->free_enemies_count == 0) return;

    /* Find a free enemy of the right type */
    for (size_t i = 0; i < manager->free_enemies_count; i++) {
        if (manager->free_enemies[i]->enemy_type == type_enemy) {
            enemy_t *enemy = manager->free_enemies[i];

            /* Swap with last free slot and pop */
            manager->free_enemies[i] = manager->free_enemies[--manager->free_enemies_count];

            enemy->position         = position;
            enemy->velocity         = (vec4_t){{{0, 0, 0, 0}}};
            enemy->time_accumulator = 0.0f;
            enemy->dirty            = true;
            manager->enemies[manager->num_enemies++] = enemy;
            transform_enemy_verts(enemy);
            center_collisionbox(enemy, position);
            add_object(scene, enemy, ENEMY_TYPE);
            return;
        }
    }
}



void update_enemy(enemy_t *enemy, real dt, camera_t *camera, scene_t *scene) {
    /* Move toward player in XZ plane only */
    vec4_t to_player = {{{
        camera->pos.x - enemy->position.x,
        0.0f,
        camera->pos.z - enemy->position.z,
        0.0f
    }}};

    /* Normalize direction */
    normalize_vec4(&to_player);
    if (get_distance(&camera->pos, &enemy->position) > 0.1f ) {
        enemy->velocity = scale_vec4(to_player, enemy->speed);
    } else {
        enemy->velocity = (vec4_t){{{0.0f, 0.0f, 0.0f, 0.0f}}};
    }
    enemy->velocity.y = 0.0f;

    enemy->position.x += enemy->velocity.x * dt;
    enemy->position.y += enemy->velocity.y * dt;
    enemy->position.z += enemy->velocity.z * dt;

    /* Only mark dirty if actually moving */
    real vel_sq = enemy->velocity.x * enemy->velocity.x +
                  enemy->velocity.z * enemy->velocity.z;
    enemy->dirty = (vel_sq > 0.0001f);

    enemy->behaviour(enemy, dt, camera, scene);

    if (enemy->dirty){
        transform_enemy_verts(enemy);
        move_collisionbox(enemy, dt);
    }
}

void update_enemies(scene_t *scene, real dt, camera_t *camera) {
    enemy_manager_t *manager = scene->enemy_manager;

    for (size_t i = 0; i < manager->num_enemies; i++) {
        enemy_t *enemy = manager->enemies[i];

        update_enemy(enemy, dt, camera, scene);
        if(enemy->collision_box->hit){
            enemy->on_hit(enemy, dt);
            enemy->collision_box->hit = false;
        }
        if (enemy->health <= 0.0f) {
            create_sparks(enemy->position, scene);
            remove_enemy(enemy, scene);
            i = i!=0?i-1:i;  /* compensate for swap-remove */
        }
    }
}
