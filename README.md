# engineC

A lightweight graphics/game engine library written in C, implementing low-level mathematics for 3D transformations with SDL3 rendering integration.

## Features

- **Vector & Matrix Algebra** - 3D and 4D vector operations, 3x3 and 4x4 matrix transformations
- **3D Rotations** - Euler angle rotations around X, Y, Z axes using Taylor series approximation
- **Perspective Projection** - Real-time 3D to 2D screen projection
- **SDL3 Rendering** - Hardware-accelerated graphics output
- **Keyboard Input** - Continuous key state monitoring with arrow keys and WASD support

## Project Structure

```
alg.c/alg.h          - Mathematics library (vectors, matrices, trigonometry)
squares.c/squares.h  - Square rendering with 3D positioning and perspective
gameloop.c/gameloop.h - SDL3 initialization and main game loop
libs.h               - Centralized header dependencies
main.c               - Entry point
```

## Core Components

### Mathematics (alg.h/alg.c)

**Vector Types:**
- `vec3_t` - 3D vector with {x, y, z}
- `vec4_t` - 4D vector with {x, y, z, w}

**Matrix Types:**
- `mat3_t` - 3x3 matrix for 2D transformations
- `mat4_t` - 4x4 matrix for 3D transformations

**Key Functions:**

*Vector Operations:*
- `add_vec3/4()` - Vector addition
- `dotprod3/4()` - Dot product
- `crossprod3/4()` - Cross product
- `scale_vec3/4()` - Scalar multiplication

*Matrix Operations:*
- `mlt_mat3/4()` - Matrix multiplication
- `add_mat3/4()` - Matrix addition
- `mat4_identity()` - Identity matrix

*Rotations:*
- `rot_x(mat, angle)` - Rotate around X axis
- `rot_y(mat, angle)` - Rotate around Y axis
- `rot_z(mat, angle)` - Rotate around Z axis

*Trigonometry:*
- `mat_sin(angle)` - Sine using Taylor series (5 terms)
- `mat_cos(angle)` - Cosine via sin(angle + π/2)

### Rendering (squares.c/squares.h)

**Square Type:**
```c
typedef struct{
    vec4_t pos;        // Position: {x, y, z, focal_length}
    double length;     // Width of square
    double width;      // Height of square
} square_t;
```

**Functions:**
- `render_square()` - Project 3D position to 2D and draw
- `update_square()` - Update position based on keyboard input
- `add_velocity()` - Apply vector velocity to position

### Game Loop (gameloop.c/gameloop.h)

Handles SDL3 window creation, event polling, and render loop coordination.

## Building

```bash
gcc -o main main.c gameloop.c squares.c alg.c -lm -lSDL3
```

Requirements:
- GCC compiler
- SDL3 library
- libm (math library)

## Controls

- **Arrow Keys** - Move square in X/Y plane
- **W/S** - Move square closer (W) or farther (S) on Z axis
- **Close Window** - Exit application

## Design Patterns

### Dual API Design

The math library provides both functional and in-place operation styles:

```c
// Functional style (returns new value)
vec4_t result = add_vec4(&v1, &v2);

// In-place style (modifies destination)
add_vec4_p(&v1, &v1, &v2);  // v1 += v2
```

### Union-Based Dual Access

Vector/matrix types use unions for flexible component access:

```c
vec4_t v = {1, 2, 3, 4};
double x = v.x;              // Named access
double x = v.vals[0];        // Array access
```

## Perspective Projection

3D coordinates are projected to 2D screen space using perspective division:

```
screen_x = (world_x / world_z) * focal_length
screen_y = (world_y / world_z) * focal_length
```

The focal length (w component) scales the projection; larger values zoom out, smaller values zoom in.

## Performance Notes

- Custom trigonometry uses Taylor series (precision ~1e-6 beyond ±2π)
- Matrix multiplications use naive O(n³) algorithm
- No SIMD optimizations currently

## Future Extensions

- [ ] 3D cube rendering with 6 square faces
- [ ] More complex geometric primitives
- [ ] Affine transformations (translation, scaling)
- [ ] Depth sorting for proper face culling
- [ ] Lighting and shading
- [ ] Texture mapping

## License

MIT License
