#include "../common/mpq.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void fail(const char *msg)
{
    fprintf(stderr, "test_mpq_compat: %s\n", msg);
    exit(1);
}

static const char *resolve_mpq_path(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-mpq=", 5) == 0) {
            return argv[i] + 5;
        }
    }

    return "data/Warcraft III/War3.mpq";
}

int main(int argc, char **argv)
{
    const char *mpq_path = resolve_mpq_path(argc, argv);
    HANDLE archive;
    HANDLE file;
    SFILE_FIND_DATA find_data;
    HANDLE find;
    DWORD bytes_read;
    DWORD size_low;
    DWORD size_high;
    BYTE header[128];
    LONG dist_hi;
    char out_path[] = "/tmp/openwarcraft3_mpq_extract.bin";
    struct stat st;
    BYTE *map_buffer;
    HANDLE map_file;
    HANDLE map_archive;
    HANDLE map_info;
    DWORD map_size;
    DWORD map_bytes_read;

    if (!SFileOpenArchive(mpq_path, 0, 0, &archive)) {
        fail("SFileOpenArchive failed");
    }

    if (!SFileOpenFileEx(archive, "ReplaceableTextures/Selection/SelectionCircleLarge.blp", SFILE_OPEN_FROM_MPQ, &file)) {
        SFileCloseArchive(archive);
        fail("SFileOpenFileEx failed for ReplaceableTextures/Selection/SelectionCircleLarge.blp");
    }

    size_low = SFileGetFileSize(file, &size_high);
    if (size_high != 0 || size_low < 256) {
        SFileCloseFile(file);
        SFileCloseArchive(archive);
        fail("SFileGetFileSize returned unexpected value");
    }

    if (!SFileReadFile(file, header, sizeof(header), &bytes_read, NULL) || bytes_read == 0) {
        SFileCloseFile(file);
        SFileCloseArchive(archive);
        fail("SFileReadFile failed");
    }

    dist_hi = 0;
    if (SFileSetFilePointer(file, 0, &dist_hi, FILE_BEGIN) != 0) {
        SFileCloseFile(file);
        SFileCloseArchive(archive);
        fail("SFileSetFilePointer failed to seek to start");
    }

    SFileCloseFile(file);

    if (!SFileExtractFile(archive, "ReplaceableTextures/Selection/SelectionCircleLarge.blp", out_path, 0)) {
        SFileCloseArchive(archive);
        fail("SFileExtractFile failed");
    }

    if (stat(out_path, &st) != 0 || st.st_size < 256) {
        unlink(out_path);
        SFileCloseArchive(archive);
        fail("extracted file invalid");
    }

    unlink(out_path);

    find = SFileFindFirstFile(archive, "*", &find_data, NULL);
    if (!find) {
        SFileCloseArchive(archive);
        fail("SFileFindFirstFile failed for root");
    }

    if (!find_data.cFileName[0]) {
        SFileFindClose(find);
        SFileCloseArchive(archive);
        fail("SFILE_FIND_DATA is empty");
    }

    SFileFindClose(find);

    if (!SFileOpenFileEx(archive, "Maps\\Campaign\\Human02.w3m", SFILE_OPEN_FROM_MPQ, &map_file)) {
        SFileCloseArchive(archive);
        fail("SFileOpenFileEx failed for nested map");
    }
    map_size = SFileGetFileSize(map_file, NULL);
    map_buffer = (BYTE *)malloc(map_size);
    if (!map_buffer) {
        SFileCloseFile(map_file);
        SFileCloseArchive(archive);
        fail("malloc failed for nested map");
    }
    if (!SFileReadFile(map_file, map_buffer, map_size, &map_bytes_read, NULL) || map_bytes_read != map_size) {
        free(map_buffer);
        SFileCloseFile(map_file);
        SFileCloseArchive(archive);
        fail("SFileReadFile failed for nested map");
    }
    SFileCloseFile(map_file);

    if (!SFileOpenArchiveFromMemory(map_buffer, map_size, 0, &map_archive)) {
        free(map_buffer);
        SFileCloseArchive(archive);
        fail("SFileOpenArchiveFromMemory failed for nested map");
    }
    if (!SFileOpenFileEx(map_archive, "war3map.w3i", SFILE_OPEN_FROM_MPQ, &map_info)) {
        SFileCloseArchive(map_archive);
        free(map_buffer);
        SFileCloseArchive(archive);
        fail("SFileOpenFileEx failed for nested war3map.w3i");
    }
    if (SFileGetFileSize(map_info, NULL) < 32) {
        SFileCloseFile(map_info);
        SFileCloseArchive(map_archive);
        free(map_buffer);
        SFileCloseArchive(archive);
        fail("nested war3map.w3i is unexpectedly small");
    }
    SFileCloseFile(map_info);
    SFileCloseArchive(map_archive);
    free(map_buffer);
    SFileCloseArchive(archive);

    printf("test_mpq_compat: ok (%s)\n", mpq_path);
    return 0;
}
