#include "handle_input.h"

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
void apply_input_to_camera(const input_state_t *input, camera_t *cam) {
    real yaw = -cam->angles.y;

    float forward_x = -sinf(yaw);
    float forward_z = cosf(yaw);

    float right_x = cosf(yaw);
    float right_z = sinf(yaw);

    // Use temporary variables for horizontal movement
    float target_x = 0;
    float target_z = 0;


        // W and S: Move along the forward vector
        if (input->forward) {
            target_x += forward_x * cam->speed;
            target_z += forward_z * cam->speed;
        }
        if (input->backward) {
            target_x -= forward_x * cam->speed;
            target_z -= forward_z * cam->speed;
        }

        // A and D: Move along the right vector
        if (input->right) {
            target_x += right_x * cam->speed;
            target_z += right_z * cam->speed;
        }
        if (input->left) {
            target_x -= right_x * cam->speed;
            target_z -= right_z * cam->speed;
        }


    // Apply ONLY to X and Z, leaving Y completely untouched
    cam->velocity.x = target_x;
    cam->velocity.z = target_z;
}
//VERY IMPORTANT FUNCTION BELOW !!!!! This basically handles the entire game interactivity, it handles the input, it handles the camera movement, it handles the entity updates, it handles the world to camera space transformation, it basically does everything that is needed for the game to run
void handle_event_and_delta(bool *running,scene_t *scene, real dt,entity_t *gun, real time, gun_state_t *gun_state){
    SDL_Event event;
    camera_t* cam = scene->camera_manager->camera;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_EVENT_QUIT){
            *running = false;
        }
        if(event.type == SDL_EVENT_MOUSE_MOTION){

            // Handle camera rotation here, we will need to pass the camera to this function, or we can just make the camera a global variable, which is not ideal but it is the easiest solution for now
            handle_camera_rotation(cam, &event);

        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            printf("Mouse button: %d\n", event.button.button);

            if (event.button.button == SDL_BUTTON_LEFT) {
                if(gun_state -> cooldown_timer < 0.01f){
                    release_bullet(scene);
                    gun_state->recoiling = true;
                    gun_state->recoil_timer = 0.0f;
                    gun_state -> cooldown_timer = 0.03f;
                }else{
                    gun_state -> cooldown_timer -= dt;
                }

            }
        }
    }

    const uint8_t *keyboard_state = (uint8_t*)SDL_GetKeyboardState(NULL);
    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        *running = false;
    }
    if(keyboard_state[SDL_SCANCODE_SPACE]){
        camera_jump(cam, -0.15f);
    }

    if(keyboard_state[SDL_SCANCODE_LSHIFT]){//!!!!TO BE SWAPPED FOR A SEPARATE FUNCTION, MAYBE SEPARATE HEADER (MOVEMENT.C) LATER IN DEV
        if(!cam->sprinting){
            cam->speed *= 3;


            cam->sprinting = true;

        }else{
            if(cam->focal_length >= 200)cam->focal_length-=200 * dt;
            if(cam -> focal_length < 200)cam->focal_length = 200;
        }
    }else{
        if(cam->sprinting){
            cam->speed /=3;
            cam->sprinting = false;

        }else{
            if(cam->focal_length <= 400)cam->focal_length+=200 * dt;
            if(cam->focal_length > 400)cam->focal_length = 400;
        }


    }
    handle_camera_translation(cam, keyboard_state, dt);
    transform_vertices(scene->camera_manager, scene->render_manager);  // pipeline.c
    update_scene_physics(scene->entity_manager, dt, cam);
    update_entities(scene->entity_manager->entities, scene->entity_manager->numentities, dt);
    follow_camera(gun, cam,time, dt,gun_state);

}
