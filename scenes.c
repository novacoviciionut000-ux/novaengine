#include "scenes.h"
#define CHUNK 16
#define EXPECTED_TRIANGLES 4096
scene_t *create_scene() {
    scene_t* scene = calloc(1, sizeof(scene_t));
    if(scene == NULL) return NULL;

    scene->allocated_mem = CHUNK;
    scene->allocated_triangles = EXPECTED_TRIANGLES;
    
    // Core Entity List
    scene->entities = calloc(scene->allocated_mem, sizeof(entity_t*));
    
    // Primary Triangle Pool
    scene->triangles = calloc(scene->allocated_triangles, sizeof(triangle_t));
    
    // Rendering Buffers (Using the *2 safety multiplier for clipping)
    scene->render_usage = calloc(scene->allocated_triangles * 2, sizeof(triangle_t));
    scene->verts = calloc(scene->allocated_triangles * 2 * 3, sizeof(SDL_Vertex));
    
    // Radix Sort & Clipping Buffers
    scene->transient_buffer = calloc(scene->allocated_triangles * 2 * 3, sizeof(vec4_t));
    scene->indices = calloc(scene->allocated_triangles * 2, sizeof(uint32_t));
    scene->temp_indices = calloc(scene->allocated_triangles * 2, sizeof(uint32_t));

    // Simple one-time check for all allocations
    if(!scene->entities || !scene->triangles || !scene->render_usage || 
       !scene->verts || !scene->transient_buffer || !scene->indices || !scene->temp_indices) {
        if(scene -> entities)free(scene->entities);
        if(scene -> triangles)free(scene->triangles);
        if(scene->render_usage)free(scene->render_usage);
        if(scene->verts)free(scene->verts);
        if(scene->transient_buffer)free(scene->transient_buffer);
        if(scene->indices)free(scene->indices);
        if(scene->temp_indices)free(scene->temp_indices);
        return NULL;
    }

    return scene;
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
        triangle_t *t2 = realloc(scene->render_usage, new_alloc_count * 2 * sizeof(triangle_t));
        SDL_Vertex *t3 = realloc(scene->verts, new_alloc_count * 2 * 3 * sizeof(SDL_Vertex));
        vec4_t *t4 = realloc(scene->transient_buffer, new_alloc_count * 2 * 3 * sizeof(vec4_t));
        uint32_t *t5 = realloc(scene->indices, new_alloc_count * 2 * sizeof(uint32_t));
        uint32_t *t6 = realloc(scene->temp_indices, new_alloc_count * 2 * sizeof(uint32_t));

        // FIXED: Added t5 and t6 to the safety check
        if(!t1 || !t2 || !t3 || !t4 || !t5 || !t6) return false;

        scene->triangles = t1;
        scene->render_usage = t2;
        scene->verts = t3;
        scene->transient_buffer = t4;
        scene->indices = t5;
        scene->temp_indices = t6;
        scene->allocated_triangles = new_alloc_count;
    }

    // 3. Register Entity and Link Triangles
    entity->pool_offset = scene->numtriangles; // Set the key BEFORE incrementing
    scene->entities[scene->numentities] = entity;
    scene->numentities++;

    add_triangles(entity, scene, entity->pool_offset);
    
    scene->numtriangles = new_total_tris; // Finalize the count
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
void destroy_scene(scene_t *scene) {
    if (!scene) return;

    // 1. Free all entities still in the list
    for (size_t i = 0; i < scene->numentities; i++) {
        free(scene->entities[i]); 
    }
    free(scene->entities);

    // 2. Free all the master triangle and vertex pools
    free(scene->triangles);
    free(scene->render_usage);
    free(scene->verts);
    
    // 3. Free the Radix Sort and Clipping scratchpads
    free(scene->transient_buffer);
    free(scene->indices);
    free(scene->temp_indices);

    // 4. Finally, free the scene struct itself
    free(scene);
}