#ifndef ALG_H
#define ALG_H
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#define M_PIdiv2 1.56079632
typedef struct{
  union{
    struct{real x,y,z;};
    real vals[3];
    
  };
  
}vec3_t;
typedef struct{
  union{
    struct{real x,y,z,w;};
    real vals[4];
    
  };
  
}vec4_t;
typedef struct{
  union{
    real m[3][3];
    real v[9];
  };
  
}mat3_t;
typedef struct{
   union{
    real m[4][4];
    real v[16];
  };
  
}mat4_t;


mat4_t mlt_mat4(const mat4_t *matA, const mat4_t *matB);
mat3_t mlt_mat3(const mat3_t *matA, const mat3_t *matB);
mat4_t add_mat4(const mat4_t *matA, const mat4_t *matB);
mat3_t add_mat3(const mat3_t *matA, const mat3_t *matB);

real dotprod3(const vec3_t *vecA, const vec3_t *vecB);
real dotprod4(const vec4_t *vecA, const vec4_t *vecB);

vec3_t crossprod3(const vec3_t *vecA, const vec3_t *vecB);//cross product only exists in 3D
vec4_t crossprod4(const vec4_t *vecA, const vec4_t *vecB);//ignore the w-component
vec3_t add_vec3(const vec3_t *vecA, const vec3_t *vecB);
vec4_t add_vec4(const vec4_t *vecA, const vec4_t *vecB);

void mlt_mat4_p(mat4_t *dest, const mat4_t *matA, const mat4_t *matB);
void mlt_mat3_p(mat3_t *dest, const mat3_t *matA, const mat3_t *matB);
void add_mat4_p(mat4_t *dest, const mat4_t *matA, const mat4_t *matB);
void add_mat3_p(mat3_t *dest, const mat3_t *matA, const mat3_t *matB);
void crossprod3_p(vec3_t *dest, const vec3_t *vecA, const vec3_t *vecB);//cross product only exists in 3D
void crossprod4_p(vec4_t *dest, const vec4_t *vecA, const vec4_t *vecB);//ignore the w-component
void add_vec3_p(vec3_t *dest, const vec3_t *vecA, const vec3_t *vecB);
void add_vec4_p(vec4_t *dest, const vec4_t *vecA, const vec4_t *vecB);
real mat_sin(real angle);
real mat_cos(real angle);

vec3_t scale_vec3(vec3_t vec, real factor);
vec4_t scale_vec4(vec4_t vec, real factor);
mat4_t mat4_identity();
mat4_t rot_z(const mat4_t *mat, real angle, bool precise);
mat4_t rot_y(const mat4_t *mat, real angle, bool precise);
mat4_t rot_x(const mat4_t *mat, real angle, bool precise);
vec4_t apply_transform(const mat4_t *transform, const vec4_t *vec);

#endif
