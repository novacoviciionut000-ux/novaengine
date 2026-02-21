#include "handle_input.h"
#include "camera.h"
input_state_t get_input(const uint8_t *keyboard_state){
    input_state_t input = {0};
    input.up = keyboard_state[SDL_SCANCODE_UP];
    input.down = keyboard_state[SDL_SCANCODE_DOWN];
    input.left = keyboard_state[SDL_SCANCODE_A];
    input.right = keyboard_state[SDL_SCANCODE_D];
    input.forward = keyboard_state[SDL_SCANCODE_W];
    input.backward = keyboard_state[SDL_SCANCODE_S];
    return input;
}
vec4_t get_velocity_from_input(const input_state_t *input, camera_t *cam) {
    vec4_t velocity = {0};
    real yaw = -cam->angles.y;

    float forward_x = -sinf(yaw); 
    float forward_z = cosf(yaw);

    float right_x = cosf(yaw);
    float right_z = sinf(yaw);
    // W and S: Move along the forward vector
    // check if the camera is grounded on the x-axis
    if(!((cam -> grounded) & 1) && !((cam -> grounded) & 0x2)){
        if (input->forward) {
            velocity.x += forward_x * cam->speed;
            velocity.z += forward_z * cam->speed;
        }
        if (input->backward) {
            velocity.x -= forward_x * cam->speed;
            velocity.z -= forward_z * cam->speed;
        }

        // A and D: Move along the right vector
        if (input->right) {
            velocity.x += right_x * cam->speed;
            velocity.z += right_z * cam->speed;
        }
        if (input->left) {
            velocity.x -= right_x * cam->speed;
            velocity.z -= right_z * cam->speed;
        }
    }

    return velocity;
}
//VERY IMPORTANT FUNCTION BELOW !!!!! This basically handles the entire game interactivity, it handles the input, it handles the camera movement, it handles the entity updates, it handles the world to camera space transformation, it basically does everything that is needed for the game to run
void handle_event_and_delta(long deltaTime, bool *running,entity_t **entities, int entity_count, camera_t *cam, real dt){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_EVENT_QUIT){
            *running = false;
        }
        if(event.type == SDL_EVENT_MOUSE_MOTION){

            // Handle camera rotation here, we will need to pass the camera to this function, or we can just make the camera a global variable, which is not ideal but it is the easiest solution for now
             handle_camera_rotation(cam, &event);
            
        }
    }
    const uint8_t *keyboard_state = (uint8_t*)SDL_GetKeyboardState(NULL);
    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        *running = false;
    }

    update_entities(entities, entity_count, dt);
    handle_camera_translation(cam, keyboard_state, dt);

    move_world_to_camera_space(cam, entities, entity_count);
    
}