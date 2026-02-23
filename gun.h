#ifndef GUN_H
#define GUN_H

#include "defines.h"
#include "obj_parsing.h"

void follow_camera(entity_t *gun, camera_t *cam);
entity_t* init_gun(char *pathname);

#endif