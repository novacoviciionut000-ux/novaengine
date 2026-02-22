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
    delta_timer *timer = NULL;
    scene_t *scene = NULL;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    entity_t **entities = NULL;
    if(!initializeGame(&window, &renderer)) {
        printf("Failed to initialize game\n");
        goto CLEANUP;
    }
    timer = initialize_time();
    if(timer == NULL)goto CLEANUP;
    bool running = true;
    scene = create_scene();
    real dt = delta_time(timer);
    if(!scene)goto CLEANUP;
    init_starfield(scene);

    const SDL_FColor SPACESHIP_WHITE = {0.1f, 0.4f, 0.1f, 1.0f};
    SDL_FColor neon_green = {0.0f, 1.0f, 0.0f, 1.0f};
    entity_t* myCube2 = create_cube_entity((vec4_t){{{1.0f, 1.0f, 0.0f, 1.0f}}}, 1, 1, 1);
    if(!add_entity(scene, myCube2))goto CLEANUP;
    entity_t *bunny = get_obj("bunny.obj", (vec4_t){{{-3.0f,1.0f,0,0}}} , neon_green);
    if(bunny == NULL){perror("opening file");goto CLEANUP;}
    if(!add_entity(scene, bunny))goto CLEANUP;
    bunny -> angles.x = M_PI;
    bunny -> dirty = true;
        myCube2 -> color = (SDL_FColor){3.0f,0.0f,1.0f,1.0f};
        myCube2 -> angular_velocity = (vec4_t){{{0.0f, 0.01f,0.0f,0.0f}}};
    cam = create_camera((vec4_t){{{.x=0, .y=0, .z=0, .w=FOCAL}}}, (eulerangles_t){.x=0, .y=0, .z=0}, 0.01f, 0.01f);
    if(!cam){
        printf("Failed to create camera\n");
        goto CLEANUP;
    }
    update_entities(scene->entities, scene->numentities, dt);
    move_world_to_camera_space(cam, scene->entities, scene->numentities);
    const long deltaTime = 5;
    long lastTime = SDL_GetTicks();
    while(running){
        dt = delta_time(timer);
        handle_event_and_delta(deltaTime, &running,scene->entities, scene->numentities, cam, dt);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        bool val = render_scene(scene, renderer, false, cam);
        handle_and_check_collision(scene, cam);

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
    if(timer) free(timer);



}