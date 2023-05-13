#include <stdio.h>

#include "../common/common.h"

#define SAME_TILE 852063

struct CliffInfo *MakeCliffInfo(struct SheetCell *sheet) {
    // cliffID, cliffModelDir, rampModelDir, texDir, texFile, name, groundTile, upperTile, cliffClass, oldID
    struct CliffInfo *list = NULL;
    for (int y = 2;; y++) {
        struct CliffInfo *info = MemAlloc(sizeof(struct CliffInfo));
        int filled = 0;
        for (struct SheetCell *cell = sheet; cell; cell = cell->lpNext) {
            if (cell->y != y)
                continue;
            filled = 1;
            switch (cell->x) {
                case kCliffInfo_cliffID: info->cliffID = *(int*)cell->text; break;
                case kCliffInfo_cliffModelDir: strcpy(info->cliffModelDir, cell->text); break;
                case kCliffInfo_rampModelDir: strcpy(info->rampModelDir, cell->text); break;
                case kCliffInfo_texDir: strcpy(info->texDir, cell->text); break;
                case kCliffInfo_texFile: strcpy(info->texFile, cell->text); break;
                case kCliffInfo_name: strcpy(info->name, cell->text); break;
                case kCliffInfo_groundTile: info->groundTile = *(int*)cell->text; break;
                case kCliffInfo_upperTile: info->upperTile = *(int*)cell->text; break;
            }
        }
        if (filled) {
            if (info->upperTile == SAME_TILE) {
                info->upperTile = info->groundTile;
            }
            if (info->groundTile == SAME_TILE) {
                info->groundTile = info->upperTile;
            }
            info->lpNext = list;
            list = info;
        } else {
            MemFree(info);
            break;
        }
    }
    return list;
}

struct TerrainInfo *MakeTerrainInfo(struct SheetCell *sheet) {
    // tileID, dir, file, comment, name, buildable, footprints, walkable, flyable, blightPri, convertTo, InBeta
    struct TerrainInfo *list = NULL;
    for (int y = 2;; y++) {
        struct TerrainInfo *info = MemAlloc(sizeof(struct TerrainInfo));
        int filled = 0;
        for (struct SheetCell *cell = sheet; cell; cell = cell->lpNext) {
            if (cell->y != y)
                continue;
            filled = 1;
            switch (cell->x) {
                case kTerrainInfo_tileID: info->dwTileID = *(int*)cell->text; break;
                case kTerrainInfo_dir: strcpy(info->sDirectory, cell->text); break;
                case kTerrainInfo_file: strcpy(info->sFilename, cell->text); break;
            }
        }
        if (filled) {
            info->lpNext = list;
            list = info;
        } else {
            MemFree(info);
            break;
        }
    }
    return list;
}

struct DoodadInfo *MakeDoodadInfo(struct SheetCell *sheet) {
    struct DoodadInfo *list = NULL;
    for (int y = 2;; y++) {
        struct DoodadInfo *info = MemAlloc(sizeof(struct DoodadInfo));
        int filled = 0;
        for (struct SheetCell *cell = sheet; cell; cell = cell->lpNext) {
            if (cell->y != y)
                continue;
            filled = 1;
            switch (cell->x) {
                case kDoodadInfo_doodID: info->doodID = *(int*)cell->text; break;
                case kDoodadInfo_dir: strcpy(info->dir, cell->text); break;
                case kDoodadInfo_file: strcpy(info->file, cell->text); break;
                case kDoodadInfo_pathTex: strcpy(info->pathTex, cell->text); break;
            }
        }
        if (filled) {
            info->lpNext = list;
            list = info;
        } else {
            MemFree(info);
            break;
        }
    }
    return list;
}

struct DestructableInfo *MakeDestructableInfo(struct SheetCell *sheet) {
    struct DestructableInfo *list = NULL;
    for (int y = 2;; y++) {
        struct DestructableInfo *info = MemAlloc(sizeof(struct DestructableInfo));
        int filled = 0;
        for (struct SheetCell *cell = sheet; cell; cell = cell->lpNext) {
            if (cell->y != y)
                continue;
            filled = 1;
            switch (cell->x) {
                case kDestructableInfo_DestructableID: info->DestructableID = *(int*)cell->text; break;
                case kDestructableInfo_category: strcpy(info->category, cell->text); break;
                case kDestructableInfo_tilesets: strcpy(info->tilesets, cell->text); break;
                case kDestructableInfo_tilesetSpecific: strcpy(info->tilesetSpecific, cell->text); break;
                case kDestructableInfo_dir: strcpy(info->dir, cell->text); break;
                case kDestructableInfo_file: strcpy(info->file, cell->text); break;
                case kDestructableInfo_lightweight: info->lightweight = atoi(cell->text); break;
                case kDestructableInfo_fatLOS: info->fatLOS = atoi(cell->text); break;
                case kDestructableInfo_texID: info->texID = atoi(cell->text); break;
                case kDestructableInfo_texFile: strcpy(info->texFile, cell->text); break;
            }
        }
        if (filled) {
            info->lpNext = list;
            list = info;
        } else {
            MemFree(info);
            break;
        }
    }
    return list;
}
