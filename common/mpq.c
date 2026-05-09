/*
 * mpq.c — Pure C MPQ archive reader (War3.mpq compatible)
 *
 * Implements reading of Warcraft III MPQ archives without external dependencies.
 * Supports:
 *  - MPQ v1/v2 format
 *  - File lookup by name
 *  - ZLIB decompression
 *  - Uncompressed files
 *
 * Based on StormLib specifications and MPQ format documentation.
 */

#include "mpq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <zlib.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MPQ_HASH_NAME_A 1
#define MPQ_HASH_NAME_B 2
#define MPQ_HASH_FILE_KEY 3
#define MPQ_KEY_HASH_TABLE 0xC3AF3770
#define MPQ_KEY_BLOCK_TABLE 0xEC83B3A3
#define MPQ_HASH_ENTRY_DELETED 0xFFFFFFFE
#define MPQ_HASH_ENTRY_FREE 0xFFFFFFFF
#define MPQ_BLOCK_INDEX_MASK 0x0FFFFFFF
#define MPQ_SECTOR_SIZE_DEFAULT 4096
#define MPQ_FILE_COMPRESS 0x00000200
#define MPQ_FILE_IMPLODE 0x00000100
#define MPQ_FILE_ENCRYPTED 0x00010000
#define MPQ_FILE_KEY_V2 0x00020000
#define MPQ_COMPRESSION_ZLIB 0x02

typedef struct {
    DWORD dwID;                 // "MPQ\x1a"
    DWORD dwHeaderSize;         // Size of MPQ header
    DWORD dwArchiveSize;        // Size of entire archive
    USHORT wFormatVersion;      // 0 for v1, 1 for v2, etc.
    USHORT wSectorSizeShift;    // Power of 2 for sector size (usually 12 = 4096)
    DWORD dwHashTablePos;       // Offset to hash table from archive start
    DWORD dwBlockTablePos;      // Offset to block table from archive start
    DWORD dwHashTableSize;      // Number of hash table entries
    DWORD dwBlockTableSize;     // Number of block table entries
} MPQ_HEADER_V1;

typedef struct {
    DWORD dwNameHash1;          // First name hash
    DWORD dwNameHash2;          // Second name hash
    USHORT wLocale;             // Locale
    BYTE bPlatform;             // Platform
    BYTE bFlags;                // Flags
    DWORD dwBlockIndex;         // Block table index (or 0xFFFFFFFF for free/deleted)
} MPQ_HASH_ENTRY;

typedef struct {
    DWORD dwBlockOffset;        // Offset from MPQ header
    DWORD dwBlockSize;          // Compressed size
    DWORD dwFileSize;           // Uncompressed size
    DWORD dwFlags;              // File flags
} MPQ_BLOCK_ENTRY;

struct mpq_cache_entry;

typedef struct {
    FILE *fp;
    char filename[256];
    DWORD base_offset;
    struct mpq_cache_entry **lookup_cache;
    DWORD lookup_cache_size;
    MPQ_HEADER_V1 header;
    MPQ_HASH_ENTRY *hashtable;
    MPQ_BLOCK_ENTRY *blocktable;
    DWORD sector_size;
    BYTE *sector_buffer;
} MPQ_ARCHIVE;

typedef struct {
    MPQ_ARCHIVE *archive;
    DWORD block_index;
    DWORD file_size;
    DWORD current_pos;
    DWORD compressed_size;
    DWORD flags;
    DWORD file_key;
    DWORD sector_count;
    DWORD *sector_offsets;
} MPQ_FILE;

typedef struct {
    MPQ_ARCHIVE *archive;
    DWORD current_index;
    DWORD file_count;
    char **files;
    char mask[MAX_PATH];
} MPQ_FIND;

typedef struct mpq_cache_entry {
    char *name;
    DWORD block_index;
    struct mpq_cache_entry *next;
} MPQ_CACHE_ENTRY;

typedef struct {
    char *name;
    DWORD hash1;
    DWORD hash2;
} MPQ_LISTFILE_ENTRY;

typedef struct mpq_listfile_bucket_entry {
    MPQ_LISTFILE_ENTRY *entry;
    struct mpq_listfile_bucket_entry *next;
} MPQ_LISTFILE_BUCKET_ENTRY;

#define STORM_BUFFER_SIZE 0x500

static DWORD storm_buffer[STORM_BUFFER_SIZE];
static BOOL storm_buffer_ready = FALSE;

static void InitStormBuffer(void)
{
    DWORD seed = 0x00100001;
    DWORD index1, index2, i;

    if (storm_buffer_ready) {
        return;
    }

    for (index1 = 0; index1 < 0x100; index1++) {
        for (index2 = index1, i = 0; i < 5; i++, index2 += 0x100) {
            DWORD temp1, temp2;

            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp1 = (seed & 0xFFFF) << 0x10;

            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp2 = (seed & 0xFFFF);

            storm_buffer[index2] = (temp1 | temp2);
        }
    }

    storm_buffer_ready = TRUE;
}

static BYTE AsciiToUpper(BYTE ch)
{
    if (ch >= 'a' && ch <= 'z') {
        return ch - 0x20;
    }
    if (ch == '/') {
        return '\\';
    }
    return ch;
}

static DWORD HashString(const char *str, DWORD hash_type)
{
    DWORD seed1 = 0x7FED7FED;
    DWORD seed2 = 0xEEEEEEEE;
    BYTE ch;

    if (!storm_buffer_ready) {
        InitStormBuffer();
    }

    while (*str) {
        ch = AsciiToUpper((BYTE)*str++);
        seed1 = storm_buffer[(hash_type * 0x100) + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
    }

    return seed1;
}

#define MPQ_HASH_KEY2_MIX 0x400

static void NormalizeMpqPath(char *s)
{
    while (*s) {
        if (*s == '/') {
            *s = '\\';
        }
        s++;
    }
}

static void TrimEdgeSlashes(char *s)
{
    size_t len = strlen(s);

    while (len > 0 && (s[len - 1] == '\\' || s[len - 1] == '/')) {
        s[--len] = '\0';
    }
    while (*s == '\\' || *s == '/') {
        memmove(s, s + 1, strlen(s));
    }
}

static DWORD CacheKeyHash(const char *str)
{
    DWORD hash = 2166136261u;

    while (*str) {
        BYTE ch = (BYTE)*str++;
        if (ch == '/') {
            ch = '\\';
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = (BYTE)(ch + 0x20);
        }
        hash ^= ch;
        hash *= 16777619u;
    }

    return hash;
}

static void CanonicalizeMpqKey(const char *src, char *dst, size_t dst_size)
{
    size_t i = 0;

    if (!src || !dst || dst_size == 0) {
        return;
    }

    while (*src && i + 1 < dst_size) {
        char ch = *src++;
        if (ch == '/') {
            ch = '\\';
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = (char)(ch + 0x20);
        }
        dst[i++] = ch;
    }
    dst[i] = '\0';
    TrimEdgeSlashes(dst);
}

static BOOL LookupCachedBlock(MPQ_ARCHIVE *mpq, const char *fileName, DWORD *block_index)
{
    char key[1024];
    DWORD hash;
    MPQ_CACHE_ENTRY *entry;

    if (!mpq->lookup_cache || mpq->lookup_cache_size == 0) {
        return FALSE;
    }

    CanonicalizeMpqKey(fileName, key, sizeof(key));
    hash = CacheKeyHash(key);
    entry = mpq->lookup_cache[hash & (mpq->lookup_cache_size - 1)];
    while (entry) {
        if (!strcmp(entry->name, key)) {
            *block_index = entry->block_index;
            return TRUE;
        }
        entry = entry->next;
    }

    return FALSE;
}

static void CacheBlockLookup(MPQ_ARCHIVE *mpq, const char *fileName, DWORD block_index)
{
    char key[1024];
    DWORD hash;
    MPQ_CACHE_ENTRY *entry;

    if (!mpq->lookup_cache || mpq->lookup_cache_size == 0) {
        return;
    }

    CanonicalizeMpqKey(fileName, key, sizeof(key));
    hash = CacheKeyHash(key);
    entry = (MPQ_CACHE_ENTRY *)malloc(sizeof(MPQ_CACHE_ENTRY));
    if (!entry) {
        return;
    }
    entry->name = (char *)malloc(strlen(key) + 1);
    if (!entry->name) {
        free(entry);
        return;
    }
    strcpy(entry->name, key);
    entry->block_index = block_index;
    entry->next = mpq->lookup_cache[hash & (mpq->lookup_cache_size - 1)];
    mpq->lookup_cache[hash & (mpq->lookup_cache_size - 1)] = entry;
}

static BOOL FindBlockIndex(MPQ_ARCHIVE *mpq, const char *fileName, DWORD hash1, DWORD hash2, DWORD *block_index)
{
    DWORD hash_pos;
    DWORD index;
    BOOL trace = getenv("OW3_MPQ_TRACE") != NULL;

    hash_pos = (hash1 & (mpq->header.dwHashTableSize - 1));
    if (trace) {
        fprintf(stderr, "MPQ lookup: %s hash1=%08x hash2=%08x table=%u start=%u\n",
                fileName, hash1, hash2, mpq->header.dwHashTableSize, hash_pos);
    }
    for (index = 0; index < mpq->header.dwHashTableSize; index++) {
        MPQ_HASH_ENTRY *entry = &mpq->hashtable[(hash_pos + index) & (mpq->header.dwHashTableSize - 1)];

        if (trace && index < 8) {
            fprintf(stderr, "MPQ lookup probe[%u]: block=%08x h1=%08x h2=%08x\n",
                    index, entry->dwBlockIndex, entry->dwNameHash1, entry->dwNameHash2);
        }
        if (entry->dwBlockIndex == MPQ_HASH_ENTRY_FREE) {
            if (trace) {
                fprintf(stderr, "MPQ lookup: stopped at free slot after %u probes\n", index);
            }
            break;
        }

        if (entry->dwBlockIndex == MPQ_HASH_ENTRY_DELETED) {
            continue;
        }

        if (entry->dwNameHash1 == hash1 && entry->dwNameHash2 == hash2) {
            *block_index = entry->dwBlockIndex & MPQ_BLOCK_INDEX_MASK;
            if (trace) {
                fprintf(stderr, "MPQ lookup: found via probe at block_index=%u\n", *block_index);
            }
            CacheBlockLookup(mpq, fileName, *block_index);
            return TRUE;
        }
    }

    if (trace) {
        fprintf(stderr, "MPQ lookup: falling back to full scan\n");
    }
    for (index = 0; index < mpq->header.dwHashTableSize; index++) {
        MPQ_HASH_ENTRY *entry = &mpq->hashtable[index];

        if (entry->dwBlockIndex == MPQ_HASH_ENTRY_DELETED ||
            entry->dwBlockIndex == MPQ_HASH_ENTRY_FREE) {
            continue;
        }

        if (entry->dwNameHash1 == hash1 && entry->dwNameHash2 == hash2) {
            *block_index = entry->dwBlockIndex & MPQ_BLOCK_INDEX_MASK;
            if (trace) {
                fprintf(stderr, "MPQ lookup: found via full scan at slot=%u block_index=%u\n", index, *block_index);
            }
            CacheBlockLookup(mpq, fileName, *block_index);
            return TRUE;
        }
    }

    if (trace) {
        fprintf(stderr, "MPQ lookup: not found after full scan\n");
    }

    return FALSE;
}

static const char *BaseNamePtr(const char *path)
{
    const char *slash1;
    const char *slash2;

    if (!path) {
        return path;
    }

    slash1 = strrchr(path, '\\');
    slash2 = strrchr(path, '/');
    if (slash2 && (!slash1 || slash2 > slash1)) {
        return slash2 + 1;
    }
    if (slash1) {
        return slash1 + 1;
    }
    return path;
}

static BOOL DecryptBlock(LPBYTE data, DWORD size, DWORD seed1)
{
    LPDWORD pdw = (LPDWORD)data;
    DWORD nblocks = size >> 2;  // Convert to DWORD count
    DWORD seed2 = 0xEEEEEEEE;
    DWORD index;

    if (!storm_buffer_ready) {
        InitStormBuffer();
    }

    for (index = 0; index < nblocks; index++) {
        DWORD value;
        DWORD seed2_base;

        seed2_base = seed2 + storm_buffer[MPQ_HASH_KEY2_MIX + (seed1 & 0xFF)];
        value = pdw[index] ^ (seed1 + seed2_base);
        pdw[index] = value;
        seed1 = ((~seed1 << 0x15) + 0x11111111) | (seed1 >> 0x0B);
        seed2 = value + seed2_base + (seed2_base << 5) + 3;
    }

    return TRUE;
}

static BOOL SectorTableLooksValid(const DWORD *offsets, DWORD sector_count, DWORD compressed_size)
{
    DWORD i;

    if (!offsets || sector_count == 0) {
        return FALSE;
    }

    if (offsets[0] == 0 || offsets[0] > compressed_size) {
        return FALSE;
    }

    for (i = 0; i < sector_count; i++) {
        if (offsets[i] > offsets[i + 1]) {
            return FALSE;
        }
        if (offsets[i + 1] > compressed_size) {
            return FALSE;
        }
    }

    return TRUE;
}

static void PreloadListfileCache(MPQ_ARCHIVE *mpq);

static BOOL TryInflateSector(const BYTE *compressed, DWORD compressed_size, DWORD uncompressed_size, BYTE *out, DWORD *out_size)
{
    z_stream zs;
    int zlib_ret;
    const BYTE *payload = compressed;
    DWORD payload_size = compressed_size;

    if (!compressed || !out || !out_size || compressed_size == 0) {
        return FALSE;
    }

    if (compressed_size > 1 && compressed[0] == MPQ_COMPRESSION_ZLIB) {
        payload = compressed + 1;
        payload_size = compressed_size - 1;
    }

    memset(&zs, 0, sizeof(zs));
    zs.avail_in = payload_size;
    zs.next_in = (Bytef *)payload;
    zs.avail_out = uncompressed_size;
    zs.next_out = out;

    zlib_ret = inflateInit(&zs);
    if (zlib_ret != Z_OK) {
        memset(&zs, 0, sizeof(zs));
        zs.avail_in = compressed_size;
        zs.next_in = (Bytef *)compressed;
        zs.avail_out = uncompressed_size;
        zs.next_out = out;
        zlib_ret = inflateInit2(&zs, -MAX_WBITS);
        if (zlib_ret != Z_OK) {
            return FALSE;
        }
    }

    zlib_ret = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    if (zlib_ret != Z_STREAM_END) {
        return FALSE;
    }

    *out_size = zs.total_out;
    return TRUE;
}

static BOOL MpqSeek(const MPQ_ARCHIVE *mpq, DWORD offset)
{
    return fseek(mpq->fp, mpq->base_offset + offset, SEEK_SET) == 0;
}

BOOL SFileOpenArchive(LPCSTR filename, DWORD priority, DWORD flags, HANDLE *archive)
{
    MPQ_ARCHIVE *mpq;
    FILE *fp;
    unsigned char probe[1024];
    size_t read;
    long archive_offset = -1;
    size_t i;

    (void)priority;
    (void)flags;

    if (!filename || !archive) {
        return FALSE;
    }

    fp = fopen(filename, "rb");
    if (!fp) {
        return FALSE;
    }

    mpq = (MPQ_ARCHIVE *)malloc(sizeof(MPQ_ARCHIVE));
    if (!mpq) {
        fclose(fp);
        return FALSE;
    }

    memset(mpq, 0, sizeof(MPQ_ARCHIVE));
    mpq->fp = fp;
    strncpy(mpq->filename, filename, sizeof(mpq->filename) - 1);
    mpq->lookup_cache_size = 4096;
    mpq->lookup_cache = (MPQ_CACHE_ENTRY **)calloc(mpq->lookup_cache_size, sizeof(MPQ_CACHE_ENTRY *));

    read = fread(probe, 1, sizeof(probe), fp);
    for (i = 0; i + 4 <= read; i++) {
        if (probe[i] == 'M' && probe[i + 1] == 'P' && probe[i + 2] == 'Q' && probe[i + 3] == 0x1A) {
            archive_offset = (long)i;
            break;
        }
    }
    if (archive_offset < 0) {
        goto fail;
    }
    mpq->base_offset = (DWORD)archive_offset;

    if (fseek(fp, mpq->base_offset, SEEK_SET) != 0) {
        goto fail;
    }

    // Read MPQ header
    if (fread(&mpq->header, sizeof(MPQ_HEADER_V1), 1, fp) != 1) {
        goto fail;
    }

    // Verify MPQ signature
    if (mpq->header.dwID != 0x1A51504D) {  // "MPQ\x1a"
        goto fail;
    }

    // Calculate sector size
    mpq->sector_size = (1 << (mpq->header.wSectorSizeShift + 9));
    if (mpq->sector_size < 512 || mpq->sector_size > 65536) {
        mpq->sector_size = MPQ_SECTOR_SIZE_DEFAULT;
    }

    // Allocate sector buffer
    mpq->sector_buffer = (BYTE *)malloc(mpq->sector_size);
    if (!mpq->sector_buffer) {
        goto fail;
    }

    // Read hash table
    mpq->hashtable = (MPQ_HASH_ENTRY *)malloc(mpq->header.dwHashTableSize * sizeof(MPQ_HASH_ENTRY));
    if (!mpq->hashtable) {
        goto fail;
    }

    if (!MpqSeek(mpq, mpq->header.dwHashTablePos)) {
        goto fail;
    }
    read = fread(mpq->hashtable, sizeof(MPQ_HASH_ENTRY), mpq->header.dwHashTableSize, fp);
    if (read != mpq->header.dwHashTableSize) {
        goto fail;
    }

    // Decrypt hash table
    DecryptBlock((LPBYTE)mpq->hashtable, mpq->header.dwHashTableSize * sizeof(MPQ_HASH_ENTRY),
                 MPQ_KEY_HASH_TABLE);

    // Read block table
    mpq->blocktable = (MPQ_BLOCK_ENTRY *)malloc(mpq->header.dwBlockTableSize * sizeof(MPQ_BLOCK_ENTRY));
    if (!mpq->blocktable) {
        goto fail;
    }

    if (!MpqSeek(mpq, mpq->header.dwBlockTablePos)) {
        goto fail;
    }
    read = fread(mpq->blocktable, sizeof(MPQ_BLOCK_ENTRY), mpq->header.dwBlockTableSize, fp);
    if (read != mpq->header.dwBlockTableSize) {
        goto fail;
    }

    // Decrypt block table
    DecryptBlock((LPBYTE)mpq->blocktable, mpq->header.dwBlockTableSize * sizeof(MPQ_BLOCK_ENTRY),
                 MPQ_KEY_BLOCK_TABLE);

    PreloadListfileCache(mpq);

    *archive = (HANDLE)mpq;
    return TRUE;

fail:
    if (mpq->lookup_cache) {
        for (i = 0; i < mpq->lookup_cache_size; i++) {
            MPQ_CACHE_ENTRY *entry = mpq->lookup_cache[i];
            while (entry) {
                MPQ_CACHE_ENTRY *next = entry->next;
                free(entry->name);
                free(entry);
                entry = next;
            }
        }
        free(mpq->lookup_cache);
    }
    if (mpq->hashtable) free(mpq->hashtable);
    if (mpq->blocktable) free(mpq->blocktable);
    if (mpq->sector_buffer) free(mpq->sector_buffer);
    free(mpq);
    fclose(fp);
    return FALSE;
}

BOOL SFileCloseArchive(HANDLE archive)
{
    MPQ_ARCHIVE *mpq = (MPQ_ARCHIVE *)archive;

    if (!mpq) {
        return FALSE;
    }

    if (mpq->hashtable) free(mpq->hashtable);
    if (mpq->blocktable) free(mpq->blocktable);
    if (mpq->sector_buffer) free(mpq->sector_buffer);
    if (mpq->lookup_cache) {
        DWORD i;
        for (i = 0; i < mpq->lookup_cache_size; i++) {
            MPQ_CACHE_ENTRY *entry = mpq->lookup_cache[i];
            while (entry) {
                MPQ_CACHE_ENTRY *next = entry->next;
                free(entry->name);
                free(entry);
                entry = next;
            }
        }
        free(mpq->lookup_cache);
    }
    if (mpq->fp) fclose(mpq->fp);
    free(mpq);

    return TRUE;
}

BOOL SFileOpenFileEx(HANDLE archive, LPCSTR fileName, DWORD searchScope, HANDLE *file)
{
    MPQ_ARCHIVE *mpq = (MPQ_ARCHIVE *)archive;
    MPQ_FILE *mpqfile;
    DWORD hash1, hash2;
    DWORD block_index;

    if (!mpq || !fileName || !file || searchScope != SFILE_OPEN_FROM_MPQ) {
        return FALSE;
    }

    // Calculate file name hashes
    hash1 = HashString(fileName, MPQ_HASH_NAME_A);
    hash2 = HashString(fileName, MPQ_HASH_NAME_B);

    if (!LookupCachedBlock(mpq, fileName, &block_index)) {
        if (!FindBlockIndex(mpq, fileName, hash1, hash2, &block_index)) {
            return FALSE;
        }
    }

    if (block_index >= mpq->header.dwBlockTableSize) {
        return FALSE;
    }

    mpqfile = (MPQ_FILE *)malloc(sizeof(MPQ_FILE));
    if (!mpqfile) {
        return FALSE;
    }

    memset(mpqfile, 0, sizeof(MPQ_FILE));
    mpqfile->archive = mpq;
    mpqfile->block_index = block_index;
    mpqfile->file_size = mpq->blocktable[block_index].dwFileSize;
    mpqfile->compressed_size = mpq->blocktable[block_index].dwBlockSize;
    mpqfile->flags = mpq->blocktable[block_index].dwFlags;
    mpqfile->current_pos = 0;
    mpqfile->file_key = 0;
    mpqfile->sector_count = 0;
    mpqfile->sector_offsets = NULL;

    if (mpqfile->flags & MPQ_FILE_ENCRYPTED) {
        mpqfile->file_key = HashString(BaseNamePtr(fileName), MPQ_HASH_FILE_KEY);
        if (mpqfile->flags & MPQ_FILE_KEY_V2) {
            mpqfile->file_key = (mpqfile->file_key + mpq->blocktable[block_index].dwBlockOffset) ^ mpqfile->file_size;
        }
    }

    if (mpqfile->flags & MPQ_FILE_COMPRESS) {
        DWORD sector_count;
        DWORD offset_count;
        MPQ_BLOCK_ENTRY *block;
        BOOL offsets_valid = FALSE;

        block = &mpq->blocktable[block_index];
        sector_count = (mpqfile->file_size + mpq->sector_size - 1) / mpq->sector_size;
        offset_count = sector_count + 1;
        mpqfile->sector_offsets = (DWORD *)malloc(offset_count * sizeof(DWORD));
        if (!mpqfile->sector_offsets) {
            free(mpqfile);
            return FALSE;
        }

        MpqSeek(mpq, block->dwBlockOffset);
        if (fread(mpqfile->sector_offsets, sizeof(DWORD), offset_count, mpq->fp) != offset_count) {
            free(mpqfile->sector_offsets);
            free(mpqfile);
            return FALSE;
        }

        offsets_valid = SectorTableLooksValid(mpqfile->sector_offsets, sector_count, mpqfile->compressed_size);
        if (!offsets_valid && (mpqfile->flags & MPQ_FILE_ENCRYPTED)) {
            DecryptBlock((LPBYTE)mpqfile->sector_offsets, offset_count * sizeof(DWORD), mpqfile->file_key - 1);
            offsets_valid = SectorTableLooksValid(mpqfile->sector_offsets, sector_count, mpqfile->compressed_size);
        }
        if (!offsets_valid) {
            free(mpqfile->sector_offsets);
            free(mpqfile);
            return FALSE;
        }

        mpqfile->sector_count = sector_count;
    }

    *file = (HANDLE)mpqfile;
    return TRUE;
}

BOOL SFileCloseFile(HANDLE file)
{
    if (file) {
        MPQ_FILE *mpqfile = (MPQ_FILE *)file;
        if (mpqfile->sector_offsets) {
            free(mpqfile->sector_offsets);
        }
        free(file);
    }
    return TRUE;
}

BOOL SFileReadFile(HANDLE file, void *buffer, DWORD toRead, LPDWORD bytesRead, LPOVERLAPPED overlapped)
{
    MPQ_FILE *mpqfile = (MPQ_FILE *)file;
    MPQ_ARCHIVE *mpq;
    MPQ_BLOCK_ENTRY *block;
    BYTE *dest = (BYTE *)buffer;
    DWORD read_so_far = 0;
    DWORD to_read_in_block;
    DWORD sector_start;
    DWORD sector_offset;
    DWORD bytes_in_sector;
    DWORD file_offset;

    (void)overlapped;

    if (!mpqfile || !buffer || toRead == 0) {
        if (bytesRead) *bytesRead = 0;
        return FALSE;
    }

    mpq = mpqfile->archive;
    block = &mpq->blocktable[mpqfile->block_index];

    if (mpqfile->current_pos >= mpqfile->file_size) {
        if (bytesRead) *bytesRead = 0;
        return TRUE;
    }

    // Limit read to file size
    if (mpqfile->current_pos + toRead > mpqfile->file_size) {
        toRead = mpqfile->file_size - mpqfile->current_pos;
    }

    file_offset = mpqfile->current_pos;

    // Determine if file is compressed
    if (!(block->dwFlags & MPQ_FILE_COMPRESS)) {
        // Uncompressed file
        MpqSeek(mpq, block->dwBlockOffset + file_offset);
        DWORD actually_read = (DWORD)fread(dest, 1, toRead, mpq->fp);
        if (actually_read > 0 && (mpqfile->flags & MPQ_FILE_ENCRYPTED)) {
            DecryptBlock(dest, actually_read, mpqfile->file_key);
        }
        mpqfile->current_pos += actually_read;
        if (bytesRead) *bytesRead = actually_read;
        return actually_read > 0;
    }

    // File is compressed - read through the sector offset table.
    if (!mpqfile->sector_offsets || mpqfile->sector_count == 0) {
        if (bytesRead) *bytesRead = 0;
        return FALSE;
    }

    sector_start = (file_offset / mpq->sector_size);
    sector_offset = (file_offset % mpq->sector_size);

    while (read_so_far < toRead && sector_start < mpqfile->sector_count) {
        DWORD sector_start_offset;
        DWORD sector_end_offset;
        DWORD sector_compressed_size;
        DWORD sector_uncompressed_size;
        BYTE *compressed;

        sector_start_offset = mpqfile->sector_offsets[sector_start];
        sector_end_offset = mpqfile->sector_offsets[sector_start + 1];

        if (sector_end_offset < sector_start_offset) {
            break;
        }

        sector_compressed_size = sector_end_offset - sector_start_offset;
        sector_uncompressed_size = mpq->sector_size;
        if (sector_start == mpqfile->sector_count - 1) {
            DWORD tail = mpqfile->file_size % mpq->sector_size;
            if (tail != 0) {
                sector_uncompressed_size = tail;
            }
        }

        compressed = (BYTE *)malloc(sector_compressed_size);
        if (!compressed) {
            break;
        }

        MpqSeek(mpq, block->dwBlockOffset + sector_start_offset);
        if (fread(compressed, 1, sector_compressed_size, mpq->fp) != sector_compressed_size) {
            free(compressed);
            break;
        }

        if (!TryInflateSector(compressed, sector_compressed_size, sector_uncompressed_size, mpq->sector_buffer, &bytes_in_sector)) {
            if (mpqfile->flags & MPQ_FILE_ENCRYPTED) {
                DecryptBlock(compressed, sector_compressed_size, mpqfile->file_key + sector_start);
                if (!TryInflateSector(compressed, sector_compressed_size, sector_uncompressed_size, mpq->sector_buffer, &bytes_in_sector)) {
                    free(compressed);
                    break;
                }
            } else if (sector_compressed_size >= sector_uncompressed_size) {
                memcpy(mpq->sector_buffer, compressed, sector_uncompressed_size);
                bytes_in_sector = sector_uncompressed_size;
            } else {
                free(compressed);
                break;
            }
        }
        free(compressed);

        if (sector_offset >= bytes_in_sector) {
            break;
        }

        to_read_in_block = bytes_in_sector - sector_offset;
        if (to_read_in_block > (toRead - read_so_far)) {
            to_read_in_block = (toRead - read_so_far);
        }

        memcpy(dest, &mpq->sector_buffer[sector_offset], to_read_in_block);
        dest += to_read_in_block;
        read_so_far += to_read_in_block;
        sector_offset = 0;
        sector_start++;
    }

    mpqfile->current_pos += read_so_far;
    if (bytesRead) *bytesRead = read_so_far;

    return read_so_far > 0 || toRead == 0;
}

DWORD SFileGetFileSize(HANDLE file, LPDWORD highSize)
{
    MPQ_FILE *mpqfile = (MPQ_FILE *)file;

    if (!mpqfile) {
        return 0;
    }

    if (highSize) {
        *highSize = 0;
    }

    return mpqfile->file_size;
}

DWORD SFileSetFilePointer(HANDLE file, LONG distance, PLONG distanceHigh, DWORD moveMethod)
{
    MPQ_FILE *mpqfile = (MPQ_FILE *)file;
    DWORD new_pos;

    if (!mpqfile) {
        return SFILE_INVALID_POS;
    }

    (void)distanceHigh;

    switch (moveMethod) {
        case FILE_BEGIN:
            new_pos = distance;
            break;
        case FILE_CURRENT:
            new_pos = mpqfile->current_pos + distance;
            break;
        case FILE_END:
            new_pos = mpqfile->file_size + distance;
            break;
        default:
            return SFILE_INVALID_POS;
    }

    if (new_pos > mpqfile->file_size) {
        return SFILE_INVALID_POS;
    }

    mpqfile->current_pos = new_pos;
    return new_pos;
}

BOOL SFileExtractFile(HANDLE archive, LPCSTR toExtract, LPCSTR extracted, DWORD flags)
{
    HANDLE file;
    FILE *out;
    DWORD file_size;
    DWORD bytes_read;
    BYTE buffer[65536];
    BOOL success = FALSE;

    (void)flags;

    if (!SFileOpenFileEx(archive, toExtract, SFILE_OPEN_FROM_MPQ, &file)) {
        return FALSE;
    }

    file_size = SFileGetFileSize(file, NULL);
    out = fopen(extracted, "wb");
    if (!out) {
        SFileCloseFile(file);
        return FALSE;
    }

    while (file_size > 0) {
        DWORD to_read = (file_size > sizeof(buffer)) ? sizeof(buffer) : file_size;
        if (!SFileReadFile(file, buffer, to_read, &bytes_read, NULL) || bytes_read == 0) {
            break;
        }
        if (fwrite(buffer, 1, bytes_read, out) != bytes_read) {
            break;
        }
        file_size -= bytes_read;
    }

    if (file_size == 0) {
        success = TRUE;
    }

    fclose(out);
    SFileCloseFile(file);
    return success;
}

static DWORD PreloadBucketHash(DWORD hash1, DWORD hash2)
{
    return hash1 ^ (hash2 * 16777619u);
}

static void PreloadListfileCache(MPQ_ARCHIVE *mpq)
{
    HANDLE list_file;
    DWORD list_size;
    BYTE *buffer = NULL;
    DWORD bytes_read = 0;
    char *cursor;
    char *line;
    size_t entry_count = 0;
    size_t bucket_count = 0;
    MPQ_LISTFILE_ENTRY *entries = NULL;
    MPQ_LISTFILE_BUCKET_ENTRY **buckets = NULL;
    size_t i;
    BOOL trace = getenv("OW3_MPQ_TRACE") != NULL;

    if (!mpq || !mpq->hashtable || !mpq->blocktable) {
        return;
    }

    if (!SFileOpenFileEx((HANDLE)mpq, "(listfile)", SFILE_OPEN_FROM_MPQ, &list_file)) {
        return;
    }

    list_size = SFileGetFileSize(list_file, NULL);
    if (list_size == 0) {
        SFileCloseFile(list_file);
        return;
    }

    buffer = (BYTE *)malloc(list_size + 1);
    if (!buffer) {
        SFileCloseFile(list_file);
        return;
    }

    if (!SFileReadFile(list_file, buffer, list_size, &bytes_read, NULL) || bytes_read == 0) {
        free(buffer);
        SFileCloseFile(list_file);
        return;
    }
    buffer[bytes_read] = '\0';
    SFileCloseFile(list_file);

    cursor = (char *)buffer;
    while (*cursor) {
        while (*cursor == '\r' || *cursor == '\n') {
            cursor++;
        }
        if (!*cursor) {
            break;
        }
        line = cursor;
        while (*cursor && *cursor != '\r' && *cursor != '\n') {
            cursor++;
        }
        if (*cursor) {
            *cursor++ = '\0';
        }
        if (*line == '\0') {
            continue;
        }
        entry_count++;
    }

    if (entry_count == 0) {
        free(buffer);
        return;
    }

    entries = (MPQ_LISTFILE_ENTRY *)calloc(entry_count, sizeof(MPQ_LISTFILE_ENTRY));
    if (!entries) {
        free(buffer);
        return;
    }

    cursor = (char *)buffer;
    for (i = 0; i < entry_count; i++) {
        while (*cursor == '\r' || *cursor == '\n') {
            cursor++;
        }
        line = cursor;
        while (*cursor && *cursor != '\r' && *cursor != '\n') {
            cursor++;
        }
        if (*cursor) {
            *cursor++ = '\0';
        }

        entries[i].name = (char *)malloc(strlen(line) + 1);
        if (!entries[i].name) {
            break;
        }
        strcpy(entries[i].name, line);
        CanonicalizeMpqKey(entries[i].name, entries[i].name, strlen(entries[i].name) + 1);
        entries[i].hash1 = HashString(entries[i].name, MPQ_HASH_NAME_A);
        entries[i].hash2 = HashString(entries[i].name, MPQ_HASH_NAME_B);
    }

    if (i != entry_count) {
        for (i = 0; i < entry_count; i++) {
            free(entries[i].name);
        }
        free(entries);
        free(buffer);
        return;
    }

    bucket_count = 1;
    while (bucket_count < entry_count * 2) {
        bucket_count <<= 1;
    }

    buckets = (MPQ_LISTFILE_BUCKET_ENTRY **)calloc(bucket_count, sizeof(MPQ_LISTFILE_BUCKET_ENTRY *));
    if (!buckets) {
        for (i = 0; i < entry_count; i++) {
            free(entries[i].name);
        }
        free(entries);
        free(buffer);
        return;
    }

    for (i = 0; i < entry_count; i++) {
        MPQ_LISTFILE_BUCKET_ENTRY *node = (MPQ_LISTFILE_BUCKET_ENTRY *)malloc(sizeof(MPQ_LISTFILE_BUCKET_ENTRY));
        DWORD slot;

        if (!node) {
            continue;
        }
        slot = PreloadBucketHash(entries[i].hash1, entries[i].hash2) & (bucket_count - 1);
        node->entry = &entries[i];
        node->next = buckets[slot];
        buckets[slot] = node;
    }

    for (i = 0; i < mpq->header.dwHashTableSize; i++) {
        MPQ_HASH_ENTRY *hash_entry = &mpq->hashtable[i];
        DWORD slot;
        MPQ_LISTFILE_BUCKET_ENTRY *node;

        if (hash_entry->dwBlockIndex == MPQ_HASH_ENTRY_FREE ||
            hash_entry->dwBlockIndex == MPQ_HASH_ENTRY_DELETED) {
            continue;
        }

        slot = PreloadBucketHash(hash_entry->dwNameHash1, hash_entry->dwNameHash2) & (bucket_count - 1);
        node = buckets[slot];
        while (node) {
            if (node->entry->hash1 == hash_entry->dwNameHash1 &&
                node->entry->hash2 == hash_entry->dwNameHash2) {
                CacheBlockLookup(mpq, node->entry->name, hash_entry->dwBlockIndex & MPQ_BLOCK_INDEX_MASK);
                break;
            }
            node = node->next;
        }
    }

    if (trace) {
        fprintf(stderr, "MPQ preload: cached %zu listfile names\n", entry_count);
    }

    for (i = 0; i < bucket_count; i++) {
        MPQ_LISTFILE_BUCKET_ENTRY *node = buckets[i];
        while (node) {
            MPQ_LISTFILE_BUCKET_ENTRY *next = node->next;
            free(node);
            node = next;
        }
    }
    for (i = 0; i < entry_count; i++) {
        free(entries[i].name);
    }
    free(entries);
    free(buckets);
    free(buffer);
}

static BOOL FilenameMatches(const char *filename, const char *mask);
static void FreeFindList(MPQ_FIND *find);
static BOOL AppendFindListEntry(MPQ_FIND *find, const char *name);

HANDLE SFileFindFirstFile(HANDLE archive, LPCSTR mask, SFILE_FIND_DATA *findData, LPCSTR listFile)
{
    MPQ_ARCHIVE *mpq = (MPQ_ARCHIVE *)archive;
    MPQ_FIND *find;
    HANDLE list_file;
    DWORD list_size;
    BYTE *list_buffer;
    DWORD bytes_read;
    char *cursor;

    (void)listFile;

    if (!mpq || !mask || !findData) {
        return NULL;
    }

    find = (MPQ_FIND *)malloc(sizeof(MPQ_FIND));
    if (!find) {
        return NULL;
    }

    memset(find, 0, sizeof(MPQ_FIND));
    find->archive = mpq;
    find->current_index = 0;
    find->file_count = 0;
    find->files = NULL;
    strncpy(find->mask, mask, sizeof(find->mask) - 1);

    if (!SFileOpenFileEx(archive, "(listfile)", SFILE_OPEN_FROM_MPQ, &list_file)) {
        free(find);
        return NULL;
    }

    list_size = SFileGetFileSize(list_file, NULL);
    list_buffer = (BYTE *)malloc(list_size + 1);
    if (!list_buffer) {
        SFileCloseFile(list_file);
        free(find);
        return NULL;
    }

    if (!SFileReadFile(list_file, list_buffer, list_size, &bytes_read, NULL) || bytes_read != list_size) {
        free(list_buffer);
        SFileCloseFile(list_file);
        FreeFindList(find);
        free(find);
        return NULL;
    }
    list_buffer[list_size] = '\0';
    SFileCloseFile(list_file);

    cursor = (char *)list_buffer;
    while (*cursor) {
        char *line_end = cursor;

        while (*line_end && *line_end != '\r' && *line_end != '\n') {
            line_end++;
        }

        if (line_end > cursor) {
            char saved = *line_end;

            *line_end = '\0';
            NormalizeMpqPath(cursor);
            TrimEdgeSlashes(cursor);
            if (FilenameMatches(cursor, mask)) {
                if (!AppendFindListEntry(find, cursor)) {
                    free(list_buffer);
                    FreeFindList(find);
                    free(find);
                    return NULL;
                }
            }
            *line_end = saved;
        }

        while (*line_end == '\r' || *line_end == '\n') {
            line_end++;
        }
        cursor = line_end;
    }

    free(list_buffer);

    if (!SFileFindNextFile((HANDLE)find, findData)) {
        FreeFindList(find);
        free(find);
        return NULL;
    }

    return (HANDLE)find;
}

static BOOL FilenameMatches(const char *filename, const char *mask)
{
    // Simple wildcard matching - "*" matches everything
    if (!mask || !mask[0] || strcmp(mask, "*") == 0) {
        return TRUE;
    }

    // For now, simple prefix matching
    size_t mask_len = strlen(mask);
    if (mask[mask_len - 1] == '*') {
        return strncmp(filename, mask, mask_len - 1) == 0;
    }

    return strcmp(filename, mask) == 0;
}

static void FreeFindList(MPQ_FIND *find)
{
    DWORD i;

    if (!find) {
        return;
    }

    for (i = 0; i < find->file_count; i++) {
        free(find->files[i]);
    }
    free(find->files);
    find->files = NULL;
    find->file_count = 0;
}

static BOOL AppendFindListEntry(MPQ_FIND *find, const char *name)
{
    char **next;
    char *copy;

    next = (char **)realloc(find->files, (find->file_count + 1) * sizeof(*next));
    if (!next) {
        return FALSE;
    }

    copy = (char *)malloc(strlen(name) + 1);
    if (!copy) {
        return FALSE;
    }

    strcpy(copy, name);
    find->files = next;
    find->files[find->file_count++] = copy;
    return TRUE;
}

BOOL SFileFindNextFile(HANDLE find, SFILE_FIND_DATA *findData)
{
    MPQ_FIND *mpqfind = (MPQ_FIND *)find;
    const char *name;

    if (!mpqfind || !findData) {
        return FALSE;
    }

    if (mpqfind->current_index >= mpqfind->file_count) {
        return FALSE;
    }

    name = mpqfind->files[mpqfind->current_index++];
    memset(findData, 0, sizeof(*findData));
    strncpy(findData->cFileName, name, sizeof(findData->cFileName) - 1);
    findData->cFileName[sizeof(findData->cFileName) - 1] = '\0';
    findData->szPlainName = findData->cFileName;
    return TRUE;
}

BOOL SFileFindClose(HANDLE find)
{
    if (find) {
        FreeFindList((MPQ_FIND *)find);
        free(find);
    }
    return TRUE;
}
