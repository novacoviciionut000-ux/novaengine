#include "obj_parsing.h"

#define EXPECTED_VERTS 1024
void scale_mesh(mesh_t *mesh, real scale) {
    for (int i = 0; i < mesh->vertex_count; i++) {
        mesh->local_verts[i].x *= scale;
        mesh->local_verts[i].y *= scale;
        mesh->local_verts[i].z *= scale;
    }
}
entity_t *get_obj(char *pathname, vec4_t position, SDL_FColor color, real scale, bool collidable, real xrot , real yrot , real zrot){
    FILE *file = fopen(pathname, "r");
    entity_t *obj = NULL;
    mesh_t *mesh = NULL;
    char *line = NULL; 
    size_t size = 0;

    if(file == NULL){
        goto CLEANUP;
    }

    obj = calloc(1, sizeof(entity_t));
    if(obj == NULL) goto CLEANUP;
    
    mesh = calloc(1, sizeof(mesh_t)); 
    if(mesh == NULL) goto CLEANUP;
    obj->mesh = mesh;

    obj->mesh->local_verts = malloc(EXPECTED_VERTS * sizeof(vec4_t));
    obj->mesh->triangle_map = malloc(sizeof(int) * EXPECTED_VERTS);
    
    if(obj->mesh->local_verts == NULL || obj->mesh->triangle_map == NULL) goto CLEANUP;

    size_t allocated = EXPECTED_VERTS, curr_verts = 0, allocated_triangles = EXPECTED_VERTS;
    size_t triangle_indice = 0;

    while(getline(&line, &size, file) != -1){
        if(strlen(line) < 3)continue;
        if(line[0] == '#')continue;
        if(line[0] == 'v' && line[1] == ' '){
            vec4_t curr_vertex = {0};
            int cnt = sscanf(line + 2, "%f %f %f %f", &curr_vertex.x, &curr_vertex.y, &curr_vertex.z, &curr_vertex.w);
            if(cnt != 4)curr_vertex.w = 1;
                obj->mesh->local_verts[curr_verts++] = curr_vertex;            
                if(curr_verts >= allocated){
                allocated += EXPECTED_VERTS;
                vec4_t *tmp = realloc(obj->mesh->local_verts, allocated * sizeof(vec4_t));                
                if(tmp == NULL){
                    goto CLEANUP;
                }
                obj->mesh->local_verts = tmp;
            }
        }
        if(line[0] == 'f' && line[1] == ' ') {
            int face_indices[16]; // Buffer to hold indices on this line
            int v_count = 0;

            // Tokenize the line by spaces to get each "v/t/n" block
            char *token = strtok(line + 2, " \t\r\n");
            while(token != NULL && v_count < 16) {
                // atoi is perfect here: it converts the first number 
                // and stops at the first slash '/' automatically.
                face_indices[v_count++] = atoi(token);
                token = strtok(NULL, " \t\r\n");
            }

            for (int i = 1; i < v_count - 1; i++) {
                // Check/Grow your triangle_map memory here (same as before)
                if(triangle_indice + 3 >= allocated_triangles) {
                    allocated_triangles += EXPECTED_VERTS;
                    int* tmp = realloc(obj->mesh->triangle_map, allocated_triangles * sizeof(int));                    
                    if(!tmp) goto CLEANUP;
                    obj->mesh->triangle_map = tmp;
                }

                // Fan the indices: 0 is the pivot
                obj->mesh->triangle_map[triangle_indice++] = face_indices[0] - 1;
                obj->mesh->triangle_map[triangle_indice++] = face_indices[i] - 1;
                obj->mesh->triangle_map[triangle_indice++] = face_indices[i+1] - 1;
            }
        }
    }

    obj -> mesh -> vertex_count = curr_verts;
    obj -> mesh -> world_verts = calloc(curr_verts,sizeof(vec4_t));
    if(obj->mesh->world_verts == NULL)goto CLEANUP;
    obj -> mesh -> camera_verts = calloc(curr_verts, sizeof(vec4_t));
    obj -> mesh -> triangle_count = triangle_indice/3;
    obj -> pos = position;
    
    // Fix: Only allocate if collidable, otherwise keep as NULL
    if(collidable) {
        obj->collision_box = calloc(1, sizeof(collisionbox_t));
        if(!obj->collision_box) goto CLEANUP;
    } else {
        obj->collision_box = NULL;
    }

    obj -> velocity = (vec4_t){{{0,0,0,0}}};
    obj -> angular_velocity = (vec4_t){{{0,0,0,0}}};
    obj -> angular_speed = 0.01f;
    obj -> color = color;
    obj->dirty = true;
    
    scale_mesh(obj->mesh, scale);
    
    // Fix: Only run collision box generation if we actually have a box
    if(collidable && obj->collision_box) {
        *(obj -> collision_box) = get_entity_collisionbox(obj);
    }
    mat4_t id = mat4_identity();
    if(xrot != 0 || yrot != 0 || zrot != 0){
        id = rot_x(&id, xrot, true);
        id = rot_y(&id, yrot, true);
        id = rot_z(&id, zrot, true);
        for(int i = 0; i < obj -> mesh->vertex_count; i++){
            obj -> mesh->local_verts[i] = apply_transform(&id, &obj->mesh->local_verts[i]);
        }
    }
    obj-> force_accumulator = (vec4_t){{{0,0,0,0}}};
    obj -> acceleration = (vec4_t){{{0,0,0,0}}};
    
    fclose(file);
    file = NULL; // Prevent double-close in CLEANUP

    rotate_entity(obj);
    free(line); // Clean up the line before returning success
    return obj;

CLEANUP:
    if (file) fclose(file);
    if (line) free(line);
    if (obj) {
        if (obj->mesh) {
            free(obj->mesh->local_verts);
            free(obj->mesh->world_verts);
            free(obj->mesh->triangle_map);
            free(obj->mesh->camera_verts);
            free(obj->mesh);
        }
        if (obj->collision_box) free(obj->collision_box);
        free(obj);
    }
    return NULL;
}