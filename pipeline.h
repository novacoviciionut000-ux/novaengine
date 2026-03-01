#ifndef PIPELINE_H
#define PIPELINE_H

#include "defines.h"
#include "camera.h"
#include "rendering.h"
#include "alg.h"

// Called from game loop each frame before sync_renderer
void transform_vertices(camera_manager_t *cam_manager, render_manager_t *render_manager);

// Called by worker threads — do not call directly
void transform_vertex_chunk(worker_t *w);

#endif