#include "gun.h"


entity_t* init_gun(char *pathname){
    entity_t* gun = get_obj(pathname, (vec4_t){{{0,0,0,0}}}, (SDL_FColor){0.5f, 0.1f, 0.2f,1.0f}, 0.01f, false, M_PI, -M_PI/2.0f,0);
    
    // 1. Move the check here!
    if(!gun) return NULL; 

    // 2. Now it's safe to touch the pointer
    return gun;
}  
void follow_camera(entity_t *gun, camera_t *cam) {
    // 1. Your perfectly dialed-in rotation
    gun->angles.x = -cam->angles.x;
    gun->angles.y = cam->angles.y;
    gun->angles.z = 0.0f;

    mat4_t cam_rot = mat4_identity();
    
    cam_rot = rot_y(&cam_rot, cam->angles.y, true); 
    cam_rot = rot_x(&cam_rot, cam->angles.x, true);
    vec4_t local_offset = { 0.03f, 0.05f, 0.075f, 0.0f}; 
    
    // 4. Transform and apply
    vec4_t rotated_offset = apply_transform(&cam_rot, &local_offset);
    vec4_t camera_position = cam->pos;
    gun->pos = add_vec4(&camera_position, &rotated_offset);
    
    gun->dirty = true;
}