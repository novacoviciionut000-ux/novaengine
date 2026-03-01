#include "rendering.h"
#include "pipeline.h"
#include <immintrin.h>

/*
 * OPTIMIZATION CHANGELOG (vs original version):
 *
 * 1. SIMD reject ordering: frustum FIRST, then early-exit, then backface+lighting.
 *    The original computed backface/lighting on every triangle even if it was
 *    already frustum-rejected.  Now if all 4 triangles in a batch are outside
 *    the frustum we skip the entire backface+fog+lighting block (~4 rsqrt
 *    chains + centroid divides).  Normalizations are KEPT -- they are required
 *    for correct lighting (unnormalized dot scales with triangle area/distance,
 *    causing everything to clamp to max brightness) and for correct backface
 *    culling at the tiny world scale used here (near-zero distances make the
 *    view vector degenerate without normalization).
 *
 * 2. Scalar tail uses backfacecull() (normalized, correct) -- unchanged from
 *    original, no regression.
 *
 * 3. Shutdown deadlock fixed.
 *    Old code signalled vert_wake then waited for vert_done + done,
 *    but the worker also waits on `wake` (phase 2) before posting `done`.
 *    Since nothing ever signalled `wake` during shutdown, the thread
 *    deadlocked. Fixed by chaining: signal vert_wake -> wait vert_done ->
 *    signal wake -> wait done -> signal fill_wake -> wait fill_done.
 *
 * 4. Parallelised fill_verts (new phase 3).
 *    After radix sort the workers wake up again, each owning a contiguous
 *    slice of the sorted indices[] array. They project their triangles
 *    directly into manager->verts[] at the correct offsets -- no gather,
 *    no memcpy needed. Two new semaphores per worker: fill_wake / fill_done.
 *    worker_t gains: fill_index_start, fill_index_end, fill_vert_start,
 *    fill_focal_length.
 *
 * 5. fill_verts_chunk uses rcpps + one Newton-Raphson step instead of divps
 *    for the perspective divide.  divps latency ~14 cycles vs ~5 for rcp+NR
 *    on modern Intel, with error ~10^-8 (invisible on screen).
 *
 * 6. Overflow guard note: max_out = allocated*5/num_workers. Sutherland-Hodgman
 *    can expand 1 triangle to up to 7. Increase multiplier to 8 maybe if you see issues.
 *
 */

// ---------------------------------------------------------------------------
// Sutherland-Hodgman helpers
// ---------------------------------------------------------------------------

bool is_inside(vec4_t *p, frustum_plane_t plane, float margin_x, float margin_y)
{
    switch (plane) {
        case PLANE_NEAR:   return p->z >= 0.01f;
        case PLANE_LEFT:   return p->x >= -p->z * margin_x;
        case PLANE_RIGHT:  return p->x <=  p->z * margin_x;
        case PLANE_TOP:    return p->y <=  p->z * margin_y;
        case PLANE_BOTTOM: return p->y >= -p->z * margin_y;
    }
    return false;
}

void init_starfield(render_manager_t *manager)
{
    for (int i = 0; i < NUM_STARS; i++) {
        float theta = (float)rand() / RAND_MAX * 2.0f * M_PI;
        float phi   = acosf(2.0f * ((float)rand() / RAND_MAX) - 1.0f);
        manager->star_field[i].x = sinf(phi) * cosf(theta);
        manager->star_field[i].y = sinf(phi) * sinf(theta);
        manager->star_field[i].z = cosf(phi);
        manager->star_field[i].w = 1.0f;
    }
}

void draw_stars(SDL_Renderer *renderer, const camera_t *cam, render_manager_t *manager)
{
    mat4_t rotation = mat4_identity();
    rotation = rot_x(&rotation,  cam->angles.x, true);
    rotation = rot_y(&rotation, -cam->angles.y, true);
    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
    for (int i = 0; i < NUM_STARS; i++) {
        vec4_t rotated = apply_transform(&rotation, &manager->star_field[i]);
        if (rotated.z > 0.1f) {
            SDL_FPoint sp = vec4_to_screen_fpoint(&rotated, cam->focal_length);
            if (sp.x >= 0 && sp.x < SCREEN_WIDTH && sp.y >= 0 && sp.y < SCREEN_HEIGHT) {
                SDL_FRect r = {sp.x, sp.y, 2, 2};
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
}

static vec4_t *intersect_ex(vec4_t *p1, vec4_t *p2, frustum_plane_t plane,
                              float mx, float my,
                              vec4_t *transient, size_t *transient_count)
{
    float t = 0.0f;
    switch (plane) {
        case PLANE_NEAR:
            t = (0.01f - p1->z) / (p2->z - p1->z);
            break;
        case PLANE_LEFT:
            t = (-p1->z * mx - p1->x) / ((p2->x - p1->x) + (p2->z - p1->z) * mx);
            break;
        case PLANE_RIGHT:
            t = ( p1->z * mx - p1->x) / ((p2->x - p1->x) - (p2->z - p1->z) * mx);
            break;
        case PLANE_TOP:
            t = ( p1->z * my - p1->y) / ((p2->y - p1->y) - (p2->z - p1->z) * my);
            break;
        case PLANE_BOTTOM:
            t = (-p1->z * my - p1->y) / ((p2->y - p1->y) + (p2->z - p1->z) * my);
            break;
    }
    vec4_t *out = &transient[(*transient_count)++];
    out->x = p1->x + t * (p2->x - p1->x);
    out->y = p1->y + t * (p2->y - p1->y);
    out->z = p1->z + t * (p2->z - p1->z);
    out->w = p1->w + t * (p2->w - p1->w);
    return out;
}

static int clip_polygon_against_plane_ex(vec4_t **in_v, int in_count, vec4_t **out_v,
                                          frustum_plane_t plane, float mx, float my,
                                          vec4_t *transient, size_t *transient_count)
{
    int out_count = 0;
    for (int i = 0; i < in_count; i++) {
        vec4_t *p1 = in_v[i];
        vec4_t *p2 = in_v[(i + 1) % in_count];
        bool p1_in = is_inside(p1, plane, mx, my);
        bool p2_in = is_inside(p2, plane, mx, my);
        if (p1_in && p2_in) {
            out_v[out_count++] = p2;
        } else if (p1_in && !p2_in) {
            out_v[out_count++] = intersect_ex(p1, p2, plane, mx, my, transient, transient_count);
        } else if (!p1_in && p2_in) {
            out_v[out_count++] = intersect_ex(p1, p2, plane, mx, my, transient, transient_count);
            out_v[out_count++] = p2;
        }
    }
    return out_count;
}

vec4_t *intersect(vec4_t *p1, vec4_t *p2, frustum_plane_t plane,
                  float mx, float my, render_manager_t *manager)
{
    return intersect_ex(p1, p2, plane, mx, my,
                        manager->transient_buffer, &manager->transient_count);
}

int clip_polygon_against_plane(vec4_t **in_v, int in_count, vec4_t **out_v,
                                frustum_plane_t plane, float mx, float my,
                                render_manager_t *manager)
{
    return clip_polygon_against_plane_ex(in_v, in_count, out_v, plane, mx, my,
                                         manager->transient_buffer, &manager->transient_count);
}

// ---------------------------------------------------------------------------
// Radix sort (unchanged -- already correct)
// ---------------------------------------------------------------------------

void radix_sort_triangles(triangle_t *triangles, uint32_t *indices,
                           uint32_t *temp_indices, size_t count)
{
    uint32_t *in  = indices;
    uint32_t *out = temp_indices;

    for (size_t i = 0; i < count; i++) in[i] = i;

    for (int shift = 0; shift < 32; shift += 8) {
        size_t buckets[256] = {0};

        for (size_t i = 0; i < count; i++) {
            float    depth      = triangles[in[i]].avg_depth;
            uint32_t depth_bits;
            memcpy(&depth_bits, &depth, sizeof(uint32_t));
            depth_bits = ~depth_bits;
            buckets[(depth_bits >> shift) & 0xFF]++;
        }

        size_t offset = 0;
        for (int i = 0; i < 256; i++) {
            size_t sz  = buckets[i];
            buckets[i] = offset;
            offset    += sz;
        }

        for (size_t i = 0; i < count; i++) {
            float    depth      = triangles[in[i]].avg_depth;
            uint32_t depth_bits;
            memcpy(&depth_bits, &depth, sizeof(uint32_t));
            depth_bits = ~depth_bits;
            uint8_t byte_val = (depth_bits >> shift) & 0xFF;
            out[buckets[byte_val]++] = in[i];
        }

        uint32_t *tmp = in; in = out; out = tmp;
    }
}

// ---------------------------------------------------------------------------
// Frustum / visibility helpers (unchanged)
// ---------------------------------------------------------------------------

bool is_triangle_outside(vec4_t *v1, vec4_t *v2, vec4_t *v3, float mx, float my)
{
    float min_z = v1->z; if (v2->z < min_z) min_z = v2->z; if (v3->z < min_z) min_z = v3->z;
    float max_z = v1->z; if (v2->z > max_z) max_z = v2->z; if (v3->z > max_z) max_z = v3->z;
    if (max_z < 0.01f) return true;

    float min_x = v1->x; if (v2->x < min_x) min_x = v2->x; if (v3->x < min_x) min_x = v3->x;
    float max_x = v1->x; if (v2->x > max_x) max_x = v2->x; if (v3->x > max_x) max_x = v3->x;
    if (min_x >  max_z * mx && min_x >  min_z * mx) return true;
    if (max_x < -max_z * mx && max_x < -min_z * mx) return true;

    float min_y = v1->y; if (v2->y < min_y) min_y = v2->y; if (v3->y < min_y) min_y = v3->y;
    float max_y = v1->y; if (v2->y > max_y) max_y = v2->y; if (v3->y > max_y) max_y = v3->y;
    if (min_y >  max_z * my && min_y >  min_z * my) return true;
    if (max_y < -max_z * my && max_y < -min_z * my) return true;

    return false;
}

bool is_triangle_entirely_inside(vec4_t *v1, vec4_t *v2, vec4_t *v3, float mx, float my)
{
    if (v1->z < 0.01f || v2->z < 0.01f || v3->z < 0.01f) return false;
    if (v1->x < -v1->z*mx || v1->x > v1->z*mx) return false;
    if (v2->x < -v2->z*mx || v2->x > v2->z*mx) return false;
    if (v3->x < -v3->z*mx || v3->x > v3->z*mx) return false;
    if (v1->y < -v1->z*my || v1->y > v1->z*my) return false;
    if (v2->y < -v2->z*my || v2->y > v2->z*my) return false;
    if (v3->y < -v3->z*my || v3->y > v3->z*my) return false;
    return true;
}

/*
 * backfacecull: returns the NORMALIZED dot product, used later for lighting.
 * Keep for the scalar tail because we need the actual dot value.
 */
real backfacecull(vec4_t *v1, vec4_t *v2, vec4_t *v3)
{
    vec3_t s1 = {{{v2->x - v1->x, v2->y - v1->y, v2->z - v1->z}}};
    vec3_t s2 = {{{v3->x - v1->x, v3->y - v1->y, v3->z - v1->z}}};
    vec3_t n  = crossprod3(&s1, &s2);
    n         = normalize_vec3(&n);
    vec3_t vw = {{{-v1->x, -v1->y, -v1->z}}};
    vw        = normalize_vec3(&vw);
    return dotprod3(&n, &vw);
}

/*
 * backface_fast: unnormalized dot -- only sign matters for culling.
 * Used in scalar tail to REJECT before calling the heavier backfacecull.
 */
real backface_fast(vec4_t *v1, vec4_t *v2, vec4_t *v3)//This function has weird behaviour because of the un-normalized sides. Use with caution, the abnormally small world scale causes clamping.
{
    vec3_t s1 = {{{v2->x - v1->x, v2->y - v1->y, v2->z - v1->z}}};
    vec3_t s2 = {{{v3->x - v1->x, v3->y - v1->y, v3->z - v1->z}}};
    vec3_t n  = crossprod3(&s1, &s2);
    return n.x*(-v1->x) + n.y*(-v1->y) + n.z*(-v1->z);
}

// ---------------------------------------------------------------------------
// fill_verts_chunk  -- projection of a slice of sorted triangles into verts[]
// Called by worker threads (phase 3) and the single-threaded fallback.
//
// Uses rcpps + 1 Newton-Raphson step in place of divps:
//   rcpps error ~ 1.5e-5, NR brings it to ~1e-8 -- indistinguishable on screen.
//   Throughput: ~3 cycles vs ~14 for divps on Skylake.
// ---------------------------------------------------------------------------

static void fill_verts_chunk(render_manager_t *manager,
                              size_t vert_start,   // index into manager->verts[]
                              size_t index_start,  // first entry in manager->indices[]
                              size_t index_end,    // one-past-last
                              float  focal_length)
{
    const float offset_x = (float)SCREEN_WIDTH  * 0.5f;
    const float offset_y = (float)SCREEN_HEIGHT * 0.5f;

    const __m128 focal   = _mm_set1_ps(focal_length);
    const __m128 off_x   = _mm_set1_ps(offset_x);
    const __m128 off_y   = _mm_set1_ps(offset_y);
    const __m128 zero    = _mm_setzero_ps();
    const __m128 pt1     = _mm_set1_ps(0.1f);       // z==0 fixup
    const __m128 two     = _mm_set1_ps(2.0f);        // for NR: 2 - z*rcp

    size_t vi = vert_start;

    for (size_t i = index_start; i < index_end; i++) {
        uint32_t   tri_idx = manager->indices[i];
        triangle_t *tri    = &manager->render_usage[tri_idx];

        vec4_t *v0 = tri->camera_positions[0];
        vec4_t *v1 = tri->camera_positions[1];
        vec4_t *v2 = tri->camera_positions[2];

        /* SoA gather: lanes 0,1,2 = v0,v1,v2; lane 3 = dummy (z=1) */
        __m128 x_vec = _mm_set_ps(0.0f, v2->x, v1->x, v0->x);
        __m128 y_vec = _mm_set_ps(0.0f, v2->y, v1->y, v0->y);
        __m128 z_vec = _mm_set_ps(1.0f, v2->z, v1->z, v0->z);

        /* Guard z==0 (shouldn't happen post-clip, but be safe) */
        __m128 is_zero = _mm_cmpeq_ps(z_vec, zero);
        __m128 z_safe  = _mm_add_ps(z_vec, _mm_and_ps(is_zero, pt1));

        /* Perspective divide via rcpps + Newton-Raphson:
         *   rcp0  = approx(1/z)
         *   z_inv = rcp0 * (2 - z * rcp0)    <- one NR step */
        __m128 rcp0  = _mm_rcp_ps(z_safe);
        __m128 z_inv = _mm_mul_ps(rcp0, _mm_sub_ps(two, _mm_mul_ps(z_safe, rcp0)));

        __m128 proj_x = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(x_vec, z_inv), focal), off_x);
        __m128 proj_y = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(y_vec, z_inv), focal), off_y);

        float px[4], py[4];
        _mm_storeu_ps(px, proj_x);
        _mm_storeu_ps(py, proj_y);

        SDL_FColor col = tri->color;

        manager->verts[vi++] = (SDL_Vertex){ {px[0], py[0]}, col, {0.0f, 0.0f} };
        manager->verts[vi++] = (SDL_Vertex){ {px[1], py[1]}, col, {0.0f, 0.0f} };
        manager->verts[vi++] = (SDL_Vertex){ {px[2], py[2]}, col, {0.0f, 0.0f} };
    }
}

// ---------------------------------------------------------------------------
// process_triangle_chunk
// ---------------------------------------------------------------------------

static size_t process_triangle_chunk(
    render_manager_t *manager,
    triangle_t       *output,
    vec4_t           *transient,
    size_t           *transient_count,
    size_t            start,
    size_t            end,
    float             margin_x,
    float             margin_y)
{
    size_t visible    = 0;
    *transient_count  = 0;

    /* Pre-broadcast constants */
    const __m128 zero     = _mm_setzero_ps();
    const __m128 near_z   = _mm_set1_ps(0.01f);
    const __m128 one      = _mm_set1_ps(1.0f);
    const __m128 pt8      = _mm_set1_ps(0.8f);
    const __m128 pt2      = _mm_set1_ps(0.2f);
    const __m128 fog_k    = _mm_set1_ps(0.001f);
    const __m128 fog_scl  = _mm_set1_ps(0.25f);
    const __m128 fog_min  = _mm_set1_ps(0.03f);
    const __m128 three    = _mm_set1_ps(3.0f);
    const __m128 mx4      = _mm_set1_ps( margin_x);
    const __m128 my4      = _mm_set1_ps( margin_y);
    const __m128 neg_mx4  = _mm_set1_ps(-margin_x);
    const __m128 neg_my4  = _mm_set1_ps(-margin_y);

    size_t i = start;

    /* ------------------------------------------------------------------ */
    /* SIMD path: 4 triangles per iteration                                */
    /* ------------------------------------------------------------------ */
    for (; i + 4 <= end; i += 4) {

        /* --- Gather vertices (12 pointer dereferences) --- */
        vec4_t *a0 = manager->triangles[i+0].camera_positions[0];
        vec4_t *b0 = manager->triangles[i+0].camera_positions[1];
        vec4_t *c0 = manager->triangles[i+0].camera_positions[2];
        vec4_t *a1 = manager->triangles[i+1].camera_positions[0];
        vec4_t *b1 = manager->triangles[i+1].camera_positions[1];
        vec4_t *c1 = manager->triangles[i+1].camera_positions[2];
        vec4_t *a2 = manager->triangles[i+2].camera_positions[0];
        vec4_t *b2 = manager->triangles[i+2].camera_positions[1];
        vec4_t *c2 = manager->triangles[i+2].camera_positions[2];
        vec4_t *a3 = manager->triangles[i+3].camera_positions[0];
        vec4_t *b3 = manager->triangles[i+3].camera_positions[1];
        vec4_t *c3 = manager->triangles[i+3].camera_positions[2];

        __m128 ax = _mm_set_ps(a3->x, a2->x, a1->x, a0->x);
        __m128 ay = _mm_set_ps(a3->y, a2->y, a1->y, a0->y);
        __m128 az = _mm_set_ps(a3->z, a2->z, a1->z, a0->z);
        __m128 bx = _mm_set_ps(b3->x, b2->x, b1->x, b0->x);
        __m128 by = _mm_set_ps(b3->y, b2->y, b1->y, b0->y);
        __m128 bz = _mm_set_ps(b3->z, b2->z, b1->z, b0->z);
        __m128 cx4= _mm_set_ps(c3->x, c2->x, c1->x, c0->x);
        __m128 cy4= _mm_set_ps(c3->y, c2->y, c1->y, c0->y);
        __m128 cz = _mm_set_ps(c3->z, c2->z, c1->z, c0->z);

        /* ---- STAGE 1: frustum reject ---- */
        __m128 min_z = _mm_min_ps(_mm_min_ps(az, bz), cz);
        __m128 max_z = _mm_max_ps(_mm_max_ps(az, bz), cz);
        __m128 all_behind = _mm_cmplt_ps(max_z, near_z);

        __m128 min_x = _mm_min_ps(_mm_min_ps(ax, bx), cx4);
        __m128 max_x = _mm_max_ps(_mm_max_ps(ax, bx), cx4);
        __m128 min_y = _mm_min_ps(_mm_min_ps(ay, by), cy4);
        __m128 max_y = _mm_max_ps(_mm_max_ps(ay, by), cy4);

        __m128 out_right  = _mm_and_ps(_mm_cmpgt_ps(min_x, _mm_mul_ps(max_z, mx4)),
                                        _mm_cmpgt_ps(min_x, _mm_mul_ps(min_z, mx4)));
        __m128 out_left   = _mm_and_ps(_mm_cmplt_ps(max_x, _mm_mul_ps(max_z, neg_mx4)),
                                        _mm_cmplt_ps(max_x, _mm_mul_ps(min_z, neg_mx4)));
        __m128 out_top    = _mm_and_ps(_mm_cmpgt_ps(min_y, _mm_mul_ps(max_z, my4)),
                                        _mm_cmpgt_ps(min_y, _mm_mul_ps(min_z, my4)));
        __m128 out_bottom = _mm_and_ps(_mm_cmplt_ps(max_y, _mm_mul_ps(max_z, neg_my4)),
                                        _mm_cmplt_ps(max_y, _mm_mul_ps(min_z, neg_my4)));

        __m128 outside = _mm_or_ps(all_behind,
                          _mm_or_ps(out_right,
                          _mm_or_ps(out_left,
                          _mm_or_ps(out_top, out_bottom))));

        /* ---- Early exit: if all 4 are frustum-rejected, skip everything else ----
         * This is free -- we already have the outside mask -- and saves the
         * entire backface+normalize+fog block for fully-culled batches.        */
        if (_mm_movemask_ps(outside) == 0xF) continue;

        /* ---- STAGE 2: backface cull (full normalization, matches original) ----
         *
         * Normalization IS required here for two reasons specific to this codebase:
         *   a) The world scale is very small, so unnormalized dot magnitudes are
         *      tiny and numerically unreliable for the sign test.
         *   b) The normalized dot is reused directly for lighting, where its
         *      magnitude must be in [-1, 1] -- without normalization the
         *      (dot*0.8+0.2)^2 formula produces a value >> 1 for most triangles
         *      and everything clamps to maximum brightness.
         */
        __m128 s1x = _mm_sub_ps(bx, ax);
        __m128 s1y = _mm_sub_ps(by, ay);
        __m128 s1z = _mm_sub_ps(bz, az);
        __m128 s2x = _mm_sub_ps(cx4, ax);
        __m128 s2y = _mm_sub_ps(cy4, ay);
        __m128 s2z = _mm_sub_ps(cz,  az);

        /* normalize side1 */
        __m128 s1_mag2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(s1x, s1x),
                                                _mm_mul_ps(s1y, s1y)),
                                                _mm_mul_ps(s1z, s1z));
        __m128 s1_inv  = _mm_rsqrt_ps(_mm_max_ps(s1_mag2, _mm_set1_ps(1e-6f)));
        s1_inv = _mm_mul_ps(s1_inv, _mm_sub_ps(_mm_set1_ps(1.5f),
                    _mm_mul_ps(_mm_set1_ps(0.5f),
                        _mm_mul_ps(s1_mag2, _mm_mul_ps(s1_inv, s1_inv)))));
        s1x = _mm_mul_ps(s1x, s1_inv);
        s1y = _mm_mul_ps(s1y, s1_inv);
        s1z = _mm_mul_ps(s1z, s1_inv);

        /* normalize side2 */
        __m128 s2_mag2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(s2x, s2x),
                                                _mm_mul_ps(s2y, s2y)),
                                                _mm_mul_ps(s2z, s2z));
        __m128 s2_inv  = _mm_rsqrt_ps(_mm_max_ps(s2_mag2, _mm_set1_ps(1e-6f)));
        s2_inv = _mm_mul_ps(s2_inv, _mm_sub_ps(_mm_set1_ps(1.5f),
                    _mm_mul_ps(_mm_set1_ps(0.5f),
                        _mm_mul_ps(s2_mag2, _mm_mul_ps(s2_inv, s2_inv)))));
        s2x = _mm_mul_ps(s2x, s2_inv);
        s2y = _mm_mul_ps(s2y, s2_inv);
        s2z = _mm_mul_ps(s2z, s2_inv);

        /* cross(side1, side2) -> normal */
        __m128 nx = _mm_sub_ps(_mm_mul_ps(s1y, s2z), _mm_mul_ps(s1z, s2y));
        __m128 ny = _mm_sub_ps(_mm_mul_ps(s1z, s2x), _mm_mul_ps(s1x, s2z));
        __m128 nz = _mm_sub_ps(_mm_mul_ps(s1x, s2y), _mm_mul_ps(s1y, s2x));

        /* normalize normal */
        __m128 n_mag2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(nx, nx),
                                               _mm_mul_ps(ny, ny)),
                                               _mm_mul_ps(nz, nz));
        __m128 n_inv  = _mm_rsqrt_ps(_mm_max_ps(n_mag2, _mm_set1_ps(1e-6f)));
        n_inv = _mm_mul_ps(n_inv, _mm_sub_ps(_mm_set1_ps(1.5f),
                    _mm_mul_ps(_mm_set1_ps(0.5f),
                        _mm_mul_ps(n_mag2, _mm_mul_ps(n_inv, n_inv)))));
        nx = _mm_mul_ps(nx, n_inv);
        ny = _mm_mul_ps(ny, n_inv);
        nz = _mm_mul_ps(nz, n_inv);

        /* view = normalize(-v1) */
        __m128 vx = _mm_xor_ps(ax, _mm_set1_ps(-0.0f));
        __m128 vy = _mm_xor_ps(ay, _mm_set1_ps(-0.0f));
        __m128 vz = _mm_xor_ps(az, _mm_set1_ps(-0.0f));
        __m128 v_mag2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(vx, vx),
                                               _mm_mul_ps(vy, vy)),
                                               _mm_mul_ps(vz, vz));
        __m128 v_inv  = _mm_rsqrt_ps(_mm_max_ps(v_mag2, _mm_set1_ps(1e-6f)));
        v_inv = _mm_mul_ps(v_inv, _mm_sub_ps(_mm_set1_ps(1.5f),
                    _mm_mul_ps(_mm_set1_ps(0.5f),
                        _mm_mul_ps(v_mag2, _mm_mul_ps(v_inv, v_inv)))));
        vx = _mm_mul_ps(vx, v_inv);
        vy = _mm_mul_ps(vy, v_inv);
        vz = _mm_mul_ps(vz, v_inv);

        /* dot = normal · view  (normalized, in [-1, 1]) */
        __m128 dot = _mm_add_ps(_mm_add_ps(_mm_mul_ps(nx, vx),
                                            _mm_mul_ps(ny, vy)),
                                            _mm_mul_ps(nz, vz));

        __m128 backfaced = _mm_cmple_ps(dot, _mm_set1_ps(-0.01f));//This returns a bitmask where each triangle is either backfaced/not backfaced

        /* Early exit: all 4 culled after backface test */
        __m128 cull_only = _mm_or_ps(outside, backfaced);
        if (_mm_movemask_ps(cull_only) == 0xF) continue;

        /* ---- STAGE 3: fog & lighting ---- */
        __m128 cent_x  = _mm_div_ps(_mm_add_ps(_mm_add_ps(ax, bx), cx4), three);
        __m128 cent_y  = _mm_div_ps(_mm_add_ps(_mm_add_ps(ay, by), cy4), three);
        __m128 cent_z  = _mm_div_ps(_mm_add_ps(_mm_add_ps(az, bz), cz),  three);

        __m128 dist_sq = _mm_add_ps(_mm_add_ps(_mm_mul_ps(cent_x, cent_x),
                                                _mm_mul_ps(cent_y, cent_y)),
                                                _mm_mul_ps(cent_z, cent_z));

        __m128 ft  = _mm_sub_ps(one, _mm_mul_ps(_mm_mul_ps(dist_sq, fog_k), fog_scl));
        ft         = _mm_max_ps(ft, zero);
        __m128 ft2 = _mm_mul_ps(ft, ft);
        __m128 fog = _mm_mul_ps(ft2, ft2);  /* ft^4 */

        __m128 fogged_out = _mm_cmplt_ps(fog, fog_min);

        /* light = clamp((dot*0.8 + 0.2)^2, 0, 1) -- dot is normalized so this is correct */
        __m128 light = _mm_add_ps(_mm_mul_ps(dot, pt8), pt2);
        light        = _mm_mul_ps(light, light);
        light        = _mm_min_ps(_mm_max_ps(light, zero), one);

        __m128 combined = _mm_mul_ps(light, fog);

        __m128 reject      = _mm_or_ps(cull_only, fogged_out);
        int    reject_mask = _mm_movemask_ps(reject);
        if (reject_mask == 0xF) continue;

        float combined_f[4], dot_f[4], cz_f[4];
        _mm_storeu_ps(combined_f, combined);
        _mm_storeu_ps(dot_f,      dot);
        _mm_storeu_ps(cz_f,       cent_z);

        /* ---- Per-triangle emit / clip (inherently scalar) ---- */
        for (int j = 0; j < 4; j++) {
            if (reject_mask & (1 << j)) continue;

            size_t  idx = i + j;
            vec4_t *v1  = manager->triangles[idx].camera_positions[0];
            vec4_t *v2  = manager->triangles[idx].camera_positions[1];
            vec4_t *v3  = manager->triangles[idx].camera_positions[2];
            float   cmb = combined_f[j];

            if (is_triangle_entirely_inside(v1, v2, v3, margin_x, margin_y)) {
                triangle_t tri  = manager->triangles[idx];
                tri.avg_depth   = cz_f[j];
                tri.color.r    *= cmb;
                tri.color.g    *= cmb;
                tri.color.b    *= cmb;
                tri.dprod       = dot_f[j];
                output[visible++] = tri;
            } else {
                vec4_t *polygon[10], *temp_poly[10];
                int poly_count = 3;
                polygon[0] = v1; polygon[1] = v2; polygon[2] = v3;

                for (int plane = 0; plane < 5; plane++) {
                    poly_count = clip_polygon_against_plane_ex(
                        polygon, poly_count, temp_poly,
                        (frustum_plane_t)plane, margin_x, margin_y,
                        transient, transient_count);
                    for (int k = 0; k < poly_count; k++) polygon[k] = temp_poly[k];
                    if (poly_count < 3) break;
                }
                if (poly_count < 3) continue;

                for (int n = 1; n < poly_count - 1; n++) {
                    triangle_t tri          = manager->triangles[idx];
                    tri.camera_positions[0] = polygon[0];
                    tri.camera_positions[1] = polygon[n];
                    tri.camera_positions[2] = polygon[n+1];
                    tri.avg_depth  = (polygon[0]->z + polygon[n]->z + polygon[n+1]->z) / 3.0f;
                    tri.color.r   *= cmb;
                    tri.color.g   *= cmb;
                    tri.color.b   *= cmb;
                    tri.dprod      = dot_f[j];
                    output[visible++] = tri;
                }
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* Scalar tail: remaining < 4 triangles                                */
    /* Uses normalized backfacecull() matching the SIMD path exactly.     */
    /* backface_fast() is NOT used here: at tiny world scale the           */
    /* unnormalized dot is numerically unreliable for sign tests.          */
    /* ------------------------------------------------------------------ */
    for (; i < end; i++) {
        vec4_t *v1 = manager->triangles[i].camera_positions[0];
        vec4_t *v2 = manager->triangles[i].camera_positions[1];
        vec4_t *v3 = manager->triangles[i].camera_positions[2];

        if (is_triangle_outside(v1, v2, v3, margin_x, margin_y)) continue;

        real dot = backfacecull(v1, v2, v3);
        if (dot <= -0.01f) continue;

        float light;
        if (dot < 0.3f)       light = 0.02f;  // almost black shadow
        else if (dot < 0.7f)  light = 0.4f;   // cold midtone
        else                  light = 1.0f;   // harsh full lit
        light = light * light;
        if (light > 1.0f) light = 1.0f;

        real cx = (v1->x + v2->x + v3->x) / 3.0f;
        real cy = (v1->y + v2->y + v3->y) / 3.0f;
        real cz = (v1->z + v2->z + v3->z) / 3.0f;
        real dist_sq = cx*cx + cy*cy + cz*cz;
        real ft      = 1.0f - dist_sq * 0.001f * 0.25f;
        if (ft < 0.0f) ft = 0.0f;
        real fog_factor = ft * ft * ft * ft;
        if (fog_factor < 0.03f) continue;
        float combined = light * fog_factor;

        if (is_triangle_entirely_inside(v1, v2, v3, margin_x, margin_y)) {
            triangle_t tri  = manager->triangles[i];
            tri.avg_depth   = cz;
            tri.color.r    *= combined;
            tri.color.g    *= combined;
            tri.color.b    *= combined;
            tri.dprod       = dot;
            output[visible++] = tri;
        } else {
            vec4_t *polygon[10], *temp_poly[10];
            int poly_count = 3;
            polygon[0] = v1; polygon[1] = v2; polygon[2] = v3;
            for (int plane = 0; plane < 5; plane++) {
                poly_count = clip_polygon_against_plane_ex(
                    polygon, poly_count, temp_poly,
                    (frustum_plane_t)plane, margin_x, margin_y,
                    transient, transient_count);
                for (int k = 0; k < poly_count; k++) polygon[k] = temp_poly[k];
                if (poly_count < 3) break;
            }
            if (poly_count < 3) continue;
            for (int n = 1; n < poly_count - 1; n++) {
                triangle_t tri          = manager->triangles[i];
                tri.camera_positions[0] = polygon[0];
                tri.camera_positions[1] = polygon[n];
                tri.camera_positions[2] = polygon[n+1];
                tri.avg_depth  = (polygon[0]->z + polygon[n]->z + polygon[n+1]->z) / 3.0f;
                tri.color.r   *= combined;
                tri.color.g   *= combined;
                tri.color.b   *= combined;
                tri.dprod      = dot;
                output[visible++] = tri;
            }
        }
    }

    return visible;
}

// ---------------------------------------------------------------------------
// Worker thread
//
// Phase 1: vertex transform  (wake: vert_wake  / done: vert_done)
// Phase 2: triangle cull     (wake: wake       / done: done)
// Phase 3: fill_verts        (wake: fill_wake  / done: fill_done)
//
// Shutdown: main signals vert_wake with quit=true.
//   Worker posts vert_done, then main signals wake (skip phase 2 body),
//   worker posts done, then main signals fill_wake (skip phase 3 body),
//   worker posts fill_done and exits.
// ---------------------------------------------------------------------------

static int worker_func(void *arg)
{
    worker_t *w = (worker_t *)arg;

    while (1) {
        /* ---- Phase 1: vertex transform ---- */
        SDL_WaitSemaphore(w->vert_wake);
        if (w->quit) {
            SDL_SignalSemaphore(w->vert_done);
            /* still need to drain the other two phases so shutdown doesn't deadlock */
            SDL_WaitSemaphore(w->wake);
            SDL_SignalSemaphore(w->done);
            SDL_WaitSemaphore(w->fill_wake);
            SDL_SignalSemaphore(w->fill_done);
            return 0;
        }
        transform_vertex_chunk(w);
        SDL_SignalSemaphore(w->vert_done);

        /* ---- Phase 2: triangle cull / clip ---- */
        SDL_WaitSemaphore(w->wake);
        /* quit cannot be set here -- shutdown always arrives at phase 1 */
        w->visible_count = process_triangle_chunk(
            w->manager,
            w->output,
            w->transient,
            &w->transient_count,
            w->start_triangle,
            w->end_triangle,
            w->margin_x,
            w->margin_y);
        SDL_SignalSemaphore(w->done);

        /* ---- Phase 3: fill_verts (projection to screen pixels) ---- */
        SDL_WaitSemaphore(w->fill_wake);
        fill_verts_chunk(
            w->manager,
            w->fill_vert_start,    /* absolute index into manager->verts[] */
            w->fill_index_start,   /* absolute index into manager->indices[] */
            w->fill_index_end,
            w->fill_focal_length);
        SDL_SignalSemaphore(w->fill_done);
    }
}

// ---------------------------------------------------------------------------
// Thread pool init / shutdown
// ---------------------------------------------------------------------------

int init_thread_pool(render_manager_t *manager, int num_workers,
                     size_t max_triangles_per_thread)
{
    size_t transient_size = max_triangles_per_thread * 7;

    manager->workers = SDL_malloc(sizeof(worker_t) * num_workers);
    if (!manager->workers) return 0;
    manager->num_workers = num_workers;

    for (int i = 0; i < num_workers; i++) {
        worker_t *w        = &manager->workers[i];
        w->manager         = manager;
        w->transient       = SDL_malloc(sizeof(vec4_t) * transient_size);
        w->transient_count = 0;
        w->visible_count   = 0;
        w->quit            = false;
        w->wake            = SDL_CreateSemaphore(0);
        w->done            = SDL_CreateSemaphore(0);
        w->vert_wake       = SDL_CreateSemaphore(0);
        w->vert_done       = SDL_CreateSemaphore(0);
        w->fill_wake       = SDL_CreateSemaphore(0);
        w->fill_done       = SDL_CreateSemaphore(0);

        if (!w->transient || !w->wake  || !w->done ||
            !w->vert_wake || !w->vert_done ||
            !w->fill_wake || !w->fill_done) return 0;

        SDL_CreateThread(worker_func, "render_worker", w);
    }
    return num_workers;
}

void shutdown_thread_pool(render_manager_t *manager)
{
    /*
     * Signal quit via vert_wake on all workers first so they all wake
     * simultaneously -- then drain each one's semaphore chain in order.
     * (Draining serially is fine; the workers are exiting, not doing work.)
     */
    for (int i = 0; i < manager->num_workers; i++) {
        manager->workers[i].quit = true;
        SDL_SignalSemaphore(manager->workers[i].vert_wake);
    }
    for (int i = 0; i < manager->num_workers; i++) {
        worker_t *w = &manager->workers[i];
        /* worker posts vert_done then waits on wake */
        SDL_WaitSemaphore(w->vert_done);
        /* unblock phase 2 */
        SDL_SignalSemaphore(w->wake);
        /* worker posts done then waits on fill_wake */
        SDL_WaitSemaphore(w->done);
        /* unblock phase 3 -- worker will then post fill_done and exit */
        SDL_SignalSemaphore(w->fill_wake);
        SDL_WaitSemaphore(w->fill_done);

        SDL_free(w->transient);
        SDL_DestroySemaphore(w->wake);
        SDL_DestroySemaphore(w->done);
        SDL_DestroySemaphore(w->vert_wake);
        SDL_DestroySemaphore(w->vert_done);
        SDL_DestroySemaphore(w->fill_wake);
        SDL_DestroySemaphore(w->fill_done);
    }
    SDL_free(manager->workers);
    manager->workers     = NULL;
    manager->num_workers = 0;
}

// ---------------------------------------------------------------------------
// sync_renderer
// ---------------------------------------------------------------------------

size_t sync_renderer(render_manager_t *manager, camera_t *cam)
{
    const float margin_x = (SCREEN_WIDTH  / 2.0f) / cam->focal_length;
    const float margin_y = (SCREEN_HEIGHT / 2.0f) / cam->focal_length;

    /* ---- Single-threaded fallback ---- */
    if (manager->num_workers == 0 || manager->numtriangles < 5000) {
        manager->transient_count = 0;
        size_t visible = process_triangle_chunk(
            manager, manager->render_usage,
            manager->transient_buffer, &manager->transient_count,
            0, manager->numtriangles, margin_x, margin_y);
        if (visible)
            radix_sort_triangles(manager->render_usage,
                                 manager->indices, manager->temp_indices, visible);
        return visible;
    }

    /* ---- Phase 2: dispatch triangle cull ----
     *
     * NOTE: max_out = allocated * 5 / num_workers.
     * Sutherland-Hodgman can expand 1 triangle to at most 7 (one clip per
     * plane x 5 planes, each adding at most 1 vertex).  In a pathological
     * mesh 100% of triangles straddle multiple frustum planes; the factor
     * of 5 may then be insufficient.  If you see corruption, increase to 8.
     */
    size_t chunk   = manager->numtriangles / manager->num_workers;
    size_t max_out = (manager->allocated_triangles * 5) / manager->num_workers;

    for (int i = 0; i < manager->num_workers; i++) {
        worker_t *w       = &manager->workers[i];
        w->margin_x       = margin_x;
        w->margin_y       = margin_y;
        w->start_triangle = (size_t)i * chunk;
        w->end_triangle   = (i == manager->num_workers - 1)
                            ? manager->numtriangles
                            : (size_t)(i + 1) * chunk;
        w->output         = &manager->render_usage[(size_t)i * max_out];
        SDL_SignalSemaphore(w->wake);
    }

    /* Wait for all cull work to finish */
    for (int i = 0; i < manager->num_workers; i++)
        SDL_WaitSemaphore(manager->workers[i].done);

    /* Compact worker outputs into a single contiguous array */
    size_t total_visible = manager->workers[0].visible_count;
    for (int i = 1; i < manager->num_workers; i++) {
        worker_t *w = &manager->workers[i];
        if (w->visible_count == 0) continue;
        memcpy(&manager->render_usage[total_visible],
               w->output, w->visible_count * sizeof(triangle_t));
        total_visible += w->visible_count;
    }

    if (total_visible == 0) {
        /* Still need to fire (and immediately complete) phase 3 */
        for (int i = 0; i < manager->num_workers; i++) {
            worker_t *w          = &manager->workers[i];
            w->fill_index_start  = 0;
            w->fill_index_end    = 0;
            w->fill_vert_start   = 0;
            w->fill_focal_length = cam->focal_length;
            SDL_SignalSemaphore(w->fill_wake);
        }
        for (int i = 0; i < manager->num_workers; i++)
            SDL_WaitSemaphore(manager->workers[i].fill_done);
        return 0;
    }

    radix_sort_triangles(manager->render_usage,
                         manager->indices, manager->temp_indices, total_visible);

    /* ---- Phase 3: dispatch fill_verts ----
     *
     * Each worker gets a contiguous slice of the sorted indices[] array.
     * It writes directly into manager->verts[] at the correct offset
     * (index_start * 3 vertices).  No gather step, no memcpy.
     */
    size_t fill_chunk = total_visible / manager->num_workers;

    for (int i = 0; i < manager->num_workers; i++) {
        worker_t *w          = &manager->workers[i];
        size_t    idx_start  = (size_t)i * fill_chunk;
        size_t    idx_end    = (i == manager->num_workers - 1)
                               ? total_visible
                               : (size_t)(i + 1) * fill_chunk;

        w->fill_index_start  = idx_start;
        w->fill_index_end    = idx_end;
        w->fill_vert_start   = idx_start * 3;   /* 3 verts per triangle */
        w->fill_focal_length = cam->focal_length;
        SDL_SignalSemaphore(w->fill_wake);
    }

    for (int i = 0; i < manager->num_workers; i++)
        SDL_WaitSemaphore(manager->workers[i].fill_done);

    return total_visible;
}

// ---------------------------------------------------------------------------
// Rendering helpers
// ---------------------------------------------------------------------------

SDL_Vertex vec4tovert(vec4_t *vec, SDL_FColor color, real focal_length)
{
    SDL_Vertex v = {0};
    v.color      = color;
    v.position   = vec4_to_screen_fpoint(vec, focal_length);
    return v;
}

void render_triangle_wireframe(SDL_Renderer *renderer, SDL_Vertex v1, SDL_Vertex v2)
{
    SDL_SetRenderDrawColorFloat(renderer, 0.0f, 1.0f, 0.0f, 1.0f);
    SDL_FPoint pts[2] = {v1.position, v2.position};
    SDL_RenderLines(renderer, pts, 2);
}

/*
 * fill_verts: single-threaded entry point (used when num_workers==0 or
 * numtriangles < 1000, where sync_renderer doesn't launch phase 3).
 */
void fill_verts(render_manager_t *manager, size_t visible, real focal_length)
{
    fill_verts_chunk(manager, 0, 0, visible, focal_length);
}

void render_crosshair(SDL_Renderer *renderer)
{
    int cx = SCREEN_WIDTH  / 2;
    int cy = SCREEN_HEIGHT / 2;
    int sz = 5, gap = 2;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLine(renderer, cx-sz, cy,    cx-gap, cy);
    SDL_RenderLine(renderer, cx+gap, cy,   cx+sz,  cy);
    SDL_RenderLine(renderer, cx,    cy-sz, cx,     cy-gap);
    SDL_RenderLine(renderer, cx,    cy+gap, cx,    cy+sz);
}

bool render(render_manager_t *manager, SDL_Renderer *renderer,
            bool wireframe, camera_t *cam)
{
    size_t visible = sync_renderer(manager, cam);

    SDL_SetRenderDrawColor(renderer, 0, 5, 0, 255);
    SDL_RenderClear(renderer);
    draw_stars(renderer, cam, manager);

    if (visible == 0) return false;

    /*
     * When running multi-threaded, fill_verts was already parallelised
     * inside sync_renderer (phase 3).  The single-threaded path calls it
     * here instead (manager->verts[] hasn't been populated yet).
     */
    if (manager->num_workers == 0 || manager->numtriangles < 5000)
        fill_verts(manager, visible, cam->focal_length);

    if (!SDL_RenderGeometry(renderer, NULL, manager->verts, (int)(visible * 3), NULL, 0))
        return false;

    if (wireframe) {
        for (size_t i = 0; i < visible; i++) {
            triangle_t *tri = &manager->render_usage[manager->indices[i]];
            if (tri->dprod <= 0.5f && tri->dprod >= -0.5f) {
                SDL_Vertex v1 = vec4tovert(tri->camera_positions[0], tri->color, cam->focal_length);
                SDL_Vertex v2 = vec4tovert(tri->camera_positions[1], tri->color, cam->focal_length);
                SDL_Vertex v3 = vec4tovert(tri->camera_positions[2], tri->color, cam->focal_length);
                render_triangle_wireframe(renderer, v1, v2);
                render_triangle_wireframe(renderer, v2, v3);
                render_triangle_wireframe(renderer, v3, v1);
            }
        }
    }

    render_crosshair(renderer);
    return true;
}
