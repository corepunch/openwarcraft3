#include "common.h"
#include <sys/stat.h>

void Sys_MkDir(LPCSTR szDirectory){
    mkdir(szDirectory, 0777);
}
