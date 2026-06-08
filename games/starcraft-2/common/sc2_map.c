#include "sc2_map.h"
#include "sc2_utils.h"
#include "common/mpq.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stddef.h>

#define SC2_MAX_CATALOG_MODELS       8192
#define SC2_MAX_CATALOG_ACTORS       8192
#define SC2_MAX_CATALOG_TERRAIN_TEX  512
#define SC2_MAX_CATALOG_CLIFFS       256
#define SC2_ARRAY_LEN(x)             ((DWORD)(sizeof(x) / sizeof((x)[0])))
#define SC2_XML_STRING_FIELD(name, field) { name, offsetof(sc2MapObject_t, field), SC2_XML_FIELD_STRING, sizeof(((sc2MapObject_t *)0)->field) }
#define SC2_XML_FIELD(name, field, type)  { name, offsetof(sc2MapObject_t, field), type, 0 }

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

typedef enum {
    SC2_XML_FIELD_DWORD,
    SC2_XML_FIELD_FLOAT,
    SC2_XML_FIELD_STRING,
    SC2_XML_FIELD_VEC3,
} sc2XmlFieldType_t;

typedef struct {
    LPCSTR            name;
    size_t            offset;
    sc2XmlFieldType_t type;
    DWORD             size;
} sc2XmlField_t;

typedef struct {
    LPCSTR name;
    DWORD  flag;
} sc2XmlFlag_t;

static sc2MapHost_t sc2_host;
static sc2Map_t     sc2_map;

static BOOL sc2_mapinfo_fourcc(sc2MapInfo_t const *mapInfo);
static void sc2_set_size(DWORD width, DWORD height);

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

static BOOL sc2_file_exists(LPCSTR path) {
    DWORD size = 0;
    HANDLE data;

    if (!path || !*path) return false;
    data = sc2_read_file(path, &size);
    if (data) sc2_free_file(data);
    return data && size > 0;
}

static void sc2_map_clear(void) {
    SAFE_DELETE(sc2_map.MapInfo, sc2_free);
    SAFE_DELETE(sc2_map.t3CellFlags, sc2_free);
    SAFE_DELETE(sc2_map.t3SyncCliffLevel, sc2_free);
    SAFE_DELETE(sc2_map.t3HeightMap, sc2_free);
    SAFE_DELETE(sc2_map.t3SyncHeightMap, sc2_free);
    SAFE_DELETE(sc2_map.t3TextureMasks, sc2_free);
    memset(&sc2_map, 0, sizeof(sc2_map));
}

static FLOAT sc2_height_scale(void) {
    return sc2_map.height_quantize_scale ? sc2_map.height_quantize_scale : 1.0f;
}

static FLOAT sc2_height_total_at_index(DWORD x, DWORD y) {
    sc2MapHeightSample_t const *sample;

    if (!sc2_map.t3HeightMap || !sc2_map.t3HeightMap->width || !sc2_map.t3HeightMap->height)
        return 0.0f;
    x = MIN(sc2_map.t3HeightMap->width - 1, x);
    y = MIN(sc2_map.t3HeightMap->height - 1, y);
    sample = &sc2_map.t3HeightMap->data[x + y * sc2_map.t3HeightMap->width];
    return ((FLOAT)sample->height + (FLOAT)sample->adjustment) * sc2_height_scale() - 1.0f;
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

    if (!data || size < sizeof(*sc2_map.MapInfo)) {
        sc2_free_file(data);
        return false;
    }
    sc2_map.MapInfo = sc2_alloc(size);
    memcpy(sc2_map.MapInfo, data, size);
    sc2_free_file(data);
    if (!sc2_mapinfo_fourcc(sc2_map.MapInfo)) {
        SAFE_DELETE(sc2_map.MapInfo, sc2_free);
        return false;
    }
    if (!sc2_map.map_name[0] && sc2_map.MapInfo->data[0])
        snprintf(sc2_map.map_name, sizeof(sc2_map.map_name), "%s", (char const *)sc2_map.MapInfo->data);
    sc2_set_size(sc2_map.MapInfo->width, sc2_map.MapInfo->height);
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
    sc2_set_size(sc2_map.width, sc2_map.height);
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

static sc2XmlField_t const sc2_object_fields[] = {
    SC2_XML_FIELD("Id", id, SC2_XML_FIELD_DWORD),
    SC2_XML_STRING_FIELD("Name", name),
    SC2_XML_STRING_FIELD("Model", model),
    SC2_XML_STRING_FIELD("File", model),
    SC2_XML_FIELD("Position", position, SC2_XML_FIELD_VEC3),
    SC2_XML_FIELD("Rotation", angle, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("Angle", angle, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("Yaw", angle, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("Scale", scale, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("Variation", variation, SC2_XML_FIELD_DWORD),
    SC2_XML_FIELD("Player", player, SC2_XML_FIELD_DWORD),
    SC2_XML_FIELD("Owner", player, SC2_XML_FIELD_DWORD),
};

static sc2XmlField_t const sc2_unit_fields[] = {
    SC2_XML_STRING_FIELD("UnitType", name),
};

static sc2XmlField_t const sc2_doodad_fields[] = {
    SC2_XML_STRING_FIELD("Type", name),
};

static sc2XmlField_t const sc2_camera_fields[] = {
    SC2_XML_FIELD("CameraTarget", camera.target, SC2_XML_FIELD_VEC3),
};

static sc2XmlField_t const sc2_camera_value_fields[] = {
    SC2_XML_FIELD("Distance", camera.distance, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("Pitch", camera.pitch, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("Yaw", camera.yaw, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("FieldOfView", camera.fov, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("NearClip", camera.znear, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("FarClip", camera.zfar, SC2_XML_FIELD_FLOAT),
    SC2_XML_FIELD("HeightOffset", camera.height_offset, SC2_XML_FIELD_FLOAT),
};

static sc2XmlFlag_t const sc2_object_flags[] = {
    { "HeightAbsolute", SC2_OBJECT_HEIGHT_ABSOLUTE },
    { "HeightOffset",   SC2_OBJECT_HEIGHT_OFFSET },
    { "ForcePlacement", SC2_OBJECT_FORCE_PLACEMENT },
};

static BOOL sc2_parse_xml_field(void *base, sc2XmlField_t const *fields, DWORD num_fields, LPCSTR name, LPCSTR value) {
    if (!base || !fields || !name || !value || !*value)
        return false;
    FOR_LOOP(i, num_fields) {
        sc2XmlField_t const *field = &fields[i];
        char *out = (char *)base + field->offset;
        if (!sc2_streqi(name, field->name))
            continue;
        switch (field->type) {
            case SC2_XML_FIELD_DWORD:
                *(DWORD *)out = (DWORD)strtoul(value, NULL, 10);
                return true;
            case SC2_XML_FIELD_FLOAT:
                *(FLOAT *)out = strtof(value, NULL);
                return true;
            case SC2_XML_FIELD_STRING:
                snprintf(out, field->size, "%s", value);
                return true;
            case SC2_XML_FIELD_VEC3:
                return sc2_parse_vec3(value, (LPVECTOR3)out);
        }
    }
    return false;
}

static void sc2_parse_object_field(sc2MapObject_t *object, sc2XmlField_t const *fields, DWORD num_fields, LPCSTR key, LPCSTR value, BOOL *has_position) {
    if (sc2_parse_xml_field(object, fields, num_fields, key, value) &&
        (sc2_streqi(key, "Position") || sc2_streqi(key, "CameraTarget")) &&
        has_position)
        *has_position = true;
}

static void sc2_parse_object_fields(sc2MapObject_t *object, sc2ObjectType_t type, LPCSTR key, LPCSTR value, BOOL *has_position) {
    sc2_parse_object_field(object, sc2_object_fields, SC2_ARRAY_LEN(sc2_object_fields), key, value, has_position);
    switch (type) {
        case SC2_OBJECT_UNIT:
            sc2_parse_object_field(object, sc2_unit_fields, SC2_ARRAY_LEN(sc2_unit_fields), key, value, has_position);
            break;
        case SC2_OBJECT_DOODAD:
            sc2_parse_object_field(object, sc2_doodad_fields, SC2_ARRAY_LEN(sc2_doodad_fields), key, value, has_position);
            break;
        case SC2_OBJECT_CAMERA:
            sc2_parse_object_field(object, sc2_camera_fields, SC2_ARRAY_LEN(sc2_camera_fields), key, value, has_position);
            break;
    }
}

static BOOL sc2_object_type(xmlNodePtr node, sc2ObjectType_t *type) {
    LPCSTR name = (char const *)node->name;

    if (sc2_contains_i(name, "ObjectUnit")) *type = SC2_OBJECT_UNIT;
    else if (sc2_contains_i(name, "ObjectDoodad")) *type = SC2_OBJECT_DOODAD;
    else if (sc2_contains_i(name, "ObjectCamera")) *type = SC2_OBJECT_CAMERA;
    else return false;
    return true;
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
    FOR_LOOP(i, SC2_ARRAY_LEN(sc2_object_flags)) {
        if (sc2_streqi(index, sc2_object_flags[i].name)) {
            object->flags |= sc2_object_flags[i].flag;
            return;
        }
    }
}

static void sc2_parse_object_node(xmlNodePtr node) {
    sc2MapObject_t object;
    BOOL has_position = false;
    sc2ObjectType_t type;
    char value[256];

    if (!node || node->type != XML_ELEMENT_NODE) return;
    if (!sc2_object_type(node, &type)) {
        for (xmlNodePtr child = node->children; child; child = child->next)
            sc2_parse_object_node(child);
        return;
    }
    memset(&object, 0, sizeof(object));
    object.scale = 1.0f;
    object.type = type;

    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
        xmlChar *text = xmlNodeListGetString(node->doc, attr->children, 1);
        if (text) {
            sc2_parse_object_fields(&object, type, (char const *)attr->name, (char const *)text, &has_position);
            xmlFree(text);
        }
    }
    for (xmlNodePtr child = node->children; child; child = child->next) {
        char index[64];
        if (child->type != XML_ELEMENT_NODE)
            continue;
        sc2_object_flag(&object, child);
        if (type == SC2_OBJECT_CAMERA &&
            sc2_contains_i((char const *)child->name, "CameraValue") &&
            sc2_xml_attr(child, "Index", index, sizeof(index)) &&
            sc2_xml_attr(child, "Value", value, sizeof(value))) {
            sc2_parse_object_field(&object,
                                   sc2_camera_value_fields,
                                   SC2_ARRAY_LEN(sc2_camera_value_fields),
                                   index,
                                   value,
                                   NULL);
            continue;
        }
        if (sc2_xml_content(child, value, sizeof(value))) {
            sc2_parse_object_fields(&object, type, (char const *)child->name, value, &has_position);
        }
    }

    if (has_position && (object.name[0] || object.model[0])) {
        if (sc2_map.num_objects < SC2_MAX_MAP_OBJECTS) {
            sc2_set_object_model(&object);
            sc2_map.objects[sc2_map.num_objects++] = object;
        }
    }
}

static void sc2_parse_objects(sc2MapSource_t *source) {
    xmlDocPtr doc = sc2_read_xml(source, "Objects");
    if (!doc) doc = sc2_read_xml(source, "Objects.xml");
    if (!doc) return;
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

static BOOL sc2_mapinfo_fourcc(sc2MapInfo_t const *mapInfo) {
    return mapInfo &&
        (mapInfo->fourcc == MAKEFOURCC('I','p','a','M') ||
         mapInfo->fourcc == MAKEFOURCC('M','a','p','I'));
}

static void sc2_set_size(DWORD width, DWORD height) {
    sc2_map.width = width;
    sc2_map.height = height;
}

static void sc2_parse_height_map(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3HeightMap", &size);

    sc2_map.t3HeightMap = sc2_alloc(size);
    memcpy(sc2_map.t3HeightMap, data, size);
    sc2_free_file(data);
}

static void sc2_parse_sync_height_map(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3SyncHeightMap", &size);

    sc2_map.t3SyncHeightMap = sc2_alloc(size);
    memcpy(sc2_map.t3SyncHeightMap, data, size);
    sc2_free_file(data);
}

static void sc2_parse_cell_flags(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3CellFlags", &size);

    sc2_map.t3CellFlags = sc2_alloc(size);
    memcpy(sc2_map.t3CellFlags, data, size);
    sc2_free_file(data);
}

static void sc2_parse_sync_cliff_level(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3SyncCliffLevel", &size);

    sc2_map.t3SyncCliffLevel = sc2_alloc(size);
    memcpy(sc2_map.t3SyncCliffLevel, data, size);
    sc2_free_file(data);
}

static void sc2_parse_texture_masks(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3TextureMasks", &size);

    sc2_map.t3TextureMasks = sc2_alloc(size);
    memcpy(sc2_map.t3TextureMasks, data, size);
    sc2_map.t3TextureMasksSize = size;
    sc2_free_file(data);
}

void SC2_MapSetHost(sc2MapHost_t const *host) {
    memset(&sc2_host, 0, sizeof(sc2_host));
    if (host) sc2_host = *host;
}

BOOL SC2_MapLoad(LPCSTR mapFilename) {
    sc2MapSource_t source;
    sc2_map_clear();
    sc2_map.cell_size = SC2_CELL_SIZE;
    if (!sc2_source_open(&source, mapFilename)) {
        return false;
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
    sc2_map.origin.x = 0.0f;
    sc2_map.origin.y = 0.0f;
    fprintf(stderr, "SC2_MapLoad: %s %ux%u objects=%u\n",
            sc2_map.map_name, (unsigned)sc2_map.width, (unsigned)sc2_map.height,
            (unsigned)sc2_map.num_objects);
    return true;
}

void SC2_MapShutdown(void) {
    sc2_map_clear();
}

sc2Map_t *SC2_MapCurrent(void) {
    return &sc2_map;
}

FLOAT SC2_MapHeightAtPoint(FLOAT x, FLOAT y) {
    FLOAT fx, fy, tx, ty;
    DWORD x0, y0, x1, y1;
    FLOAT h00, h10, h01, h11, h0, h1;

    if (!sc2_map.t3HeightMap || !sc2_map.t3HeightMap->width || !sc2_map.t3HeightMap->height) {
        return 0.0f;
    }
    fx = (x - sc2_map.origin.x) / (sc2_map.cell_size ? sc2_map.cell_size : 1.0f);
    fy = (y - sc2_map.origin.y) / (sc2_map.cell_size ? sc2_map.cell_size : 1.0f);
    fx = MIN(MAX(fx, 0.0f), (FLOAT)(sc2_map.width ? sc2_map.width : sc2_map.t3HeightMap->width - 1));
    fy = MIN(MAX(fy, 0.0f), (FLOAT)(sc2_map.height ? sc2_map.height : sc2_map.t3HeightMap->height - 1));
    x0 = (DWORD)floorf(fx);
    y0 = (DWORD)floorf(fy);
    x1 = x0 + 1;
    y1 = y0 + 1;
    tx = fx - (FLOAT)x0;
    ty = fy - (FLOAT)y0;
    h00 = sc2_height_total_at_index(x0, y0);
    h10 = sc2_height_total_at_index(x1, y0);
    h01 = sc2_height_total_at_index(x0, y1);
    h11 = sc2_height_total_at_index(x1, y1);
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
