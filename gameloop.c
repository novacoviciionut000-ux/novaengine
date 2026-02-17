#include "gameloop.h"
#include "squares.h"

void initializeGame(SDL_Window** window, SDL_Renderer** renderer) {

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL could not initialize: %s\n", SDL_GetError());
        exit(1);
    }

    *window = SDL_CreateWindow("Game Loop", 640, 480, 0);
    if (*window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        exit(1);
    }
    *renderer = SDL_CreateRenderer(*window, NULL);
    if (*renderer == NULL) {
        printf("Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        exit(1);
    }
}
void cleanUp(SDL_Window* window, SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
square_t create_square(vec4_t pos, double length, double width) {
    square_t sq;
    sq.pos = pos;
    sq.length = length;
    sq.width = width;
    return sq;
}
void gameLoop(){
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    initializeGame(&window, &renderer);
    bool running = true;
    vec4_t position = {320, 240, 10, 45}; // x, y, z, focal_length
    square_t sq = create_square(position, 50, 50);
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT){
                running = false;
            }
        }
        const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
        const long deltaTime = 100;
        long lastTime = 0;
        long currentTime = SDL_GetTicks();
        if(currentTime - lastTime >= deltaTime) {
            lastTime = currentTime;
            update_square(&sq, keyboard_state);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        render_square(&sq, renderer);


        // Game rendering logic goes here
        SDL_RenderPresent(renderer);


    }

    cleanUp(window, renderer);


}