#ifndef COLLISIONS_H
#define COLLISIONS_H
#include "entities.h"
#include "scenes.h"
#include <math.h>
#include "camera.h"
#include "defines.h"
void handle_cam_collision(camera_t *cam, entity_t *entity);
bool check_collision(entity_t *entity, camera_t *cam);
void handle_and_check_collision(scene_t *scene, camera_t *cam);
collisionbox_t get_entity_collisionbox(entity_t *entity);
#endif