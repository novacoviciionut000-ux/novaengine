#include "collisions.h"

bool check_collision(entity_t *entity, camera_t *cam) {
    if (entity->collision_box == NULL) return false;
    
    real r = cam->radius;

    // Expand the box by camera radius
    real min_x = entity->collision_box->min_x - r;
    real max_x = entity->collision_box->max_x + r;
    real min_y = entity->collision_box->min_y - r;
    real max_y = entity->collision_box->max_y + r;
    real min_z = entity->collision_box->min_z - r;
    real max_z = entity->collision_box->max_z + r;

    real px = cam->pos.x;
    real py = cam->pos.y;
    real pz = cam->pos.z;

    // Check if camera point is inside expanded AABB
    if ((px >= min_x && px <= max_x) && 
        (py >= min_y && py <= max_y) && 
        (pz >= min_z && pz <= max_z)) 
    {
        return true;
    }
    
    return false;
}

void handle_cam_collision(camera_t *cam, entity_t *entity) {
    real r = cam->radius;

    // 1. Get the expanded bounds (Minkowski Sum)
    real min_x = entity->collision_box->min_x - r;
    real max_x = entity->collision_box->max_x + r;
    real min_y = entity->collision_box->min_y - r;
    real max_y = entity->collision_box->max_y + r;
    real min_z = entity->collision_box->min_z - r;
    real max_z = entity->collision_box->max_z + r;

    // 2. Calculate distances to each face
    real overlap_left   = cam->pos.x - min_x;
    real overlap_right  = max_x - cam->pos.x;
    real overlap_bottom = cam->pos.y - min_y;
    real overlap_top    = max_y - cam->pos.y;
    real overlap_back   = cam->pos.z - min_z;
    real overlap_front  = max_z - cam->pos.z;

    // 3. Find the smallest overlap (The MTV)
    real min_overlap = overlap_left;
    int axis = 0;   // 0:x, 1:y, 2:z
    int side = 1;  // Direction to push

    if (overlap_right < min_overlap)  { min_overlap = overlap_right;  axis = 0; side = 1;  }
    if (overlap_bottom < min_overlap) { min_overlap = overlap_bottom; axis = 1; side = -1; }
    if (overlap_top < min_overlap)    { min_overlap = overlap_top;    axis = 1; side = 1;  }
    if (overlap_back < min_overlap)   { min_overlap = overlap_back;   axis = 2; side = -1; }
    if (overlap_front < min_overlap)  { min_overlap = overlap_front;  axis = 2; side = 1;  }

    // 4. Push the camera back by the smallest overlap
    if (axis == 0)      cam->pos.x += (side == 1) ? min_overlap : -min_overlap;
    else if (axis == 1) cam->pos.y += (side == 1) ? min_overlap : -min_overlap;
    else if (axis == 2) cam->pos.z += (side == 1) ? min_overlap : -min_overlap;
}

void handle_and_check_collision(scene_t *scene, camera_t *cam) {
    real check_treshold = 5.0f;
    for (int i = 0; i < scene->numentities; i++) {
        real dx = scene->entities[i]->pos.x - cam->pos.x;
        real dy = scene->entities[i]->pos.y - cam->pos.y;
        real dz = scene->entities[i]->pos.z - cam->pos.z;
        real dist_sq = (dx * dx) + (dy * dy) + (dz * dz);
        if(dist_sq > check_treshold)continue;
        if (check_collision(scene->entities[i], cam)) {
            handle_cam_collision(cam, scene->entities[i]);
        }
    }
}
collisionbox_t get_entity_collisionbox(entity_t *entity){
    real min_y = entity->mesh->world_verts[0].y;
    real max_y = entity->mesh->world_verts[0].y;
    real min_x = entity->mesh->world_verts[0].x;
    real max_x = entity->mesh->world_verts[0].x;
    real min_z = entity->mesh->world_verts[0].z;
    real max_z = entity->mesh->world_verts[0].z;

    for(int i = 1; i < entity->mesh->vertex_count; i++){
        min_y = entity->mesh->world_verts[i].y < min_y ? entity->mesh->world_verts[i].y : min_y;
        max_y = entity->mesh->world_verts[i].y > max_y ? entity->mesh->world_verts[i].y : max_y;
        min_x = entity->mesh->world_verts[i].x < min_x ? entity->mesh->world_verts[i].x : min_x;
        max_x = entity->mesh->world_verts[i].x > max_x ? entity->mesh->world_verts[i].x : max_x;
        min_z = entity->mesh->world_verts[i].z < min_z ? entity->mesh->world_verts[i].z : min_z;
        max_z = entity->mesh->world_verts[i].z > max_z ? entity->mesh->world_verts[i].z : max_z;
    }

    collisionbox_t col = (collisionbox_t){min_x, max_x, min_y, max_y, min_z, max_z};
    return col;
}