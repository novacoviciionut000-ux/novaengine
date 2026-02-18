#include "gameloop.h"
#include "squares.h"

bool initializeGame(SDL_Window** window, SDL_Renderer** renderer) {

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL could not initialize: %s\n", SDL_GetError());
        return false;
    }

    *window = SDL_CreateWindow("Game Loop", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (*window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        return false;
    }
    *renderer = SDL_CreateRenderer(*window, NULL);
    if (*renderer == NULL) {
        printf("Could not create renderer: %s\n", SDL_GetError());
        return false;
    }
    return true;
}
void cleanUp(SDL_Window* window, SDL_Renderer* renderer) {
    if(renderer)SDL_DestroyRenderer(renderer);
    if(window)SDL_DestroyWindow(window);
    SDL_Quit();
}
square_t create_square(vec4_t pos, real length, real width) {
    square_t sq;
    sq.pos = pos;
    sq.length = length;
    sq.width = width;
    return sq;
}
void handle_event_and_delta(long deltaTime, long *lastTime, bool *running, cube_t *myCube){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_EVENT_QUIT){
            *running = false;
        }
    }
    const uint8_t *keyboard_state = (uint8_t*)SDL_GetKeyboardState(NULL);
    long currentTime = SDL_GetTicks();
    if(currentTime - *lastTime >= deltaTime) {
        *lastTime = currentTime;
        update_cube(myCube, keyboard_state);
    }
}
void gameLoop(){
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if(!initializeGame(&window, &renderer)) {
        printf("Failed to initialize game\n");
        goto CLEANUP;
    }
    bool running = true;
    cube_t* myCube = create_cube((vec4_t){.x=0, .y=0, .z=400, .w=0}, 300.0, 300.0, 300.0);
    if(!myCube){
        printf("Failed to create cube\n");
        goto CLEANUP;
    }
    const long deltaTime = 5;
    long lastTime = SDL_GetTicks();
    while(running){
        handle_event_and_delta(deltaTime, &lastTime, &running, myCube);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        render_cube(myCube, renderer);


        // Game rendering logic goes here
        SDL_RenderPresent(renderer);


    }
    CLEANUP:
    if(myCube){
        free(myCube);
    }
    cleanUp(window, renderer);




}