//NOTE : This header currently makes the connection between the camera logic and rendering.c which is where my threads "live".


#include "pipeline.h"



// Single-threaded vertex transform — used as fallback(when the cost of managing multiple threads beats the benefits - the limit needs to be tweaked and checked.)
// NOTE: Currently, we are multithreading over meshes, not vertices. I think that is better for my use case because the gather phase is shorter and my scene is
// going to contain a lot of meshes anyways.
static void transform_vertices_single(camera_manager_t *cam_manager, mat4_t *rotation, vec4_t *neg_cam) {
    for(size_t i = 0; i < cam_manager->num_meshes; i++) {
        mesh_t *mesh = cam_manager->meshes[i];
        for(int j = 0; j < mesh->vertex_count; j++) {
            vec4_t translated = add_vec4(&mesh->world_verts[j], neg_cam);
            mesh->camera_verts[j] = apply_transform(rotation, &translated);
        }
    }
}

// Called by each worker thread for its mesh slice
void transform_vertex_chunk(worker_t *w) {
    vec4_t neg_cam = {{{-w->cam_pos.x, -w->cam_pos.y, -w->cam_pos.z, 0}}};
    for(size_t m = w->start_mesh; m < w->end_mesh; m++) {
        mesh_t *mesh = w->cam_manager->meshes[m];
        for(int j = 0; j < mesh->vertex_count; j++) {
            vec4_t translated = add_vec4(&mesh->world_verts[j], &neg_cam);
            mesh->camera_verts[j] = apply_transform(&w->rotation, &translated);
        }
    }
}

// Main entry point — called from game loop each frame
// Distributes vertex transform work across workers, then blocks until done
void transform_vertices(camera_manager_t *cam_manager, render_manager_t *render_manager) {
    camera_t *cam = cam_manager->camera;

    // Build rotation matrix once — shared read-only across all workers
    mat4_t rotation = mat4_identity();
    rotation = rot_x(&rotation, cam->angles.x, true);
    rotation = rot_y(&rotation, -cam->angles.y, true);

    vec4_t neg_cam = {{{-cam->pos.x, -cam->pos.y, -cam->pos.z, 0}}};

    // Fall back to single threaded if no workers or no meshes
   if(render_manager->num_workers == 0 || cam_manager->num_meshes == 0) {
       transform_vertices_single(cam_manager, &rotation, &neg_cam);
       return;
    }

    // Split meshes evenly across workers
    size_t meshes_per_worker = cam_manager->num_meshes / render_manager->num_workers;

    for(int i = 0; i < render_manager->num_workers; i++) {
        worker_t *w = &render_manager->workers[i];
        w->cam_manager = cam_manager;
        w->rotation    = rotation;
        w->cam_pos     = cam->pos;
        w->start_mesh  = i * meshes_per_worker;
        w->end_mesh    = (i == render_manager->num_workers - 1)
                         ? cam_manager->num_meshes
                         : (size_t)(i + 1) * meshes_per_worker;
        SDL_SignalSemaphore(w->vert_wake);
    }

    // Wait for all workers to finish
    for(int i = 0; i < render_manager->num_workers; i++)
        SDL_WaitSemaphore(render_manager->workers[i].vert_done);
}
