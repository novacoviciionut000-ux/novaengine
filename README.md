# SDL3 Software 3D Engine

A from-scratch 3D game engine written in C, using SDL3 purely as a windowing/input/2D-rasterization backend. All 3D math, transforms, clipping, culling, depth sorting, and projection are hand-rolled — SDL is only used to blit the final triangles (`SDL_RenderGeometry`) to the screen.

The project is a small FPS-style test bed: a walking/jumping camera, `.obj`-loaded meshes, a gun with bobbing/recoil, drone enemies that shoot back, hitscan-free projectile bullets, and AABB collision.

## Features

- **Custom SIMD math library** (`alg.c`) — SSE-accelerated mat4/mat3 multiply, dot/cross products, normalization (rsqrt + Newton-Raphson), vec3/vec4 ops.
- **Software 3D pipeline** (`pipeline.c`, `rendering.c`, `projection_math.c`):
  - World → camera space vertex transform, parallelized across worker threads by mesh.
  - Per-triangle frustum culling and Sutherland-Hodgman clipping (5 planes) done 4-at-a-time with SSE.
  - Backface culling + directional lighting + distance fog, computed in the same SIMD batch.
  - Radix sort (by depth) for painter's-algorithm-style back-to-front rendering.
  - Perspective-correct projection to screen space via `rcpps` + one Newton-Raphson refinement step.
- **Multithreaded render worker pool** — a 3-phase semaphore-driven pipeline (vertex transform → cull/clip → screen-space fill) that falls back to single-threaded execution for small scenes.
- **OBJ/MTL parser** (`obj_parsing.c`) — loads vertices, triangulates n-gon faces, resolves per-face materials from a companion `.mtl` file, with dynamic growth of vertex/triangle buffers.
- **Scene/entity management** (`scenes.c`, `entities.c`) — a shared triangle render pool indexed by entities/particles/enemies, with swap-remove and pool-offset patching on object removal.
- **Enemies** (`enemy.c`) — pooled drone enemies with simple chase AI, timed ranged attacks, and a hit/health/death lifecycle.
- **Gun & particles** (`gun.c`) — procedural view-model bob and damped-sine recoil, plus a particle system for bullets and hit sparks.
- **Physics & collision** (`physics.c`, `collisions.c`) — basic gravity/impulse integration for the camera and AABB (Minkowski-sum) collision resolution against static/dynamic boxes, plus closest-point sphere-vs-AABB bullet hit detection.
- **Input & camera** (`handle_input.c`, `camera.c`) — mouse-look with gimbal-lock clamping, WASD movement relative to view direction, sprint/FOV kick, jump, and a damage vignette overlay.
- **Timers** (`timers.c`) — high-resolution delta-time and accumulated real-time clocks via `SDL_GetPerformanceCounter`.

## Project Layout

| File | Responsibility |
|---|---|
| `main.c` | Entry point — calls `gameLoop()`. |
| `gameloop.c` | SDL init/teardown and the main frame loop (update → collide → render). |
| `alg.c` | SIMD vector/matrix math, trig helpers, transforms. |
| `pipeline.c` | World-to-camera vertex transform, single- and multi-threaded. |
| `rendering.c` | Triangle culling/clipping/lighting/fog, radix sort, projection, thread pool, `SDL_RenderGeometry` submission. |
| `projection_math.c` | vec4 → screen-space point conversion. |
| `scenes.c` | Scene lifecycle, entity/particle/enemy registration, shared render pool management. |
| `entities.c` | Generic entity update/rotation/mesh transform, cube mesh generation. |
| `enemy.c` | Enemy pool, AI behaviour, spawning, and per-frame updates. |
| `gun.c` | View-model bobbing and recoil animation. |
| `physics.c` | Gravity, impulses, and force/velocity/position integration. |
| `collisions.c` | AABB collision checks and resolution, bullet-vs-box hit detection. |
| `camera.c` | Camera creation, rotation, translation, damage vignette. |
| `handle_input.c` | Keyboard/mouse polling and per-frame input dispatch. |
| `obj_parsing.c` | `.obj`/`.mtl` loader and mesh construction/loading utilities (`get_obj`). |
| `timers.c` | Delta-time and real-time clock utilities. |

## Requirements

- A C compiler with SSE4.1 intrinsics support (`<immintrin.h>`) — e.g. GCC or Clang.
- [SDL3](https://github.com/libsdl-org/SDL) development libraries.
- `.obj`/`.mtl` model assets (e.g. `gun.obj`, `Rtwo_low.obj`) available on the working directory path at runtime.

## Building

There's no build script included yet — compile all `.c` files together and link against SDL3, e.g.:

```sh
gcc *.c -o game -lSDL3 -lm -msse4.1 -O2
```

Adjust flags/paths as needed for your SDL3 installation and platform (Windows builds get a `getline` shim automatically via `obj_parsing.c`).

## Running

```sh
./game
```

Controls:
- `W`/`A`/`S`/`D` — move (relative to look direction)
- Mouse — look around
- `Left Shift` — sprint (with a FOV zoom-out kick)
- `Space` — jump
- `Left Mouse Button` — fire gun
- `Esc` — quit

## Status / Known Rough Edges

This is an actively evolving hobby/learning project — several files carry inline notes about incomplete or fragile areas:
- `physics.c` is explicitly marked incomplete and in need of debugging (entity-level physics integration is written but not yet wired into the update loop).
- The ordering of `update_particles` → `gather_meshes` → `gather_collision_boxes` → `update_enemies` in `gameLoop()` is fragile — reordering can cause deadlocks between rendering, enemy, and collision systems.
- `transform_enemy_verts` in `enemy.c` has a TODO to unify with a generic transform system shared with entities.
- The triangle-clipping overflow guard (`max_out = allocated * 5 / num_workers`) may be insufficient for pathological meshes; a note in `rendering.c` suggests bumping the multiplier if corruption appears.