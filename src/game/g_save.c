#include "g_local.h"

#include <StormLib.h>

void ReadLevel(LPCSTR filename) {
    HANDLE mapArchive;
    gi.ExtractFile(filename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &mapArchive);
//    CM_ReadPathMap(mapArchive);
//    CM_ReadDoodads(mapArchive);
//    CM_ReadUnitDoodads(mapArchive);
//    CM_ReadHeightmap(mapArchive);
//    CM_ReadInfo(mapArchive);
//    CM_ReadUnits(mapArchive);
    SFileCloseArchive(mapArchive);
}
