#include "rendering.h"
int cmp(const void* ptrA, const void* ptrb){
    if(((triangle_t*)ptrA)->avg_depth < ((triangle_t*)ptrb)->avg_depth) return 1;
    if(((triangle_t*)ptrA)->avg_depth > ((triangle_t*)ptrb)->avg_depth) return -1;
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
real backfacecull(vec4_t *v1, vec4_t *v2, vec4_t *v3){
    vec4_t negative1 = scale_vec4(*v1, -1.0f);
    vec4_t side_1 = add_vec4(v3, &negative1);
    vec4_t side_2 = add_vec4(v2, &negative1);
    vec3_t real_side1 = vec4tovec3(&side_1);
    vec3_t real_side2 = vec4tovec3(&side_2);
    vec3_t normal = crossprod3(&real_side1, &real_side2);
    vec4_t tmp  = (scale_vec4(*v1,-1.0f));
    vec3_t view = vec4tovec3(&tmp);
    return dotprod3(&normal,&view);
}
bool painters_algorithm(entity_t *entity, int* actual_count){
    //we sort the triangles of the entity by their average depth, and then we render them in order from farthest to closest, this is a very simple implementation of the painter's algorithm, it is not very efficient but it works for our purposes, since we are only rendering a few entities with a few triangles each, it should be fine for now, but if we want to render more complex scenes with more entities and more triangles, we will need to implement a more efficient sorting algorithm, such as quicksort or mergesort, or we can use a spatial data structure such as a BSP tree or a octree to sort the triangles more efficiently
    int triangle_count = entity->mesh->triangle_count;
    int* triangle_map = entity->mesh->triangle_map;
    vec4_t* world_verts = entity->mesh->camera_verts;
    int visible_count = 0;
    for(int i = 0; i < triangle_count * 3; i+=3){
        int idx0 = triangle_map[i];
        int idx1 = triangle_map[i+1];
        int idx2 = triangle_map[i+2];//basically, we take the pairs of 3 from the triangle_map (indices of the vertices that form the triangles)
        vec4_t v1 = entity->mesh->camera_verts[idx0];
        vec4_t v2 = entity->mesh->camera_verts[idx1];
        vec4_t v3 = entity->mesh->camera_verts[idx2];//these are the points that make up the triangle
        if(backfacecull(&v1,&v2,&v3) > 0.001f) continue;
        


        entity->triangles[visible_count].idx0 = idx0;
        entity->triangles[visible_count].idx1 = idx1;//then, we put them in their corresponding triangle slot in the entity's triangle buffer(already allocated at creation)
        entity->triangles[visible_count].idx2 = idx2;//the actual source of the vertices we use for rendering are made below based on this step.
        entity->triangles[visible_count].avg_depth = (world_verts[idx0].z + world_verts[idx1].z + world_verts[idx2].z) / 3.0f;
        //i implemented it this like this because before that, i was allocating a triangles array on each frame and freeing it. That was a performance killer
        visible_count++;
    }
    qsort(entity->triangles, visible_count, sizeof(triangle_t), cmp);
    int actual_pos = 0;
    *actual_count = visible_count;
    return true;
}
SDL_Vertex vec4tovert(entity_t *entity, vec4_t *vec){
    SDL_Vertex vert = {0};
    vert.color = entity->color;
    vert.position = vec4_to_screen_fpoint(vec);
    return vert;
}
bool fill_entity(entity_t *entity, SDL_Renderer *renderer){
    int visible_count = 0;
    if(!painters_algorithm(entity, &visible_count)){
        return false;
    }
    int curr_vertex = 0;
    for(int i = 0; i < visible_count; i++){
        int idx0 = entity->triangles[i].idx0;
        int idx1 = entity->triangles[i].idx1;
        int idx2 = entity->triangles[i].idx2;
        vec4_t vert0 = entity->mesh->camera_verts[idx0];//we now take the vertices that make up the triangle (3D space)
        vec4_t vert1 = entity->mesh->camera_verts[idx1];
        vec4_t vert2 = entity->mesh->camera_verts[idx2];
        if(vert0.z <= 0.001f || vert1.z <= 0.001f || vert2.z <= 0.001f){
            continue;
        }
        //culling logic MUST come before the arithmetic in vec4tovert which basically does expensive operations to convert 3d coordinates to screen space.

        SDL_Vertex v1 = vec4tovert(entity,&vert0);// we convert them to SDL_Vertex which is in 2D space(vec4tovert calls projection_math.c functions, check that)
        SDL_Vertex v2 = vec4tovert(entity,&vert1);
        SDL_Vertex v3 = vec4tovert(entity,&vert2);



        //there are 2d vertices, they are already sorted so we can just feed them to render_geometry without any indice map.
        entity->vertices[curr_vertex++] = v1;//now we feed them into the entity's vertices(they are already ordered by painter's algorithm)
        entity->vertices[curr_vertex++] = v2;
        entity->vertices[curr_vertex++] = v3;
 
    }
    if(curr_vertex == 0)return true;
    //now we feed the vertices(pairs of 3's technically) into SDL_RenderGeometry with the map parameter set to NULL, as they already in-order
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
    if(entity->mesh->indice_map != NULL){//this is nullable because in most cases we DO NOT want a wireframe entity, mostly for testing purposes.
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

//NOTE: If we spawn something behind a bunny, the thing behind it clips through. To prevent this, we need a Z-Buffer, basically , we take all the triangle data, store it in a massive array, and only then use painter's algorithm.
//also, maybe swap the quicksort for radix-sort.