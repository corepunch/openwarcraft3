#include "../server/server.h"

#include <float.h>
#include <limits.h>

#define MAX_ITERATIONS 0xffff

typedef struct {
    point2_t parent;
    int f, g, h, s;
} pathNode_t;

typedef struct routeNode_s {
    struct routeNode_s *next;
    int x;
    int y;
    int step;
    int price;
    bool closed;
} routeNode_t;

typedef struct {
    BYTE unused:1;
    BYTE nowalk:1;
    BYTE nofly:1;
    BYTE nobuild:1;
    BYTE unused2:1;
    BYTE blight:1;
    BYTE nowater:1;
    BYTE unknown:1;
} pathMapCell_t;

struct {
    DWORD width;
    DWORD height;
    pathMapCell_t *original;
    pathMapCell_t *data;
    routeNode_t *heatmap;
} pathmap = { 0 };

static edict_t *active_heatmap_goal = NULL;
static DWORD active_heatmap_generation = 0;

static point2_t LocationToPathMap(LPCVECTOR2 location);

static int const dx[] = {-1, 1, 0, 0, -1, -1, 1, 1};
static int const dy[] = {0, 0, -1, 1, -1, 1, -1, 1};
static int const gv[] = {10, 10, 10, 10, 14, 14, 14, 14};

#ifdef DEBUG_PATHFINDING
LPCOLOR32 pathDebug = NULL;
static void CM_FillDebugObstacles(void) {
    memset(pathDebug, 0x0, sizeof(pathmap.width * pathmap.height * sizeof(COLOR32)));
    FOR_LOOP(i, pathmap.width * pathmap.height) {
        pathDebug[i].r = pathmap.data[i].nowalk ? 255 : 0;
        pathDebug[i].g = 0;
        pathDebug[i].b = 0;
        pathDebug[i].a = 255;
    }
}
#endif

inline static pathMapCell_t *path_node(DWORD x, DWORD y) {
    int const index = x + y * pathmap.width;
    return &pathmap.data[index];
}

inline static routeNode_t *heatmap(DWORD x, DWORD y) {
    int const index = x + y * pathmap.width;
    return &pathmap.heatmap[index];
}

inline static bool is_valid_point(DWORD x, DWORD y) {
    return x < pathmap.width && y < pathmap.height;
}

inline static bool is_obstacle(DWORD x, DWORD y) {
    pathMapCell_t const *node = path_node(x, y);
    return !node || node->nowalk;
}

static void reset_pathmap_data(void) {
    if (pathmap.data && pathmap.original) {
        memcpy(pathmap.data, pathmap.original, pathmap.width * pathmap.height);
    }
}

static void clear_heatmap(void) {
    FOR_LOOP(i, pathmap.width * pathmap.height) {
        pathmap.heatmap[i].next = NULL;
        pathmap.heatmap[i].closed = false;
        pathmap.heatmap[i].step = 0;
        pathmap.heatmap[i].price = 0;
    }
}

static void apply_entity_obstacles(edict_t const *ignore) {
    FOR_LOOP(i, ge->num_edicts){
        edict_t *ent = EDICT_NUM(i);
        if (!ent->inuse || ent == ignore) {
            continue;
        }

        point2_t p = LocationToPathMap(&ent->s.origin2);
        if (ent->pathtex) {
            pathTex_t *pt = ent->pathtex;
            FOR_LOOP(x, pt->width) {
                FOR_LOOP(y, pt->height) {
                    int px = (int)x + p.x - (int)pt->width / 2;
                    int py = (int)y + p.y - (int)pt->height / 2;
                    if (is_valid_point(px, py)) {
                        path_node(px, py)->nowalk |= pt->map[x + y * pt->width].b;
                    }
                }
            }
        } else if (!(ent->svflags & SVF_MONSTER)) {
            DWORD radius = ent->collision / 24;
            FOR_LOOP(x, MAX(1, radius * 2)) {
                FOR_LOOP(y, MAX(1, radius * 2)) {
                    int px = (int)x + p.x - (int)radius;
                    int py = (int)y + p.y - (int)radius;
                    if (is_valid_point(px, py)) {
                        path_node(px, py)->nowalk |= 1;
                    }
                }
            }
        }
    }
}

static bool is_pathable_node(int x, int y) {
    return is_valid_point(x, y) && !is_obstacle(x, y);
}

static FLOAT pathmap_cell_world_size(void) {
    FLOAT cell_x = FLT_MAX;
    FLOAT cell_y = FLT_MAX;

    if (pathmap.width > 0) {
        VECTOR2 a = CM_GetDenormalizedMapPosition(0, 0);
        VECTOR2 b = CM_GetDenormalizedMapPosition(1.f / pathmap.width, 0);
        cell_x = fabsf(b.x - a.x);
    }
    if (pathmap.height > 0) {
        VECTOR2 a = CM_GetDenormalizedMapPosition(0, 0);
        VECTOR2 b = CM_GetDenormalizedMapPosition(0, 1.f / pathmap.height);
        cell_y = fabsf(b.y - a.y);
    }
    return MAX(1.f, MIN(cell_x, cell_y));
}

static bool is_pathable_node_for_radius_cells(int x, int y, int radius_cells) {
    if (!is_pathable_node(x, y)) {
        return false;
    }
    for (int py = y - radius_cells; py <= y + radius_cells; py++) {
        for (int px = x - radius_cells; px <= x + radius_cells; px++) {
            if (!is_pathable_node(px, py)) {
                return false;
            }
        }
    }
    return true;
}

static bool closest_pathable_node(LPCVECTOR2 location, FLOAT radius, point2_t *out) {
    VECTOR2 n = CM_GetNormalizedMapPosition(location->x, location->y);
    FLOAT fx = n.x * pathmap.width;
    FLOAT fy = n.y * pathmap.height;
    int tx = (int)floorf(fx);
    int ty = (int)floorf(fy);
    int max_radius = (int)MAX(pathmap.width, pathmap.height);
    int radius_cells = (int)ceilf(MAX(0.f, radius) / pathmap_cell_world_size());
    FLOAT best_dist = FLT_MAX;
    point2_t best = { 0, 0 };
    bool found = false;

    if (!pathmap.data || !pathmap.original || !pathmap.width || !pathmap.height) {
        return false;
    }
    if (is_pathable_node_for_radius_cells(tx, ty, radius_cells)) {
        *out = (point2_t){ tx, ty };
        return true;
    }

    for (int search_radius = 1; search_radius <= max_radius && !found; search_radius++) {
        for (int y = ty - search_radius; y <= ty + search_radius; y++) {
            for (int x = tx - search_radius; x <= tx + search_radius; x++) {
                if (x != tx - search_radius && x != tx + search_radius &&
                    y != ty - search_radius && y != ty + search_radius) {
                    continue;
                }
                if (!is_pathable_node_for_radius_cells(x, y, radius_cells)) {
                    continue;
                }

                FLOAT cx = x + 0.5f;
                FLOAT cy = y + 0.5f;
                FLOAT dist = (cx - fx) * (cx - fx) + (cy - fy) * (cy - fy);
                if (!found || dist < best_dist) {
                    best_dist = dist;
                    best = (point2_t){ x, y };
                    found = true;
                }
            }
        }
    }

    if (found) {
        *out = best;
    }
    return found;
}

BOOL CM_ClosestPathablePointForRadius(LPCVECTOR2 location, FLOAT radius, LPVECTOR2 out) {
    point2_t point;

    if (!location || !out || !pathmap.data || !pathmap.original) {
        return false;
    }

    reset_pathmap_data();
    apply_entity_obstacles(NULL);
    if (!closest_pathable_node(location, radius, &point)) {
        return false;
    }

    *out = CM_GetDenormalizedMapPosition((point.x + 0.5f) / pathmap.width,
                                         (point.y + 0.5f) / pathmap.height);
    return true;
}

BOOL CM_ClosestPathablePoint(LPCVECTOR2 location, LPVECTOR2 out) {
    return CM_ClosestPathablePointForRadius(location, 0, out);
}

VECTOR2 compute_directiom(DWORD x, DWORD y) {
    int prices[8];
    int min_price = INT_MAX;

    FOR_LOOP(dir, 8) {
        prices[dir] = INT_MAX;
    }

    FOR_LOOP(dir, 8) {
        int new_x = x + dx[dir];
        int new_y = y + dy[dir];
        if (!is_valid_point(new_x, new_y) ||
            is_obstacle(new_x, new_y) ||
            !heatmap(new_x, new_y)->closed)
            continue;
        prices[dir] = heatmap(new_x, new_y)->price;
        min_price = MIN(prices[dir], min_price);
    }
    
    VECTOR2 direction = { 0, 0 };
    FOR_LOOP(dir, 8) {
        if (prices[dir] == INT_MAX) {
            continue;
        }
        float k = 10.f / MAX(1, 10 + (prices[dir] - min_price));
        VECTOR2 dirvec = { dx[dir], dy[dir] };
        Vector2_normalize(&dirvec);
        direction.x += dirvec.x * k;
        direction.y += dirvec.y * k;
    }
    return direction;
}

VECTOR2 get_flow_direction(DWORD heatmapindex, float fnx, float fny) {
    VECTOR2 n = CM_GetNormalizedMapPosition(fnx, fny);
    n.x *= pathmap.width;
    n.y *= pathmap.height;
    DWORD dx = floorf(n.x), dy = floorf(n.y);
    VECTOR2 a = compute_directiom(dx, dy);
    VECTOR2 b = compute_directiom(dx + 1, dy);
    VECTOR2 c = compute_directiom(dx + 1, dy + 1);
    VECTOR2 d = compute_directiom(dx, dy + 1);
    VECTOR2 ab = Vector2_lerp(&a, &b, n.x - dx);
    VECTOR2 cd = Vector2_lerp(&d, &c, n.x - dx);
    return Vector2_lerp(&ab, &cd, n.y - dy);
}

static point2_t LocationToPathMap(LPCVECTOR2 location) {
    VECTOR2 n_target = CM_GetNormalizedMapPosition(location->x, location->y);
    return (point2_t) { n_target.x * pathmap.width, n_target.y * pathmap.height };
}

DWORD build_heatmap(point2_t target) {
#ifdef DEBUG_PATHFINDING
    CM_FillDebugObstacles();
#endif
    
    routeNode_t *open = heatmap(target.x, target.y);
    routeNode_t **next = &open->next;
    open->closed = true;
    
    for (int iter = 0; open && iter < MAX_ITERATIONS; iter++, open = open->next) {
        FOR_LOOP(i, 8) {
            int new_x = open->x + dx[i];
            int new_y = open->y + dy[i];
            if (!is_valid_point(new_x, new_y) ||
                is_obstacle(new_x, new_y) ||
                heatmap(new_x, new_y)->closed)
                continue;
            routeNode_t *neighbor = heatmap(new_x, new_y);
            *next = neighbor;
            next = &neighbor->next;
            neighbor->closed = true;
            neighbor->step = open->step + 1;
            neighbor->price = open->price + gv[i];
        }
    }

#ifdef DEBUG_PATHFINDING
    FOR_LOOP(i, pathmap.width * pathmap.height) {
        VECTOR2 dir = compute_directiom(i % pathmap.width, i / pathmap.width);
        pathDebug[i].r = pathmap.data[i].nowalk ? 255 : 0;
//        pathDebug[i].g = (atan2(dir.y, dir.x) + M_PI) * 255 / (2 * M_PI);//pathmap.heatmap[i].step > 0 ? pathmap.heatmap[i].price * 2 : 255;
    }
#endif
    return 0;
}

DWORD CM_BuildHeatmap(edict_t *goalentity) {
    point2_t target = LocationToPathMap(&goalentity->s.origin2);

    if (goalentity == active_heatmap_goal && active_heatmap_generation != 0) {
        return active_heatmap_generation;
    }

    reset_pathmap_data();
    clear_heatmap();
    apply_entity_obstacles(goalentity);
    if (!is_pathable_node(target.x, target.y)) {
        closest_pathable_node(&goalentity->s.origin2, 0, &target);
    }
    build_heatmap(target);

    active_heatmap_goal = goalentity;
    active_heatmap_generation++;
    if (active_heatmap_generation == 0) {
        active_heatmap_generation = 1;
    }
    return active_heatmap_generation;
}

void CM_ReadPathMap(HANDLE archive) {
    HANDLE file;
    DWORD header, version;
    active_heatmap_goal = NULL;
    active_heatmap_generation = 0;
    SFileOpenFileEx(archive, "war3map.wpm", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &header, 4, NULL, NULL);
    SFileReadFile(file, &version, 4, NULL, NULL);
    SFileReadFile(file, &pathmap.width, 4, NULL, NULL);
    SFileReadFile(file, &pathmap.height, 4, NULL, NULL);
    pathmap.data = MemAlloc(pathmap.width * pathmap.height);
    pathmap.original = MemAlloc(pathmap.width * pathmap.height);
    pathmap.heatmap = MemAlloc(pathmap.width * pathmap.height * sizeof(routeNode_t));
    SFileReadFile(file, pathmap.original, pathmap.width * pathmap.height, 0, 0);
    SFileCloseFile(file);

    FOR_LOOP(i, pathmap.width * pathmap.height) {
        pathmap.heatmap[i].x = i % pathmap.width;
        pathmap.heatmap[i].y = i / pathmap.width;
    }

#ifdef DEBUG_PATHFINDING
    pathDebug = MemAlloc(pathmap.width * pathmap.height * sizeof(COLOR32));
    CM_FillDebugObstacles();
#endif
}
