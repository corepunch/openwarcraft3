#include "server.h"

#define AREA_DEPTH 4
#define AREA_NODES 32

#define STRUCT_FROM_LINK(l,t,m) ((t *)((BYTE *)l - (long long)&(((t *)0)->m)))
#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,EDICT,area)

KNOWN_AS(areanode_s, AREANODE);

struct areanode_s {
    int axis;        // -1 = leaf node
    float dist;
    struct areanode_s *children[2];
//    link_t trigger_edicts;
    LINK solid_edicts;
};

static AREANODE sv_areanodes[AREA_NODES];
static DWORD sv_numareanodes;

void ClearLink (LPLINK l) {
    l->prev = l->next = l;
}

void RemoveLink (LPLINK l) {
    l->next->prev = l->prev;
    l->prev->next = l->next;
}

void InsertLinkBefore (LPLINK l, LPLINK before) {
    l->next = before;
    l->prev = before->prev;
    l->prev->next = l;
    l->next->prev = l;
}

#define GetAxis(vec, axis) (*((LPCFLOAT)(vec)+axis))
#define SetAxis(vec, axis, value) (*((LPFLOAT)(vec)+axis))=value

LPAREANODE SV_CreateAreaNode(DWORD depth, LPCVECTOR2 mins, LPCVECTOR2 maxs) {
    LPAREANODE anode = &sv_areanodes[sv_numareanodes++];
    VECTOR2 size = Vector2_sub(maxs, mins);
    VECTOR2 mins1 = *mins, mins2 = *mins, maxs1 = *maxs, maxs2 = *maxs;

    ClearLink (&anode->solid_edicts);
 
    if (depth == AREA_DEPTH) {
        anode->axis = -1;
        anode->children[0] = anode->children[1] = NULL;
        return anode;
    }
        
    anode->axis = size.x < size.y;
    anode->dist = 0.5 * (GetAxis(maxs, anode->axis) + GetAxis(mins, anode->axis));
    
    SetAxis(&maxs1, anode->axis, anode->dist);
    SetAxis(&mins2, anode->axis, anode->dist);

    anode->children[0] = SV_CreateAreaNode(depth+1, &mins2, &maxs2);
    anode->children[1] = SV_CreateAreaNode(depth+1, &mins1, &maxs1);
    
    return anode;
}

void SV_ClearWorld(void) {
    memset(sv_areanodes, 0, sizeof(sv_areanodes));
    sv_numareanodes = 0;
    BOX2 bounds = CM_GetWorldBounds();
    SV_CreateAreaNode(0, &bounds.min, &bounds.max);
}

void SV_UnlinkEntity(LPEDICT ent) {
    if (!ent->area.prev)
        return;        // not linked in anywhere
    RemoveLink(&ent->area);
    ent->area.prev = ent->area.next = NULL;
}

void SV_LinkEntity(LPEDICT ent) {
    SV_UnlinkEntity(ent);
    
    if (ent == ge->edicts)
        return; // don't add the world

    if (!ent->inuse)
        return;

    VECTOR2 const size = { ent->collision, ent->collision };
    VECTOR2 const eps = { 1, 1 };
    
    ent->areanum = 0;
    ent->bounds.min = Vector2_sub(&ent->s.origin2, &size);
    ent->bounds.max = Vector2_add(&ent->s.origin2, &size);

    // because movement is clipped an epsilon away from an actual edge,
    // we must fully check even when bounding boxes don't quite touch
    ent->bounds.min = Vector2_sub(&ent->s.origin2, &eps);
    ent->bounds.max = Vector2_add(&ent->s.origin2, &eps);

    LPAREANODE node = sv_areanodes;
    while (1) {
        if (node->axis == -1)
            break;
        if (GetAxis(&ent->bounds.min, node->axis) > node->dist)
            node = node->children[0];
        else if (GetAxis(&ent->bounds.max, node->axis) < node->dist)
            node = node->children[1];
        else
            break; // crosses the node
    }
    InsertLinkBefore(&ent->area, &node->solid_edicts);
}

typedef struct {
    BOX2 bounds;
    LPEDICT *list;
    DWORD maxcount;
    DWORD count;
    BOOL (*pred)(LPCEDICT);
} areaworker_t;

void SV_AreaEdicts_r(LPCAREANODE node, areaworker_t *worker) {
    LPCLINK start = &node->solid_edicts;

    for (LPCLINK l = start->next; l != start; l = l->next) {
        LPEDICT check = EDICT_FROM_AREA(l);

        if (   check->bounds.min.x > worker->bounds.max.x
            || check->bounds.min.y > worker->bounds.max.y
            || check->bounds.min.x < worker->bounds.min.x
            || check->bounds.min.y < worker->bounds.min.y)
            continue; // not touching

        if (worker->count == worker->maxcount) {
            fprintf(stdout, "SV_AreaEdicts: MAXCOUNT\n");
            return;
        }

        if (!worker->pred || worker->pred(check)) {
            worker->list[worker->count++] = check;
        }
    }

    if (node->axis == -1)
        return; // terminal node

    // recurse down both sides
    if (GetAxis(&worker->bounds.max, node->axis) > node->dist)
        SV_AreaEdicts_r(node->children[0], worker);
    
    if (GetAxis(&worker->bounds.min, node->axis) < node->dist)
        SV_AreaEdicts_r(node->children[1], worker);
}

DWORD SV_AreaEdicts(LPCBOX2 area, LPEDICT *list, DWORD maxcount, BOOL (*pred)(LPCEDICT)) {
    areaworker_t w = {
        .bounds = *area,
        .list = list,
        .count = 0,
        .maxcount = maxcount,
        .pred = pred,
    };
    SV_AreaEdicts_r(sv_areanodes, &w);
    return 0;
}
