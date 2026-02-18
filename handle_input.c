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
void get_velocity_from_input(const input_state_t *input, entity_t *ent, long deltaTime){
    ent->velocity = (vec4_t){{{0,0,0,0}}};
    if(input->up) {
        ent -> velocity.y = (ent -> speed) * deltaTime;
    }
    if(input->down) {
        ent -> velocity.y = -(ent -> speed) * deltaTime;
    }
    if(input->left) {
        ent -> velocity.x = (ent -> speed) * deltaTime;
    }
    if(input->right) {
        ent -> velocity.x = -(ent -> speed) * deltaTime;
    }
    if(input->forward){
        ent -> velocity.z = (-ent -> speed) * deltaTime;
    }
    if(input->backward){
        ent -> velocity.z = (ent -> speed) * deltaTime;
    }
}

void handle_event_and_delta(long deltaTime, long *lastTime, bool *running,entity_t **entities, int entity_count){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_EVENT_QUIT){
            *running = false;
        }
        if(event.type == SDL_EVENT_MOUSE_MOTION){
            if(event.motion.state & SDL_BUTTON_LMASK){
                for(int i = 0;i < entity_count;i++){
                    entities[i]->angles.y += event.motion.xrel * SENSITIVITY;
                    entities[i]->angles.x += event.motion.yrel * SENSITIVITY;
                    if (entities[i]->angles.x < -1.5f) entities[i]->angles.x = -1.5f;
                    if (entities[i]->angles.x > 1.5f) entities[i]->angles.x = 1.5f;
                }
            }
        }
    }
    const uint8_t *keyboard_state = (uint8_t*)SDL_GetKeyboardState(NULL);
    long currentTime = SDL_GetTicks();
    input_state_t input = get_input(keyboard_state);


    if(currentTime - *lastTime >= deltaTime) {
        *lastTime = currentTime;    
        for(int i = 0; i < entity_count; i++){
            get_velocity_from_input(&input, entities[i], deltaTime);
        }

    }
    update_entities(entities, entity_count);
}