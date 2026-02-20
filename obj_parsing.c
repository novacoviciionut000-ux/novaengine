#include "obj_parsing.h"

#define EXPECTED_VERTS 1024
entity_t *get_obj(char *pathname, vec4_t position,SDL_FColor color){
    FILE *file = fopen(pathname, "r");
    entity_t *obj = NULL;
    mesh_t *mesh = NULL;
    if(file == NULL){
        goto CLEANUP;
    }

    obj = calloc(1, sizeof(entity_t));
    if(obj == NULL) goto CLEANUP;
    
    mesh = calloc(1, sizeof(mesh_t)); // Now all internal pointers are safely NULL
    if(mesh == NULL) goto CLEANUP;
    obj->mesh = mesh;

    // Allocate and immediately bind so CLEANUP can find them if a crash happens
    obj->mesh->local_verts = malloc(EXPECTED_VERTS * sizeof(vec4_t));
    obj->mesh->triangle_map = malloc(sizeof(int) * EXPECTED_VERTS);
    
    if(obj->mesh->local_verts == NULL || obj->mesh->triangle_map == NULL) goto CLEANUP;

    size_t allocated = EXPECTED_VERTS, curr_verts = 0, allocated_triangles = EXPECTED_VERTS;
    size_t triangle_indice = 0;
    char *line = NULL;size_t size = 0;
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

            // Convert the N-gon into a fan of triangles
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
    free(line);
    line = NULL;
    obj -> mesh -> vertex_count = curr_verts;
    obj -> mesh -> world_verts = calloc(curr_verts,sizeof(vec4_t));
    if(obj->mesh->world_verts == NULL)goto CLEANUP;
    obj -> mesh -> camera_verts = calloc(curr_verts, sizeof(vec4_t));
    obj -> mesh -> indice_map = NULL;//we upload non-wireframe entities
    obj -> mesh -> triangle_count = triangle_indice/3;
    obj -> mesh -> indice_count = 0;
    obj -> pos = position;
    obj -> velocity = (vec4_t){{{0,0,0,0}}};
    obj -> angular_velocity = (vec4_t){{{0,0,0,0}}};
    obj -> angular_speed = 0.01f;
    obj -> color = color;
    obj->movable = true;
    obj -> triangles = calloc(triangle_indice/3, sizeof(triangle_t));
    if(obj -> triangles == NULL)goto CLEANUP;
    obj -> vertices = calloc(triangle_indice, sizeof(SDL_Vertex));
    if(obj -> vertices == NULL)goto CLEANUP;
    fclose(file);
    return obj;
    CLEANUP:
    if (obj) {
        if (obj->mesh) {
            free(obj->mesh->local_verts);
            free(obj->mesh->world_verts);
            free(obj->mesh->triangle_map);
            free(obj->mesh->camera_verts);
            free(obj->mesh);
        }
        free(obj->triangles);
        free(obj->vertices);
        free(obj);
    }
    if (file) fclose(file);
    free(line);
    return NULL;
}

