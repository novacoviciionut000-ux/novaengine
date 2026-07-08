#include "obj_parsing.h"
#define EXPECTED_VERTS 1024

#ifdef _WIN32
#include <stddef.h>
static ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    if (!lineptr || !n || !stream) return -1;
    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (!*lineptr) return -1;
    }
    size_t pos = 0;
    int c;
    while ((c = fgetc(stream)) != EOF) {
        if (pos + 1 >= *n) {
            *n *= 2;
            char *tmp = realloc(*lineptr, *n);
            if (!tmp) return -1;
            *lineptr = tmp;
        }
        (*lineptr)[pos++] = (char)c;
        if (c == '\n') break;
    }
    if (pos == 0) return -1;
    (*lineptr)[pos] = '\0';
    return (ssize_t)pos;
}
#endif

void scale_mesh(mesh_t *mesh, real scale) {
    for (int i = 0; i < mesh->vertex_count; i++) {
        mesh->local_verts[i].x *= scale;
        mesh->local_verts[i].y *= scale;
        mesh->local_verts[i].z *= scale;
    }
}

static int parse_mtl(const char *obj_path, const char *mtl_filename,
                     char mat_names[][64], SDL_FColor *mat_colors, int max_mats) {
    char mtl_path[512];
    FILE *f = NULL;

    char *last_slash = strrchr(obj_path, '/');
    if (!last_slash) last_slash = strrchr(obj_path, '\\');
    if (last_slash) {
        size_t dir_len = last_slash - obj_path + 1;
        strncpy(mtl_path, obj_path, dir_len);
        mtl_path[dir_len] = '\0';
        strncat(mtl_path, mtl_filename, sizeof(mtl_path) - dir_len - 1);
    } else {
        strncpy(mtl_path, mtl_filename, sizeof(mtl_path) - 1);
        mtl_path[sizeof(mtl_path) - 1] = '\0';
    }

    f = fopen(mtl_path, "r");

    if (!f) {
        strncpy(mtl_path, obj_path, sizeof(mtl_path) - 1);
        mtl_path[sizeof(mtl_path) - 1] = '\0';
        char *dot = strrchr(mtl_path, '.');
        if (dot) {
            strncpy(dot, ".mtl", sizeof(mtl_path) - (dot - mtl_path) - 1);
            f = fopen(mtl_path, "r");
        }
    }

    if (!f) return 0;

    char *line = NULL;
    size_t size = 0;
    int count = -1;

    while (getline(&line, &size, f) != -1) {
        if (line[0] == '#') continue;
        if (strncmp(line, "newmtl ", 7) == 0 && count + 1 < max_mats) {
            count++;
            sscanf(line + 7, "%63s", mat_names[count]);
            mat_colors[count] = (SDL_FColor){0.8f, 0.8f, 0.8f, 1.0f};
        } else if (strncmp(line, "Kd ", 3) == 0 && count >= 0) {
            sscanf(line + 3, "%f %f %f",
                   &mat_colors[count].r,
                   &mat_colors[count].g,
                   &mat_colors[count].b);
            mat_colors[count].a = 1.0f;
        }
    }

    free(line);
    fclose(f);
    return count + 1;
}

mesh_t *parse_mesh(FILE *file, const char *pathname, SDL_FColor color, bool force_color) {
    mesh_t *mesh = calloc(1, sizeof(mesh_t));
    if (!mesh) return NULL;

    mesh->local_verts     = malloc(EXPECTED_VERTS * sizeof(vec4_t));
    mesh->triangle_map    = malloc(EXPECTED_VERTS * sizeof(int));
    mesh->triangle_colors = malloc(EXPECTED_VERTS * sizeof(SDL_FColor));

    if (!mesh->local_verts || !mesh->triangle_map || !mesh->triangle_colors) {
        free(mesh->local_verts);
        free(mesh->triangle_map);
        free(mesh->triangle_colors);
        free(mesh);
        return NULL;
    }

    size_t allocated           = EXPECTED_VERTS;
    size_t curr_verts          = 0;
    size_t allocated_triangles = EXPECTED_VERTS;
    size_t triangle_indice     = 0;

    #define MAX_MATS 64
    char       mat_names[MAX_MATS][64];
    SDL_FColor mat_colors[MAX_MATS];
    int        mat_count    = 0;
    SDL_FColor active_color = color;

    char *line = NULL;
    size_t size = 0;

    while (getline(&line, &size, file) != -1) {
        if (strlen(line) < 3) continue;
        if (line[0] == '#') continue;

        if (!force_color && strncmp(line, "mtllib ", 7) == 0) {
            char mtl_name[256];
            sscanf(line + 7, "%255s", mtl_name);
            mat_count = parse_mtl(pathname, mtl_name, mat_names, mat_colors, MAX_MATS);
        }

        else if (!force_color && strncmp(line, "usemtl ", 7) == 0) {
            char wanted[64];
            sscanf(line + 7, "%63s", wanted);
            active_color = color;
            for (int m = 0; m < mat_count; m++) {
                if (strcmp(mat_names[m], wanted) == 0) {
                    active_color = mat_colors[m];
                    break;
                }
            }
        }

        else if (line[0] == 'v' && line[1] == ' ') {
            vec4_t v = {0};
            int cnt = sscanf(line + 2, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
            if (cnt != 4) v.w = 1.0f;
            mesh->local_verts[curr_verts++] = v;

            if (curr_verts >= allocated) {
                allocated += EXPECTED_VERTS;
                vec4_t *tmp = realloc(mesh->local_verts, allocated * sizeof(vec4_t));
                if (!tmp) { free(line); goto MESH_CLEANUP; }
                mesh->local_verts = tmp;
            }
        }

        else if (line[0] == 'f' && line[1] == ' ') {
            int face_indices[16];
            int v_count = 0;

            char *token = strtok(line + 2, " \t\r\n");
            while (token && v_count < 16) {
                face_indices[v_count++] = atoi(token);
                token = strtok(NULL, " \t\r\n");
            }

            for (int i = 1; i < v_count - 1; i++) {
                if (triangle_indice + 3 >= allocated_triangles) {
                    allocated_triangles += EXPECTED_VERTS;

                    int *tm = realloc(mesh->triangle_map, allocated_triangles * sizeof(int));
                    SDL_FColor *tc = realloc(mesh->triangle_colors, allocated_triangles * sizeof(SDL_FColor));

                    if (tm) mesh->triangle_map    = tm;
                    if (tc) mesh->triangle_colors = tc;
                    if (!tm || !tc) { free(line); goto MESH_CLEANUP; }
                }

                mesh->triangle_map[triangle_indice]     = face_indices[0] - 1;
                mesh->triangle_map[triangle_indice + 1] = face_indices[i] - 1;
                mesh->triangle_map[triangle_indice + 2] = face_indices[i + 1] - 1;
                mesh->triangle_colors[triangle_indice / 3] = active_color;
                triangle_indice += 3;
            }
        }
    }

    free(line);

    if (curr_verts == 0 || triangle_indice == 0) goto MESH_CLEANUP;

    mesh->vertex_count   = curr_verts;
    mesh->world_verts    = calloc(curr_verts, sizeof(vec4_t));
    mesh->camera_verts   = calloc(curr_verts, sizeof(vec4_t));
    if (!mesh->world_verts || !mesh->camera_verts) goto MESH_CLEANUP;

    mesh->triangle_count = triangle_indice / 3;
    return mesh;

MESH_CLEANUP:
    free(mesh->local_verts);
    free(mesh->world_verts);
    free(mesh->camera_verts);
    free(mesh->triangle_map);
    free(mesh->triangle_colors);
    free(mesh);
    return NULL;
}

void center_mesh(mesh_t *mesh) {
    vec4_t centroid = {{{0,0,0,0}}};
    for (int i = 0; i < mesh->vertex_count; i++) {
        centroid.x += mesh->local_verts[i].x;
        centroid.y += mesh->local_verts[i].y;
        centroid.z += mesh->local_verts[i].z;
    }
    centroid.x /= mesh->vertex_count;
    centroid.y /= mesh->vertex_count;
    centroid.z /= mesh->vertex_count;

    for (int i = 0; i < mesh->vertex_count; i++) {
        mesh->local_verts[i].x -= centroid.x;
        mesh->local_verts[i].y -= centroid.y;
        mesh->local_verts[i].z -= centroid.z;
    }
}
void center_collisionbox_loadup(entity_t* entity, vec4_t pos){
    collisionbox_t *col = entity->collision_box;
    col -> min_x += pos.x;
    col -> max_x += pos.x;
    col -> min_y += pos.y;
    col -> max_y += pos.y;
    col -> min_z += pos.z;
    col -> max_z += pos.z;
}
void *get_obj(char *pathname, vec4_t position, SDL_FColor color, real scale,
              bool collidable, real xrot, real yrot, real zrot, bool force_color, int object_type) {

    FILE *file = fopen(pathname, "r");
    if (!file) return NULL;

    mesh_t *mesh = parse_mesh(file, pathname, color, force_color);
    fclose(file);
    if (!mesh) return NULL;
    center_mesh(mesh);
    scale_mesh(mesh, scale);

    if (xrot != 0 || yrot != 0 || zrot != 0) {
        mat4_t id = mat4_identity();
        id = rot_x(&id, xrot, true);
        id = rot_y(&id, yrot, true);
        id = rot_z(&id, zrot, true);
        for (int i = 0; i < mesh->vertex_count; i++)
            mesh->local_verts[i] = apply_transform(&id, &mesh->local_verts[i]);
    }

    if (object_type == ENTITY_TYPE) {
        entity_t *obj = calloc(1, sizeof(entity_t));
        if (!obj) { return NULL; }

        obj->mesh             = mesh;
        obj->pos              = position;
        obj->velocity         = (vec4_t){{{0,0,0,0}}};
        obj->angular_velocity = (vec4_t){{{0,0,0,0}}};
        obj->angular_speed    = 0.01f;
        obj->color            = color;
        obj->dirty            = true;
        obj->force_accumulator = (vec4_t){{{0,0,0,0}}};
        obj->acceleration     = (vec4_t){{{0,0,0,0}}};
        obj->on_hit           = on_hit_entity;
        obj->HUD              = false;
        rotate_entity(obj);
        if (collidable) {
            obj->collision_box = calloc(1, sizeof(collisionbox_t));
            if (!obj->collision_box) { free(obj); return NULL; }
            *(obj->collision_box) = get_entity_collisionbox(obj->mesh);
            center_collisionbox_loadup(obj, position);
            obj -> collision_box -> hit = false;
        }

        return (void*)obj;

    } else if (object_type == PARTICLE_TYPE) {
        particle_t *part = calloc(1, sizeof(particle_t));
        if (!part) { return NULL; }

        part->mesh     = mesh;
        part->position = position;
        part->velocity = (vec4_t){{{0,0,0,0}}};
        part->color    = color;
        part->lifetime = 0.0f;
        part->gravity  = 0.0f;

        return (void*)part;

    } else if (object_type == ENEMY_TYPE) {
        enemy_t *enemy = calloc(1, sizeof(enemy_t));
        if (!enemy) { return NULL; }

        enemy->mesh     = mesh;
        enemy->position = position;
        enemy->velocity = (vec4_t){{{0,0,0,0}}};
        enemy->color    = color;
        enemy->health   = 100;
        enemy->collision_box = calloc(1, sizeof(collisionbox_t));
        if (!enemy->collision_box) return NULL;
        *(enemy->collision_box) = get_entity_collisionbox(enemy->mesh);

        enemy->collision_box->hit = false;
        return (void*)enemy;
    }

    free(mesh->local_verts);
    free(mesh->world_verts);
    free(mesh->camera_verts);
    free(mesh->triangle_map);
    free(mesh->triangle_colors);
    free(mesh);
    return NULL;
}
