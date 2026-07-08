#include "handle_input.h"
#include "camera.h"

camera_t* create_camera(vec4_t pos, eulerangles_t angles, real speed, real angular_speed) {
    camera_t* cam = calloc(1, sizeof(camera_t));
    if(!cam) return NULL;
    cam->pos           = pos;
    cam->speed         = speed;
    cam->angular_speed = angular_speed;
    cam->angles        = angles;
    cam->radius        = 0.03f;
    cam->sprinting     = false;
    cam->focal_length  = 400;
    cam->mass          = 5.0f;
    cam->velocity      = (vec4_t){{{0,0,0,0}}};
    cam -> health = 100;
    cam->grounded      = 0;
    return cam;
}

void handle_camera_rotation(camera_t *cam, const SDL_Event *event) {
    if(event->type == SDL_EVENT_MOUSE_MOTION) {
        cam->angles.y += event->motion.xrel * SENSITIVITY;
        cam->angles.x += event->motion.yrel * SENSITIVITY;
        cam->angles.x = cam->angles.x >  M_PIdiv2 ?  M_PIdiv2 : cam->angles.x;//Gimbal lock prevention
        cam->angles.x = cam->angles.x < -M_PIdiv2 ? -M_PIdiv2 : cam->angles.x;
    }
}
void draw_damage_vignette(SDL_Renderer *renderer, camera_t *camera, real max_health) {
    real hp_ratio = camera->health / max_health;
    if (hp_ratio >= 1.0f) return;  // full health, skip

    uint8_t alpha = (uint8_t)((1.0f - hp_ratio) * 60.0f);  // max 180/255 alpha

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, alpha);
    SDL_RenderFillRect(renderer, NULL);  // NULL = full screen
}
vec4_t get_camera_velocity(camera_t *cam, const uint8_t *keyboard_state, real dt) {
    input_state_t input = get_input(keyboard_state);
    apply_input_to_camera(&input, cam);
    return scale_vec4(cam->velocity, dt * WORLD_SCALE_FACTOR);
}

void handle_camera_translation(camera_t *cam, const uint8_t *keyboard_state, real dt) {
    vec4_t camera_vel = get_camera_velocity(cam, keyboard_state, dt);
    cam->pos = add_vec4(&cam->pos, &camera_vel);
}
