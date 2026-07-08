#include "collisions.h"

bool check_collision(collisionbox_t* box, camera_t *cam) {
    if (box == NULL) return false;

    real r = cam->radius;

    // Expand the box by camera radius
    real min_x = box->min_x - r;
    real max_x = box->max_x + r;
    real min_y = box->min_y - r;
    real max_y = box->max_y + r;
    real min_z = box->min_z - r;
    real max_z = box->max_z + r;

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
bool check_bullet_collision(particle_t *bullet, collision_manager_t *manager) {
    if(bullet->type_particle != PARTICLE_BULLET)return false;
    for (size_t i = 0; i < manager->num_boxes; i++) {
        collisionbox_t *box = manager->collision_boxes[i];
        if(box == NULL) continue;
        // 1. Find the closest point on the AABB to the bullet center
        // We "clamp" the bullet's position to the box boundaries
        real closest_x = fmaxf(box->min_x, fminf(bullet->position.x, box->max_x));
        real closest_y = fmaxf(box->min_y, fminf(bullet->position.y, box->max_y));
        real closest_z = fmaxf(box->min_z, fminf(bullet->position.z, box->max_z));

        // 2. Calculate the squared distance between the bullet and this closest point
        real dx = bullet->position.x - closest_x;
        real dy = bullet->position.y - closest_y;
        real dz = bullet->position.z - closest_z;

        real distance_sq = (dx * dx) + (dy * dy) + (dz * dz);

        // 3. If distance is less than radius (squared for speed), it's a hit
        if (distance_sq < (bullet->collision_radius * bullet->collision_radius)) {
            box -> hit = true;
            return true;
        }
    }
    return false;
}
void handle_cam_collision(camera_t *cam, collisionbox_t *collisionbox) {
    real r = cam->radius;

    // 1. Get the expanded bounds (Minkowski Sum)
    real min_x = collisionbox->min_x - r;
    real max_x = collisionbox->max_x + r;
    real min_y = collisionbox->min_y - r;
    real max_y = collisionbox->max_y + r;
    real min_z = collisionbox->min_z - r;
    real max_z = collisionbox->max_z + r;

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

void handle_and_check_collision(collision_manager_t *manager, camera_t *cam) {
    real check_treshold = 50.0f;// this is to be swapped for the "beefiest" collisionbox in the game
    for (size_t i = 0; i < manager->num_boxes; i++) {
        if(manager->collision_boxes[i] == NULL)continue;
        real colx = (manager->collision_boxes[i]->min_x + manager->collision_boxes[i]->max_x)/2;
        real coly = (manager->collision_boxes[i]->min_y + manager->collision_boxes[i]->max_y)/2;
        real colz = (manager->collision_boxes[i]->min_z + manager->collision_boxes[i]->max_z)/2;
        real dx = colx - cam->pos.x;//this is an optimization.Basically, if the entity is far away, there is no point in checking it's collision
        real dy = coly - cam->pos.y;
        real dz = colz - cam->pos.z;
        real dist_sq = (dx * dx) + (dy * dy) + (dz * dz);
        if(dist_sq > check_treshold)continue;
        if (check_collision(manager->collision_boxes[i], cam)) {
            handle_cam_collision(cam, manager->collision_boxes[i]);
        }
    }
}
collisionbox_t get_entity_collisionbox(mesh_t *mesh){
    real min_y = mesh->world_verts[0].y;
    real max_y = mesh->world_verts[0].y;
    real min_x = mesh->world_verts[0].x;
    real max_x = mesh->world_verts[0].x;
    real min_z = mesh->world_verts[0].z;
    real max_z = mesh->world_verts[0].z;

    for(int i = 1; i < mesh->vertex_count; i++){
        min_y = mesh->world_verts[i].y < min_y ? mesh->world_verts[i].y : min_y;
        max_y = mesh->world_verts[i].y > max_y ? mesh->world_verts[i].y : max_y;
        min_x = mesh->world_verts[i].x < min_x ? mesh->world_verts[i].x : min_x;
        max_x = mesh->world_verts[i].x > max_x ? mesh->world_verts[i].x : max_x;
        min_z = mesh->world_verts[i].z < min_z ? mesh->world_verts[i].z : min_z;
        max_z = mesh->world_verts[i].z > max_z ? mesh->world_verts[i].z : max_z;
    }

    collisionbox_t col = (collisionbox_t){min_x, max_x, min_y, max_y, min_z, max_z, false};
    return col;
}
