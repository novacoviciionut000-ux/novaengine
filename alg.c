#include "alg.h"

// --- OPERATII MATRICE ---

mat4_t mlt_mat4(const mat4_t *matA, const mat4_t *matB){
    mat4_t res = {0};
    for(uint8_t rep = 0; rep < 4; rep++){
        for(uint8_t curr_it = 0; curr_it < 4; curr_it++){
            for(uint8_t curr_ptr = 0; curr_ptr < 4; curr_ptr++){
                res.m[rep][curr_it] += (matA->m)[rep][curr_ptr] * (matB->m)[curr_ptr][curr_it];      
            }
        }
    }
    return res;
}

mat3_t mlt_mat3(const mat3_t *matA, const mat3_t *matB){
    mat3_t res = {0};
    for(uint8_t rep = 0; rep < 3; rep++){
        for(uint8_t curr_it = 0; curr_it < 3; curr_it++){
            for(uint8_t curr_ptr = 0; curr_ptr < 3; curr_ptr++){
                res.m[rep][curr_it] += (matA->m)[rep][curr_ptr] * (matB->m)[curr_ptr][curr_it];
            }
        }
    }
    return res;
}

mat4_t add_mat4(const mat4_t *matA, const mat4_t *matB){
    mat4_t res = {0};
    for(uint8_t i = 0; i < 4; i++){
        for(uint8_t j = 0; j < 4; j++){
            res.m[i][j] = (matA->m)[i][j] + (matB->m)[i][j];
        }
    }
    return res;
}

mat3_t add_mat3(const mat3_t *matA, const mat3_t *matB){
    mat3_t res = {0};
    for(uint8_t i = 0; i < 3; i++){
        for(uint8_t j = 0; j < 3; j++){
            res.m[i][j] = (matA->m)[i][j] + (matB->m)[i][j];
        }
    }
    return res;
}

// --- OPERATII VECTORI ---

real dotprod3(const vec3_t *vecA, const vec3_t *vecB){
    return (vecA->x * vecB->x) + (vecA->y * vecB->y) + (vecA->z * vecB->z);
}

real dotprod4(const vec4_t *vecA, const vec4_t *vecB){
    return (vecA->x * vecB->x) + (vecA->y * vecB->y) + (vecA->z * vecB->z) + (vecA->w * vecB->w);
}

vec3_t crossprod3(const vec3_t *vecA, const vec3_t *vecB){
    vec3_t res = {0};
    res.x = (vecA->y * vecB->z) - (vecA->z * vecB->y);
    res.y = -1 * ((vecA->x * vecB->z) - (vecA->z * vecB->x));
    res.z = (vecA->x * vecB->y) - (vecA->y * vecB->x);
    return res;
}

vec4_t crossprod4(const vec4_t *vecA, const vec4_t *vecB){
    vec4_t res = {0};
    res.x = (vecA->y * vecB->z) - (vecA->z * vecB->y);
    res.y = -1 * ((vecA->x * vecB->z) - (vecA->z * vecB->x));
    res.z = (vecA->x * vecB->y) - (vecA->y * vecB->x);
    res.w = 0;
    return res;
}

vec3_t add_vec3(const vec3_t *vecA, const vec3_t *vecB){
    vec3_t res = {0};
    res.x = vecA->x + vecB->x;
    res.y = vecA->y + vecB->y;
    res.z = vecA->z + vecB->z;
    return res;
}

vec4_t add_vec4(const vec4_t *vecA, const vec4_t *vecB){
    vec4_t res = {0};
    res.x = vecA->x + vecB->x;
    res.y = vecA->y + vecB->y;
    res.z = vecA->z + vecB->z;
    res.w = vecA->w + vecB->w;
    return res;
}

vec3_t scale_vec3(vec3_t vec, real factor){
    vec3_t res = {0};
    res.x = vec.x * factor;
    res.y = vec.y * factor;
    res.z = vec.z * factor;
    return res;
}

vec4_t scale_vec4(vec4_t vec, real factor){
    vec4_t res = {0};
    res.x = vec.x * factor;
    res.y = vec.y * factor;
    res.z = vec.z * factor;
    res.w = vec.w * factor;
    return res;
}

// --- FUNCTII CU POINTERI (IN-PLACE) ---

void mlt_mat4_p(mat4_t *dest, const mat4_t *matA, const mat4_t *matB){
    *dest = mlt_mat4(matA, matB);
}

void mlt_mat3_p(mat3_t *dest, const mat3_t *matA, const mat3_t *matB){
    *dest = mlt_mat3(matA, matB);
}

void add_mat4_p(mat4_t *dest, const mat4_t *matA, const mat4_t *matB){
    *dest = add_mat4(matA, matB);
}

void add_mat3_p(mat3_t *dest, const mat3_t *matA, const mat3_t *matB){
    *dest = add_mat3(matA, matB);
}

void crossprod3_p(vec3_t *dest, const vec3_t *vecA, const vec3_t *vecB){
    *dest = crossprod3(vecA, vecB);
}

void crossprod4_p(vec4_t *dest, const vec4_t *vecA, const vec4_t *vecB){
    *dest = crossprod4(vecA, vecB);
}

void add_vec3_p(vec3_t *dest, const vec3_t *vecA, const vec3_t *vecB){
    *dest = add_vec3(vecA, vecB);
}

void add_vec4_p(vec4_t *dest, const vec4_t *vecA, const vec4_t *vecB){
    *dest = add_vec4(vecA, vecB);
}

// --- TRIGONOMETRIE SI TRANSFORMARI ---

real mat_sin(real angle){
    real initial_angle = angle;
    real anglesquared = initial_angle * initial_angle;
    real first_term = initial_angle * anglesquared;
    real second_term = first_term * anglesquared;
    real third_term = second_term * anglesquared;
    real fourth_term = third_term * anglesquared;
    real threefinv = 0.1666666666666666, fivefinv = 0.0083333333333333, sevenfinv = 0.0001984126984127, ninefinv = 0.0000027557319224;
    return initial_angle - (first_term * threefinv) + (second_term * fivefinv) - (third_term * sevenfinv) + (fourth_term * ninefinv);
}
//mat_sin foloseste seria lui Taylor pentru a calcula sinusul unui unghi dat in radiani. Aceasta metoda este eficienta pentru unghiuri mici, dar poate avea erori semnificative pentru unghiuri mari din cauza convergentei lente a seriei.
real mat_cos(real angle){
    return mat_sin(angle + M_PIdiv2);
}

mat4_t mat4_identity(){
    mat4_t I = {
        .m = {
            {1.0, 0.0, 0.0, 0.0},
            {0.0, 1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0, 0.0},
            {0.0, 0.0, 0.0, 1.0}
        }
        
        
        
    };
    return I;
}
//matricile de rotatie sunt construite pornind de la matricea identitate, apoi se modifică elementele corespunzătoare pentru a reprezenta rotația în jurul axei specificate. Funcțiile de rotație multiplică matricea de intrare cu matricea de rotație rezultată pentru a obține noua matrice transformata.

//we use the precise boolean as a flag to determine whether the caller needs accuracy or speed. If precise is false, we use our custom mat_sin and mat_cos functions,
// which are faster but less accurate, especially for larger angles. If precise is true, we use the standard library's sin and cos functions, which are more accurate but slower.
//Beware, the inaccuracy of mat_sin and mat_cos can lead to significant visual artifacts when rotating objects by large angles, especially 
//if the angles are not normalized to the range of -2*PI to 2*PI, as the error accumulates with larger inputs. For small angles (e.g., less than 30 degrees), 
//the approximation is generally good enough for most applications, 
//but for angles approaching or exceeding 90 degrees, the error can become quite noticeable.
//In general, for applications where performance is critical and the angles are small, using mat_sin and mat_cos with precise set to false can be a good choice. However, for applications that require high visual fidelity or involve large rotations, it's advisable to use the standard library functions by setting precise to true.
//For ANYTHING that runs in the game loop indefinetely, please use the standard sin and cosin functions.Using the
//Taylor series implementation WILL cause cumulative visual artifacts

mat4_t rot_z(const mat4_t *mat, real angle, bool precise){
    mat4_t res = mat4_identity();
    real sine = 0, cosine = 0;
    if(!precise){
        cosine = mat_cos(angle);
        sine = mat_sin(angle);
    } else {
        cosine = cos(angle);
        sine = sin(angle);
    }
    res.m[0][0] = cosine;
    res.m[0][1] = -sine;
    res.m[1][0] = sine;
    res.m[1][1] = cosine;
    return mlt_mat4(mat, &res);
}

mat4_t rot_y(const mat4_t *mat, real angle, bool precise){
    mat4_t res = mat4_identity();
    real sine = 0, cosine = 0;
    if(!precise){
        cosine = mat_cos(angle);
        sine = mat_sin(angle);
    } else {
        cosine = cos(angle);
        sine = sin(angle);
    }
    res.m[0][0] = cosine;
    res.m[0][2] = sine;
    res.m[2][0] = -sine;
    res.m[2][2] = cosine;
    return mlt_mat4(mat, &res);
}

mat4_t rot_x(const mat4_t *mat, real angle, bool precise){
    mat4_t res = mat4_identity();
    real sine = 0, cosine = 0;
    if(!precise){
        cosine = mat_cos(angle);
        sine = mat_sin(angle);
    } else {
        cosine = cos(angle);
        sine = sin(angle);
    }

    res.m[1][1] = cosine;
    res.m[1][2] = -sine;
    res.m[2][1] = sine;
    res.m[2][2] = cosine;
    return mlt_mat4(mat, &res);
}
vec4_t apply_transform(const mat4_t *transform, const vec4_t *vec){
    vec4_t res = {0};
    res.x = (transform->m[0][0] * vec->x) + (transform->m[0][1] * vec->y) + (transform->m[0][2] * vec->z) + (transform->m[0][3] * vec->w);
    res.y = (transform->m[1][0] * vec->x) + (transform->m[1][1] * vec->y) + (transform->m[1][2] * vec->z) + (transform->m[1][3] * vec->w);
    res.z = (transform->m[2][0] * vec->x) + (transform->m[2][1] * vec->y) + (transform->m[2][2] * vec->z) + (transform->m[2][3] * vec->w);
    res.w = (transform->m[3][0] * vec->x) + (transform->m[3][1] * vec->y) + (transform->m[3][2] * vec->z) + (transform->m[3][3] * vec->w);
    return res;
}
real get_distance(vec4_t *vecA, vec4_t *vecB){
    real dist = sqrtf((vecA->x - vecB->x) * (vecA->x - vecB->x) + 
    (vecA->y - vecB->y) * (vecA->y - vecB->y)+
    (vecA->z - vecB->z) * (vecA->z - vecB->z));
    return dist;
}
vec3_t vec4tovec3(vec4_t *vec){
    vec3_t vec3 = {0};
    vec3.x = vec->x;
    vec3.y = vec->y;
    vec3.z = vec->z;
    return vec3;


}
vec3_t normalize_vec3(vec3_t *v){
    float mag_sq = dotprod3(v, v);
    if (mag_sq < 0.000001f) return (vec3_t){0, 0, 0}; // Guard against zero-length
    vec3_t normalized = scale_vec3(*v, 1.0f/sqrtf(mag_sq));
    return normalized;
}
