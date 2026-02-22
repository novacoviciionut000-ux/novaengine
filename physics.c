#include "physics.h"

#define gravity 1.2f
void apply_impulse(entity_t *entity, vec4_t impulse) {
    // Delta V = Impulse / Mass
    vec4_t delta_v = scale_vec4(impulse, 1.0f / entity->mass);
    entity->velocity = add_vec4(&entity->velocity, &delta_v);
}
void apply_force(entity_t *entity, vec4_t force) {
    // F = ma -> a = F/m. For simplicity, we can just add to acceleration
    entity->acceleration = add_vec4(&entity->acceleration, &force);
}
void apply_impulse_to_camera(camera_t *cam, vec4_t impulse) {
    if (cam->mass <= 0.0f) return; // Prevent division by zero

    // Calculate Delta V: I / m
    vec4_t delta_v;
    delta_v.x = impulse.x / cam->mass;
    delta_v.y = impulse.y / cam->mass;
    delta_v.z = impulse.z / cam->mass;

    // Add Delta V to current velocity
    cam->velocity.x += delta_v.x;
    cam->velocity.y += delta_v.y;
    cam->velocity.z += delta_v.z;
}
void camera_jump(camera_t *cam, real jump_power) {
    cam->velocity.y = jump_power;
}

void apply_camera_gravity(camera_t *cam, real gravity_pull, real dt) {
    // 1. If we are "in the air" (Y is negative) or moving "up" (velocity is negative)
    if (cam->pos.y < 0.0f || cam->velocity.y < 0.0f) {
        
        // Apply gravity (positive pulls DOWN toward 0)
        cam->velocity.y += gravity_pull * dt;

        // TERMINAL VELOCITY CLAMP: Prevent the "teleport" bug
        // Don't let the camera fall or rise faster than 50 units/frame
        if (cam->velocity.y > 50.0f) cam->velocity.y = 50.0f;
        if (cam->velocity.y < -50.0f) cam->velocity.y = -50.0f;
    }

    // 2. FLOOR RESOLUTION
    if (cam->pos.y >= 0.0f) {
        cam->pos.y = 0.0f;
        
        // Only kill velocity if we are actually hitting the floor (moving positive)
        if (cam->velocity.y > 0.0f) {
            cam->velocity.y = 0.0f;
        }
    }
}
void update_entity_physics(entity_t *entity, float dt) {
    // 1. Accumulate Constant Forces (Gravity)
    if (entity->pos.y > 0) {//to be replaced by a grounded flag
        vec4_t g_force = {{{0, -9.81f * entity->mass, 0, 0}}};
        apply_force(entity, g_force);
    }

    // 3. Integrate Acceleration: a = F / m
    vec4_t acc = scale_vec4(entity->force_accumulator, 1.0f / entity->mass);
    
    // 4. Integrate Velocity: v = v + a * dt
    vec4_t tmp = scale_vec4(acc, dt);
    entity->velocity = add_vec4(&entity->velocity, &tmp);

    // 5. Integrate Position: p = p + v * dt
    vec4_t tmp2 = scale_vec4(entity->velocity, dt);
    entity->pos = add_vec4(&entity->pos, &tmp2);

    // 6. Reset Forces for next frame
    entity->force_accumulator = (vec4_t){0, 0, 0, 0};
}
void update_scene_physics(scene_t *scene, float dt,camera_t *cam){
    //for(int i = 0; i<scene->numentities;i++){
        //update_entity_physics(scene->entities[i], dt);
    //}
    apply_camera_gravity(cam, gravity, dt);

}