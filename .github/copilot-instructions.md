# AI Agent Instructions for engineC

## Project Overview
**engineC** is a graphics/game engine library written in C, implementing low-level mathematics for 3D transformations with SDL3 rendering integration. Core components handle vector/matrix algebra and graphics rendering.

## Architecture

### Core Components
- **alg.c/alg.h**: Mathematics library implementing:
  - **Vector operations** (vec3_t, vec4_t): addition, dot product, cross product, scaling
  - **Matrix operations** (mat3_t, mat4_t): multiplication, addition, identity, 3D rotations
  - **Custom trigonometry**: `mat_sin()`, `mat_cos()` using Taylor series for embedded compatibility
  
- **main.c**: SDL3 graphics initialization and rendering demo
- **libs.h**: Centralized header aggregating dependencies (stdint, stdlib, stdio, math, SDL3)

### Data Patterns
The project uses **union-based dual access patterns** for flexibility:
```c
typedef struct {
  union {
    struct { double x, y, z; };  // Named component access
    double vals[3];              // Array access for iteration
  };
} vec3_t;
```
Apply this pattern when extending vector/matrix types.

## Function Naming Conventions

**Regular operations** (return values):
- `mlt_matX()`, `add_matX()` - matrix operations
- `mlt_vecX()`, `add_vecX()` - vector operations

**In-place operations** (suffix `_p` for pointer/destructive):
- `mlt_mat4_p(dest, a, b)` - writes result to `dest`
- `add_vec3_p(dest, a, b)` - modifies `dest` in-place

This dual API design allows flexibility between functional and imperative styles.

## Critical Implementation Details

1. **Trigonometry approximation**: Custom `mat_sin()` uses Taylor series (5 terms) instead of stdlib. Angles in radians; precision degrades significantly past ±2π. Use built-in `math.h` functions if precision >1e-6 is needed.

2. **Rotation matrices**: Always operate on identity or previous transformation matrix via `rot_z/y/x()`. Right-multiply semantics: `rot_z(&mat, angle)` applies rotation to existing transform.

3. **Matrix initialization**: Initialize matrices with `{0}` then set values, or use `mat4_identity()` for transforms.

## Workflows & Commands

**Building**: No Makefile visible; compilation likely via command line. SDL3 dependency required.
```bash
gcc -o main main.c alg.c -lm -lSDL3
```

**Testing**: 
- `test.c` contains validation logic for trigonometry and rotation matrices
- `test_sdl` directory suggests SDL-specific tests
- Check printed matrix values against expected rotations (cos/sin values)

**Debugging**: Use `print_mat4()` utility in test.c as template for verifying matrix states during transformation chains.

## Extension Points

- **New vector sizes**: Create `vecN_t` using union pattern, add `dotprodN()`, `scale_vecN()` operations
- **3D transformations**: Extend rotation functions (scale, translation matrices needed for full affine transforms)
- **Rendering**: `main.c` SDL3 setup can be extended for more complex graphics pipelines

## Files to Reference
- [alg.h](alg.h) - Public API and type definitions
- [alg.c](alg.c) - Implementation (Section markers: "MATRICE", "VECTORI", "TRIGONOMETRIE")
- [test.c](test.c) - Validation patterns for math correctness
