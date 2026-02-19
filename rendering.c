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
    return dot_product > -4.0f ? true:false;//i added a buffer to prevent clipping, this is to be removed in the future as it causes objects to appear off the screen 
}
bool painters_algorithm(entity_t *entity){
    //we sort the triangles of the entity by their average depth, and then we render them in order from farthest to closest, this is a very simple implementation of the painter's algorithm, it is not very efficient but it works for our purposes, since we are only rendering a few entities with a few triangles each, it should be fine for now, but if we want to render more complex scenes with more entities and more triangles, we will need to implement a more efficient sorting algorithm, such as quicksort or mergesort, or we can use a spatial data structure such as a BSP tree or a octree to sort the triangles more efficiently
    int triangle_count = entity->mesh->triangle_count;
    int* triangle_map = entity->mesh->triangle_map;
    vec4_t* world_verts = entity->mesh->camera_verts;
    for(int i = 0; i < triangle_count * 3; i+=3){
        int idx0 = triangle_map[i];
        int idx1 = triangle_map[i+1];
        int idx2 = triangle_map[i+2];
        int triangle_idx = i / 3;
        entity->triangles[triangle_idx].idx0 = idx0;
        entity->triangles[triangle_idx].idx1 = idx1;
        entity->triangles[triangle_idx].idx2 = idx2;
        entity->triangles[triangle_idx].avg_depth = (world_verts[idx0].z + world_verts[idx1].z + world_verts[idx2].z) / 3.0f;
    }
    qsort(entity->triangles, triangle_count, sizeof(triangle_t), cmp);
    int actual_pos = 0;
    for(int i = 0; i < triangle_count; i++){
        int idx0 = entity->triangles[i].idx0;
        int idx1 = entity->triangles[i].idx1;
        int idx2 = entity->triangles[i].idx2;
        entity->mesh->triangle_map[actual_pos++] = idx0;
        entity->mesh->triangle_map[actual_pos++] = idx1;
        entity->mesh->triangle_map[actual_pos++] = idx2;
    }
    return true;
}
SDL_Vertex vec4tovert(entity_t *entity, vec4_t *vec){
    SDL_Vertex vert = {0};
    vert.color = entity->color;
    vert.position = vec4_to_screen_fpoint(vec);
    return vert;
}
bool fill_entity(entity_t *entity, SDL_Renderer *renderer){
    if(!painters_algorithm(entity)){
        return false;
    }
    int curr_vertex = 0;
    for(int i = 0; i < entity->mesh->triangle_count * 3; i+=3){
        int idx0 = entity->mesh->triangle_map[i];
        int idx1 = entity->mesh->triangle_map[i+1];
        int idx2 = entity->mesh->triangle_map[i+2];
        vec4_t vert0 = entity->mesh->camera_verts[idx0];
        vec4_t vert1 = entity->mesh->camera_verts[idx1];
        vec4_t vert2 = entity->mesh->camera_verts[idx2];
        if(vert0.z <= 0.001f || vert1.z <= 0.001f || vert2.z <= 0.001f){
            continue;
        }


        SDL_Vertex v1 = vec4tovert(entity,&vert0);
        SDL_Vertex v2 = vec4tovert(entity,&vert1);
        SDL_Vertex v3 = vec4tovert(entity,&vert2);



        
        entity->vertices[curr_vertex++] = v1;
        entity->vertices[curr_vertex++] = v2;
        entity->vertices[curr_vertex++] = v3;
 
    }
    if(curr_vertex == 0)return true;
    if(!SDL_RenderGeometry(renderer, NULL, entity->vertices, curr_vertex, NULL, 0)){
        return false;

    }   
    return true;
}
bool wireframerender(entity_t *entity, SDL_Renderer *renderer){
    for(int i = 0; i < entity->mesh->indice_count; i += 2){
        int index1 = entity->mesh->indice_map[i];
        int index2 = entity->mesh->indice_map[i+1];
        vec4_t vert1 = entity->mesh->camera_verts[index1];
        vec4_t vert2 = entity->mesh->camera_verts[index2];
        if(vert1.z <= 0.001f || vert2.z <= 0.001f){
            continue;
        }
        SDL_Point point1 = vec4_to_screen_point(&vert1);
        SDL_Point point2 = vec4_to_screen_point(&vert2);
        SDL_RenderLine(renderer, point1.x, point1.y, point2.x, point2.y);
    }
    return true;
}
bool render_entity(entity_t *entity, SDL_Renderer *renderer){
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    if(entity->mesh->indice_map != NULL){
        if(!wireframerender(entity, renderer)){
            return false;
        }
    }
    if(entity->mesh->triangle_map != NULL){// if the triangle map is NULL, we do not render it (nullable because we may want a wireframe entity)
        if(!fill_entity(entity,renderer)){
            return false;
        }
    }
    return true;

}
bool render_entities(entity_t **entity, SDL_Renderer *renderer, int entity_count,camera_t *cam){
    for(int i = 0; i < entity_count; i++){
        if(in_sight(&entity[i]->pos, cam)){
            if(!render_entity(entity[i], renderer)){
                return false;
            }
        }
    }
    return true;
}