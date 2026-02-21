#include "scenes.h"
#define CHUNK 16
#define EXPECTED_TRIANGLES 4096
scene_t *create_scene(){
    scene_t* scene = calloc(1, sizeof(scene_t));
    if(scene == NULL)goto CLEANUP;
    scene -> allocated_mem = CHUNK;
    scene -> allocated_triangles = EXPECTED_TRIANGLES;
    scene -> numentities = 0;
    scene -> numtriangles = 0;
    scene -> entities = calloc(scene->allocated_mem, sizeof(entity_t*));
    if(scene->entities == NULL)goto CLEANUP;
    scene -> triangles = calloc(scene->allocated_triangles, sizeof(triangle_t));
    if(scene -> triangles == NULL)goto CLEANUP;
    scene -> render_usage = calloc(scene->allocated_triangles, sizeof(triangle_t));
    if(scene -> render_usage == NULL) goto CLEANUP;
    scene -> verts = calloc(scene-> allocated_triangles * 3, sizeof(SDL_Vertex));
    return scene;
    CLEANUP:
    if(scene -> triangles) free(scene->triangles);
    if(scene -> render_usage) free(scene -> render_usage);
    if(scene -> verts) free(scene -> verts);
    if(scene -> entities) free(scene -> entities);
    if(scene) free(scene);
    return NULL;
}
void add_triangles(entity_t *entity, scene_t *scene, size_t write_index){
    for(int i = 0; i < entity->mesh->triangle_count * 3; i += 3){
        triangle_t *curr = &scene->triangles[write_index++];
        
        // Link to the stable memory addresses in the entity's mesh
        curr->camera_positions[0] = &entity->mesh->camera_verts[entity->mesh->triangle_map[i]];
        curr->camera_positions[1] = &entity->mesh->camera_verts[entity->mesh->triangle_map[i+1]];
        curr->camera_positions[2] = &entity->mesh->camera_verts[entity->mesh->triangle_map[i+2]];
        curr->color = entity->color;
        //rendering will refresh this every frame but just in order for the first frame to render fine
        curr->avg_depth = (curr->camera_positions[0]->z + 
                           curr->camera_positions[1]->z + 
                           curr->camera_positions[2]->z) / 3.0f;
    }
}

bool add_entity(scene_t *scene, entity_t *entity){
    size_t new_alloc_count = 0;
    // 1. Handle Entity Array Capacity
    if(scene->numentities >= scene->allocated_mem){
        scene->allocated_mem += CHUNK;
        entity_t **tmp = realloc(scene->entities, scene->allocated_mem * sizeof(entity_t*));
        if(tmp == NULL) return false; 
        scene->entities = tmp;
    }

    // 2. Handle Triangle Pool Capacity
    size_t new_total_tris = scene->numtriangles + entity->mesh->triangle_count;
    if(new_total_tris >= scene->allocated_triangles){
        new_alloc_count = scene->allocated_triangles;
        while(new_alloc_count <= new_total_tris) {
            new_alloc_count += EXPECTED_TRIANGLES;
        }

        // Try all reallocs first
        triangle_t *t1 = realloc(scene->triangles, new_alloc_count * sizeof(triangle_t));
        triangle_t *t2 = realloc(scene->render_usage, new_alloc_count * 2 * sizeof(triangle_t));// we over-allocate to prevent buffer overflows as at runtime 1 triangle may be split into two.
        SDL_Vertex *t3 = realloc(scene->verts, new_alloc_count * 2 * 3 * sizeof(SDL_Vertex));

        // If ANY fail, we have a problem (in a real engine you'd handle this more gracefully)
        if(!t1 || !t2 || !t3) return false;

        // Only apply changes if all succeeded
        scene->triangles = t1;
        scene->render_usage = t2;
        scene->verts = t3;
        scene->allocated_triangles = new_alloc_count;
    }

    // 3. Register Entity and Link Triangles
    entity->pool_offset = scene->numtriangles; // Set the key BEFORE incrementing
    scene->entities[scene->numentities] = entity;
    scene->numentities++;

    add_triangles(entity, scene, entity->pool_offset);
    
    scene->numtriangles = new_total_tris; // Finalize the count
    scene->transient_buffer = calloc(new_alloc_count * 2 * 3, sizeof(vec4_t));
    if(!scene->transient_buffer)return false;
    return true;
}
void _patch_scene_after_removal(scene_t *scene, size_t start_offset, size_t triangle_count) {
    size_t move_count = scene->numtriangles - (start_offset + triangle_count);
    
    if (move_count > 0) {
        memmove(&scene->triangles[start_offset], 
                &scene->triangles[start_offset + triangle_count], 
                move_count * sizeof(triangle_t));
    }
    
    scene->numtriangles -= triangle_count;

    // Update pool_offsets for everyone who shifted
    for (size_t i = 0; i < scene->numentities; i++) {
        if (scene->entities[i]->pool_offset > start_offset) {
            scene->entities[i]->pool_offset -= triangle_count;
        }
    }
}

void remove_entity_by_ptr(scene_t *scene, entity_t *target) {
    if (!scene || !target) return;

    // 1. Handle the triangle pool surgery
    _patch_scene_after_removal(scene, target->pool_offset, target->mesh->triangle_count);

    // 2. Remove from pointer list and FREE
    for (size_t i = 0; i < scene->numentities; i++) {
        if (scene->entities[i] == target) {
            // Swap-and-Pop
            scene->entities[i] = scene->entities[scene->numentities - 1];//The order doesn't matter so we can just swap with the last element
            scene->numentities--;
            
            free(target); 
            return;
        }
    }
}