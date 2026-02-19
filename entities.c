#include "entities.h"

void rotate_entity(entity_t *entity){
    mat4_t id = mat4_identity();
    mat4_t m_rot_z = rot_z(&id, entity->angles.z, true);
    mat4_t m_rot_y = rot_y(&m_rot_z, entity->angles.y, true);
    mat4_t final_matrix = rot_x(&m_rot_y, entity->angles.x, true);
    for(int i = 0; i < entity->mesh->vertex_count; i++){
        // A. Rotate (from Local Storage)
        vec4_t rotated = apply_transform(&final_matrix, &entity->mesh->local_verts[i]);

        // B. Translate (Add Position)
        vec4_t world_pos = add_vec4(&rotated, &entity->pos);


        entity->mesh->world_verts[i] = world_pos;
    }

}

void add_angular_velocity(eulerangles_t *angles, vec4_t angular_velocity){
    angles->x += angular_velocity.x;
    angles->y += angular_velocity.y;
    angles->z += angular_velocity.z;
}
void update_entity(entity_t *entity){

    entity->pos = add_vec4(&entity->pos, &entity->velocity);
    add_angular_velocity(&entity->angles, entity->angular_velocity);
    rotate_entity(entity);

}
void update_entities(entity_t **entities, int entity_count){
    for(int i = 0; i < entity_count; i++){
        update_entity(entities[i]);
    }
}

void free_entity(entity_t *entity){

    free(entity->mesh->local_verts);
    free(entity->mesh->world_verts);
    free(entity->mesh->indice_map);
    free(entity->mesh->triangle_map);    
    free(entity->mesh->camera_verts);
    free(entity->triangles);
    free(entity->vertices);
    free(entity->mesh);
    free(entity);
}
int* create_cube_triangles(){
    int* triangle_map = malloc(36 * sizeof(int));
    int cube_triangle_map[] = {
        0,1,2, 0,2,3, //bottom face
        4,5,6, 4,6,7, //top face
        0,1,5, 0,5,4, //front face
        1,2,6, 1,6,5, //right face
        2,3,7, 2,7,6, //back face
        3,0,4, 3,4,7 //left face
    };
    for(int i = 0; i < 36; i++){
        triangle_map[i] = cube_triangle_map[i];
    }
    return triangle_map;
}
vec4_t* create_cube_local_vertices(real length, real width, real height){
    vec4_t* local_verts = malloc(8 * sizeof(vec4_t));
    real half_length = length / 2.0;
    real half_width = width / 2.0;
    real half_height = height / 2.0;
    //bottom face
    local_verts[0] = (vec4_t){.x=-half_length, .y=-half_width, .z=half_height, .w=FOCAL}; //front left
    local_verts[1] = (vec4_t){.x=half_length, .y=-half_width, .z=half_height, .w=FOCAL}; //front right
    local_verts[2] = (vec4_t){.x=half_length, .y=-half_width, .z=-half_height, .w=FOCAL}; //back right
    local_verts[3] = (vec4_t){.x=-half_length, .y=-half_width, .z=-half_height, .w=FOCAL}; //back left
    //top face
    local_verts[4] = (vec4_t){.x=-half_length, .y=half_width, .z=half_height, .w=FOCAL}; //front left
    local_verts[5] = (vec4_t){.x=half_length, .y=half_width, .z=half_height, .w=FOCAL}; //front right
    local_verts[6] = (vec4_t){.x=half_length, .y=half_width, .z=-half_height, .w=FOCAL}; //back right
    local_verts[7] = (vec4_t){.x=-half_length, .y=half_width, .z=-half_height, .w=FOCAL}; //back left

    return local_verts;
}
int* create_cube_indice_map(){
    int* indice_map = malloc(24 * sizeof(int));
    int cube_indice_map[] = {
        0,1, 1,2, 2,3, 3,0, //bottom face
        4,5, 5,6, 6,7, 7,4, //top face
        0,4, 1,5, 2,6, 3,7 //vertical edges
    };
    for(int i = 0; i < 24; i++){
        indice_map[i] = cube_indice_map[i];
    }
    return indice_map;
}

mesh_t* create_mesh(vec4_t *local_verts, int num_verts, int num_indices, int* indice_map, int triangle_count, int* triangle_map){
    mesh_t *mesh = malloc(sizeof(mesh_t));
    mesh->local_verts = local_verts;
    mesh->world_verts = malloc(num_verts * sizeof(vec4_t));
    if(mesh->world_verts == NULL){
        free(mesh);
        return NULL;
    }
    mesh -> camera_verts = malloc(num_verts * sizeof(vec4_t));
    if(mesh->camera_verts == NULL){
        free(mesh->world_verts);
        free(mesh);
        return NULL;
    }
    mesh->vertex_count = num_verts;
    mesh -> triangle_map = NULL;
    mesh->  triangle_count = 0;
    if(triangle_count != 0 && triangle_map != NULL){
        mesh -> triangle_count = triangle_count;//these two parameters CAN be set to 0 in the case of wireframe rendering.
        mesh -> triangle_map = triangle_map;
    }
    mesh -> indice_map = indice_map;
    mesh->indice_count = num_indices;
    return mesh;
}
entity_t* create_entity(vec4_t pos, mesh_t *mesh, bool movable){
    entity_t *entity = malloc(sizeof(entity_t));
    entity->pos = pos;
    entity->velocity = (vec4_t){{{0,0,0,0}}};
    entity->angular_velocity = (vec4_t){{{0,0,0,0}}};
    entity->movable = movable;
    entity->mesh = mesh;
    entity->speed = 0.01f;
    entity->angular_speed = 0.01f;
    entity->angles = (eulerangles_t){0,0,0};
    SDL_Vertex* verts = mesh->triangle_count != 0?malloc(entity->mesh->triangle_count * 3 * sizeof(SDL_Vertex)):NULL;
    entity -> vertices = verts;
    triangle_t* triangles = mesh->triangle_count!=0?malloc(entity->mesh->triangle_count * sizeof(triangle_t)):NULL;
    entity->triangles = triangles;
    return entity;
}
entity_t* create_cube_entity(vec4_t pos, real length, real width, real height){
    vec4_t* local_verts = create_cube_local_vertices(length, width, height);
    int* indice_map = create_cube_indice_map();
    int* triangle_map = create_cube_triangles();

    if(indice_map == NULL || local_verts == NULL || triangle_map == NULL){
        if(indice_map)free(indice_map);
        if(local_verts)free(local_verts);
        if(triangle_map)free(triangle_map);
        return NULL;
    }  
    mesh_t* cube_mesh = create_mesh(local_verts, 8, 24, indice_map,12,triangle_map);
    entity_t* cube_entity = create_entity(pos, cube_mesh, true);
    return cube_entity;
}
entity_t* create_diamond_entity(vec4_t pos, real size) {
    int num_verts = 6;
    int num_indices = 24; // 12 edges * 2 points each

    vec4_t* local_verts = malloc(num_verts * sizeof(vec4_t));
    
    // Define the 6 points of the octahedron
    local_verts[0] = (vec4_t){.x =  0,    .y =  size, .z =  0,    .w = FOCAL}; // Top
    local_verts[1] = (vec4_t){.x =  0,    .y = -size, .z =  0,    .w = FOCAL}; // Bottom
    local_verts[2] = (vec4_t){.x = -size, .y =  0,    .z =  size, .w = FOCAL}; // Front-Left
    local_verts[3] = (vec4_t){.x =  size, .y =  0,    .z =  size, .w = FOCAL}; // Front-Right
    local_verts[4] = (vec4_t){.x =  size, .y =  0,    .z = -size, .w = FOCAL}; // Back-Right
    local_verts[5] = (vec4_t){.x = -size, .y =  0,    .z = -size, .w = FOCAL}; // Back-Left

    // Connect the dots (Wireframe indices)
    int* indices = malloc(num_indices * sizeof(int));
    int map[] = {
        0,2, 0,3, 0,4, 0,5, // Top point to all 4 middle corners
        1,2, 1,3, 1,4, 1,5, // Bottom point to all 4 middle corners
        2,3, 3,4, 4,5, 5,2  // The middle square ring
    };
    for(int i = 0; i < num_indices; i++) indices[i] = map[i];

    mesh_t* mesh = create_mesh(local_verts, num_verts, num_indices, indices,0,NULL);
    return create_entity(pos, mesh, true);
}
entity_t* create_car_entity(vec4_t pos, real scale) {
    int num_verts = 8;
    int num_indices = 30; // 15 lines (2 points each)

    vec4_t* local_verts = malloc(num_verts * sizeof(vec4_t));
    
    // --- Body (Base) ---
    local_verts[0] = (vec4_t){.x = -1.0 * scale, .y = -0.5 * scale, .z =  2.0 * scale, .w = 1.0}; // Front-Bottom-Left
    local_verts[1] = (vec4_t){.x =  1.0 * scale, .y = -0.5 * scale, .z =  2.0 * scale, .w = 1.0}; // Front-Bottom-Right
    local_verts[2] = (vec4_t){.x =  1.0 * scale, .y = -0.5 * scale, .z = -2.0 * scale, .w = 1.0}; // Back-Bottom-Right
    local_verts[3] = (vec4_t){.x = -1.0 * scale, .y = -0.5 * scale, .z = -2.0 * scale, .w = 1.0}; // Back-Bottom-Left

    // --- Cabin (Roof) ---
    local_verts[4] = (vec4_t){.x = -0.7 * scale, .y =  0.6 * scale, .z =  0.5 * scale, .w = 1.0}; // Cabin-Front-Left
    local_verts[5] = (vec4_t){.x =  0.7 * scale, .y =  0.6 * scale, .z =  0.5 * scale, .w = 1.0}; // Cabin-Front-Right
    local_verts[6] = (vec4_t){.x =  0.7 * scale, .y =  0.6 * scale, .z = -1.5 * scale, .w = 1.0}; // Cabin-Back-Right
    local_verts[7] = (vec4_t){.x = -0.7 * scale, .y =  0.6 * scale, .z = -1.5 * scale, .w = 1.0}; // Cabin-Back-Left

    int* indices = malloc(num_indices * sizeof(int));
    int map[] = {
        0,1, 1,2, 2,3, 3,0, // Bottom chassis square
        4,5, 5,6, 6,7, 7,4, // Roof square
        0,4, 1,5, 2,6, 3,7, // Pillars connecting roof to base
        0,2, 1,3,           // Cross-bracing for the "hood" look
        4,6                 // Center roof line
    };
    for(int i = 0; i < num_indices; i++) indices[i] = map[i];

    // CRITICAL: Ensure create_mesh uses num_verts and num_indices!
    mesh_t* mesh = create_mesh(local_verts, num_verts, num_indices, indices,0,NULL);
    return create_entity(pos, mesh, true);
}
//To do: OBJ loader / parser, so that we can easily add new entities without hardcoding their vertices and indices in the code, we can just create a .obj file for the entity and load it at runtime, this will also allow us to use more complex models without having to manually define all the vertices and indices in the code, which is very error-prone and time-consuming, especially for complex models with hundreds or thousands of vertices and indices
