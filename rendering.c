#include "rendering.h"
int cmp(const void* ptrA, const void* ptrB) {
    const triangle_t* a = (const triangle_t*)ptrA;
    const triangle_t* b = (const triangle_t*)ptrB;

    if (a->avg_depth < b->avg_depth) return 1;//trebuie crescator
    if (a->avg_depth > b->avg_depth) return -1;
    
    return 0;
}

real backfacecull(vec4_t *v1, vec4_t *v2, vec4_t *v3){
    vec4_t negative1 = scale_vec4(*v1, -1.0f);
    
    // SWAPPED THESE TWO LINES
    vec4_t side_1 = add_vec4(v2, &negative1); // side_1 is now v2 - v1
    vec4_t side_2 = add_vec4(v3, &negative1); // side_2 is now v3 - v1
    
    vec3_t real_side1 = vec4tovec3(&side_1);
    vec3_t real_side2 = vec4tovec3(&side_2);
    
    // Normalize sides to prevent the NaN floating point bug on small triangles!
    real_side1 = normalize_vec3(&real_side1);
    real_side2 = normalize_vec3(&real_side2);
    
    vec3_t normal = crossprod3(&real_side1, &real_side2);
    normal = normalize_vec3(&normal);
    
    vec4_t tmp  = (scale_vec4(*v1, -1.0f));
    vec3_t view = vec4tovec3(&tmp);
    view = normalize_vec3(&view);
    
    return dotprod3(&normal, &view);
}
size_t sync_scene(scene_t *scene){
    size_t visible = 0;
    for(size_t i = 0; i < scene -> numtriangles;i++){
        vec4_t *v1 = scene->triangles[i].camera_positions[0];
        vec4_t *v2 = scene->triangles[i].camera_positions[1];
        vec4_t *v3 = scene->triangles[i].camera_positions[2];
        if(v1->z < 0.01f || v2->z < 0.01f || v3->z < 0.01f)continue;
        real dot = 0;
        if((dot = backfacecull(v1,v2,v3)) < -0.001f)continue;
        real intensity = dot;
        float light = (intensity * 0.8f) + 0.2f; 
        if (light > 1.0f) light = 1.0f;
        real z1 = v1->z;
        real z2 = v2->z;
        real z3 = v3->z;
        scene -> triangles[i].avg_depth = (z1 + z2 + z3) / 3;
        scene->render_usage[visible] = scene->triangles[i];
        scene->render_usage[visible].color.r *= light;
        scene->render_usage[visible].color.g *= light;
        scene->render_usage[visible].color.b *= light;//some basic shading.
        visible++;
    }
    if(visible != 0)qsort(scene->render_usage, visible, sizeof(triangle_t), cmp);
    return visible;
}
SDL_Vertex vec4tovert(vec4_t *vec, SDL_FColor color){
    SDL_Vertex vert = {0};
    vert.color = color;
    vert.position = vec4_to_screen_fpoint(vec);
    return vert;
}
void fill_verts(scene_t *scene, size_t visible){
    size_t index = 0;
    for(size_t i = 0; i < visible;i++){
        scene->verts[index++] = vec4tovert(scene->render_usage[i].camera_positions[0], scene->render_usage[i].color);
        scene->verts[index++] = vec4tovert(scene->render_usage[i].camera_positions[1], scene->render_usage[i].color);
        scene->verts[index++] = vec4tovert(scene->render_usage[i].camera_positions[2], scene->render_usage[i].color);
    }
}
bool render_scene(scene_t *scene, SDL_Renderer *renderer){
    size_t visible = sync_scene(scene);
    if(visible == 0)return false;
    fill_verts(scene, visible);
    if(!SDL_RenderGeometry(renderer, NULL, scene->verts, visible * 3, NULL, 0))return false;
    return true;
}