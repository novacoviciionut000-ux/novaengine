#ifndef ALG_H
#define ALG_H

#include <math.h>
#include <stdbool.h>
#include <immintrin.h>
#include "defines.h"

// ============================================================
//  COMPILER HINTS
// ============================================================
#define FORCE_INLINE static inline __attribute__((always_inline))
#define HOT          __attribute__((hot))
#define PURE         __attribute__((pure))
#define M_PIdiv2     1.57079632f

// ============================================================
//  MATRIX OPS
// ============================================================
HOT mat4_t mlt_mat4(const mat4_t *matA, const mat4_t *matB);
    mat3_t mlt_mat3(const mat3_t *matA, const mat3_t *matB);
HOT mat4_t add_mat4(const mat4_t *matA, const mat4_t *matB);
    mat3_t add_mat3(const mat3_t *matA, const mat3_t *matB);
    mat4_t mat4_identity(void);

// ============================================================
//  ROTATION & TRANSFORM
// ============================================================
HOT mat4_t rot_z(const mat4_t *mat, real angle, bool precise);
HOT mat4_t rot_y(const mat4_t *mat, real angle, bool precise);
HOT mat4_t rot_x(const mat4_t *mat, real angle, bool precise);
HOT vec4_t apply_transform(const mat4_t *transform, const vec4_t *vec);

// ============================================================
//  VECTOR OPS
// ============================================================
PURE real  dotprod3(const vec3_t *vecA, const vec3_t *vecB);
PURE real  dotprod4(const vec4_t *vecA, const vec4_t *vecB);
     vec3_t crossprod3(const vec3_t *vecA, const vec3_t *vecB);
     vec4_t crossprod4(const vec4_t *vecA, const vec4_t *vecB);
     vec3_t add_vec3(const vec3_t *vecA, const vec3_t *vecB);
     vec4_t add_vec4(const vec4_t *vecA, const vec4_t *vecB);
     vec3_t scale_vec3(vec3_t vec, real factor);
     vec4_t scale_vec4(vec4_t vec, real factor);
     vec3_t normalize_vec3(vec3_t *v);
     vec4_t normalize_vec4(vec4_t *v);
     vec3_t vec4tovec3(vec4_t *vec);
PURE real   get_distance(vec4_t *vecA, vec4_t *vecB);

// ============================================================
//  IN-PLACE POINTER VARIANTS
// ============================================================
void mlt_mat4_p (mat4_t *dest, const mat4_t *matA, const mat4_t *matB);
void mlt_mat3_p (mat3_t *dest, const mat3_t *matA, const mat3_t *matB);
void add_mat4_p (mat4_t *dest, const mat4_t *matA, const mat4_t *matB);
void add_mat3_p (mat3_t *dest, const mat3_t *matA, const mat3_t *matB);
void crossprod3_p(vec3_t *dest, const vec3_t *vecA, const vec3_t *vecB);
void crossprod4_p(vec4_t *dest, const vec4_t *vecA, const vec4_t *vecB);
void add_vec3_p  (vec3_t *dest, const vec3_t *vecA, const vec3_t *vecB);
void add_vec4_p  (vec4_t *dest, const vec4_t *vecA, const vec4_t *vecB);

// ============================================================
//  TRIG
// ============================================================
real mat_sin(real angle);
real mat_cos(real angle);

// ============================================================
//  INLINED HOT-PATH FUNCTIONS (zero call overhead)
// ============================================================
FORCE_INLINE real dotprod4_fast(const vec4_t *a, const vec4_t *b) {
    return _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(a->vals),
                                    _mm_loadu_ps(b->vals), 0xF1));
}

FORCE_INLINE vec4_t add_vec4_fast(const vec4_t *a, const vec4_t *b) {
    vec4_t out;
    _mm_storeu_ps(out.vals, _mm_add_ps(_mm_loadu_ps(a->vals),
                                        _mm_loadu_ps(b->vals)));
    return out;
}

FORCE_INLINE vec4_t scale_vec4_fast(const vec4_t *v, real f) {
    vec4_t out;
    _mm_storeu_ps(out.vals, _mm_mul_ps(_mm_loadu_ps(v->vals), _mm_set1_ps(f)));
    return out;
}

FORCE_INLINE vec4_t apply_transform_fast(const mat4_t * restrict t,
                                          const vec4_t * restrict v) {
    __m128 vec = _mm_loadu_ps(v->vals);
    vec4_t out;
    out.x = _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(t->m[0]), vec, 0xF1));
    out.y = _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(t->m[1]), vec, 0xF1));
    out.z = _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(t->m[2]), vec, 0xF1));
    out.w = _mm_cvtss_f32(_mm_dp_ps(_mm_loadu_ps(t->m[3]), vec, 0xF1));
    return out;
}

#endif // ALG_H
