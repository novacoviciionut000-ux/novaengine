#ifndef DEF_H
#define DEF_H
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define FOCAL 100 // this is the focal length, it is used for perspective projection
#define USE_DOUBLE_PRECISION 
    #ifdef USE_DOUBLE_PRECISION
        typedef double real;
    #else
        typedef float real;
    #endif
#define vel 1
#endif