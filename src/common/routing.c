#include "server.h"

#include <limits.h>
#include <SDL.h>

typedef struct {
    point2_t parent;
    int f, g, h, s;
} pathNode_t;

typedef struct routeNode_s {
    struct routeNode_s *next;
    int x;
    int y;
    int step;
    bool closed;
} routeNode_t;

struct {
    DWORD width;
    DWORD height;
    pathMapCell_t *data;
    pathNode_t *nodes;
    uint8_t *closed_set;
    routeNode_t *heatmap;
} pathmap = { 0 };

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

inline static pathMapCell_t const *path_node(DWORD x, DWORD y) {
    int const index = x + y * pathmap.width;
    return &pathmap.data[index];
}

inline static routeNode_t *heatmap(DWORD x, DWORD y) {
    int const index = x + y * pathmap.width;
    return &pathmap.heatmap[index];
}

inline static pathNode_t *node(DWORD x, DWORD y) {
    int const index = x + y * pathmap.width;
    return &pathmap.nodes[index];
}

inline static uint8_t *closed_set(DWORD x, DWORD y) {
    int const index = x + y * pathmap.width;
    return &pathmap.closed_set[index];
}

inline static bool is_valid_point(DWORD x, DWORD y) {
    return x < pathmap.width && y < pathmap.height;
}

inline static bool is_obstacle(DWORD x, DWORD y) {
    pathMapCell_t const *node = path_node(x, y);
    return !node || node->nowalk;
}

inline static int calculate_h(int x, int y, int target_x, int target_y) {
    return abs(target_x - x) + abs(target_y - y);
}

static point2_t* reconstruct_path(point2_t start, point2_t target) {
    printf("path: %d\n", node(target.x, target.y)->g + 1);
    int path_length = node(target.x, target.y)->g + 1;
    point2_t* path = MemAlloc(path_length * sizeof(point2_t));
    point2_t current = target;
    for (int i = path_length - 1; i >= 0; i--) {
#ifdef DEBUG_PATHFINDING
        pathDebug[current.x + current.y * pathmap.width].g = 255;
#endif
        path[i] = current;
        current = node(current.x, current.y)->parent;
    }
    return path;
}

static point2_t* find_path2(point2_t start, point2_t target) {
//    memset(pathmap.closed_set, 0, pathmap.width * pathmap.height);
//    memset(pathmap.nodes, 0, pathmap.width * pathmap.height * sizeof(pathNode_t));
    
#ifdef DEBUG_PATHFINDING
    CM_FillDebugObstacles();
#endif
    
    int start_time = SDL_GetTicks();
    
    FOR_LOOP(i, pathmap.width * pathmap.height) {
        pathmap.heatmap[i].next = NULL;
        pathmap.heatmap[i].closed = false;
        pathmap.heatmap[i].step = 0;
    }
    
    routeNode_t *open = heatmap(target.x, target.y);
    routeNode_t **next = &open->next;
    open->closed = true;
    
    for (int iter = 0; open && iter < 20000; iter++, open = open->next) {
        for (int i = 0; i < 8; i++) {
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
        }
    }
    
    printf("Computed in %d ticks\n", SDL_GetTicks() - start_time);

#ifdef DEBUG_PATHFINDING
    FOR_LOOP(i, pathmap.width * pathmap.height) {
        pathDebug[i].g = pathmap.heatmap[i].step > 0 ? pathmap.heatmap[i].step : 255;
    }
#endif

    return NULL;
}

static point2_t* find_path(point2_t start, point2_t target) {
    memset(pathmap.closed_set, 0, pathmap.width * pathmap.height);
    memset(pathmap.nodes, 0, pathmap.width * pathmap.height * sizeof(pathNode_t));
    
#ifdef DEBUG_PATHFINDING
    CM_FillDebugObstacles();
#endif
    int start_time = SDL_GetTicks();

    for (int i = 0; i < pathmap.width; i++) {
        for (int j = 0; j < pathmap.height; j++) {
            node(i, j)->f = INT_MAX;
        }
    }
    
    node(start.x, start.y)->f = 0;
    node(start.x, start.y)->g = 0;
    node(start.x, start.y)->h = 0;
    node(start.x, start.y)->parent = start;
    
    point2_t min = { start.x, start.y };
    point2_t max = { start.x, start.y };
    
    DWORD iteration = 0;
    
    for (;; iteration++) {
        DWORD min_f = INT_MAX;
        point2_t current = { 0, 0 };
        
        for (int i = min.x; i <= max.x; i++) {
            for (int j = min.y; j <= max.y; j++) {
                if (!*closed_set(i, j) && node(i, j)->f < min_f) {
                    min_f = node(i, j)->f;
                    current.x = i;
                    current.y = j;
                }
            }
        }

        // Goal reached
        if (current.x == target.x && current.y == target.y)
            break;
        
        *closed_set(current.x, current.y) = true;
        
        // Explore neighbors
        for (int i = 0; i < 8; i++) {
            int new_x = current.x + dx[i];
            int new_y = current.y + dy[i];
            if (!is_valid_point(new_x, new_y) ||
                is_obstacle(new_x, new_y) ||
                *closed_set(new_x, new_y))
                continue;
            
            int g = node(current.x, current.y)->g + 1;
            int s = node(current.x, current.y)->s + gv[i];
            int h = calculate_h(new_x, new_y, target.x, target.y);
            int f = s + h;
            
            min.x = MIN(min.x, new_x);
            min.y = MIN(min.y, new_y);
            max.x = MAX(max.x, new_x);
            max.y = MAX(max.y, new_y);
            
            if (node(new_x, new_y)->f > f) {
                node(new_x, new_y)->f = f;
                node(new_x, new_y)->g = g;
                node(new_x, new_y)->h = h;
                node(new_x, new_y)->s = s;
                node(new_x, new_y)->parent = current;
            }
        }
    }
    printf("Computed in %d ticks\n", SDL_GetTicks() - start_time);

    printf("Found path in %d iterations\n", iteration);
    return reconstruct_path(start, target);
}

pathPoint_t* CM_FindPath(LPCVECTOR2 start, LPCVECTOR2 target) {
    VECTOR2 n_start = CM_GetNormalizedMapPosition(start->x, start->y);
    VECTOR2 n_target = CM_GetNormalizedMapPosition(target->x, target->y);
    point2_t p_start = { n_start.x * pathmap.width, n_start.y * pathmap.height };
    point2_t p_target = { n_target.x * pathmap.width, n_target.y * pathmap.height };
    point2_t *p_path = find_path(p_start, p_target);
    pathPoint_t *path = NULL;
    pathPoint_t **next = &path;
    if (p_path) {
        for (point2_t const *it = p_path;; it++) {
            *next = MemAlloc(sizeof(struct pathPoint_s));
            float x = (it->x + 0.5f) / (float)pathmap.width;
            float y = (it->y + 0.5f) / (float)pathmap.height;
            (*next)->point = CM_GetDenormalizedMapPosition(x, y);
            next = &((*next)->next);
            if (it->x == p_target.x && it->y == p_target.y)
                break;
        }
        return path;
    } else {
        return NULL;
    }
}

void CM_ReadPathMap(HANDLE archive) {
    HANDLE file;
    DWORD header, version;
    SFileOpenFileEx(archive, "war3map.wpm", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &header, 4, NULL, NULL);
    SFileReadFile(file, &version, 4, NULL, NULL);
    SFileReadFile(file, &pathmap.width, 4, NULL, NULL);
    SFileReadFile(file, &pathmap.height, 4, NULL, NULL);
    pathmap.data = MemAlloc(pathmap.width * pathmap.height);
    pathmap.closed_set = MemAlloc(pathmap.width * pathmap.height);
    pathmap.nodes = MemAlloc(pathmap.width * pathmap.height * sizeof(pathNode_t));
    pathmap.heatmap = MemAlloc(pathmap.width * pathmap.height * sizeof(routeNode_t));
    SFileReadFile(file, pathmap.data, pathmap.width * pathmap.height, 0, 0);
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

