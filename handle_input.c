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
void handle_event_and_delta(long deltaTime, long *lastTime, bool *running,entity_t **entities, int entity_count, camera_t *cam){
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
    long currentTime = SDL_GetTicks();

    if(currentTime - *lastTime >= deltaTime) {
        *lastTime = currentTime;    
        update_entities(entities, entity_count);//this is fragile.Basically, move_world_to_camera_space "temporarily" corrupts the world state for the render to show.
        //Then, after the render is done, the world state is restored to its original state, which is a bit hacky but it works for now, and it allows us to avoid having to create a separate set of vertices for the camera space, which would be more memory intensive and more complex to manage, but if we want to optimize this in the future, we can definitely do that by creating a separate set of vertices for the camera space and then we can just copy the world vertices to the camera vertices and then apply the 
        //transformations to the camera vertices without affecting the world vertices, which would be a cleaner solution but it is more complex to implement, so for now we will stick with this solution and then we can optimize it later if we need to
        //update_grounded(cam, entities, entity_count);
        handle_camera_translation(cam, keyboard_state);

        move_world_to_camera_space(cam, entities, entity_count);
    }
}