#include "rendering.h"

// --- Sutherland-Hodgman Helper Functions ---

typedef enum { PLANE_NEAR, PLANE_LEFT, PLANE_RIGHT, PLANE_TOP, PLANE_BOTTOM } frustum_plane_t;

bool is_inside(vec4_t *p, frustum_plane_t plane, float margin_x, float margin_y) {
    switch (plane) {
        case PLANE_NEAR:   return p->z >= 0.01f;
        case PLANE_LEFT:   return p->x >= -p->z * margin_x;
        case PLANE_RIGHT:  return p->x <=  p->z * margin_x;
        case PLANE_TOP:    return p->y <=  p->z * margin_y;
        case PLANE_BOTTOM: return p->y >= -p->z * margin_y;
    }
    return false;
}

void init_starfield(scene_t *scene) {//just for fun and to have a "frame of reference" in the void :)
    for (int i = 0; i < NUM_STARS; i++) {
        // Randomly scatter stars in a sphere with radius 1
        float theta = (float)rand() / RAND_MAX * 2.0f * M_PI;
        float phi = acosf(2.0f * ((float)rand() / RAND_MAX) - 1.0f);
        
        scene->star_field[i].x = sinf(phi) * cosf(theta);
        scene->star_field[i].y = sinf(phi) * sinf(theta);
        scene->star_field[i].z = cosf(phi);
        scene->star_field[i].w = 1.0f;
    }
}
void draw_stars(SDL_Renderer *renderer, const camera_t *cam, scene_t *scene) {
    // 1. Create the rotation matrix to transform world-space stars to camera-space
    mat4_t rotation = mat4_identity();
    // We use the same inverse angles as the world transformation
    rotation = rot_x(&rotation, cam->angles.x, true);
    rotation = rot_y(&rotation, -cam->angles.y, true);

    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255); 

    for (int i = 0; i < NUM_STARS; i++) {
        // 2. Rotate the star vector
        vec4_t rotated = apply_transform(&rotation, &scene->star_field[i]);

        // 3. Since stars have no translation, rotated.z is their actual depth
        if (rotated.z > 0.1f) {
            // Project the star into screen coordinates
            SDL_FPoint screen_pos = vec4_to_screen_fpoint(&rotated);
            
            if (screen_pos.x >= 0 && screen_pos.x < SCREEN_WIDTH &&
                screen_pos.y >= 0 && screen_pos.y < SCREEN_HEIGHT) {
                
                // Draw a 2x2 star so it's actually visible
                SDL_FRect star_rect = {screen_pos.x, screen_pos.y, 2, 2};
                SDL_RenderFillRect(renderer, &star_rect);
            }
        }
    }
}
vec4_t* intersect(vec4_t *p1, vec4_t *p2, frustum_plane_t plane, float margin_x, float margin_y, scene_t *scene) {
    float t = 0;
    switch (plane) {
        case PLANE_NEAR:   t = (0.1f - p1->z) / (p2->z - p1->z); break;
        case PLANE_LEFT:   t = (-p1->z * margin_x - p1->x) / ((p2->x - p1->x) + (p2->z - p1->z) * margin_x); break;
        case PLANE_RIGHT:  t = ( p1->z * margin_x - p1->x) / ((p2->x - p1->x) - (p2->z - p1->z) * margin_x); break;
        case PLANE_TOP:    t = ( p1->z * margin_y - p1->y) / ((p2->y - p1->y) - (p2->z - p1->z) * margin_y); break;
        case PLANE_BOTTOM: t = (-p1->z * margin_y - p1->y) / ((p2->y - p1->y) + (p2->z - p1->z) * margin_y); break;
    }

    vec4_t *out = &scene->transient_buffer[scene->transient_count++];
    out->x = p1->x + t * (p2->x - p1->x);
    out->y = p1->y + t * (p2->y - p1->y);
    out->z = p1->z + t * (p2->z - p1->z);
    out->w = p1->w + t * (p2->w - p1->w);
    return out;
}

int clip_polygon_against_plane(vec4_t **in_v, int in_count, vec4_t **out_v, frustum_plane_t plane, float mx, float my, scene_t *scene) {
    int out_count = 0;
    for (int i = 0; i < in_count; i++) {
        vec4_t *p1 = in_v[i];
        vec4_t *p2 = in_v[(i + 1) % in_count];

        bool p1_in = is_inside(p1, plane, mx, my);
        bool p2_in = is_inside(p2, plane, mx, my);

        if (p1_in && p2_in) {
            out_v[out_count++] = p2;
        } else if (p1_in && !p2_in) {
            out_v[out_count++] = intersect(p1, p2, plane, mx, my, scene);
        } else if (!p1_in && p2_in) {
            out_v[out_count++] = intersect(p1, p2, plane, mx, my, scene);
            out_v[out_count++] = p2;
        }
    }
    return out_count;
}

// --- End Sutherland-Hodgman Helpers ---

void radix_sort_triangles(triangle_t *triangles, uint32_t *indices, uint32_t *temp_indices, size_t count) {
    uint32_t *in = indices;
    uint32_t *out = temp_indices;

    for (size_t i = 0; i < count; i++) in[i] = i;

    for (int shift = 0; shift < 32; shift += 8) {
        size_t buckets[256] = {0};

        // 1. Counting Pass
        for (size_t i = 0; i < count; i++) {
            float depth = triangles[in[i]].avg_depth;
            uint32_t depth_bits;
            
            // Safe bit-cast to avoid strict-aliasing warnings
            memcpy(&depth_bits, &depth, sizeof(uint32_t));
            
            // creates a descending order (Far to Near).
            depth_bits = ~depth_bits;

            uint8_t byte_val = (depth_bits >> shift) & 0xFF;
            buckets[byte_val]++;
        }

        // 2. Prefix Sum
        size_t current_offset = 0;
        for (int i = 0; i < 256; i++) {
            size_t bucket_size = buckets[i];
            buckets[i] = current_offset;
            current_offset += bucket_size;
        }

        // 3. Sorting Pass
        for (size_t i = 0; i < count; i++) {
            float depth = triangles[in[i]].avg_depth;
            uint32_t depth_bits;
            
            memcpy(&depth_bits, &depth, sizeof(uint32_t));
            depth_bits = ~depth_bits; // Must match the counting pass flip!

            uint8_t byte_val = (depth_bits >> shift) & 0xFF;
            out[buckets[byte_val]++] = in[i];
        }

        // 4. Swap buffers
        uint32_t *tmp = in;
        in = out;
        out = tmp;
    }
}
bool is_triangle_outside(vec4_t *v1, vec4_t *v2, vec4_t *v3, float mx, float my) {
    // 1. Find the bounds of the triangle
    float min_z = v1->z; if (v2->z < min_z) min_z = v2->z; if (v3->z < min_z) min_z = v3->z;
    float max_z = v1->z; if (v2->z > max_z) max_z = v2->z; if (v3->z > max_z) max_z = v3->z;

    // Fast-fail: entirely behind the near plane
    if (max_z < 0.01f) return true;

    float min_x = v1->x; if (v2->x < min_x) min_x = v2->x; if (v3->x < min_x) min_x = v3->x;
    float max_x = v1->x; if (v2->x > max_x) max_x = v2->x; if (v3->x > max_x) max_x = v3->x;
    
    // Check X margins
    if (min_x >  max_z * mx && min_x >  min_z * mx) return true;
    if (max_x < -max_z * mx && max_x < -min_z * mx) return true;

    float min_y = v1->y; if (v2->y < min_y) min_y = v2->y; if (v3->y < min_y) min_y = v3->y;
    float max_y = v1->y; if (v2->y > max_y) max_y = v2->y; if (v3->y > max_y) max_y = v3->y;

    // Check Y margins
    if (min_y >  max_z * my && min_y >  min_z * my) return true;
    if (max_y < -max_z * my && max_y < -min_z * my) return true;

    return false; // Not entirely outside
}
real backfacecull(vec4_t *v1, vec4_t *v2, vec4_t *v3){
    vec4_t negative1 = scale_vec4(*v1, -1.0f);
    
    vec4_t side_1 = add_vec4(v2, &negative1); // side_1 is  v2 - v1
    vec4_t side_2 = add_vec4(v3, &negative1); // side_2 is  v3 - v1
    
    vec3_t real_side1 = vec4tovec3(&side_1);
    vec3_t real_side2 = vec4tovec3(&side_2);
    
    // Normalize sides to prevent the NaN floating point bug on small triangles
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
        if(dot < 0.01f) continue;
        if (is_triangle_outside(v1, v2, v3, margin_x, margin_y)) continue; // if the triangle is outside, there is no point of running the math.
        
        real intensity = dot;
        float light = (intensity * 0.8f) + 0.2f; 
        if (light > 1.0f) light = 1.0f;

        // 2. CLIP POLYGON (Sutherland-Hodgman)
        vec4_t *polygon[10];
        vec4_t *temp_poly[10];
        int poly_count = 3;

        polygon[0] = v1; polygon[1] = v2; polygon[2] = v3;

        for (int plane = 0; plane < 5; plane++) {
            poly_count = clip_polygon_against_plane(polygon, poly_count, temp_poly, (frustum_plane_t)plane, margin_x, margin_y, scene);
            for (int k = 0; k < poly_count; k++) polygon[k] = temp_poly[k];
            if (poly_count < 3) break;
        }

        if (poly_count < 3) continue;

        // 3. PROCESS THE SURVIVING PIECES (Triangle Fanning)
        for (int n = 1; n < poly_count - 1; n++) {
            vec4_t *cv1 = polygon[0];
            vec4_t *cv2 = polygon[n];
            vec4_t *cv3 = polygon[n + 1];

            triangle_t tri = scene->triangles[i];
            tri.camera_positions[0] = cv1;
            tri.camera_positions[1] = cv2;
            tri.camera_positions[2] = cv3;

            // 1. Keep Z-axis for perfect Painter's Algorithm sorting
            tri.avg_depth = (cv1->z + cv2->z + cv3->z) / 3.0f;

            // 2. Calculate Radial Distance locally for the fog
            real center_x = (cv1->x + cv2->x + cv3->x) / 3.0f;
            real center_y = (cv1->y + cv2->y + cv3->y) / 3.0f;
            real center_z = tri.avg_depth; 

            real radial_distance = sqrtf((center_x * center_x) + 
                                        (center_y * center_y) + 
                                        (center_z * center_z));

            // 3. Apply the fog using the spherical radial distance
            real fog_density = 0.5f;
            real fog_factor = exp(-radial_distance * fog_density);
            if(fog_factor < 0.03f)continue;
            // Clamp it so it doesn't go crazy
            if (fog_factor > 1.0f) fog_factor = 1.0f;

            tri.color.r *= light * fog_factor;
            tri.color.g *= light * fog_factor;
            tri.color.b *= light * fog_factor;
            tri.dprod = dot;

            scene->render_usage[visible++] = tri;
        }
    }
    
    if(visible != 0) radix_sort_triangles(scene->render_usage, scene->indices, scene->temp_indices, visible);
    return visible;
}

SDL_Vertex vec4tovert(vec4_t *vec, SDL_FColor color){
    SDL_Vertex vert = {0};
    vert.color = color;
    vert.position = vec4_to_screen_fpoint(vec);
    return vert;
}

void render_triangle_wireframe(SDL_Renderer* renderer, SDL_Vertex v1, SDL_Vertex v2) {
    SDL_SetRenderDrawColorFloat(renderer, 0.0f, 1.0f, 0.0f, 1.0f);
    SDL_FPoint contour_points[2] = {
        v1.position,
        v2.position,
    };

    SDL_RenderLines(renderer, contour_points, 2);
}

void fill_verts(scene_t *scene, size_t visible){
    size_t vert_index = 0;
    for(size_t i = 0; i < visible; i++){
        uint32_t tri_idx = scene->indices[i];
        triangle_t *tri = &scene->render_usage[tri_idx];
        SDL_Vertex v1 = vec4tovert(tri->camera_positions[0], tri->color);
        SDL_Vertex v2 = vec4tovert(tri->camera_positions[1], tri->color);
        SDL_Vertex v3 = vec4tovert(tri->camera_positions[2], tri->color);
        scene->verts[vert_index++] = v1;
        scene->verts[vert_index++] = v2;
        scene->verts[vert_index++] = v3;
    }
}

bool render_scene(scene_t *scene, SDL_Renderer *renderer, bool wireframe, camera_t *cam){
    size_t visible = sync_scene(scene);
    SDL_SetRenderDrawColor(renderer, 0, 5, 0, 255); // Dark green void
    SDL_RenderClear(renderer);
    draw_stars(renderer, cam,scene);
    if(visible == 0) return false;
    fill_verts(scene, visible); 

    
    // 2. Draw the solid geometry FIRST
    if(!SDL_RenderGeometry(renderer, NULL, scene->verts, visible * 3, NULL, 0)) return false;
    
    // 3. Draw the outlines ON TOP
    if(wireframe){
        for(size_t i = 0; i < visible; i++){
            triangle_t *tri = &scene->render_usage[scene->indices[i]];
            if(tri->dprod <= 0.5f && tri->dprod >= -0.5f){
                SDL_Vertex v1 = vec4tovert(tri->camera_positions[0], tri->color);
                SDL_Vertex v2 = vec4tovert(tri->camera_positions[1], tri->color);
                SDL_Vertex v3 = vec4tovert(tri->camera_positions[2], tri->color);
                
                // Draw all 3 lines to complete the contour
                render_triangle_wireframe(renderer, v1, v2);
                render_triangle_wireframe(renderer, v2, v3);
                render_triangle_wireframe(renderer, v3, v1);
            }
        }
    }
    
    return true;
}