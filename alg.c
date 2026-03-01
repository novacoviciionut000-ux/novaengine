#include "alg.h"

// ============================================================
//  INTERNAL HELPERS
// ============================================================

/*
 * Horizontal sum of all 4 lanes of a __m128.
 * Uses movehdup + movehl to avoid the slower hadd_ps.
 * 3 instructions, no memory round-trips.
 */
FORCE_INLINE real hsum_ps(__m128 v) {
    __m128 shuf = _mm_movehdup_ps(v);        // [v1, v1, v3, v3]
    __m128 sum  = _mm_add_ps(v, shuf);       // [v0+v1, -, v2+v3, -]
    shuf        = _mm_movehl_ps(shuf, sum);  // [v2+v3, -, -, -]
    return _mm_cvtss_f32(_mm_add_ss(sum, shuf));
}

// ============================================================
//  MAT4 MULTIPLY
//
//  Strategy: transpose B into 4 column vectors up front, then
//  for each of A's 4 rows we do 4 hsum(row * col) ops.
//  This minimises re-loading B and keeps everything in xmm regs.
//  With -O2 the compiler will keep b_col0..3 live in registers.
// ============================================================
HOT mat4_t mlt_mat4(const mat4_t * restrict matA,
                    const mat4_t * restrict matB) {
    mat4_t res;

    // Pre-build B's columns (column j = B->m[0][j], B->m[1][j], ...)
    // _mm_set_ps arg order is (lane3, lane2, lane1, lane0)
    __m128 bc0 = _mm_set_ps(matB->m[3][0], matB->m[2][0], matB->m[1][0], matB->m[0][0]);
    __m128 bc1 = _mm_set_ps(matB->m[3][1], matB->m[2][1], matB->m[1][1], matB->m[0][1]);
    __m128 bc2 = _mm_set_ps(matB->m[3][2], matB->m[2][2], matB->m[1][2], matB->m[0][2]);
    __m128 bc3 = _mm_set_ps(matB->m[3][3], matB->m[2][3], matB->m[1][3], matB->m[0][3]);

    for (int i = 0; i < 4; i++) {
        __m128 row = _mm_loadu_ps(matA->m[i]);
        res.m[i][0] = hsum_ps(_mm_mul_ps(row, bc0));
        res.m[i][1] = hsum_ps(_mm_mul_ps(row, bc1));
        res.m[i][2] = hsum_ps(_mm_mul_ps(row, bc2));
        res.m[i][3] = hsum_ps(_mm_mul_ps(row, bc3));
    }

    return res;
}

// ============================================================
//  MAT4 ADD — 4 x _mm_add_ps covers all 16 floats
// ============================================================
HOT mat4_t add_mat4(const mat4_t * restrict matA,
                    const mat4_t * restrict matB) {
    mat4_t res;
    _mm_storeu_ps(res.m[0], _mm_add_ps(_mm_loadu_ps(matA->m[0]), _mm_loadu_ps(matB->m[0])));
    _mm_storeu_ps(res.m[1], _mm_add_ps(_mm_loadu_ps(matA->m[1]), _mm_loadu_ps(matB->m[1])));
    _mm_storeu_ps(res.m[2], _mm_add_ps(_mm_loadu_ps(matA->m[2]), _mm_loadu_ps(matB->m[2])));
    _mm_storeu_ps(res.m[3], _mm_add_ps(_mm_loadu_ps(matA->m[3]), _mm_loadu_ps(matB->m[3])));
    return res;
}

// ============================================================
//  MAT3 — 9 floats; not worth SIMD (alignment holes + overhead)
// ============================================================
mat3_t mlt_mat3(const mat3_t * restrict matA,
                const mat3_t * restrict matB) {
    mat3_t res = {0};
    for (int i = 0; i < 3; i++)
        for (int k = 0; k < 3; k++) {
            // Hoist A[i][k] out of the inner loop — reduces loads
            real aik = matA->m[i][k];
            for (int j = 0; j < 3; j++)
                res.m[i][j] += aik * matB->m[k][j];
        }
    return res;
}

mat3_t add_mat3(const mat3_t * restrict matA,
                const mat3_t * restrict matB) {
    mat3_t res;
    for (int i = 0; i < 9; i++)
        res.v[i] = matA->v[i] + matB->v[i];
    return res;
}

// ============================================================
//  VECTOR DOT PRODUCTS
// ============================================================

/*
 * vec3 dot: load x,y,z into 128-bit reg with 0 in w lane,
 * then dp_ps with mask 0x71 (dot lanes 0-2, write to lane 0).
 */
PURE real dotprod3(const vec3_t * restrict vecA,
                   const vec3_t * restrict vecB) {
    __m128 a = _mm_set_ps(0.0f, vecA->z, vecA->y, vecA->x);
    __m128 b = _mm_set_ps(0.0f, vecB->z, vecB->y, vecB->x);
    return _mm_cvtss_f32(_mm_dp_ps(a, b, 0x71));
}

/*
 * vec4 dot: vals[] union member gives us a contiguous float[4]
 * pointer for an unaligned SSE load, then dp_ps 0xF1.
 */
PURE real dotprod4(const vec4_t * restrict vecA,
                   const vec4_t * restrict vecB) {
    __m128 a = _mm_loadu_ps(vecA->vals);
    __m128 b = _mm_loadu_ps(vecB->vals);
    return _mm_cvtss_f32(_mm_dp_ps(a, b, 0xF1));
}

// ============================================================
//  CROSS PRODUCTS
//
//  The classic SIMD shuffle trick for cross product:
//    a × b = [ ay*bz - az*by,
//              az*bx - ax*bz,
//              ax*by - ay*bx ]
//
//  Using shuffle masks (y,z,x,w) = _MM_SHUFFLE(3,0,2,1)
//                    (z,x,y,w) = _MM_SHUFFLE(3,1,0,2)
//
//  This is 2 shuffles + 2 muls + 1 sub — no branches, no
//  scalar extraction.
// ============================================================
FORCE_INLINE __m128 cross_sse(__m128 a, __m128 b) {
    __m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 a_zxy = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_zxy = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
    return _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
}

vec3_t crossprod3(const vec3_t * restrict vecA,
                  const vec3_t * restrict vecB) {
    __m128 a   = _mm_set_ps(0.0f, vecA->z, vecA->y, vecA->x);
    __m128 b   = _mm_set_ps(0.0f, vecB->z, vecB->y, vecB->x);
    __m128 res = cross_sse(a, b);
    vec3_t out;
    out.x = _mm_cvtss_f32(res);
    out.y = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(1,1,1,1)));
    out.z = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(2,2,2,2)));
    return out;
}

vec4_t crossprod4(const vec4_t * restrict vecA,
                  const vec4_t * restrict vecB) {
    __m128 a   = _mm_loadu_ps(vecA->vals);
    __m128 b   = _mm_loadu_ps(vecB->vals);
    __m128 res = cross_sse(a, b);
    // zero w lane
    res = _mm_and_ps(res, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1)));
    vec4_t out;
    _mm_storeu_ps(out.vals, res);
    return out;
}

// ============================================================
//  VECTOR ADD / SCALE
// ============================================================
vec3_t add_vec3(const vec3_t * restrict vecA,
                const vec3_t * restrict vecB) {
    vec3_t out;
    out.x = vecA->x + vecB->x;
    out.y = vecA->y + vecB->y;
    out.z = vecA->z + vecB->z;
    return out;
}
// NOTE: vec3 is only 12 bytes; unaligned SSE load spans 16 bytes and may
// cross a cache line or page boundary — scalar is safer and equally fast here.

vec4_t add_vec4(const vec4_t * restrict vecA,
                const vec4_t * restrict vecB) {
    vec4_t out;
    _mm_storeu_ps(out.vals, _mm_add_ps(_mm_loadu_ps(vecA->vals),
                                        _mm_loadu_ps(vecB->vals)));
    return out;
}

vec3_t scale_vec3(vec3_t vec, real factor) {
    return (vec3_t){{{ vec.x * factor, vec.y * factor, vec.z * factor }}};
}

vec4_t scale_vec4(vec4_t vec, real factor) {
    vec4_t out;
    _mm_storeu_ps(out.vals, _mm_mul_ps(_mm_loadu_ps(vec.vals),
                                        _mm_set1_ps(factor)));
    return out;
}

// ============================================================
//  IN-PLACE POINTER WRAPPERS
// ============================================================
void mlt_mat4_p (mat4_t *d, const mat4_t *a, const mat4_t *b) { *d = mlt_mat4(a, b); }
void mlt_mat3_p (mat3_t *d, const mat3_t *a, const mat3_t *b) { *d = mlt_mat3(a, b); }
void add_mat4_p (mat4_t *d, const mat4_t *a, const mat4_t *b) { *d = add_mat4(a, b); }
void add_mat3_p (mat3_t *d, const mat3_t *a, const mat3_t *b) { *d = add_mat3(a, b); }
void crossprod3_p(vec3_t *d, const vec3_t *a, const vec3_t *b) { *d = crossprod3(a, b); }
void crossprod4_p(vec4_t *d, const vec4_t *a, const vec4_t *b) { *d = crossprod4(a, b); }
void add_vec3_p  (vec3_t *d, const vec3_t *a, const vec3_t *b) { *d = add_vec3(a, b); }
void add_vec4_p  (vec4_t *d, const vec4_t *a, const vec4_t *b) { *d = add_vec4(a, b); }

// ============================================================
//  TRIGONOMETRY
// ============================================================

/*
 * Taylor series sin — accurate for small normalised angles only.
 * The coefficients are 1/3!, 1/5!, 1/7!, 1/9! pre-computed.
 * For the game loop always pass precise=true (use sinf/cosf).
 */
real mat_sin(real angle) {
    real a2 = angle * angle;
    real a3 = angle * a2;
    real a5 = a3    * a2;
    real a7 = a5    * a2;
    real a9 = a7    * a2;
    return angle
         - a3 * 0.16666666666666f
         + a5 * 0.00833333333333f
         - a7 * 0.00019841269841f
         + a9 * 0.00000275573192f;
}

real mat_cos(real angle) {
    return mat_sin(angle + (real)M_PIdiv2);
}

// ============================================================
//  MAT4 IDENTITY
// ============================================================
mat4_t mat4_identity(void) {
    mat4_t I = {{{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }}};
    return I;
}

// ============================================================
//  ROTATION MATRICES
//  mlt_mat4 uses SIMD internally, so the multiply is hot too.
// ============================================================
HOT mat4_t rot_z(const mat4_t *mat, real angle, bool precise) {
    real c = precise ? cosf(angle) : mat_cos(angle);
    real s = precise ? sinf(angle) : mat_sin(angle);
    mat4_t R = mat4_identity();
    R.m[0][0] =  c;  R.m[0][1] = -s;
    R.m[1][0] =  s;  R.m[1][1] =  c;
    return mlt_mat4(mat, &R);
}

HOT mat4_t  rot_y(const mat4_t *mat, real angle, bool precise) {
    real c = precise ? cosf(angle) : mat_cos(angle);
    real s = precise ? sinf(angle) : mat_sin(angle);
    mat4_t R = mat4_identity();
    R.m[0][0] =  c;  R.m[0][2] =  s;
    R.m[2][0] = -s;  R.m[2][2] =  c;
    return mlt_mat4(mat, &R);
}

HOT mat4_t  rot_x(const mat4_t *mat, real angle, bool precise) {
    real c = precise ? cosf(angle) : mat_cos(angle);
    real s = precise ? sinf(angle) : mat_sin(angle);
    mat4_t R = mat4_identity();
    R.m[1][1] =  c;  R.m[1][2] = -s;
    R.m[2][1] =  s;  R.m[2][2] =  c;
    return mlt_mat4(mat, &R);
}

// ============================================================
//  APPLY TRANSFORM  (mat4 × vec4)
//  4 × dp_ps — each row dotted with the vector in one SSE op.
// ============================================================
HOT vec4_t apply_transform(const mat4_t * restrict transform,
                            const vec4_t * restrict vec) {
    __m128 v = _mm_loadu_ps(vec->vals);
    vec4_t res;
    res.x = _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(transform->m[0]), v, 0xF1));
    res.y = _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(transform->m[1]), v, 0xF1));
    res.z = _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(transform->m[2]), v, 0xF1));
    res.w = _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(transform->m[3]), v, 0xF1));
    return res;
}

// ============================================================
//  DISTANCE  (3D Euclidean, ignores w)
//  Sub 4 lanes, zero w, dp_ps, rsqrt+Newton-Raphson.
//  rsqrt is ~12-bit accurate; one NR step brings it to ~23-bit
//  (float precision), avoiding the full sqrtf call.
// ============================================================
real get_distance(vec4_t * restrict vecA, vec4_t * restrict vecB) {
    __m128 diff = _mm_sub_ps(_mm_loadu_ps(vecA->vals),
                              _mm_loadu_ps(vecB->vals));
    // zero w lane with an AND mask
    diff = _mm_and_ps(diff, _mm_castsi128_ps(
               _mm_set_epi32(0, -1, -1, -1)));
    __m128 dist2 = _mm_dp_ps(diff, diff, 0x71); // dot x,y,z into lane 0
    // rsqrt + one Newton-Raphson: x1 = x0 * (1.5 - 0.5*d*x0^2)
    __m128 rsqrt = _mm_rsqrt_ss(dist2);
    __m128 half  = _mm_mul_ss(_mm_set_ss(0.5f), dist2);
    __m128 nr    = _mm_mul_ss(rsqrt,
                       _mm_sub_ss(_mm_set_ss(1.5f),
                           _mm_mul_ss(half, _mm_mul_ss(rsqrt, rsqrt))));
    // distance = 1 / rsqrt(d²) = dist
    return _mm_cvtss_f32(_mm_rcp_ss(nr));
}

// ============================================================
//  VEC4 → VEC3  (trivial, no SIMD benefit)
// ============================================================
vec3_t vec4tovec3(vec4_t *vec) {
    return (vec3_t){{{ vec->x, vec->y, vec->z }}};
}

// ============================================================
//  NORMALIZE VEC3
//  rsqrt_ss + one Newton-Raphson refinement.
//  Avoids the full sqrtf + division path entirely.
// ============================================================
vec3_t normalize_vec3(vec3_t * restrict v) {
    __m128 a  = _mm_set_ps(0.0f, v->z, v->y, v->x);
    __m128 d2 = _mm_dp_ps(a, a, 0x77); // dot x,y,z → lanes 0,1,2

    float mag2 = _mm_cvtss_f32(d2);
    if (__builtin_expect(mag2 < 1e-6f, 0))
        return (vec3_t){{{0.0f, 0.0f, 0.0f}}};

    // Fast reciprocal sqrt (SSE) + Newton-Raphson for float precision
    __m128 rs  = _mm_rsqrt_ss(_mm_set_ss(mag2));
    __m128 h   = _mm_set_ss(0.5f * mag2);
    __m128 nr  = _mm_mul_ss(rs, _mm_sub_ss(_mm_set_ss(1.5f),
                     _mm_mul_ss(h, _mm_mul_ss(rs, rs))));
    __m128 inv = _mm_shuffle_ps(nr, nr, 0x00); // broadcast to all lanes

    __m128 res = _mm_mul_ps(a, inv);
    vec3_t out;
    out.x = _mm_cvtss_f32(res);
    out.y = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(1,1,1,1)));
    out.z = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(2,2,2,2)));
    return out;
}
vec4_t normalize_vec4(vec4_t * restrict v) {
    __m128 a  = _mm_set_ps(0.0f, v->z, v->y, v->x); // w=0, ignore it
    __m128 d2 = _mm_dp_ps(a, a, 0x77); // dot x,y,z only → lanes 0,1,2
    float mag2 = _mm_cvtss_f32(d2);
    if (__builtin_expect(mag2 < 1e-6f, 0))
        return (vec4_t){{{0.0f, 0.0f, 0.0f, 0.0f}}};
    __m128 rs  = _mm_rsqrt_ss(_mm_set_ss(mag2));
    __m128 h   = _mm_set_ss(0.5f * mag2);
    __m128 nr  = _mm_mul_ss(rs, _mm_sub_ss(_mm_set_ss(1.5f),
                     _mm_mul_ss(h, _mm_mul_ss(rs, rs))));
    __m128 inv = _mm_shuffle_ps(nr, nr, 0x00);
    __m128 res = _mm_mul_ps(a, inv);
    vec4_t out;
    out.x = _mm_cvtss_f32(res);
    out.y = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(1,1,1,1)));
    out.z = _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(2,2,2,2)));
    out.w = 0.0f; // w stays 0 for direction vectors
    return out;
}
