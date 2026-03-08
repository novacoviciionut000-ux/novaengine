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
squares.c/squares.h  - Cube rendering with 3D positioning and perspective
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

**Cube Type:**
```c
typedef struct {
    vec4_t pos;              // World position
    vec4_t local_verts[8];   // Vertices relative to center
    vec4_t world_verts[8];   // Transformed world coordinates
    eulerangles_t angles;    // Rotation angles (x, y, z in radians)
} cube_t;
```



Handles SDL3 window creation, event polling, and render loop coordination.

## Building

```bash
```

Requirements:
- GCC compiler
- SDL3 library
- libm (math library)




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

The focal length (w component) scales the projection; larger values zoom out, smaller values zoom in. ! TO DO: Tweak the focal length, it causes distortion.

## Future Extensions

# Main Systems
    -Rendering:
        The rendering works like a "dumb painter". In the first frame, it gets a fed a raw buffer of all the triangles in the scene(my triangle_t struct contains pointers to camera vertices to avoid the "gathering phase" each frame.
        Then, the triangle data gets split up and fed to my workers to handle the culling, applying lightning, fog, sutherland-hogdman(diving triangles into smaller triangles when they are near the edge of the screen), the "expensive math".
        The "expensive math" is also optimized with SIMD (Single Instruction, Multiple Data), as well as my linear algebra algebra(it is written using SIMD to process at least 4 floats at once.)
        After applying the "expensive math" to triangles, the threads all join and go back to sleep for my radix-sort(painter's algorithm), paralellizing radix-sort, while doable, wouldn't have been beneficial(way too much overhead).
        Painter's algorithm has limitations, especially when you have BIG, OVERLAPPING triangles, the depth may look "off", but it's the best i can do without a Z-Buffer, which would require either switching to opengl
        Or writing my own rasterizer, which would be a project of it's own. So, for now, i will stick to the min-Z implementation of the painter's algorithm.
        After the painter's algorithm is done(they are sorted), i wake up my workers again so they can handle perspective projection(converting the triangles into SDL_Vertex with the perspective projection formula.
        In the end, all of the vertices that were converted into 2D space from 3D are fed to SDL_RenderGeomtry() which handles the ACTUAL paiting to the screen.
        When anything dies(wheter it be particle, enemy or entity), i update the pool_offset field in each of the objects in the scene, and i memmove the triangles buffer by the removed object's triangle count.
        NOTE:A realloc may be triggered if something absolutely crazy happens on-screen, but it is virtually once every blue moon.

    -Particles:
        The particles system works on a particle-pool model. Basically, each particle has a buffer where there are pre-allocated particles(it is a matrix-type thing where each particle has a line number associated to it.)
        When a particle is needed, it is fetched from it's specific buffer, and removed from the buffer(replaced with the last particle in the free particles list), and it's mesh is fed to the camera and triangles to the renderer.
        Basically, because the particles are essentially treated as regular game objects, the customizability is infinite, you can have a lot of scatter-patterns and cool things to do with them.
        They are also collidable for some of them, the only particle that matches this criteria now is the bullet, which has a circular "collision box". An interesting problem was the fact that the bullets would
        Explode upon firing either by the player(camera) or by an enemity type(that was because the bullet was colliding with the object's own collisionbox.) . To fix that, i added an offset in the direction that the bullet would be fired.
        That offset may need to be tweaked at some point if irregular collisionboxes appear.
    -Enemies:
        They work simmilar to the particles, although , for this one, the buffer is a simple linear-buffer, not a matrix. My logic was that particles appear and pop every frame, but enemies do that more rarely, 
        So an O(n) Look-Up time, for this specific use case wasn't as bad and doing it like my particles would've over-engineered something that happens rarely.
        Each one of them has an onHit() function pointer that currently only decreases health but it can be modified easily for more complex behaviour.
    -Camera:
        It's kind of a bad design, but the camera baiscally just handles stuff like making the screen red when taking damage, rotating the camera, translating it(based on it's velocities and inputs).
        The function that actually handles converting world-space into camera-space is pipeline.c which is kind of a "link" header so that my "camera" can "talk" to my rendering.c , which is where the
        worker threads "live" without the camera logic needing to know what a render_manager_t is. It's kind of a bad solution, but it keeps the separation of concerns relatively "neat".
        The way the conversion works is basically, in my scenes.c header, i have a gather_meshes() function that gathers all meshes at the start of each frame.
        Then, these meshes are fed to my workers who multiply with the "view matrix". Basically, it rotates and moves the whole world in the direction opposite to the camera's movement.
        NOTE:The worker threads work on meshes, meaning that while my world doesn't like HUGE high-poly singular meshes, it scales very well when the world is composed of many small meshes,
        Which is very suiting for a low-poly game with a lot of particles flying around.
    -Collisions:
        This header basically handles everything that is collision-related
        This header receives an array of collisionboxes(it is stored inside collision_manager) and it compares it against bullets in the scene, and the camera. For now entity on entity / enemy on enemy collisions are not supported
        But they will be introduced soon.
    -Scenes
        The scene_t is basically a "god object", not in the "anti pattern" way, but in the sense that any single block of memory allocated in the game is accesible from this scene pointer.
        This basically makes it easy to prevent memory leaks, dangling pointers and so on. The scene struct is divided into multiple "managers", each with it's own duty, the render manager is passed to the renderer,
        the enemy_manager to the enemy logic and so on.It also has functions like the "patching", which is done when an entity is removed, it basically just memmoves the triangles buffer and subtracts pool offsets.
    -Entities:
        This system has some "old code" that i will polish soon so i am not going to make a detailed description of it as a lot of things are going to change( basically, i am gonna make a transform struct that holds all the data and a pointer back
        to the entity, maybe, and i am going to try to paralelize transforming entities the same way i did my rendering, etc.
    -Physics:
        This is also unfinished. For now, the only thing that works is camera gravity, but it will be fixed in the future.
#Pictures and Visuals
        
## License

MIT License
