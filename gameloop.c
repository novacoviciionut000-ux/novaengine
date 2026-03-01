#include "gameloop.h"
bool initializeGame(SDL_Window** window, SDL_Renderer** renderer) {

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL could not initialize: %s\n", SDL_GetError());
        return false;
    }

    *window = SDL_CreateWindow("Game Loop", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS);
    if (*window) {
        // 2. Explicitly center it in SDL3
        SDL_SetWindowPosition(*window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
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

// TO DO : each collisionbox has a boolean "hit". It is set when a bullet collides with it.
// Then, you loop through all the entities and check if their collisionbox is hit.
// If they are hit, you call a function pointer onhit().
void gameLoop(){
    SDL_FColor light_brown = {
        .r = 0.71f, // ~181 in 8-bit
        .g = 0.54f, // ~137 in 8-bit
        .b = 0.36f, // ~92 in 8-bit
        .a = 1.0f   // Fully opaque
    };
    bool running = true;
    //pointer declaration and SDL setup
    delta_timer *timer = NULL;
    real_timer *real_time = NULL;
    scene_t *scene = NULL;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    gun_state_t *gun_state = calloc(1, sizeof(gun_state_t));
    if(!gun_state)goto CLEANUP;
    gun_state->cooldown_timer = 0.0f;
    if(!initializeGame(&window, &renderer)) {
        printf("Failed to initialize game\n");
        goto CLEANUP;
    }
    //END POINTER DECLARATION


    // clock initialization
        timer = initialize_time();
        printf("Timer initialized\n");
        real_time = initialize_clock();
        printf("Real time initialized\n");

        if(real_time == NULL)goto CLEANUP;
        if(timer == NULL)goto CLEANUP;
        real dt = delta_time(timer);
        real time = get_time(real_time);

    //end clocks
    //color stuff
        //const SDL_FColor SPACESHIP_WHITE = {0.1f, 0.4f, 0.1f, 1.0f};
        SDL_FColor neon_green = {0.0f, 1.0f, 0.0f, 1.0f};
    //end color stuff

    //scene creation
        scene = create_scene();
        if(!scene)goto CLEANUP;
        camera_t *cam = scene->camera_manager->camera;
    //end scene creation
    //starfield
        init_starfield(scene->render_manager);
        printf("star field initialized\n");
    //end starfield
    //entity creation
        entity_t *gun = init_gun("gun.obj");
        printf("gun initialized\n");
        if(!gun)goto CLEANUP;
        if(!add_object(scene, (void*)gun, ENTITY_TYPE))goto CLEANUP;
      entity_t *bunny2 = (entity_t*)get_obj("Rtwo_low.obj", (vec4_t){{{1.0f,0.2f,0,0}}} , neon_green, 0.003f, true,0,0,0, false, ENTITY_TYPE);
      printf("bunny2 initialized\n");
       if(bunny2 == NULL){perror("opening file");goto CLEANUP;}
       if(!add_object(scene, (void*)bunny2, ENTITY_TYPE))goto CLEANUP;
       printf("bunny2 added to scene\n");

      //  entity_t *landscape = (entity_t*)get_obj("landscape.obj", (vec4_t){{{0,-5.0f,0,0}}} , light_brown, 1.0f, false,0,M_PI,M_PI, false, ENTITY_TYPE);
       // printf("landscape initialized\n");
       // if(landscape == NULL){perror("opening file");goto CLEANUP;}
        //if(!add_object(scene, (void*)landscape, ENTITY_TYPE))goto CLEANUP;
       // printf("landscape added to scene\n");

    //end entity creation
    //entity tweaking
       bunny2 -> angles.x = M_PI;
       bunny2 -> dirty = true;
    //end entity tweaking
    //camera creation

    //end camera creation
    //first frame preparation
    add_enemy(DRONE_TYPE, scene, (vec4_t){{{0,0,0,0}}});
    add_enemy(DRONE_TYPE, scene, (vec4_t){{{1.0f,0.0f,1.0f,0.0f}}});
    printf("enemy added to scene\n");
    update_entities(scene->entity_manager->entities, scene->entity_manager->numentities, dt);
    printf("entities updated\n");

    transform_vertices(scene->camera_manager, scene->render_manager);  // pipeline.c
    printf("vertices transformed\n");
    int fps_counter = 0;
    real second_timer = 0;
    //end frame preparation
    while(running){
        dt = delta_time(timer);
        time = get_time(real_time);
        fps_counter++;
        second_timer += dt;
        if(second_timer >= 1.0f){
            printf("FPS: %d\n", fps_counter);
            fps_counter = 0;
            second_timer = 0;
        }
        update_particles(scene, dt, cam);
        gather_meshes(scene);
        gather_collision_boxes(scene);
        update_enemies(scene, dt, cam);//FOR NOW, the order of this functions is fragile. DO NOT change it unless you wanna debug deadlocks between 3 files.
        handle_event_and_delta(&running,scene, dt,gun, time, gun_state);
        update_bullets(scene);
        handle_and_check_collision(scene->collision_manager, cam);
        second_timer += dt;
        fps_counter++;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        bool val = render(scene->render_manager, renderer, false, cam);//the return value is useless for now, but maybe will be useful in the future.
        draw_damage_vignette(renderer, cam, 100.0f);
        SDL_RenderPresent(renderer);
        // Game rendering logic goes here


    }
    CLEANUP:
    cleanUp(window, renderer);
    destroy_scene(scene);
    if(timer) free(timer);
    destroy_clock(real_time);
    free(gun_state);



}
