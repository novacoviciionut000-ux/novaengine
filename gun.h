#ifndef GUN_H
#define GUN_H

#include "defines.h"
#include "obj_parsing.h"

void follow_camera(entity_t *gun, camera_t *cam, real time, real dt, gun_state_t *gun_state);
entity_t* init_gun(char *pathname);

#endif
