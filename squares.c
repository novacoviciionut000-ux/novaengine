#include "squares.h"
void add_velocity(square_t *sq, vec4_t velocity) {
    sq->pos = add_vec4(&sq->pos, &velocity);
}
void update_square(square_t *sq, const Uint8 *keyboard_state) {
    // Update based on keyboard state
    if(keyboard_state[SDL_SCANCODE_UP]) {
        vec4_t velocity = {.x=0, .y=-vel, .z=0, .w=0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_DOWN]) {
        vec4_t velocity = {.x=0, .y=vel, .z=0, .w=0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_LEFT]) {
        vec4_t velocity = {.x=-vel, .y=0, .z=0, .w=0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_RIGHT]) {
        vec4_t velocity = {.x=vel, .y=0, .z=0, .w=0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_W]) {
        vec4_t velocity = {.x=0, .y=0, .z=-vel, .w=0};
        add_velocity(sq, velocity);
    }
    if(keyboard_state[SDL_SCANCODE_S]) {
        vec4_t velocity = {.x=0, .y=0, .z=vel, .w=0};
        add_velocity(sq, velocity); 
    }
}
void render_square(const square_t *sq, SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    real x_pos = (sq->pos.x / sq->pos.z)*sq->pos.w;
    real y_pos = (sq->pos.y / sq->pos.z)*sq->pos.w;
    
    SDL_FRect rect = {x_pos , y_pos, (sq->length/sq->pos.z)*sq->pos.w, (sq->width/sq->pos.z)*sq->pos.w};
    SDL_RenderFillRect(renderer, &rect);
}

void render_cube(const cube_t *cube, SDL_Renderer *renderer){
    //the cube is made of 12 lines
    SDL_Point points[8];

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    //read the header for explanation of the vertex order and which lines to draw
    //bottom face:
    //this is the cumulative matrix, basically
    mat4_t id = mat4_identity();
    mat4_t m_rot_z = rot_z(&id, cube->angles.z, true);
    mat4_t m_rot_y = rot_y(&m_rot_z, cube->angles.y, true);
    mat4_t final_matrix = rot_x(&m_rot_y, cube->angles.x, true);//the respective rotation matrices, they are applied in the order of x, then y, then z, so we need to multiply them in reverse order
    int y_offset = SCREEN_HEIGHT / 2;//we need to add these so that the cube is actually visible on the screen
    int x_offset = SCREEN_WIDTH / 2;
    for(int i = 0; i < 8; i++){
        // A. Rotate (from Local Storage)
        vec4_t rotated = apply_transform(&final_matrix, &cube->local_verts[i]);

        // B. Translate (Add Position)
        vec4_t world_pos = add_vec4(&rotated, &cube->pos);

        // C. Project (3D -> 2D)
        if(world_pos.z <= 0.1) world_pos.z = 0.1; 

        points[i].x = (int)((world_pos.x / world_pos.z) * world_pos.w + x_offset);
        points[i].y = (int)((world_pos.y / world_pos.z) * world_pos.w + y_offset);
    }


    SDL_RenderLine(renderer, points[0].x, points[0].y, points[1].x, points[1].y);
    SDL_RenderLine(renderer, points[1].x, points[1].y, points[2].x, points[2].y);
    SDL_RenderLine(renderer, points[2].x, points[2].y, points[3].x, points[3].y);
    SDL_RenderLine(renderer, points[3].x, points[3].y, points[0].x, points[0].y);
    //top face:
    SDL_RenderLine(renderer, points[4].x, points[4].y, points[5].x, points[5].y);
    SDL_RenderLine(renderer, points[5].x, points[5].y, points[6].x, points[6].y);
    SDL_RenderLine(renderer, points[6].x, points[6].y, points[7].x, points[7].y);
    SDL_RenderLine(renderer, points[7].x, points[7].y, points[4].x, points[4].y);
    //vertical edges
    for(int i = 0; i < 4; i++){
        SDL_RenderLine(renderer, points[i].x, points[i].y, points[i+4].x, points[i+4].y);
    }
}

void update_cube(cube_t *cube, const Uint8 *keyboard_state){
    if(keyboard_state[SDL_SCANCODE_UP]) {
        cube->angles.x += 0.01;
    }
    if(keyboard_state[SDL_SCANCODE_DOWN]) {
        cube->angles.x -= 0.01;
    }
    if(keyboard_state[SDL_SCANCODE_LEFT]) {
        cube->angles.y += 0.01;
    }
    if(keyboard_state[SDL_SCANCODE_RIGHT]) {
        cube->angles.y -= 0.01;
    }
    if(keyboard_state[SDL_SCANCODE_W]){
        cube -> pos.z -= vel;
    }
    if(keyboard_state[SDL_SCANCODE_S]){
        cube -> pos.z += vel;   
    }
    if(keyboard_state[SDL_SCANCODE_A]){
        cube -> pos.x -= vel;
    }
    if(keyboard_state[SDL_SCANCODE_D]){
        cube -> pos.x += vel;
    }
}
void initialize_cube_position(cube_t** cube, vec4_t pos, real length, real width, real height){
    real half_length = length / 2.0;
    real half_width = width / 2.0;
    real half_height = height / 2.0;
    (*cube)->pos = pos;
    //world vertices are the actual positions of the vertices in the world, they will be transformed by the rotation and translation to get the final positions for rendering, local vertices are the positions of the vertices relative to the center of the cube, they are used as a template for the transformations
    //bottom face
    (*cube)->world_verts[0] = (vec4_t){.x=(*cube)->pos.x - half_length, .y=(*cube)->pos.y - half_width, .z=(*cube)->pos.z + half_height, .w=FOCAL}; //front left
    (*cube)->world_verts[1] = (vec4_t){.x=(*cube)->pos.x + half_length, .y=(*cube)->pos.y - half_width, .z=(*cube)->pos.z + half_height, .w=FOCAL}; //front right
    (*cube)->world_verts[2] = (vec4_t){.x=(*cube)->pos.x + half_length, .y=(*cube)->pos.y - half_width, .z=(*cube)->pos.z - half_height, .w=FOCAL}; //back right
    (*cube)->world_verts[3] = (vec4_t){.x=(*cube)->pos.x - half_length, .y=(*cube)->pos.y - half_width, .z=(*cube)->pos.z - half_height, .w=FOCAL}; //back left
    //top face
    (*cube)->world_verts[4] = (vec4_t){.x=(*cube)->pos.x - half_length, .y=(*cube)->pos.y + half_width, .z=(*cube)->pos.z + half_height, .w=FOCAL}; //front left
    (*cube)->world_verts[5] = (vec4_t){.x=(*cube)->pos.x + half_length, .y=(*cube)->pos.y + half_width, .z=(*cube)->pos.z + half_height, .w=FOCAL}; //front right
    (*cube)->world_verts[6] = (vec4_t){.x=(*cube)->pos.x + half_length, .y=(*cube)->pos.y + half_width, .z=(*cube)->pos.z - half_height, .w=FOCAL}; //back right
    (*cube)->world_verts[7] = (vec4_t){.x=(*cube)->pos.x - half_length, .y=(*cube)->pos.y + half_width, .z=(*cube)->pos.z - half_height, .w=FOCAL}; //back left
    //local vertices are defined relative to the center, so we just need to center it around 0,0,0
    (*cube)->local_verts[0] = (vec4_t){.x=-half_length, .y=-half_width, .z=half_height, .w=FOCAL}; //front left
    (*cube)->local_verts[1] = (vec4_t){.x=half_length, .y=-half_width, .z=half_height, .w=FOCAL}; //front right
    (*cube)->local_verts[2] = (vec4_t){.x=half_length, .y=-half_width, .z=-half_height, .w=FOCAL}; //back right
    (*cube)->local_verts[3] = (vec4_t){.x=-half_length, .y=-half_width, .z=-half_height, .w=FOCAL}; //back left
    (*cube)->local_verts[4] = (vec4_t){.x=-half_length, .y=half_width, .z=half_height, .w=FOCAL}; //front left
    (*cube)->local_verts[5] = (vec4_t){.x=half_length, .y=half_width, .z=half_height, .w=FOCAL}; //front right
    (*cube)->local_verts[6] = (vec4_t){.x=half_length, .y=half_width, .z=-half_height, .w=FOCAL}; //back right
    (*cube)->local_verts[7] = (vec4_t){.x=-half_length, .y=half_width, .z=-half_height, .w=FOCAL}; //back left

}
cube_t* create_cube(vec4_t pos, real length, real width, real height){
    
    //we will create the cube centered around the position given, so we need to calculate the vertices based on that
    cube_t *cube = malloc(sizeof(cube_t));
    if(cube == NULL){
        return NULL;
    }
    //the center is right in the middle (pos.x, pos.y, pos.z) is basically the arithmetic mean of the vertices, so we can calculate the vertices by adding/subtracting half of the length, width and height from the position
    //world vertices are the actual positions of the vertices in the world, they will be transformed by the rotation and translation to get the final positions for rendering, local vertices are the positions of the vertices relative to the center of the cube, they are used as a template for the transformations
    //bottom face
    initialize_cube_position(&cube, pos, length, width, height);
    cube->angles = (eulerangles_t){0,0,0};
    return cube;//it is quite a lot to be on the stack, so we allocate it on the heap and return a pointer to it, the caller is responsible for freeing the memory when it is no longer needed

}