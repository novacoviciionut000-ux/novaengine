#ifndef RENDERING_H
#define RENDERING_H
#include "defines.h"
#include "projection_math.h"
#include <immintrin.h>
#include <stdlib.h>
typedef enum { PLANE_NEAR, PLANE_LEFT, PLANE_RIGHT, PLANE_TOP, PLANE_BOTTOM } frustum_plane_t;

// Initialization & Core Rendering
void init_starfield(render_manager_t *manager);
bool render(render_manager_t *manager, SDL_Renderer *renderer, bool wireframe, camera_t *cam);
int init_thread_pool(render_manager_t *manager, int num_workers, size_t max_triangles_per_thread);
void shutdown_thread_pool(render_manager_t *manager);
// Pipeline Steps
size_t sync_renderer(render_manager_t *manager, camera_t *cam);
void fill_verts(render_manager_t *manager, size_t visible, real focal_length);

// Visual FX & Overlays
void draw_stars(SDL_Renderer *renderer, const camera_t *cam, render_manager_t *manager);
void render_crosshair(SDL_Renderer *renderer);
void render_triangle_wireframe(SDL_Renderer* renderer, SDL_Vertex v1, SDL_Vertex v2);

// Math & Culling
real backfacecull(vec4_t *v1, vec4_t *v2, vec4_t *v3);
bool is_triangle_outside(vec4_t *v1, vec4_t *v2, vec4_t *v3, float mx, float my);
void radix_sort_triangles(triangle_t *triangles, uint32_t *indices, uint32_t *temp_indices, size_t count);
SDL_Vertex vec4tovert(vec4_t *vec, SDL_FColor color, real focal_length);

// Sutherland-Hodgman Clipping
bool is_inside(vec4_t *p, frustum_plane_t plane, float margin_x, float margin_y);
vec4_t* intersect(vec4_t *p1, vec4_t *p2, frustum_plane_t plane, float margin_x, float margin_y, render_manager_t *manager);
int clip_polygon_against_plane(vec4_t **in_v, int in_count, vec4_t **out_v, frustum_plane_t plane, float mx, float my, render_manager_t *manager);

#endif // RENDERING_H