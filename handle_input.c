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
vec4_t get_velocity_from_input(const input_state_t *input, camera_t *cam){
    vec4_t velocity = {0};
    if(input->up) {
       velocity.y = cam -> speed;
    }
    if(input->down) {
        velocity.y = -cam -> speed;
    }
    if(input->left) {
        velocity.x = cam -> speed;
    }
    if(input->right) {
        velocity.x = -cam -> speed;
    }
    if(input->forward){
        velocity.z = -cam -> speed;
    }
    if(input->backward){
        velocity.z = cam -> speed;
    }
    return velocity;
}

void handle_event_and_delta(long deltaTime, long *lastTime, bool *running,entity_t **entities, int entity_count, camera_t *cam){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_EVENT_QUIT){
            *running = false;
        }
        if(event.type == SDL_EVENT_MOUSE_MOTION){
            if(event.motion.state & SDL_BUTTON_LMASK){
                // Handle camera rotation here, we will need to pass the camera to this function, or we can just make the camera a global variable, which is not ideal but it is the easiest solution for now
                handle_camera_rotation(cam, &event);
            }
        }
    }
    const uint8_t *keyboard_state = (uint8_t*)SDL_GetKeyboardState(NULL);
    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        *running = false;
    }
    long currentTime = SDL_GetTicks();
    input_state_t input = get_input(keyboard_state);
    handle_camera_translation(cam, keyboard_state);

    if(currentTime - *lastTime >= deltaTime) {
        *lastTime = currentTime;    
        handle_camera_translation(cam, keyboard_state);
        update_entities(entities, entity_count);

        move_world_to_camera_space(cam, entities, entity_count);
    }
}