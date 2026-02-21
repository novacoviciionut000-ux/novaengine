#include "gameloop.h"
#define EXPECTED_ENTITY_COUNT 15
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
    SDL_SetWindowRelativeMouseMode(*window, true);
    SDL_HideCursor();
    return true;
}
void cleanUp(SDL_Window* window, SDL_Renderer* renderer) {
    if(renderer)SDL_DestroyRenderer(renderer);
    if(window)SDL_DestroyWindow(window);
    SDL_Quit();
}


void gameLoop(){
    size_t entity_count = 0;
    camera_t *cam = NULL;
    scene_t *scene = NULL;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    entity_t **entities = NULL;
    if(!initializeGame(&window, &renderer)) {
        printf("Failed to initialize game\n");
        goto CLEANUP;
    }
    bool running = true;
    scene = create_scene();
    if(!scene)goto CLEANUP;
    const SDL_FColor SPACESHIP_WHITE = {0.2f, 0.0f, 0.2f, 1.0f};
    entity_t* myCube2 = create_cube_entity((vec4_t){{{0.3f, 1.0f, 0.0f, 1.0f}}}, 1, 1, 1);
    entity_t *spaceship = get_obj("spaceship.obj", (vec4_t){{{0.0f,-1.0f, 0.0f,0.0f}}}, SPACESHIP_WHITE);
    if(!add_entity(scene, spaceship))goto CLEANUP;
    if(!add_entity(scene, myCube2))goto CLEANUP;
    entity_t *bunny = get_obj("bunny.obj", (vec4_t){{{0,0,0,0}}} , (SDL_FColor){0.0f, 0.8f, 0.8f, 1.0f});
    if(!add_entity(scene, bunny))goto CLEANUP;
        bunny->angles.x = M_PI;
        myCube2 -> color = (SDL_FColor){1.0f,0.0f,1.0f,1.0f};
        myCube2 -> angular_velocity = (vec4_t){{{0.0f, 0.01f,0.0f,0.0f}}};
    cam = create_camera((vec4_t){{{.x=0, .y=0, .z=0, .w=FOCAL}}}, (eulerangles_t){.x=0, .y=0, .z=0}, 0.01f, 0.01f);
    if(!cam){
        printf("Failed to create camera\n");
        goto CLEANUP;
    }
    update_entities(scene->entities, scene->numentities);
    move_world_to_camera_space(cam, scene->entities, scene->numentities);
    const long deltaTime = 5;
    long lastTime = SDL_GetTicks();
    while(running){
        handle_event_and_delta(deltaTime, &lastTime, &running,scene->entities, scene->numentities, cam);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        bool val = render_scene(scene, renderer);

        SDL_RenderPresent(renderer);
        // Game rendering logic goes here


    }
    CLEANUP:
    for(size_t i = 0; i < entity_count; i++){
        if(entities[i]) {
            free_entity(entities[i]);
        }
    }
    if(entities)
        free(entities);
    if(cam)
        free(cam);
    cleanUp(window, renderer);
    destroy_scene(scene);




}