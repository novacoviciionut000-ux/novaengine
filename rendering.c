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
vec4_t* intersect_near_plane(vec4_t *p1, vec4_t *p2, scene_t *scene) {
    float z_near = 0.01f;
    float t = (z_near - p1->z) / (p2->z - p1->z);
    
    //the next slot in the transient buffer
    vec4_t *out = &scene->transient_buffer[scene->transient_count++];
    
    out->x = p1->x + t * (p2->x - p1->x);
    out->y = p1->y + t * (p2->y - p1->y);
    out->z = z_near;
    out->w = p1->w + t * (p2->w - p1->w); 
    
    // Return the safe pointer
    return out;
}
int clip_triangle(triangle_t *in_tri, triangle_t *out_tri_1, triangle_t *out_tri_2, scene_t *scene) {
    float z_near = 0.01f;
    
    vec4_t* inside_pts[3];  int inside_count = 0;
    vec4_t* outside_pts[3]; int outside_count = 0;

    // Sort the pointers into inside/outside
    for (int i = 0; i < 3; i++) {
        if (in_tri->camera_positions[i]->z >= z_near) {
            inside_pts[inside_count++] = in_tri->camera_positions[i];
        } else {
            outside_pts[outside_count++] = in_tri->camera_positions[i];
        }
    }

    if (inside_count == 0) return 0; // Entirely behind

    if (inside_count == 3) { // Entirely in front
        *out_tri_1 = *in_tri; 
        return 1;
    }

    if (inside_count == 1 && outside_count == 2) { // 1 Point Inside (Forms 1 Triangle)
        out_tri_1->color = in_tri->color;
        
        out_tri_1->camera_positions[0] = inside_pts[0];
        out_tri_1->camera_positions[1] = intersect_near_plane(inside_pts[0], outside_pts[0], scene);
        out_tri_1->camera_positions[2] = intersect_near_plane(inside_pts[0], outside_pts[1], scene);
        
        return 1;
    }

    if (inside_count == 2 && outside_count == 1) { // 2 Points Inside (Forms 2 Triangles)
        out_tri_1->color = in_tri->color;
        out_tri_2->color = in_tri->color;
        
        out_tri_1->camera_positions[0] = inside_pts[0];
        out_tri_1->camera_positions[1] = inside_pts[1];
        out_tri_1->camera_positions[2] = intersect_near_plane(inside_pts[0], outside_pts[0], scene);
        
        out_tri_2->camera_positions[0] = inside_pts[1];
        out_tri_2->camera_positions[1] = out_tri_1->camera_positions[2]; 
        out_tri_2->camera_positions[2] = intersect_near_plane(inside_pts[1], outside_pts[0], scene);
        
        return 2;
    }
    
    return 0;
}
size_t sync_scene(scene_t *scene){
    size_t visible = 0;
    scene->transient_count = 0;
    
    // Calculate margins once
    const float margin_x = (SCREEN_WIDTH / 2.0f) / FOCAL;
    const float margin_y = (SCREEN_HEIGHT / 2.0f) / FOCAL;

    for(size_t i = 0; i < scene->numtriangles; i++){
        vec4_t *v1 = scene->triangles[i].camera_positions[0];
        vec4_t *v2 = scene->triangles[i].camera_positions[1];
        vec4_t *v3 = scene->triangles[i].camera_positions[2];
        
        
        // 1. BACKFACE CULL FIRST
        real dot = backfacecull(v1, v2, v3);
        if(dot < -0.001f) continue;
        
        real intensity = dot;
        float light = (intensity * 0.8f) + 0.2f; 
        if (light > 1.0f) light = 1.0f;
        
        // 2. CLIP TRIANGLE
        triangle_t clipped[2];
        int num_clipped = clip_triangle(&scene->triangles[i], &clipped[0], &clipped[1], scene);

        // 3. PROCESS THE SURVIVING PIECES
        for (int n = 0; n < num_clipped; n++) {
            // Grab pointers to the newly clipped vertices
            vec4_t *cv1 = clipped[n].camera_positions[0];
            vec4_t *cv2 = clipped[n].camera_positions[1];
            vec4_t *cv3 = clipped[n].camera_positions[2];

            // 4. FRUSTUM CULLING (Moved inside the loop, using the new cv pointers!)
            if (cv1->x < -cv1->z * margin_x && cv2->x < -cv2->z * margin_x && cv3->x < -cv3->z * margin_x) continue;
            if (cv1->x > cv1->z * margin_x && cv2->x > cv2->z * margin_x && cv3->x > cv3->z * margin_x) continue;
            if (cv1->y < -cv1->z * margin_y && cv2->y < -cv2->z * margin_y && cv3->y < -cv3->z * margin_y) continue;
            if (cv1->y > cv1->z * margin_y && cv2->y > cv2->z * margin_y && cv3->y > cv3->z * margin_y) continue;

            // 5. RECALCULATE DEPTH AND PUSH TO RENDERER
            clipped[n].avg_depth = (cv1->z + cv2->z + cv3->z) / 3.0f;
            
            scene->render_usage[visible] = clipped[n];
            scene->render_usage[visible].color.r *= light;
            scene->render_usage[visible].color.g *= light;
            scene->render_usage[visible].color.b *= light;
            
            visible++;
        }
    }
    
    if(visible != 0) qsort(scene->render_usage, visible, sizeof(triangle_t), cmp);
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