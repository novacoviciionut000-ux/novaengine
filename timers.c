#include "timers.h"

delta_timer* initialize_time(){
    delta_timer *timer = calloc(1, sizeof(delta_timer));
    if(!timer)return NULL;
    uint64_t start = SDL_GetPerformanceCounter();//basically, this is the "first time", all upcoming delta_time() calls will update this value.
    uint64_t freq = SDL_GetPerformanceFrequency();//the frequency does not change, ever.
    timer->start = start;
    timer->freq = freq;
    return timer;
}

real delta_time(delta_timer *timer){
    uint64_t curr_time = SDL_GetPerformanceCounter(); 
    uint64_t elapsed = curr_time - timer->start;
    real seconds = (real)((double)elapsed / (double)timer->freq);
    timer->start = curr_time;
    return seconds;
}

real_timer* initialize_clock(){
    real_timer* time = calloc(1, sizeof(real_timer));
    if(time == NULL)return NULL;
    time -> timer = initialize_time();
    if(time -> timer == NULL)return NULL;
    time -> curr_time = 0;
    return time;

}

real get_time(real_timer* clock){
    clock -> curr_time += (double)delta_time(clock->timer);
    return clock->curr_time;

}
void destroy_clock(real_timer* clock){
    free(clock -> timer);
    free(clock);
}