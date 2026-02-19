#ifndef DEF_H
#define DEF_H
#define SCREEN_WIDTH 1440
#define SCREEN_HEIGHT 1080
#define ANGULAR_VELOCITY 0.01f
#define SENSITIVITY 0.01f
#define FOCAL 400 // this is the focal length, it is used for perspective projection
#define USE_DOUBLE_PRECISION 
    #ifdef USE_DOUBLE_PRECISION
        typedef double real;
    #else
        typedef float real;
    #endif
#define vel 1
#endif