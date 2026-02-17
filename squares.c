#include "squares.h"
void add_velocity(square_t *sq, vec4_t velocity) {
    sq->pos = add_vec4(&sq->pos, &velocity);
}
void update_square(square_t *sq, const Uint8 *keyboard_state) {
    // Update based on keyboard state
    if(keyboard_state[SDL_SCANCODE_UP]) {
        vec4_t velocity = {0, -vel, 0, 0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_DOWN]) {
        vec4_t velocity = {0, vel, 0, 0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_LEFT]) {
        vec4_t velocity = {-vel, 0, 0, 0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_RIGHT]) {
        vec4_t velocity = {vel, 0, 0, 0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_W]) {
        vec4_t velocity = {0, 0, -vel, 0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_S]) {
        vec4_t velocity = {0, 0, vel, 0};
        add_velocity(sq, velocity); 
    }
}
void render_square(const square_t *sq, SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    double x_pos = (sq->pos.x / sq->pos.z)*sq->pos.w;
    double y_pos = (sq->pos.y / sq->pos.z)*sq->pos.w;
    
    SDL_FRect rect = {x_pos , y_pos, (sq->length/sq->pos.z)*sq->pos.w, (sq->width/sq->pos.z)*sq->pos.w};
    SDL_RenderFillRect(renderer, &rect);
}