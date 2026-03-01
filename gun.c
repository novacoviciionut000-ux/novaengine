#include "gun.h"
#define BOB_SPEED 10.0f
#define BOB_AMOUNT 0.3f
#define RECOIL_SPEED 3.0f
#define RECOIL_DAMPING 1.0f
#define RECOIL_AMOUNT 0.1f
entity_t* init_gun(char *pathname){
    entity_t* gun = (entity_t*)get_obj(pathname, (vec4_t){{{0,0,0,0}}}, (SDL_FColor){0.5f, 0.1f, 0.2f,1.0f}, 0.01f, false, M_PI, -M_PI/2.0f,0, true, ENTITY_TYPE);
    if(!gun) return NULL;

    return gun;
}
void move_gun(entity_t *gun, vec4_t *offset) {
    for(int i = 0; i < gun->mesh->vertex_count; i++) {
        gun->mesh->camera_verts[i] = add_vec4(&gun->mesh->local_verts[i], offset);
    }
}
void follow_camera(entity_t *gun, camera_t *cam, real time, real dt, gun_state_t *gun_state) {
    real speed = sqrtf(cam->velocity.x * cam->velocity.x + cam->velocity.z * cam->velocity.z);
    real normalized_speed = fminf(speed, 1.0f);
    real bob_y = sinf(time * BOB_SPEED) * BOB_AMOUNT * normalized_speed;
    real bob_x = sinf(time * BOB_SPEED * 0.5f) * BOB_AMOUNT * normalized_speed;

    // Recoil — dampened sine over [0, π] so it starts and ends at 0
    real recoil_z = 0.0f;
    if (gun_state->recoiling) {
        gun_state->recoil_timer += dt * RECOIL_SPEED;  // RECOIL_SPEED controls duration
        recoil_z = sinf(gun_state->recoil_timer) * expf(-gun_state->recoil_timer * RECOIL_DAMPING) * RECOIL_AMOUNT;
        if (gun_state->recoil_timer >= M_PI)
            gun_state->recoiling = false;
    }

    vec4_t local_offset = {{{bob_x + 0.04f, 0.05f + bob_y - recoil_z * 0.5, 0.06f - recoil_z, 0.0f}}};
    move_gun(gun, &local_offset);
    gun->dirty = false;
}
