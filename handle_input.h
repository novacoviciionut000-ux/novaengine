#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#include <SDL3/SDL.h>
#include "alg.h"
#include "defines.h"
#include "entities.h"
#include <stdbool.h>

typedef struct{
    bool up, down, left, right, forward, backward;

}input_state_t;
void get_velocity_from_input(const input_state_t *input, entity_t *ent, long deltaTime);
void handle_event_and_delta(long deltaTime, long *lastTime, bool *running,entity_t **entities, int entity_count);
input_state_t get_input(const uint8_t *keyboard_state);

#endif 