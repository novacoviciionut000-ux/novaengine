#ifndef COLLISIONS_H
#define COLLISIONS_H
#include "entities.h"
#include "scenes.h"
#include <math.h>
#include "camera.h"
#include "defines.h"
void handle_cam_collision(camera_t *cam, collisionbox_t *box);
bool check_collision(collisionbox_t *box, camera_t *cam);
void handle_and_check_collision(collision_manager_t *manager, camera_t *cam);
collisionbox_t get_entity_collisionbox(mesh_t *mesh);
bool check_bullet_collision(particle_t *bullet, collision_manager_t *manager);
#endif
