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
    entity->dirty = false;

}
bool isZero_vec(vec4_t vecA){
    if(vecA.x <= 0.001f && vecA.y <= 0.001f && vecA.z <= 0.001f)return true;
    return false;
}
void add_angular_velocity(eulerangles_t *angles, vec4_t angular_velocity){
    angles->x += angular_velocity.x;
    angles->y += angular_velocity.y;
    angles->z += angular_velocity.z;
}
void update_entity(entity_t *entity){

    if(!isZero_vec(entity->velocity)){
        entity->pos = add_vec4(&entity->pos, &entity->velocity);
        entity->dirty = true;
    }
    if(!isZero_vec(entity->angular_velocity)){
        add_angular_velocity(&entity->angles, entity->angular_velocity);
        entity->dirty = true;
    }
    if(entity->dirty)
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
    free(entity->mesh->triangle_map);    
    free(entity->mesh->camera_verts);
    free(entity->mesh);
    free(entity);
}
int* create_cube_triangles(){
    int* triangle_map = malloc(36 * sizeof(int));
    int cube_triangle_map[] = {
            // Front Face
            0, 1, 5,    0, 5, 4, 
            
            // Right Face
            1, 2, 6,    1, 6, 5,
            
            // Back Face
            2, 3, 7,    2, 7, 6,
            
            // Left Face
            3, 0, 4,    3, 4, 7,
            
            // Top Face
            4, 5, 6,    4, 6, 7,
            
            // Bottom Face
            3, 2, 1,    3, 1, 0  
        };
    for(int i = 0; i < 36; i++) triangle_map[i] = cube_triangle_map[i];
    return triangle_map;
}
vec4_t* create_cube_local_vertices(real length, real width, real height){
    vec4_t* local_verts = malloc(8 * sizeof(vec4_t));
    real half_length = length / 2.0;
    real half_width = width / 2.0;
    real half_height = height / 2.0;
    //bottom face
    local_verts[0] = (vec4_t){.x=-half_length, .y=-half_width, .z=half_height, .w=1}; //front left
    local_verts[1] = (vec4_t){.x=half_length, .y=-half_width, .z=half_height, .w=1}; //front right
    local_verts[2] = (vec4_t){.x=half_length, .y=-half_width, .z=-half_height, .w=1}; //back right
    local_verts[3] = (vec4_t){.x=-half_length, .y=-half_width, .z=-half_height, .w=1}; //back left
    //top face
    local_verts[4] = (vec4_t){.x=-half_length, .y=half_width, .z=half_height, .w=1}; //front left
    local_verts[5] = (vec4_t){.x=half_length, .y=half_width, .z=half_height, .w=1}; //front right
    local_verts[6] = (vec4_t){.x=half_length, .y=half_width, .z=-half_height, .w=1}; //back right
    local_verts[7] = (vec4_t){.x=-half_length, .y=half_width, .z=-half_height, .w=1}; //back left

    return local_verts;
}

mesh_t* create_mesh(vec4_t *local_verts, int num_verts, int triangle_count, int* triangle_map){
    mesh_t *mesh = calloc(1,sizeof(mesh_t));
    mesh->local_verts = local_verts;
    mesh->world_verts = calloc(num_verts , sizeof(vec4_t));
    if(mesh->world_verts == NULL){
        free(mesh);
        return NULL;
    }
    mesh -> camera_verts = calloc(num_verts , sizeof(vec4_t));
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
    return mesh;
}
entity_t* create_entity(vec4_t pos, mesh_t *mesh){
    entity_t *entity = calloc(1,sizeof(entity_t));
    entity->color = (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}; // Default to solid white
    entity->pos = pos;
    entity->velocity = (vec4_t){{{0,0,0,0}}};
    entity->angular_velocity = (vec4_t){{{0,0,0,0}}};
    entity->dirty = true;
    entity->mesh = mesh;
    entity->speed = 0.01f;
    entity->angular_speed = 0.01f;
    entity->angles = (eulerangles_t){0,0,0};
    rotate_entity(entity);
    return entity;
}
entity_t* create_cube_entity(vec4_t pos, real length, real width, real height){
    vec4_t* local_verts = create_cube_local_vertices(length, width, height);
    int* triangle_map = create_cube_triangles();

    if(local_verts == NULL || triangle_map == NULL){
        if(local_verts)free(local_verts);
        if(triangle_map)free(triangle_map);
        return NULL;
    }  
    mesh_t* cube_mesh = create_mesh(local_verts, 8,12,triangle_map);
    entity_t* cube_entity = create_entity(pos, cube_mesh);
    return cube_entity;
}


//To do: OBJ loader / parser, so that we can easily add new entities without hardcoding their vertices and indices in the code, we can just create a .obj file for the entity and load it at runtime, this will also allow us to use more complex models without having to manually define all the vertices and indices in the code, which is very error-prone and time-consuming, especially for complex models with hundreds or thousands of vertices and indices
