#include "g_local.h"

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
    pathMapCell_t *data;
    routeNode_t *heatmap;
} pathmap = { 0 };

static int const dx[] = {-1, 1, 0, 0, -1, -1, 1, 1};
static int const dy[] = {0, 0, -1, 1, -1, 1, -1, 1};
//static int const gv[] = {10, 10, 10, 10, 14, 14, 14, 14};

#pragma pack (push, 1)
typedef struct {
    BYTE id_length, colormap_type, image_type;
    WORD colormap_index, colormap_length;
    BYTE colormap_size;
    WORD x_origin, y_origin, width, height;
    BYTE pixel_size, attributes;
} tgaHeader_t;
#pragma pack (pop)

#define TGA_ORIGIN_MASK 0x30

pathTex_t *LoadTGA(const BYTE* mem, size_t size) {
    tgaHeader_t *header = (tgaHeader_t*)mem;
    const BYTE *tga = mem + sizeof(tgaHeader_t) + header->id_length;
    DWORD columns = header->width;
    DWORD rows = header->height;
    DWORD numPixels = columns * rows;
    switch (header->image_type) {
        case 2:  break;
        case 3:  break;
//        case 10: break;
        default: return NULL;
    }
    switch (header->colormap_type) {
        case 0:  break;
        default: return NULL;
    }
    switch (header->pixel_size) {
        case 32: break;
        case 24: break;
        case 8:  break;
        default: return NULL;
    }
    // Allocate memory for decoded image
    pathTex_t *pathTex = gi.MemAlloc(numPixels * sizeof(COLOR32) + sizeof(pathTex_t));
    if (!pathTex)
        return NULL;
    pathTex->width = columns;
    pathTex->height = rows;
    // Uncompressed RGB image
    if (header->image_type==2 || header->image_type==3) {
        for (int row=rows-1; row>=0; row--) {
            for (int column=0; column<columns; column++) {
                LPBYTE dest = (LPBYTE)&pathTex->map[row * columns];
                BYTE value;
                switch (header->pixel_size) {
                    case 8:
                        value = *tga++;
                        *dest++ = value;
                        *dest++ = value;
                        *dest++ = value;
                        *dest++ = 0xff;
                        break;
                    case 24:
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        *dest++ = 0xff;
                        break;
                    case 32:
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        break;
                }
            }
        }
        return pathTex;
    }
    gi.MemFree(pathTex);
    return NULL;
}

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

inline static bool is_valid_point(DWORD x, DWORD y) {
    return x < pathmap.width && y < pathmap.height;
}

inline static bool is_obstacle(DWORD x, DWORD y) {
    pathMapCell_t const *node = path_node(x, y);
    return !node || node->nowalk;
}

VECTOR2 compute_directiom(DWORD x, DWORD y) {
    DWORD prices[8] = { INT_MAX };
    DWORD min_price = INT_MAX;

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
        float k = 10.f / (10 + (prices[dir] - min_price));
        VECTOR2 dirvec = { dx[dir], dy[dir] };
        Vector2_normalize(&dirvec);
        direction.x += dirvec.x * k;
        direction.y += dirvec.y * k;
    }
    return direction;
}
//
//VECTOR2 get_flow_direction(DWORD heatmapindex, float fnx, float fny) {
//    VECTOR2 n = CM_GetNormalizedMapPosition(fnx, fny);
//    n.x *= pathmap.width;
//    n.y *= pathmap.height;
//    DWORD dx = floorf(n.x), dy = floorf(n.y);
//    VECTOR2 a = compute_directiom(dx, dy);
//    VECTOR2 b = compute_directiom(dx+1, dy);
//    VECTOR2 c = compute_directiom(dx+1, dy+1);
//    VECTOR2 d = compute_directiom(dx, dy+1);
//    VECTOR2 ab = Vector2_lerp(&a, &b, n.x - dx);
//    VECTOR2 cd = Vector2_lerp(&d, &c, n.x - dx);
//    return Vector2_lerp(&ab, &cd, n.y - dy);
//}
//
//handle_t build_heatmap(point2_t target) {
//#ifdef DEBUG_PATHFINDING
//    CM_FillDebugObstacles();
//#endif
//
//    FOR_LOOP(i, pathmap.width * pathmap.height) {
//        pathmap.heatmap[i].next = NULL;
//        pathmap.heatmap[i].closed = false;
//        pathmap.heatmap[i].step = 0;
//    }
//
//    routeNode_t *open = heatmap(target.x, target.y);
//    routeNode_t **next = &open->next;
//    open->closed = true;
//
//    for (int iter = 0; open && iter < 20000; iter++, open = open->next) {
//        FOR_LOOP(i, 8) {
//            int new_x = open->x + dx[i];
//            int new_y = open->y + dy[i];
//            if (!is_valid_point(new_x, new_y) ||
//                is_obstacle(new_x, new_y) ||
//                heatmap(new_x, new_y)->closed)
//                continue;
//            routeNode_t *neighbor = heatmap(new_x, new_y);
//            *next = neighbor;
//            next = &neighbor->next;
//            neighbor->closed = true;
//            neighbor->step = open->step + 1;
//            neighbor->price = open->price + gv[i];
//        }
//    }
//
//#ifdef DEBUG_PATHFINDING
//    FOR_LOOP(i, pathmap.width * pathmap.height) {
//        VECTOR2 dir = compute_directiom(i % pathmap.width, i / pathmap.width);
//        pathDebug[i].g = (atan2(dir.y, dir.x) + M_PI) * 255 / (2 * M_PI);//pathmap.heatmap[i].step > 0 ? pathmap.heatmap[i].price * 2 : 255;
//    }
//#endif
//    return 0;
//}
//
//handle_t CM_BuildHeatmap(LPCVECTOR2 target) {
//    VECTOR2 n_target = CM_GetNormalizedMapPosition(target->x, target->y);
//    point2_t p_target = { n_target.x * pathmap.width, n_target.y * pathmap.height };
//    return build_heatmap(p_target);
//}
//
//void CM_ReadPathMap(HANDLE archive) {
//    HANDLE file;
//    DWORD header, version;
//    SFileOpenFileEx(archive, "war3map.wpm", SFILE_OPEN_FROM_MPQ, &file);
//    SFileReadFile(file, &header, 4, NULL, NULL);
//    SFileReadFile(file, &version, 4, NULL, NULL);
//    SFileReadFile(file, &pathmap.width, 4, NULL, NULL);
//    SFileReadFile(file, &pathmap.height, 4, NULL, NULL);
//    pathmap.data = MemAlloc(pathmap.width * pathmap.height);
//    pathmap.heatmap = MemAlloc(pathmap.width * pathmap.height * sizeof(routeNode_t));
//    SFileReadFile(file, pathmap.data, pathmap.width * pathmap.height, 0, 0);
//    SFileCloseFile(file);
//
//    FOR_LOOP(i, pathmap.width * pathmap.height) {
//        pathmap.heatmap[i].x = i % pathmap.width;
//        pathmap.heatmap[i].y = i / pathmap.width;
//    }
//
//#ifdef DEBUG_PATHFINDING
//    pathDebug = MemAlloc(pathmap.width * pathmap.height * sizeof(COLOR32));
//    CM_FillDebugObstacles();
//#endif
//}
//
