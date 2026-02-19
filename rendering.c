#include "rendering.h"
int cmp(const void* ptrA, const void* ptrb){
    if(((triangle_t*)ptrA)->avg_depth < ((triangle_t*)ptrb)->avg_depth) return -1;
    if(((triangle_t*)ptrA)->avg_depth > ((triangle_t*)ptrb)->avg_depth) return 1;
    return 0;
}
vec4_t get_forward(camera_t *cam){
    vec4_t forward = {0};
    forward.x = sinf(cam->angles.y);
    forward.y = 0.0f;
    forward.z = cosf(cam->angles.y);
    forward.w = 0.0f;
    return forward;
}
bool in_sight(const vec4_t *vert, camera_t *cam){
    vec4_t forward = get_forward(cam);
    vec4_t dir_to_vert = (vec4_t){
        .x = vert->x - cam->pos.x,
        .y = vert->y - cam->pos.y,
        .z = vert->z - cam->pos.z,
        .w = 0.0f
    };
    real dot_product = dotprod4(&dir_to_vert, &forward);
    return dot_product > -100000.0f ? true:false;//i added a buffer to prevent clipping
}
bool painters_algorithm(entity_t *entity){
    //we sort the triangles of the entity by their average depth, and then we render them in order from farthest to closest, this is a very simple implementation of the painter's algorithm, it is not very efficient but it works for our purposes, since we are only rendering a few entities with a few triangles each, it should be fine for now, but if we want to render more complex scenes with more entities and more triangles, we will need to implement a more efficient sorting algorithm, such as quicksort or mergesort, or we can use a spatial data structure such as a BSP tree or a octree to sort the triangles more efficiently
    int triangle_count = entity->mesh->triangle_count;
    int* triangle_map = entity->mesh->triangle_map;
    vec4_t* world_verts = entity->mesh->world_verts;
    triangle_t *triangles = malloc(triangle_count * sizeof(triangle_t));
    if(!triangles) return false;
    for(int i = 0; i < triangle_count * 3; i+=3){
        int idx0 = triangle_map[i];
        int idx1 = triangle_map[i+1];
        int idx2 = triangle_map[i+2];
        int triangle_idx = i / 3;
        triangles[triangle_idx].idx0 = idx0;
        triangles[triangle_idx].idx1 = idx1;
        triangles[triangle_idx].idx2 = idx2;
        triangles[triangle_idx].avg_depth = (world_verts[idx0].z + world_verts[idx1].z + world_verts[idx2].z) / 3.0f;
    }
    qsort(triangles, triangle_count, sizeof(triangle_t), cmp);
    int actual_pos = 0;
    for(int i = 0; i < triangle_count; i++){
        int idx0 = triangles[i].idx0;
        int idx1 = triangles[i].idx1;
        int idx2 = triangles[i].idx2;
        entity->mesh->triangle_map[actual_pos++] = idx0;
        entity->mesh->triangle_map[actual_pos++] = idx1;
        entity->mesh->triangle_map[actual_pos++] = idx2;
    }
    free(triangles);//!!!!!!!!!!!!!!!TO DO : OPTIMIZE THIS, we are allocating and freeing every frame which WILL be a performance killer.
    return true;
}

bool fill_entity(entity_t *entity, SDL_Renderer *renderer,camera_t *cam){
    if(!painters_algorithm(entity)){
        return false;
    }
    for(int i = 0; i < entity->mesh->triangle_count * 3; i+=3){
        int idx0 = entity->mesh->triangle_map[i];
        int idx1 = entity->mesh->triangle_map[i+1];
        int idx2 = entity->mesh->triangle_map[i+2];
        vec4_t vert0 = entity->mesh->world_verts[idx0];
        vec4_t vert1 = entity->mesh->world_verts[idx1];
        vec4_t vert2 = entity->mesh->world_verts[idx2];
        if(!in_sight(&vert0, cam) && !in_sight(&vert1, cam) && !in_sight(&vert2, cam)){
            continue;
        }
        if(vert0.z <= 0.05 || vert1.z <= 0.05 || vert2.z <= 0.05){
            continue;
        }
        SDL_FPoint p0 = vec4_to_screen_fpoint(&vert0);
        SDL_FPoint p1 = vec4_to_screen_fpoint(&vert1);
        SDL_FPoint p2 = vec4_to_screen_fpoint(&vert2);

        
        SDL_Vertex vertices[3] = {
            {.position = p0, .color = {1.0f, 0.0f, 0.0f, 1.0f}},
            {.position = p1, .color = {1.0f, 0.0f, 0.0f, 1.0f}},
            {.position = p2, .color = {1.0f, 0.0f, 0.0f, 1.0f}}
        };
        if(!SDL_RenderGeometry(renderer, NULL, vertices, 3, NULL, 0)){
            return false;

        }   
    }
    return true;
}
bool wireframerender(entity_t *entity, SDL_Renderer *renderer,camera_t* cam){
    for(int i = 0; i < entity->mesh->indice_count; i += 2){
        int index1 = entity->mesh->indice_map[i];
        int index2 = entity->mesh->indice_map[i+1];
        vec4_t vert1 = entity->mesh->world_verts[index1];
        vec4_t vert2 = entity->mesh->world_verts[index2];
        if(vert1.z <= 0.1) vert1.z = 0.1;
        if(vert2.z <= 0.1) vert2.z = 0.1;
        if(!in_sight(&vert1, cam) && !in_sight(&vert2, cam)){
            continue;
        }
        if(vert1.z <= 0.05 || vert2.z <= 0.05){
            continue;
        }
        SDL_Point point1 = vec4_to_screen_point(&vert1);
        SDL_Point point2 = vec4_to_screen_point(&vert2);
        SDL_RenderLine(renderer, point1.x, point1.y, point2.x, point2.y);
    }
    return true;
}
bool render_entity(entity_t *entity, SDL_Renderer *renderer,camera_t* cam){
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    if(entity->mesh->indice_map != NULL){
        if(!wireframerender(entity, renderer, cam)){
            return false;
        }
    }
    if(entity->mesh->triangle_map != NULL){// if the triangle map is NULL, we do not render it (nullable because we may want a wireframe entity)
        if(!fill_entity(entity,renderer, cam)){
            return false;
        }
    }
    return true;

}
bool render_entities(entity_t **entity, SDL_Renderer *renderer, int entity_count,camera_t *cam){
    for(int i = 0; i < entity_count; i++){
        if(in_sight(&entity[i]->pos, cam)){
            if(!render_entity(entity[i], renderer, cam)){
                return false;
            }
        }
    }
    return true;
}