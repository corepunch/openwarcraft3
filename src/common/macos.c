#include "common.h"
#include <sys/stat.h>

void Sys_MkDir(LPCSTR directory){
    mkdir(directory, 0777);
}
