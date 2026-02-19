#include "handle_input.h"
#include "camera.h"
camera_t* create_camera(vec4_t pos, eulerangles_t angles, real speed, real angular_speed){
    camera_t* cam = malloc(sizeof(camera_t));
    if(!cam){
        return NULL;
    }
    cam->pos = pos;
    cam->speed = speed;
    cam->angular_speed = angular_speed;
    cam->angles = angles;
    return cam;
}
void handle_camera_rotation(camera_t *cam, const SDL_Event *event){
    if(event->type == SDL_EVENT_MOUSE_MOTION){
        cam->angles.y += event->motion.xrel * SENSITIVITY;
        //cam->angles.x += event->motion.yrel * SENSITIVITY;
        // Clamp the pitch to prevent flipping
        if (cam->angles.x > M_PIdiv2) {
            cam->angles.x = M_PIdiv2;
        }
        if (cam->angles.x < -M_PIdiv2) {
            cam->angles.x = -M_PIdiv2;
        }
        
    }
}
void handle_camera_translation(camera_t *cam, const uint8_t *keyboard_state){
    vec4_t velocity = {0};
    input_state_t input = get_input(keyboard_state);
    velocity = get_velocity_from_input(&input, cam);
    cam->pos = add_vec4(&cam->pos, &velocity);
}
void move_world_to_camera_space(const camera_t *cam, entity_t **entities, int entity_count){
    mat4_t rotation = mat4_identity();
    rotation = rot_y(&rotation, -cam->angles.y, true);
    //rotation = rot_x(&rotation, -cam->angles.x, true);
    for(int i = 0; i < entity_count;i++){
        for(int j = 0; j < entities[i]->mesh->vertex_count; j++){

            // Translate the world vertex by the inverse of the camera position
            vec4_t translated = add_vec4(&entities[i]->mesh->world_verts[j], &(vec4_t){.x=-cam->pos.x, .y=-cam->pos.y, .z=-cam->pos.z, .w=0});
            // Rotate the translated vertex by the inverse of the camera rotation
            entities[i]->mesh->world_verts[j] = apply_transform(&rotation, &translated);
        }
    }

}

