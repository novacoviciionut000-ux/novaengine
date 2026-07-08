#ifndef TIMERS_H
#define TIMERS_H
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include "defines.h"

real_timer* initialize_clock();
real delta_time(delta_timer *timer);
real get_time(real_timer* clock);
delta_timer* initialize_time();
void destroy_clock(real_timer* clock);
#endif