#include "sc2_map.h"
#include "common/mpq.h"
#include <ctype.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define SC2_MAX_CATALOG_MODELS       8192
#define SC2_MAX_CATALOG_ACTORS       8192
#define SC2_MAX_CATALOG_TERRAIN_TEX  512
#define SC2_MAX_CATALOG_CLIFFS       256
#define SC2_MAPINFO_MIN_SIZE         16
#define SC2_HMAP_HEADER_SIZE         32
#define SC2_HMAP_CHUNK_SIZE          6
#define SC2_SMAP_HEADER_SIZE         64
#define SC2_SMAP_CHUNK_SIZE          4
#define SC2_TERRAIN_HEADER_SIZE      32

typedef struct {
    HANDLE archive;
    HANDLE archive_data;
    char   base[MAX_PATHLEN];
} sc2MapSource_t;

typedef struct {
    char id[64];
    char path[256];
} sc2CatalogModel_t;

typedef struct {
    char id[64];
    char model[64];
} sc2CatalogActor_t;

typedef struct {
    char name[64];
    char model[256];
} sc2ResolvedObjectModel_t;

typedef struct {
    char id[64];
    char diffuse[256];
    char normal[256];
} sc2CatalogTerrainTex_t;

typedef struct {
    char id[64];
    char mesh[64];
} sc2CatalogCliff_t;

typedef struct {
    DWORD models_count;
    DWORD actors_count;
    DWORD terrain_tex_count;
    DWORD cliffs_count;
    sc2CatalogModel_t models[SC2_MAX_CATALOG_MODELS];
    sc2CatalogActor_t actors[SC2_MAX_CATALOG_ACTORS];
    sc2CatalogTerrainTex_t terrain_tex[SC2_MAX_CATALOG_TERRAIN_TEX];
    sc2CatalogCliff_t cliffs[SC2_MAX_CATALOG_CLIFFS];
} sc2Catalog_t;

static sc2MapHost_t sc2_host;
static sc2Map_t     sc2_map;

static DWORD sc2_read_le32(BYTE const *p);
static USHORT sc2_read_le16(BYTE const *p);
static LONG sc2_read_i32(BYTE const *p);
static BOOL sc2_mapinfo_magic(LPBYTE data, DWORD size);
static void sc2_set_full_size(DWORD width, DWORD height);
static void sc2_set_playable_bounds(LONG left, LONG bottom, LONG right, LONG top);
static void sc2_map_position_to_playable(LPVECTOR3 position);
static BOOL sc2_playable_crop(DWORD src_w, DWORD src_h, BOOL vertices, DWORD *left, DWORD *bottom, DWORD *width, DWORD *height);

static HANDLE sc2_alloc(long size) {
    return sc2_host.mem_alloc ? sc2_host.mem_alloc(size) : NULL;
}

static void sc2_free(HANDLE mem) {
    if (!mem) return;
    if (sc2_host.mem_free) sc2_host.mem_free(mem);
}

static HANDLE sc2_read_file(LPCSTR filename, LPDWORD size) {
    return sc2_host.read_file ? sc2_host.read_file(filename, size) : NULL;
}

static void sc2_free_file(HANDLE file) {
    if (!file) return;
    if (sc2_host.free_file) sc2_host.free_file(file);
}

static BOOL sc2_streqi(LPCSTR a, LPCSTR b) {
    return a && b && !strcasecmp(a, b);
}

static BOOL sc2_contains_i(LPCSTR text, LPCSTR needle) {
    char a[128], b[64];
    DWORD i;
    if (!text || !needle) return false;
    snprintf(a, sizeof(a), "%s", text);
    snprintf(b, sizeof(b), "%s", needle);
    for (i = 0; a[i]; i++) a[i] = (char)tolower((unsigned char)a[i]);
    for (i = 0; b[i]; i++) b[i] = (char)tolower((unsigned char)b[i]);
    return strstr(a, b) != NULL;
}

static BOOL sc2_has_extension_i(LPCSTR path, LPCSTR ext) {
    DWORD path_len;
    DWORD ext_len;

    if (!path || !ext) return false;
    path_len = (DWORD)strlen(path);
    ext_len = (DWORD)strlen(ext);
    return path_len >= ext_len && !strcasecmp(path + path_len - ext_len, ext);
}

static BOOL sc2_path_has_dir(LPCSTR path) {
    return path && (strchr(path, '\\') || strchr(path, '/'));
}

static void sc2_normalize_slashes(LPSTR path) {
    if (!path) return;
    for (char *p = path; *p; p++) if (*p == '/') *p = '\\';
}

static void sc2_append_extension(LPSTR path, DWORD size, LPCSTR ext) {
    if (!path || !ext || !*path || strchr(strrchr(path, '\\') ? strrchr(path, '\\') : path, '.')) return;
    strncat(path, ext, size - strlen(path) - 1);
}

static BOOL sc2_file_exists(LPCSTR path) {
    DWORD size = 0;
    HANDLE data;

    if (!path || !*path) return false;
    data = sc2_read_file(path, &size);
    if (data) sc2_free_file(data);
    return data && size > 0;
}

static void sc2_camel_to_underscore(LPCSTR in, LPSTR out, DWORD out_size) {
    DWORD w = 0;

    if (!out || out_size == 0) return;
    out[0] = '\0';
    if (!in) return;
    for (DWORD i = 0; in[i] && w + 1 < out_size; i++) {
        BOOL split = i > 0 && isupper((unsigned char)in[i]) &&
                     (islower((unsigned char)in[i - 1]) ||
                      (in[i + 1] && islower((unsigned char)in[i + 1])));
        if (split && w + 1 < out_size) out[w++] = '_';
        out[w++] = in[i];
    }
    out[w] = '\0';
}

static DWORD sc2_hash32(LPCSTR str) {
    DWORD hash = 2166136261u;
    while (str && *str) hash = (hash ^ (BYTE)*str++) * 16777619u;
    return hash;
}

static void sc2_map_clear(void) {
    FOR_LOOP(i, SC2_MAX_TERRAIN_TEXTURES) {
        SAFE_DELETE(sc2_map.texture_masks[i], sc2_free);
    }
    SAFE_DELETE(sc2_map.cell_flags, sc2_free);
    SAFE_DELETE(sc2_map.cliff_levels, sc2_free);
    SAFE_DELETE(sc2_map.height_map, sc2_free);
    SAFE_DELETE(sc2_map.height_adjust_map, sc2_free);
    memset(&sc2_map, 0, sizeof(sc2_map));
}

static void sc2_set_default_camera(void) {
    sc2_map.camera_distance = 34.07f;
    sc2_map.camera_pitch = 56.0f;
    sc2_map.camera_yaw = 180.0f;
    sc2_map.camera_fov = 27.8f;
    sc2_map.camera_znear = 0.1f;
    sc2_map.camera_zfar = 400.0f;
}

static BOOL sc2_parse_vec3(LPCSTR text, LPVECTOR3 out) {
    char const *p = text;
    char *end;
    if (!text || !out) return false;
    out->x = strtof(p, &end);
    if (end == p) return false;
    p = end;
    while (*p == ',' || isspace((unsigned char)*p)) p++;
    out->y = strtof(p, &end);
    if (end == p) return false;
    p = end;
    while (*p == ',' || isspace((unsigned char)*p)) p++;
    out->z = strtof(p, &end);
    if (end == p) out->z = 0.0f;
    return true;
}

static BOOL sc2_has_nonspace(LPCSTR text) {
    if (!text) return false;
    while (*text) {
        if (!isspace((unsigned char)*text)) return true;
        text++;
    }
    return false;
}

static BOOL sc2_source_open(sc2MapSource_t *source, LPCSTR mapFilename) {
    DWORD size = 0;
    memset(source, 0, sizeof(*source));
    if (mapFilename && *mapFilename) {
        source->archive_data = sc2_read_file(mapFilename, &size);
        if (source->archive_data && size > 0 &&
            SFileOpenArchiveFromMemory(source->archive_data, size, 0, &source->archive)) {
            return true;
        }
        snprintf(source->base, sizeof(source->base), "%s", mapFilename);
    }
    return true;
}

static void sc2_source_close(sc2MapSource_t *source) {
    if (source->archive) SFileCloseArchive(source->archive);
    sc2_free_file(source->archive_data);
    memset(source, 0, sizeof(*source));
}

static HANDLE sc2_source_read(sc2MapSource_t *source, LPCSTR filename, LPDWORD size) {
    PATHSTR path;
    HANDLE file;
    DWORD file_size;
    LPBYTE data;

    if (size) *size = 0;
    if (!source || !filename || !*filename) return NULL;

    if (source->archive) {
        if (!SFileOpenFileEx(source->archive, filename, SFILE_OPEN_FROM_MPQ, &file)) {
            snprintf(path, sizeof(path), "%s", filename);
            for (char *p = path; *p; p++) if (*p == '/') *p = '\\';
            if (!SFileOpenFileEx(source->archive, path, SFILE_OPEN_FROM_MPQ, &file))
                return NULL;
        }
        file_size = SFileGetFileSize(file, NULL);
        data = sc2_alloc(file_size + 1);
        if (!SFileReadFile(file, data, file_size, NULL, NULL)) {
            SFileCloseFile(file);
            sc2_free(data);
            return NULL;
        }
        SFileCloseFile(file);
        data[file_size] = 0;
        if (size) *size = file_size;
        return data;
    }

    if (!source->base[0])
        return sc2_read_file(filename, size);
    snprintf(path, sizeof(path), "%s/%s", source->base, filename);
    data = sc2_read_file(path, size);
    if (!data) {
        snprintf(path, sizeof(path), "%s\\%s", source->base, filename);
        data = sc2_read_file(path, size);
    }
    return data;
}

static xmlDocPtr sc2_read_xml(sc2MapSource_t *source, LPCSTR filename) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, filename, &size);
    xmlDocPtr doc = NULL;
    if (data && size > 0) {
        doc = xmlReadMemory((char const *)data, (int)size, filename, NULL,
                            XML_PARSE_RECOVER | XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    }
    sc2_free_file(data);
    return doc;
}

static xmlDocPtr sc2_read_global_xml(LPCSTR filename) {
    DWORD size = 0;
    LPBYTE data = sc2_read_file(filename, &size);
    xmlDocPtr doc = NULL;

    if (!data && filename) {
        PATHSTR path;
        snprintf(path, sizeof(path), "%s", filename);
        for (char *p = path; *p; p++) if (*p == '\\') *p = '/';
        data = sc2_read_file(path, &size);
    }
    if (data && size > 0) {
        doc = xmlReadMemory((char const *)data, (int)size, filename, NULL,
                            XML_PARSE_RECOVER | XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    }
    sc2_free_file(data);
    return doc;
}

static xmlDocPtr sc2_read_catalog_xml(LPCSTR root, LPCSTR filename) {
    DWORD archive_size = 0;
    DWORD file_size;
    LPBYTE archive_data;
    LPBYTE data;
    HANDLE archive;
    HANDLE file;
    xmlDocPtr doc = NULL;
    PATHSTR path;

    if (!root || !*root) return sc2_read_global_xml(filename);
    archive_data = sc2_read_file(root, &archive_size);
    if (!archive_data) {
        snprintf(path, sizeof(path), "%s", root);
        for (char *p = path; *p; p++) if (*p == '/') *p = '\\';
        archive_data = sc2_read_file(path, &archive_size);
    }
    if (!archive_data || archive_size == 0 ||
        !SFileOpenArchiveFromMemory(archive_data, archive_size, 0, &archive)) {
        sc2_free_file(archive_data);
        return NULL;
    }
    if (!SFileOpenFileEx(archive, filename, SFILE_OPEN_FROM_MPQ, &file)) {
        snprintf(path, sizeof(path), "%s", filename);
        for (char *p = path; *p; p++) if (*p == '\\') *p = '/';
        if (!SFileOpenFileEx(archive, path, SFILE_OPEN_FROM_MPQ, &file)) {
            SFileCloseArchive(archive);
            sc2_free_file(archive_data);
            return NULL;
        }
    }
    file_size = SFileGetFileSize(file, NULL);
    data = sc2_alloc(file_size + 1);
    if (data && SFileReadFile(file, data, file_size, NULL, NULL)) {
        data[file_size] = 0;
        doc = xmlReadMemory((char const *)data, (int)file_size, filename, NULL,
                            XML_PARSE_RECOVER | XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    }
    sc2_free(data);
    SFileCloseFile(file);
    SFileCloseArchive(archive);
    sc2_free_file(archive_data);
    return doc;
}

static BOOL sc2_xml_attr(xmlNodePtr node, LPCSTR attr_name, LPSTR buffer, DWORD size) {
    xmlChar *text;

    if (!node || !attr_name || !buffer || !size) return false;
    text = xmlGetProp(node, (xmlChar const *)attr_name);
    if (!text) return false;
    snprintf(buffer, size, "%s", (char const *)text);
    xmlFree(text);
    return buffer[0] != '\0';
}

static LPCSTR sc2_xml_content(xmlNodePtr node, char *buffer, DWORD size) {
    xmlChar *text;
    if (!node || !buffer || size == 0) return NULL;
    text = xmlNodeGetContent(node);
    if (!text) return NULL;
    snprintf(buffer, size, "%s", (char const *)text);
    xmlFree(text);
    return buffer;
}

static void sc2_add_terrain_texture(LPCSTR path) {
    sc2TerrainTexture_t *texture;
    char buffer[256];
    char *ext;

    if (!path || !*path || sc2_map.num_terrain_textures >= SC2_MAX_TERRAIN_TEXTURES)
        return;
    snprintf(buffer, sizeof(buffer), "%s", path);
    for (char *p = buffer; *p; p++) {
        if (*p == '/') *p = '\\';
    }
    if (sc2_path_has_dir(buffer)) {
        sc2_append_extension(buffer, sizeof(buffer), ".dds");
    }
    FOR_LOOP(i, sc2_map.num_terrain_textures) {
        if (!strcasecmp(sc2_map.terrain_textures[i].diffuse, buffer)) {
            return;
        }
    }

    texture = &sc2_map.terrain_textures[sc2_map.num_terrain_textures++];
    snprintf(texture->diffuse, sizeof(texture->diffuse), "%s", buffer);
    snprintf(texture->normal, sizeof(texture->normal), "%s", buffer);
    ext = sc2_path_has_dir(texture->normal) ? strrchr(texture->normal, '.') : NULL;
    if (ext && sc2_has_extension_i(texture->normal, ".dds")) {
        snprintf(ext, sizeof(texture->normal) - (DWORD)(ext - texture->normal), "_normal.dds");
    }
}

static void sc2_parse_terrain_value(LPCSTR key, LPCSTR value) {
    if (!key || !value || !*value)
        return;
    if (sc2_contains_i(key, "normal")) {
        DWORD index = sc2_map.num_terrain_textures ? sc2_map.num_terrain_textures - 1 : 0;
        if (index < SC2_MAX_TERRAIN_TEXTURES) {
            snprintf(sc2_map.terrain_textures[index].normal,
                     sizeof(sc2_map.terrain_textures[index].normal),
                     "%s",
                     value);
        }
    } else if (sc2_contains_i(key, "texture") ||
               sc2_contains_i(key, "diffuse") ||
               sc2_contains_i(key, "blend") ||
               sc2_contains_i(key, "file") ||
               strstr(value, ".dds") ||
               strstr(value, ".DDS")) {
        sc2_add_terrain_texture(value);
    }
}

static void sc2_parse_cliff_set_node(xmlNodePtr node) {
    char value[64];
    DWORD index;

    if (!node || !sc2_streqi((char const *)node->name, "cliffSet"))
        return;
    if (!sc2_xml_attr(node, "i", value, sizeof(value)))
        return;
    index = (DWORD)strtoul(value, NULL, 10);
    if (index >= SC2_MAX_CLIFF_SETS)
        return;
    if (!sc2_xml_attr(node, "name", sc2_map.cliff_sets[index].name, sizeof(sc2_map.cliff_sets[index].name)))
        return;
    sc2_map.num_cliff_sets = MAX(sc2_map.num_cliff_sets, index + 1);
}

static void sc2_parse_cliff_cell_node(xmlNodePtr node) {
    sc2CliffCell_t *cell;
    char value[64];

    if (!node || !sc2_streqi((char const *)node->name, "cc") || sc2_map.num_cliff_cells >= SC2_MAX_CLIFF_CELLS)
        return;
    cell = &sc2_map.cliff_cells[sc2_map.num_cliff_cells];
    memset(cell, 0, sizeof(*cell));
    if (!sc2_xml_attr(node, "i", value, sizeof(value)))
        return;
    cell->index = (DWORD)strtoul(value, NULL, 10);
    if (sc2_xml_attr(node, "f", value, sizeof(value))) cell->flags = (DWORD)strtoul(value, NULL, 10);
    if (sc2_xml_attr(node, "cid", value, sizeof(value))) cell->cliff_set = (DWORD)strtoul(value, NULL, 10);
    if (sc2_xml_attr(node, "cvar", value, sizeof(value))) cell->variant = (DWORD)strtoul(value, NULL, 10);
    sc2_map.num_cliff_cells++;
}

static void sc2_map_try_size_field(LPCSTR key, LPCSTR value) {
    VECTOR3 v;
    if (!key || !value || !*value) return;
    if ((sc2_contains_i(key, "width") || sc2_streqi(key, "x")) && atoi(value) > 0)
        sc2_map.width = (DWORD)atoi(value);
    else if ((sc2_contains_i(key, "height") || sc2_streqi(key, "y")) && atoi(value) > 0)
        sc2_map.height = (DWORD)atoi(value);
    else if ((sc2_contains_i(key, "size") || sc2_contains_i(key, "bounds")) && sc2_parse_vec3(value, &v)) {
        if (v.x > 0) sc2_map.width = (DWORD)v.x;
        if (v.y > 0) sc2_map.height = (DWORD)v.y;
    } else if ((sc2_contains_i(key, "name") || sc2_contains_i(key, "title")) && !sc2_map.map_name[0]) {
        snprintf(sc2_map.map_name, sizeof(sc2_map.map_name), "%s", value);
    }
}

static void sc2_parse_mapinfo_node(xmlNodePtr node) {
    char value[256];
    if (!node || node->type != XML_ELEMENT_NODE) return;
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
        xmlChar *text = xmlNodeListGetString(node->doc, attr->children, 1);
        if (text) {
            LPCSTR key = sc2_streqi((char const *)attr->name, "value") ?
                         (char const *)node->name : (char const *)attr->name;
            sc2_map_try_size_field(key, (char const *)text);
            xmlFree(text);
        }
    }
    if (sc2_xml_content(node, value, sizeof(value)) && sc2_has_nonspace(value))
        sc2_map_try_size_field((char const *)node->name, value);
    for (xmlNodePtr child = node->children; child; child = child->next)
        sc2_parse_mapinfo_node(child);
}

static BOOL sc2_parse_mapinfo_binary(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "MapInfo", &size);
    DWORD offset;
    DWORD width;
    DWORD height;
    LONG left, bottom, right, top;

    if (!sc2_mapinfo_magic(data, size)) {
        sc2_free_file(data);
        return false;
    }
    width = sc2_read_le32(data + 8);
    height = sc2_read_le32(data + 12);
    offset = 16;
    FOR_LOOP(i, 2) {
        DWORD string_start = offset;
        while (offset < size && data[offset])
            offset++;
        if (offset >= size) {
            sc2_free_file(data);
            return false;
        }
        if (i == 0 && offset > string_start && !sc2_map.map_name[0]) {
            snprintf(sc2_map.map_name, sizeof(sc2_map.map_name), "%s", (char const *)data + string_start);
        }
        offset++;
    }
    if (offset + 16 > size) {
        sc2_free_file(data);
        return false;
    }
    left = sc2_read_i32(data + offset);
    bottom = sc2_read_i32(data + offset + 4);
    right = sc2_read_i32(data + offset + 8);
    top = sc2_read_i32(data + offset + 12);
    sc2_set_full_size(width, height);
    sc2_set_playable_bounds(left, bottom, right, top);
    sc2_free_file(data);
    return true;
}

static void sc2_parse_mapinfo(sc2MapSource_t *source) {
    xmlDocPtr doc;

    if (sc2_parse_mapinfo_binary(source))
        return;
    doc = sc2_read_xml(source, "MapInfo");
    if (!doc) doc = sc2_read_xml(source, "MapInfo.xml");
    if (!doc) return;
    sc2_parse_mapinfo_node(xmlDocGetRootElement(doc));
    sc2_set_full_size(sc2_map.width, sc2_map.height);
    xmlFreeDoc(doc);
}

static void sc2_parse_terrain_node(xmlNodePtr node) {
    char value[256];

    if (!node || node->type != XML_ELEMENT_NODE)
        return;
    sc2_parse_cliff_set_node(node);
    sc2_parse_cliff_cell_node(node);
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
        xmlChar *text = xmlNodeListGetString(node->doc, attr->children, 1);
        if (text) {
            LPCSTR key = (sc2_contains_i((char const *)node->name, "texture") &&
                          sc2_streqi((char const *)attr->name, "name")) ?
                         (char const *)node->name : (char const *)attr->name;
            if (sc2_streqi((char const *)node->name, "heightMap") &&
                sc2_streqi((char const *)attr->name, "tileSet")) {
                snprintf(sc2_map.tile_set, sizeof(sc2_map.tile_set), "%s", (char const *)text);
            } else if (sc2_streqi((char const *)node->name, "vertData")) {
                if (sc2_streqi((char const *)attr->name, "quantizeBias")) {
                    sc2_map.height_quantize_bias = strtof((char const *)text, NULL);
                } else if (sc2_streqi((char const *)attr->name, "quantizeScale")) {
                    sc2_map.height_quantize_scale = strtof((char const *)text, NULL);
                } else if (sc2_streqi((char const *)attr->name, "standardHeight")) {
                    sc2_map.standard_height = strtof((char const *)text, NULL);
                }
            }
            sc2_parse_terrain_value(key, (char const *)text);
            xmlFree(text);
        }
    }
    if (sc2_xml_content(node, value, sizeof(value)) && sc2_has_nonspace(value)) {
        sc2_parse_terrain_value((char const *)node->name, value);
    }
    for (xmlNodePtr child = node->children; child; child = child->next) {
        sc2_parse_terrain_node(child);
    }
}

static void sc2_parse_terrain(sc2MapSource_t *source) {
    xmlDocPtr doc = sc2_read_xml(source, "t3Terrain.xml");
    if (!doc) doc = sc2_read_xml(source, "t3Terrain");
    if (!doc) return;
    sc2_parse_terrain_node(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
}

static void sc2_set_object_model(sc2MapObject_t *object) {
    if (!object->model[0] && object->name[0] && object->type == SC2_OBJECT_UNIT)
        snprintf(object->model, sizeof(object->model), "Assets\\Units\\Terran\\%s\\%s.m3", object->name, object->name);
}

static void sc2_object_field(sc2MapObject_t *object, LPCSTR key, LPCSTR value, BOOL *has_position) {
    VECTOR3 v;
    if (!object || !key || !value || !*value) return;
    if ((sc2_contains_i(key, "model") || sc2_contains_i(key, "file")) && !object->model[0])
        snprintf(object->model, sizeof(object->model), "%s", value);
    else if (sc2_streqi(key, "id")) {
        return;
    } else if ((sc2_contains_i(key, "type") || sc2_contains_i(key, "unit") || sc2_contains_i(key, "doodad") ||
                sc2_streqi(key, "name")) && !object->name[0])
        snprintf(object->name, sizeof(object->name), "%s", value);
    else if (sc2_streqi(key, "x") || sc2_contains_i(key, "posx")) {
        object->position.x = strtof(value, NULL); *has_position = true;
    } else if (sc2_streqi(key, "y") || sc2_contains_i(key, "posy")) {
        object->position.y = strtof(value, NULL); *has_position = true;
    } else if (sc2_streqi(key, "z") || sc2_contains_i(key, "posz")) {
        object->position.z = strtof(value, NULL);
    } else if (sc2_contains_i(key, "position") && sc2_parse_vec3(value, &v)) {
        object->position = v; *has_position = true;
    } else if (sc2_contains_i(key, "angle") || sc2_contains_i(key, "yaw") || sc2_contains_i(key, "rotation")) {
        object->angle = strtof(value, NULL);
    } else if (sc2_contains_i(key, "scale")) {
        object->scale = strtof(value, NULL);
    } else if (sc2_contains_i(key, "variation")) {
        object->variation = (DWORD)strtoul(value, NULL, 10);
    } else if (sc2_contains_i(key, "player") || sc2_contains_i(key, "owner")) {
        object->player = (DWORD)strtoul(value, NULL, 10);
    }
}

static void sc2_object_flag(sc2MapObject_t *object, xmlNodePtr node) {
    char index[64];
    char value[64];

    if (!object || !sc2_contains_i((char const *)node->name, "Flag"))
        return;
    if (!sc2_xml_attr(node, "Index", index, sizeof(index)) ||
        !sc2_xml_attr(node, "Value", value, sizeof(value)) ||
        !atoi(value))
        return;
    if (sc2_streqi(index, "HeightAbsolute")) object->flags |= SC2_OBJECT_HEIGHT_ABSOLUTE;
    else if (sc2_streqi(index, "HeightOffset")) object->flags |= SC2_OBJECT_HEIGHT_OFFSET;
    else if (sc2_streqi(index, "ForcePlacement")) object->flags |= SC2_OBJECT_FORCE_PLACEMENT;
}

static void sc2_parse_object_node(xmlNodePtr node) {
    sc2MapObject_t object;
    BOOL has_position = false;
    BOOL is_unit;
    BOOL is_doodad;
    char value[256];

    if (!node || node->type != XML_ELEMENT_NODE) return;
    is_unit = sc2_contains_i((char const *)node->name, "ObjectUnit");
    is_doodad = sc2_contains_i((char const *)node->name, "ObjectDoodad");
    if (!is_unit && !is_doodad) {
        for (xmlNodePtr child = node->children; child; child = child->next)
            sc2_parse_object_node(child);
        return;
    }
    memset(&object, 0, sizeof(object));
    object.scale = 1.0f;
    object.type = is_doodad ? SC2_OBJECT_DOODAD : SC2_OBJECT_UNIT;

    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
        xmlChar *text = xmlNodeListGetString(node->doc, attr->children, 1);
        if (text) {
            sc2_object_field(&object, (char const *)attr->name, (char const *)text, &has_position);
            xmlFree(text);
        }
    }
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        sc2_object_flag(&object, child);
        if (sc2_xml_content(child, value, sizeof(value)))
            sc2_object_field(&object, (char const *)child->name, value, &has_position);
    }

    if (has_position && (object.name[0] || object.model[0])) {
        if (sc2_map.num_objects < SC2_MAX_MAP_OBJECTS) {
            sc2_set_object_model(&object);
            sc2_map_position_to_playable(&object.position);
            sc2_map.objects[sc2_map.num_objects++] = object;
        }
    }
}

static BOOL sc2_camera_value(xmlNodePtr node, LPCSTR name, LPSTR buffer, DWORD size) {
    if (sc2_xml_attr(node, name, buffer, size)) return true;
    if (!strcasecmp(name, "value")) return sc2_xml_attr(node, "Value", buffer, size);
    if (!strcasecmp(name, "index")) return sc2_xml_attr(node, "Index", buffer, size);
    return false;
}

static void sc2_parse_camera_node(xmlNodePtr node) {
    char name[64] = "";
    char value[128];
    VECTOR3 target;
    DWORD priority;
    FLOAT distance, pitch, yaw, fov, znear, zfar, height_offset;

    if (!node || node->type != XML_ELEMENT_NODE)
        return;
    if (!sc2_contains_i((char const *)node->name, "ObjectCamera"))
        goto children;

    sc2_xml_attr(node, "Name", name, sizeof(name));
    if (!sc2_xml_attr(node, "CameraTarget", value, sizeof(value)) &&
        !sc2_xml_attr(node, "Position", value, sizeof(value))) {
        goto children;
    }
    if (!sc2_parse_vec3(value, &target))
        goto children;

    priority = !sc2_map.has_camera ? 1 : 0;
    if (sc2_contains_i(name, "StartGame")) priority = 2;
    if (sc2_contains_i(name, "StartGame02")) priority = 3;
    if (priority < sc2_map.camera_priority)
        goto children;

    distance = sc2_map.camera_distance;
    pitch = sc2_map.camera_pitch;
    yaw = sc2_map.camera_yaw;
    fov = sc2_map.camera_fov;
    znear = sc2_map.camera_znear;
    zfar = sc2_map.camera_zfar;
    height_offset = sc2_map.camera_height_offset;

    for (xmlNodePtr child = node->children; child; child = child->next) {
        char index[64];
        if (child->type != XML_ELEMENT_NODE || !sc2_contains_i((char const *)child->name, "CameraValue"))
            continue;
        if (!sc2_camera_value(child, "Index", index, sizeof(index)) ||
            !sc2_camera_value(child, "Value", value, sizeof(value)))
            continue;
        if (sc2_streqi(index, "Distance")) distance = strtof(value, NULL);
        else if (sc2_streqi(index, "Pitch")) pitch = strtof(value, NULL);
        else if (sc2_streqi(index, "Yaw")) yaw = strtof(value, NULL);
        else if (sc2_streqi(index, "FieldOfView")) fov = strtof(value, NULL);
        else if (sc2_streqi(index, "NearClip")) znear = strtof(value, NULL);
        else if (sc2_streqi(index, "FarClip")) zfar = strtof(value, NULL);
        else if (sc2_streqi(index, "HeightOffset")) height_offset = strtof(value, NULL);
    }

    sc2_map.has_camera = true;
    sc2_map.camera_priority = priority;
    sc2_map_position_to_playable(&target);
    sc2_map.camera_target = target;
    sc2_map.camera_distance = distance;
    sc2_map.camera_pitch = pitch;
    sc2_map.camera_yaw = yaw;
    sc2_map.camera_fov = fov;
    sc2_map.camera_znear = znear;
    sc2_map.camera_zfar = zfar;
    sc2_map.camera_height_offset = height_offset;

children:
    for (xmlNodePtr child = node->children; child; child = child->next)
        sc2_parse_camera_node(child);
}

static void sc2_parse_objects(sc2MapSource_t *source) {
    xmlDocPtr doc = sc2_read_xml(source, "Objects");
    if (!doc) doc = sc2_read_xml(source, "Objects.xml");
    if (!doc) return;
    sc2_parse_camera_node(xmlDocGetRootElement(doc));
    sc2_parse_object_node(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
}

static void sc2_catalog_add_model(sc2Catalog_t *catalog, LPCSTR id, LPCSTR path) {
    sc2CatalogModel_t *model;

    if (!catalog || !id || !*id || !path || !*path) return;
    FOR_LOOP(i, catalog->models_count) {
        if (!strcasecmp(catalog->models[i].id, id)) {
            snprintf(catalog->models[i].path, sizeof(catalog->models[i].path), "%s", path);
            sc2_normalize_slashes(catalog->models[i].path);
            return;
        }
    }
    if (catalog->models_count >= SC2_MAX_CATALOG_MODELS) return;
    model = &catalog->models[catalog->models_count++];
    snprintf(model->id, sizeof(model->id), "%s", id);
    snprintf(model->path, sizeof(model->path), "%s", path);
    sc2_normalize_slashes(model->path);
}

static void sc2_catalog_add_actor(sc2Catalog_t *catalog, LPCSTR id, LPCSTR model_id) {
    sc2CatalogActor_t *actor;

    if (!catalog || !id || !*id || !model_id || !*model_id) return;
    FOR_LOOP(i, catalog->actors_count) {
        if (!strcasecmp(catalog->actors[i].id, id)) {
            snprintf(catalog->actors[i].model, sizeof(catalog->actors[i].model), "%s", model_id);
            return;
        }
    }
    if (catalog->actors_count >= SC2_MAX_CATALOG_ACTORS) return;
    actor = &catalog->actors[catalog->actors_count++];
    snprintf(actor->id, sizeof(actor->id), "%s", id);
    snprintf(actor->model, sizeof(actor->model), "%s", model_id);
}

static void sc2_catalog_add_terrain_tex(sc2Catalog_t *catalog, LPCSTR id, LPCSTR diffuse, LPCSTR normal) {
    sc2CatalogTerrainTex_t *tex;

    if (!catalog || !id || !*id || !diffuse || !*diffuse) return;
    FOR_LOOP(i, catalog->terrain_tex_count) {
        if (!strcasecmp(catalog->terrain_tex[i].id, id)) {
            snprintf(catalog->terrain_tex[i].diffuse, sizeof(catalog->terrain_tex[i].diffuse), "%s", diffuse);
            snprintf(catalog->terrain_tex[i].normal, sizeof(catalog->terrain_tex[i].normal), "%s", normal ? normal : "");
            sc2_normalize_slashes(catalog->terrain_tex[i].diffuse);
            sc2_normalize_slashes(catalog->terrain_tex[i].normal);
            return;
        }
    }
    if (catalog->terrain_tex_count >= SC2_MAX_CATALOG_TERRAIN_TEX) return;
    tex = &catalog->terrain_tex[catalog->terrain_tex_count++];
    snprintf(tex->id, sizeof(tex->id), "%s", id);
    snprintf(tex->diffuse, sizeof(tex->diffuse), "%s", diffuse);
    snprintf(tex->normal, sizeof(tex->normal), "%s", normal ? normal : "");
    sc2_normalize_slashes(tex->diffuse);
    sc2_normalize_slashes(tex->normal);
}

static void sc2_catalog_add_cliff(sc2Catalog_t *catalog, LPCSTR id, LPCSTR mesh) {
    sc2CatalogCliff_t *cliff;

    if (!catalog || !id || !*id || !mesh || !*mesh) return;
    FOR_LOOP(i, catalog->cliffs_count) {
        if (!strcasecmp(catalog->cliffs[i].id, id)) {
            snprintf(catalog->cliffs[i].mesh, sizeof(catalog->cliffs[i].mesh), "%s", mesh);
            return;
        }
    }
    if (catalog->cliffs_count >= SC2_MAX_CATALOG_CLIFFS) return;
    cliff = &catalog->cliffs[catalog->cliffs_count++];
    snprintf(cliff->id, sizeof(cliff->id), "%s", id);
    snprintf(cliff->mesh, sizeof(cliff->mesh), "%s", mesh);
}

static LPCSTR sc2_catalog_model_path(sc2Catalog_t const *catalog, LPCSTR id) {
    if (!catalog || !id || !*id) return NULL;
    FOR_LOOP(i, catalog->models_count) {
        if (!strcasecmp(catalog->models[i].id, id)) return catalog->models[i].path;
    }
    return NULL;
}

static LPCSTR sc2_catalog_cliff_mesh(sc2Catalog_t const *catalog, LPCSTR id) {
    if (!catalog || !id || !*id) return NULL;
    FOR_LOOP(i, catalog->cliffs_count) {
        if (!strcasecmp(catalog->cliffs[i].id, id)) return catalog->cliffs[i].mesh;
    }
    return NULL;
}

static BOOL sc2_cliff_mesh_exists(LPCSTR mesh) {
    char path[256];

    if (!mesh || !*mesh) return false;
    snprintf(path, sizeof(path), "Assets\\Cliffs\\%s\\%s_ABBB_00.m3", mesh, mesh);
    return sc2_file_exists(path);
}

static LPCSTR sc2_cliff_mesh_fallback(LPCSTR name) {
    DWORD len;

    if (!name || !*name) return NULL;
    if (sc2_cliff_mesh_exists(name)) return name;
    len = (DWORD)strlen(name);
    if (len >= 6 && !strcasecmp(name + len - 6, "Cliff0") && sc2_cliff_mesh_exists("CliffNatural0"))
        return "CliffNatural0";
    if (len >= 6 && !strcasecmp(name + len - 6, "Cliff1") && sc2_cliff_mesh_exists("CliffMade0"))
        return "CliffMade0";
    return NULL;
}

static LPCSTR sc2_catalog_actor_model(sc2Catalog_t const *catalog, LPCSTR id) {
    if (!catalog || !id || !*id) return NULL;
    FOR_LOOP(i, catalog->actors_count) {
        if (!strcasecmp(catalog->actors[i].id, id)) return catalog->actors[i].model;
    }
    return NULL;
}

static LPCSTR sc2_catalog_terrain_diffuse(sc2Catalog_t const *catalog, LPCSTR id, LPCSTR *normal) {
    char key[64];

    if (normal) *normal = NULL;
    if (!catalog || !id || !*id) return NULL;
    snprintf(key, sizeof(key), "%s", id);
    if (sc2_has_extension_i(key, ".dds")) {
        char *ext = strrchr(key, '.');
        if (ext) *ext = '\0';
    }
    FOR_LOOP(i, catalog->terrain_tex_count) {
        if (!strcasecmp(catalog->terrain_tex[i].id, key)) {
            if (normal) *normal = catalog->terrain_tex[i].normal;
            return catalog->terrain_tex[i].diffuse;
        }
    }
    return NULL;
}

static BOOL sc2_terrain_texture_path_from_tileset(LPCSTR id,
                                                  LPSTR diffuse,
                                                  DWORD diffuse_size,
                                                  LPSTR normal,
                                                  DWORD normal_size) {
    char suffix[128];
    char path[256];
    DWORD tile_len;

    if (!id || !*id || !sc2_map.tile_set[0] || !diffuse || !normal)
        return false;
    tile_len = (DWORD)strlen(sc2_map.tile_set);
    if (strncasecmp(id, sc2_map.tile_set, tile_len) || !id[tile_len])
        return false;
    sc2_camel_to_underscore(id + tile_len, suffix, sizeof(suffix));
    snprintf(path, sizeof(path), "Assets\\Textures\\%s_%s.dds", sc2_map.tile_set, suffix);
    if (!sc2_file_exists(path))
        return false;
    snprintf(diffuse, diffuse_size, "%s", path);
    snprintf(path, sizeof(path), "Assets\\Textures\\%s_%sNormal.dds", sc2_map.tile_set, suffix);
    snprintf(normal, normal_size, "%s", sc2_file_exists(path) ? path : diffuse);
    return true;
}

static void sc2_parse_model_catalog_file(sc2Catalog_t *catalog, LPCSTR root_name) {
    xmlDocPtr doc = sc2_read_catalog_xml(root_name, "GameData\\ModelData.xml");
    xmlNodePtr root;

    if (!doc) return;
    root = xmlDocGetRootElement(doc);
    for (xmlNodePtr node = root ? root->children : NULL; node; node = node->next) {
        char id[64];
        char path[256] = "";

        if (node->type != XML_ELEMENT_NODE || !sc2_contains_i((char const *)node->name, "CModel"))
            continue;
        if (!sc2_xml_attr(node, "id", id, sizeof(id))) continue;
        for (xmlNodePtr child = node->children; child; child = child->next) {
            if (child->type == XML_ELEMENT_NODE && sc2_streqi((char const *)child->name, "Model")) {
                sc2_xml_attr(child, "value", path, sizeof(path));
                break;
            }
        }
        sc2_catalog_add_model(catalog, id, path);
    }
    xmlFreeDoc(doc);
}

static void sc2_parse_actor_catalog_file(sc2Catalog_t *catalog, LPCSTR root_name) {
    xmlDocPtr doc = sc2_read_catalog_xml(root_name, "GameData\\ActorData.xml");
    xmlNodePtr root;

    if (!doc) return;
    root = xmlDocGetRootElement(doc);
    for (xmlNodePtr node = root ? root->children : NULL; node; node = node->next) {
        char id[64];
        char unit_name[64] = "";
        char model_id[64] = "";
        BOOL actor_node;

        if (node->type != XML_ELEMENT_NODE) continue;
        actor_node = sc2_contains_i((char const *)node->name, "CActorUnit") ||
                     sc2_contains_i((char const *)node->name, "CActorDoodad");
        if (!actor_node || !sc2_xml_attr(node, "id", id, sizeof(id))) continue;
        sc2_xml_attr(node, "unitName", unit_name, sizeof(unit_name));
        for (xmlNodePtr child = node->children; child; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (sc2_streqi((char const *)child->name, "Model")) {
                sc2_xml_attr(child, "value", model_id, sizeof(model_id));
                break;
            }
        }
        if (!model_id[0]) snprintf(model_id, sizeof(model_id), "%s", id);
        sc2_catalog_add_actor(catalog, id, model_id);
        if (unit_name[0]) sc2_catalog_add_actor(catalog, unit_name, model_id);
    }
    xmlFreeDoc(doc);
}

static void sc2_parse_terrain_tex_catalog_file(sc2Catalog_t *catalog, LPCSTR root_name) {
    xmlDocPtr doc = sc2_read_catalog_xml(root_name, "GameData\\TerrainTexData.xml");
    xmlNodePtr root;

    if (!doc) return;
    root = xmlDocGetRootElement(doc);
    for (xmlNodePtr node = root ? root->children : NULL; node; node = node->next) {
        char id[64];
        char diffuse[256] = "";
        char normal[256] = "";

        if (node->type != XML_ELEMENT_NODE || !sc2_contains_i((char const *)node->name, "CTerrainTex"))
            continue;
        if (!sc2_xml_attr(node, "id", id, sizeof(id))) continue;
        for (xmlNodePtr child = node->children; child; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (sc2_streqi((char const *)child->name, "Texture"))
                sc2_xml_attr(child, "value", diffuse, sizeof(diffuse));
            else if (sc2_streqi((char const *)child->name, "Normalmap"))
                sc2_xml_attr(child, "value", normal, sizeof(normal));
        }
        sc2_catalog_add_terrain_tex(catalog, id, diffuse, normal);
    }
    xmlFreeDoc(doc);
}

static void sc2_parse_cliff_catalog_file(sc2Catalog_t *catalog, LPCSTR root_name) {
    xmlDocPtr doc = sc2_read_catalog_xml(root_name, "GameData\\CliffData.xml");
    xmlNodePtr root;

    if (!doc) return;
    root = xmlDocGetRootElement(doc);
    for (xmlNodePtr node = root ? root->children : NULL; node; node = node->next) {
        char id[64];
        char mesh[64] = "";

        if (node->type != XML_ELEMENT_NODE || !sc2_contains_i((char const *)node->name, "CCliff"))
            continue;
        if (!sc2_xml_attr(node, "id", id, sizeof(id))) continue;
        for (xmlNodePtr child = node->children; child; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (sc2_streqi((char const *)child->name, "CliffMesh")) {
                sc2_xml_attr(child, "value", mesh, sizeof(mesh));
                break;
            }
        }
        sc2_catalog_add_cliff(catalog, id, mesh);
    }
    xmlFreeDoc(doc);
}

static void sc2_parse_catalogs(sc2Catalog_t *catalog) {
    static LPCSTR const roots[] = {
        "Mods/Core.SC2Mod/Base.SC2Data",
        "Mods/Liberty.SC2Mod/Base.SC2Data",
        "Mods/LibertyMulti.SC2Mod/Base.SC2Data",
        "Campaigns/LibertyStory.SC2Campaign/Base.SC2Data",
        "Campaigns/Liberty.SC2Campaign/Base.SC2Data",
        NULL,
    };

    sc2_parse_model_catalog_file(catalog, "");
    sc2_parse_actor_catalog_file(catalog, "");
    sc2_parse_terrain_tex_catalog_file(catalog, "");
    sc2_parse_cliff_catalog_file(catalog, "");
    for (DWORD i = 0; roots[i]; i++) {
        sc2_parse_model_catalog_file(catalog, roots[i]);
        sc2_parse_actor_catalog_file(catalog, roots[i]);
        sc2_parse_terrain_tex_catalog_file(catalog, roots[i]);
        sc2_parse_cliff_catalog_file(catalog, roots[i]);
    }
}

static void sc2_resolve_terrain_textures(sc2Catalog_t const *catalog) {
    FOR_LOOP(i, sc2_map.num_terrain_textures) {
        char stem[128];
        char diffuse_path[256];
        char normal_path[256];
        LPCSTR normal;
        LPCSTR diffuse = sc2_catalog_terrain_diffuse(catalog, sc2_map.terrain_textures[i].diffuse, &normal);

        if (diffuse) {
            snprintf(sc2_map.terrain_textures[i].diffuse, sizeof(sc2_map.terrain_textures[i].diffuse), "%s", diffuse);
            snprintf(sc2_map.terrain_textures[i].normal, sizeof(sc2_map.terrain_textures[i].normal), "%s",
                     normal && *normal ? normal : diffuse);
            continue;
        }
        if (sc2_path_has_dir(sc2_map.terrain_textures[i].diffuse))
            continue;
        if (sc2_terrain_texture_path_from_tileset(sc2_map.terrain_textures[i].diffuse,
                                                  sc2_map.terrain_textures[i].diffuse,
                                                  sizeof(sc2_map.terrain_textures[i].diffuse),
                                                  sc2_map.terrain_textures[i].normal,
                                                  sizeof(sc2_map.terrain_textures[i].normal))) {
            continue;
        }
        sc2_camel_to_underscore(sc2_map.terrain_textures[i].diffuse, stem, sizeof(stem));
        snprintf(diffuse_path, sizeof(diffuse_path), "Assets\\Textures\\%s.dds", stem);
        snprintf(normal_path, sizeof(normal_path), "Assets\\Textures\\%sNormal.dds", stem);
        if (sc2_file_exists(diffuse_path)) {
            snprintf(sc2_map.terrain_textures[i].diffuse, sizeof(sc2_map.terrain_textures[i].diffuse), "%s", diffuse_path);
            snprintf(sc2_map.terrain_textures[i].normal, sizeof(sc2_map.terrain_textures[i].normal), "%s",
                     sc2_file_exists(normal_path) ? normal_path : diffuse_path);
        }
    }
}

static void sc2_resolve_cliff_sets(sc2Catalog_t const *catalog) {
    FOR_LOOP(i, sc2_map.num_cliff_sets) {
        LPCSTR mesh = sc2_catalog_cliff_mesh(catalog, sc2_map.cliff_sets[i].name);
        if (!mesh || !*mesh)
            mesh = sc2_cliff_mesh_fallback(sc2_map.cliff_sets[i].name);
        if (mesh && *mesh) {
            snprintf(sc2_map.cliff_sets[i].mesh, sizeof(sc2_map.cliff_sets[i].mesh), "%s", mesh);
        } else if (sc2_map.cliff_sets[i].name[0]) {
            snprintf(sc2_map.cliff_sets[i].mesh,
                     sizeof(sc2_map.cliff_sets[i].mesh),
                     "%s",
                     sc2_map.cliff_sets[i].name);
        }
    }
}

static BOOL sc2_try_object_model_path(sc2MapObject_t *object, LPCSTR prefix) {
    char path[256];

    if (!object || !prefix || !*prefix || !object->name[0]) return false;
    snprintf(path, sizeof(path), "%s\\%s\\%s.m3", prefix, object->name, object->name);
    if (sc2_file_exists(path)) {
        snprintf(object->model, sizeof(object->model), "%s", path);
        return true;
    }
    snprintf(path, sizeof(path), "%s\\%s\\%s_%02u.m3", prefix, object->name, object->name, object->variation);
    if (sc2_file_exists(path)) {
        snprintf(object->model, sizeof(object->model), "%s", path);
        return true;
    }
    snprintf(path, sizeof(path), "%s\\%s\\%s_00.m3", prefix, object->name, object->name);
    if (sc2_file_exists(path)) {
        snprintf(object->model, sizeof(object->model), "%s", path);
        return true;
    }
    return false;
}

static void sc2_resolve_object_model_candidates(sc2MapObject_t *object) {
    static struct {
        LPCSTR name;
        LPCSTR model;
    } const aliases[] = {
        { "Civilian", "Assets\\Units\\Terran\\ColonistMale\\ColonistMale.m3" },
        { "CivilianFemale", "Assets\\Units\\Terran\\ColonistFemale\\ColonistFemale_00.m3" },
        { "LogisticsHeadquarters", "Assets\\Doodads\\TRaynor01RadioTower\\TRaynor01RadioTower.m3" },
        { NULL, NULL },
    };
    static LPCSTR const prefixes[] = {
        "Assets\\Units\\Critters",
        "Assets\\Buildings\\Terran",
        "Assets\\Units\\Terran",
        "Assets\\Doodads",
        "Assets\\Effects\\Terran",
        NULL,
    };

    if (!object || !object->name[0]) return;
    for (DWORD i = 0; aliases[i].name; i++) {
        if (!strcasecmp(object->name, aliases[i].name) && sc2_file_exists(aliases[i].model)) {
            snprintf(object->model, sizeof(object->model), "%s", aliases[i].model);
            return;
        }
    }
    for (DWORD i = 0; prefixes[i]; i++) {
        if (sc2_try_object_model_path(object, prefixes[i])) return;
    }
}

static void sc2_resolve_object_models(sc2Catalog_t const *catalog) {
    sc2ResolvedObjectModel_t resolved[SC2_MAX_MAP_OBJECTS];
    DWORD resolved_count = 0;

    memset(resolved, 0, sizeof(resolved));
    FOR_LOOP(i, sc2_map.num_objects) {
        sc2MapObject_t *object = &sc2_map.objects[i];
        LPCSTR model_id;
        LPCSTR path;

        if (!object->name[0]) continue;
        FOR_LOOP(j, resolved_count) {
            if (!strcasecmp(resolved[j].name, object->name)) {
                snprintf(object->model, sizeof(object->model), "%s", resolved[j].model);
                goto next_object;
            }
        }
        model_id = sc2_catalog_actor_model(catalog, object->name);
        if (!model_id) model_id = object->name;
        path = sc2_catalog_model_path(catalog, model_id);
        if (path && *path) {
            snprintf(object->model, sizeof(object->model), "%s", path);
        } else {
            sc2_resolve_object_model_candidates(object);
        }
        if (resolved_count < SC2_MAX_MAP_OBJECTS) {
            snprintf(resolved[resolved_count].name, sizeof(resolved[resolved_count].name), "%s", object->name);
            snprintf(resolved[resolved_count].model, sizeof(resolved[resolved_count].model), "%s", object->model);
            resolved_count++;
        }
next_object:
        ;
    }
}

static void sc2_resolve_catalogs(void) {
    sc2Catalog_t *catalog = sc2_alloc(sizeof(*catalog));

    if (!catalog) return;
    memset(catalog, 0, sizeof(*catalog));
    sc2_parse_catalogs(catalog);
    sc2_resolve_terrain_textures(catalog);
    sc2_resolve_cliff_sets(catalog);
    sc2_resolve_object_models(catalog);
    sc2_free(catalog);
}

static DWORD sc2_read_le32(BYTE const *p) {
    return (DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}

static USHORT sc2_read_le16(BYTE const *p) {
    return (USHORT)((USHORT)p[0] | ((USHORT)p[1] << 8));
}

static LONG sc2_read_i32(BYTE const *p) {
    return (LONG)sc2_read_le32(p);
}

static BOOL sc2_mapinfo_magic(LPBYTE data, DWORD size) {
    return data && size >= SC2_MAPINFO_MIN_SIZE &&
        (!memcmp(data, "IpaM", 4) || !memcmp(data, "MapI", 4));
}

static void sc2_set_playable_bounds(LONG left, LONG bottom, LONG right, LONG top) {
    if (right <= left || top <= bottom)
        return;
    sc2_map.has_playable_bounds = true;
    sc2_map.playable_left = left;
    sc2_map.playable_bottom = bottom;
    sc2_map.playable_right = right;
    sc2_map.playable_top = top;
    sc2_map.width = (DWORD)(right - left);
    sc2_map.height = (DWORD)(top - bottom);
}

static void sc2_set_full_size(DWORD width, DWORD height) {
    sc2_map.full_width = width;
    sc2_map.full_height = height;
    if (!sc2_map.has_playable_bounds) {
        sc2_map.width = width;
        sc2_map.height = height;
    }
}

static void sc2_map_position_to_playable(LPVECTOR3 position) {
    if (!position || !sc2_map.has_playable_bounds)
        return;
    position->x -= (FLOAT)sc2_map.playable_left;
    position->y -= (FLOAT)sc2_map.playable_bottom;
}

static BOOL sc2_playable_crop(DWORD src_w,
                              DWORD src_h,
                              BOOL vertices,
                              DWORD *left,
                              DWORD *bottom,
                              DWORD *width,
                              DWORD *height) {
    DWORD right;
    DWORD top;

    if (!left || !bottom || !width || !height || !src_w || !src_h)
        return false;
    *left = 0;
    *bottom = 0;
    *width = src_w;
    *height = src_h;
    if (!sc2_map.has_playable_bounds)
        return true;
    if (sc2_map.playable_left < 0 || sc2_map.playable_bottom < 0)
        return false;
    *left = (DWORD)sc2_map.playable_left;
    *bottom = (DWORD)sc2_map.playable_bottom;
    right = (DWORD)sc2_map.playable_right + (vertices ? 1u : 0u);
    top = (DWORD)sc2_map.playable_top + (vertices ? 1u : 0u);
    if (*left >= right || *bottom >= top || right > src_w || top > src_h)
        return false;
    *width = right - *left;
    *height = top - *bottom;
    return true;
}

static void sc2_parse_height_map(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3HeightMap", &size);
    DWORD w, h, crop_x, crop_y, crop_w, crop_h;
    FLOAT scale, bias;

    if (!data || size < SC2_HMAP_HEADER_SIZE || memcmp(data, "HMAP", 4) != 0) {
        sc2_free_file(data);
        return;
    }
    w = sc2_read_le32(data + 8);
    h = sc2_read_le32(data + 12);
    if (!w || !h || w > 4096 || h > 4096 ||
        size < SC2_HMAP_HEADER_SIZE + w * h * SC2_HMAP_CHUNK_SIZE ||
        !sc2_playable_crop(w, h, true, &crop_x, &crop_y, &crop_w, &crop_h)) {
        sc2_free_file(data);
        return;
    }
    sc2_map.height_map = sc2_alloc(crop_w * crop_h * sizeof(*sc2_map.height_map));
    sc2_map.height_adjust_map = sc2_alloc(crop_w * crop_h * sizeof(*sc2_map.height_adjust_map));
    if (!sc2_map.height_map || !sc2_map.height_adjust_map) {
        SAFE_DELETE(sc2_map.height_map, sc2_free);
        SAFE_DELETE(sc2_map.height_adjust_map, sc2_free);
        sc2_free_file(data);
        return;
    }
    scale = sc2_map.height_quantize_scale ? sc2_map.height_quantize_scale : 1.0f;
    bias = sc2_map.height_quantize_bias;
    FOR_LOOP(y, crop_h) {
        FOR_LOOP(x, crop_w) {
            DWORD src = (crop_x + x) + (crop_y + y) * w;
            LPBYTE chunk = data + SC2_HMAP_HEADER_SIZE + src * SC2_HMAP_CHUNK_SIZE;
            USHORT height_adjustment = sc2_read_le16(chunk);
            USHORT height_base = sc2_read_le16(chunk + 2);
            FLOAT adjust = height_adjustment * scale;
            // HACK: zero map height and use adjust map for all height adjustments, so that cliffs can be reconstructed more accurately
            sc2_map.height_adjust_map[x + y * crop_w] = 0;//adjust;
            sc2_map.height_map[x + y * crop_w] =
                0;//height_base * scale - bias - sc2_map.standard_height - 1.0f + adjust;
        }
    }
    sc2_map.height_map_width = crop_w;
    sc2_map.height_map_height = crop_h;
    if (!sc2_map.width && crop_w > 1) sc2_map.width = crop_w - 1;
    if (!sc2_map.height && crop_h > 1) sc2_map.height = crop_h - 1;
    sc2_free_file(data);
}

static void sc2_parse_sync_height_map(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3SyncHeightMap", &size);
    DWORD w, h, crop_x, crop_y, crop_w, crop_h;

    if (!data || size < SC2_SMAP_HEADER_SIZE || memcmp(data, "SMAP", 4) != 0) {
        sc2_free_file(data);
        return;
    }
    w = sc2_read_le32(data + 8);
    h = sc2_read_le32(data + 12);
    if (!w || !h || w > 4096 || h > 4096 ||
        size < SC2_SMAP_HEADER_SIZE + w * h * SC2_SMAP_CHUNK_SIZE ||
        !sc2_playable_crop(w, h, true, &crop_x, &crop_y, &crop_w, &crop_h)) {
        sc2_free_file(data);
        return;
    }
    if (!sc2_map.height_map ||
        sc2_map.height_map_width != crop_w ||
        sc2_map.height_map_height != crop_h) {
        sc2_free_file(data);
        return;
    }
    FOR_LOOP(y, crop_h) {
        FOR_LOOP(x, crop_w) {
            DWORD src = (crop_x + x) + (crop_y + y) * w;
            LPBYTE chunk = data + SC2_SMAP_HEADER_SIZE + src * SC2_SMAP_CHUNK_SIZE;
            SHORT delta = (SHORT)sc2_read_le16(chunk);
            FLOAT sync_adjust = (FLOAT)delta / 256.0f;
            sc2_map.height_map[x + y * crop_w] += sync_adjust;
            if (sc2_map.height_adjust_map)
                sc2_map.height_adjust_map[x + y * crop_w] += sync_adjust;
        }
    }
    sc2_free_file(data);
}

static void sc2_parse_cell_flags(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3CellFlags", &size);
    if (data && size >= SC2_TERRAIN_HEADER_SIZE && memcmp(data, "LFCT", 4) == 0) {
        DWORD w = sc2_read_le32(data + 24);
        DWORD h = sc2_read_le32(data + 28);
        DWORD crop_x, crop_y, crop_w, crop_h;
        if (w > 0 && h > 0 && w < 4096 && h < 4096 &&
            size >= SC2_TERRAIN_HEADER_SIZE + w * h &&
            sc2_playable_crop(w, h, false, &crop_x, &crop_y, &crop_w, &crop_h)) {
            if (!sc2_map.has_playable_bounds) {
                sc2_map.width = crop_w;
                sc2_map.height = crop_h;
            }
            if (crop_w && crop_h) {
                sc2_map.cell_flags = sc2_alloc(crop_w * crop_h);
                if (sc2_map.cell_flags) {
                    FOR_LOOP(y, crop_h) {
                        memcpy(sc2_map.cell_flags + y * crop_w,
                               data + SC2_TERRAIN_HEADER_SIZE + crop_x + (crop_y + y) * w,
                               crop_w);
                    }
                    sc2_map.cell_flags_width = crop_w;
                    sc2_map.cell_flags_height = crop_h;
                }
            }
        }
    }
    sc2_free_file(data);
}

static void sc2_parse_sync_cliff_level(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3SyncCliffLevel", &size);
    DWORD w, h, crop_x, crop_y, crop_w, crop_h;

    if (!data || size < SC2_TERRAIN_HEADER_SIZE || memcmp(data, "CLIF", 4) != 0) {
        sc2_free_file(data);
        return;
    }
    w = sc2_read_le32(data + 8);
    h = sc2_read_le32(data + 12);
    if (!w || !h || w > 4096 || h > 4096 ||
        size < SC2_TERRAIN_HEADER_SIZE + w * h * sizeof(USHORT) ||
        !sc2_playable_crop(w, h, false, &crop_x, &crop_y, &crop_w, &crop_h)) {
        sc2_free_file(data);
        return;
    }
    sc2_map.cliff_levels = sc2_alloc(crop_w * crop_h * sizeof(USHORT));
    if (sc2_map.cliff_levels) {
        FOR_LOOP(y, crop_h) {
            memcpy(sc2_map.cliff_levels + y * crop_w,
                   data + SC2_TERRAIN_HEADER_SIZE + ((crop_y + y) * w + crop_x) * sizeof(USHORT),
                   crop_w * sizeof(USHORT));
        }
        sc2_map.cliff_level_width = crop_w;
        sc2_map.cliff_level_height = crop_h;
    }
    sc2_free_file(data);
}

static BYTE sc2_texture_mask_nibble(BYTE byte, DWORD pixel) {
    return pixel & 1 ? byte & 0x0F : byte >> 4;
}

static void sc2_decode_texture_mask_block(LPBYTE out, LPBYTE src, DWORD width, DWORD height, DWORD blocks_x, DWORD block) {
    DWORD bx = block % blocks_x;
    DWORD by = block / blocks_x;

    FOR_LOOP(y, 64) {
        FOR_LOOP(x, 64) {
            DWORD px = bx * 64 + x;
            DWORD py = by * 64 + y;
            if (px >= width || py >= height)
                continue;
            out[px + py * width] = sc2_texture_mask_nibble(src[y * 32 + x / 2], x);
        }
    }
}

static void sc2_parse_texture_masks(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3TextureMasks", &size);
    DWORD w, h;
    DWORD blocks_x, blocks_y;
    DWORD packed_layer_size, block_layer_size;
    DWORD layer_stride, layers;

    if (!data || size < 64 || memcmp(data, "MASK", 4) != 0) {
        sc2_free_file(data);
        return;
    }
    w = sc2_read_le32(data + 12);
    h = sc2_read_le32(data + 16);
    if (!w || !h || w > 4096 || h > 4096) {
        sc2_free_file(data);
        return;
    }
    packed_layer_size = (w * h) / 2;
    blocks_x = (w + 63) / 64;
    blocks_y = (h + 63) / 64;
    block_layer_size = blocks_x * blocks_y * 64 * 32;
    layer_stride = block_layer_size;
    if (!layer_stride || size < 64 + layer_stride || (size - 64) % layer_stride != 0) {
        layer_stride = packed_layer_size;
    }
    if (!layer_stride) {
        sc2_free_file(data);
        return;
    }
    layers = MIN(SC2_MAX_TERRAIN_TEXTURES, (size - 64) / layer_stride);
    sc2_map.texture_mask_width = w;
    sc2_map.texture_mask_height = h;
    sc2_map.num_texture_masks = layers;

    FOR_LOOP(layer, layers) {
        LPBYTE out = sc2_alloc(w * h);
        LPBYTE src = data + 64 + layer * layer_stride;
        if (!out) {
            break;
        }
        memset(out, 0, w * h);
        if (layer_stride == block_layer_size && block_layer_size > 0) {
            FOR_LOOP(block, blocks_x * blocks_y) {
                sc2_decode_texture_mask_block(out, src + block * 64 * 32, w, h, blocks_x, block);
            }
        } else {
            FOR_LOOP(i, w * h) {
                out[i] = sc2_texture_mask_nibble(src[i / 2], i);
            }
        }
        sc2_map.texture_masks[layer] = out;
    }
    sc2_free_file(data);
}

static void sc2_add_default_object(LPCSTR name, LPCSTR model, FLOAT x, FLOAT y, FLOAT angle, DWORD player) {
    sc2MapObject_t *object;
    if (sc2_map.num_objects >= SC2_MAX_MAP_OBJECTS) return;
    object = &sc2_map.objects[sc2_map.num_objects++];
    memset(object, 0, sizeof(*object));
    object->type = SC2_OBJECT_UNIT;
    snprintf(object->name, sizeof(object->name), "%s", name);
    snprintf(object->model, sizeof(object->model), "%s", model);
    object->position = (VECTOR3){ x, y, 0.0f };
    object->angle = angle;
    object->scale = 1.0f;
    object->player = player;
}

static void sc2_generate_default_map(void) {
    if (!sc2_map.width) sc2_map.width = SC2_DEFAULT_MAP_WIDTH;
    if (!sc2_map.height) sc2_map.height = SC2_DEFAULT_MAP_HEIGHT;
    if (!sc2_map.map_name[0]) snprintf(sc2_map.map_name, sizeof(sc2_map.map_name), "SC2 Terran Test");
    if (sc2_map.num_terrain_textures == 0) {
        sc2_add_terrain_texture("Assets\\Textures\\Terrain\\BelShir\\BelShirGrass_Diffuse.dds");
        sc2_add_terrain_texture("Assets\\Textures\\Terrain\\BelShir\\BelShirDirt_Diffuse.dds");
    }
    if (sc2_map.num_objects > 0) return;
    sc2_map.generated = true;
    sc2_add_default_object("CommandCenter", "Assets\\Units\\Terran\\CommandCenter\\CommandCenter.m3", 48, 48, 0, 1);
    sc2_add_default_object("SCV", "Assets\\Units\\Terran\\SCV\\SCV.m3", 42, 44, 0, 1);
    sc2_add_default_object("SCV", "Assets\\Units\\Terran\\SCV\\SCV.m3", 45, 42, 0, 1);
    sc2_add_default_object("Marine", "Assets\\Units\\Terran\\Marine\\Marine.m3", 55, 51, 0, 1);
    sc2_add_default_object("Marine", "Assets\\Units\\Terran\\Marine\\Marine.m3", 57, 53, 0, 1);
}

void SC2_MapSetHost(sc2MapHost_t const *host) {
    memset(&sc2_host, 0, sizeof(sc2_host));
    if (host) sc2_host = *host;
}

BOOL SC2_MapLoad(LPCSTR mapFilename) {
    sc2MapSource_t source;
    sc2_map_clear();
    sc2_map.cell_size = SC2_CELL_SIZE;
    sc2_set_default_camera();
    if (!sc2_source_open(&source, mapFilename)) {
        sc2_generate_default_map();
        return true;
    }
    sc2_parse_mapinfo(&source);
    sc2_parse_terrain(&source);
    sc2_parse_height_map(&source);
    sc2_parse_sync_height_map(&source);
    sc2_parse_cell_flags(&source);
    sc2_parse_sync_cliff_level(&source);
    sc2_parse_texture_masks(&source);
    sc2_parse_objects(&source);
    sc2_source_close(&source);
    sc2_resolve_catalogs();
    sc2_generate_default_map();
    sc2_map.origin.x = 0.0f;
    sc2_map.origin.y = 0.0f;
    fprintf(stderr, "SC2_MapLoad: %s %ux%u objects=%u%s\n",
            sc2_map.map_name, (unsigned)sc2_map.width, (unsigned)sc2_map.height,
            (unsigned)sc2_map.num_objects, sc2_map.generated ? " generated" : "");
    return true;
}

void SC2_MapShutdown(void) {
    sc2_map_clear();
}

sc2Map_t *SC2_MapCurrent(void) {
    return &sc2_map;
}

FLOAT SC2_MapHeightAtPoint(FLOAT x, FLOAT y) {
    VECTOR2 n;
    FLOAT fx, fy, tx, ty;
    DWORD x0, y0, x1, y1;
    FLOAT h00, h10, h01, h11, h0, h1;

    if (!sc2_map.height_map || !sc2_map.height_map_width || !sc2_map.height_map_height) {
        return 0.0f;
    }
    n = SC2_MapNormalizedPosition(x, y);
    fx = MIN(MAX(n.x, 0.0f), 1.0f) * (FLOAT)(sc2_map.height_map_width - 1);
    fy = MIN(MAX(n.y, 0.0f), 1.0f) * (FLOAT)(sc2_map.height_map_height - 1);
    x0 = (DWORD)floorf(fx);
    y0 = (DWORD)floorf(fy);
    x1 = MIN(sc2_map.height_map_width - 1, x0 + 1);
    y1 = MIN(sc2_map.height_map_height - 1, y0 + 1);
    tx = fx - (FLOAT)x0;
    ty = fy - (FLOAT)y0;
    h00 = sc2_map.height_map[x0 + y0 * sc2_map.height_map_width];
    h10 = sc2_map.height_map[x1 + y0 * sc2_map.height_map_width];
    h01 = sc2_map.height_map[x0 + y1 * sc2_map.height_map_width];
    h11 = sc2_map.height_map[x1 + y1 * sc2_map.height_map_width];
    h0 = h00 + (h10 - h00) * tx;
    h1 = h01 + (h11 - h01) * tx;
    return h0 + (h1 - h0) * ty;
}

BOX2 SC2_MapBounds(void) {
    return (BOX2){
        .min = sc2_map.origin,
        .max = {
            sc2_map.origin.x + sc2_map.width * sc2_map.cell_size,
            sc2_map.origin.y + sc2_map.height * sc2_map.cell_size,
        },
    };
}

VECTOR2 SC2_MapNormalizedPosition(FLOAT x, FLOAT y) {
    BOX2 bounds = SC2_MapBounds();
    return (VECTOR2){
        (x - bounds.min.x) / MAX(1.0f, bounds.max.x - bounds.min.x),
        (y - bounds.min.y) / MAX(1.0f, bounds.max.y - bounds.min.y),
    };
}

VECTOR2 SC2_MapDenormalizedPosition(FLOAT x, FLOAT y) {
    BOX2 bounds = SC2_MapBounds();
    return (VECTOR2){
        bounds.min.x + x * (bounds.max.x - bounds.min.x),
        bounds.min.y + y * (bounds.max.y - bounds.min.y),
    };
}

DWORD SC2_MapObjectClassId(sc2MapObject_t const *object) {
    return object && object->name[0] ? sc2_hash32(object->name) : 0;
}
