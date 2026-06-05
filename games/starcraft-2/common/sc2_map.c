#include "sc2_map.h"
#include "common/mpq.h"
#include <ctype.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

typedef struct {
    HANDLE archive;
    HANDLE archive_data;
    char   base[MAX_PATHLEN];
} sc2MapSource_t;

static sc2MapHost_t sc2_host;
static sc2Map_t     sc2_map;

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
    memset(&sc2_map, 0, sizeof(sc2_map));
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
    if (!strstr(buffer, ".dds") && !strstr(buffer, ".DDS")) {
        strncat(buffer, ".dds", sizeof(buffer) - strlen(buffer) - 1);
    }
    FOR_LOOP(i, sc2_map.num_terrain_textures) {
        if (!strcasecmp(sc2_map.terrain_textures[i].diffuse, buffer)) {
            return;
        }
    }

    texture = &sc2_map.terrain_textures[sc2_map.num_terrain_textures++];
    snprintf(texture->diffuse, sizeof(texture->diffuse), "%s", buffer);
    snprintf(texture->normal, sizeof(texture->normal), "%s", buffer);
    ext = strrchr(texture->normal, '.');
    if (ext) {
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
            sc2_map_try_size_field((char const *)attr->name, (char const *)text);
            xmlFree(text);
        }
    }
    if (sc2_xml_content(node, value, sizeof(value)))
        sc2_map_try_size_field((char const *)node->name, value);
    for (xmlNodePtr child = node->children; child; child = child->next)
        sc2_parse_mapinfo_node(child);
}

static void sc2_parse_mapinfo(sc2MapSource_t *source) {
    xmlDocPtr doc = sc2_read_xml(source, "MapInfo");
    if (!doc) doc = sc2_read_xml(source, "MapInfo.xml");
    if (!doc) return;
    sc2_parse_mapinfo_node(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
}

static void sc2_parse_terrain_node(xmlNodePtr node) {
    char value[256];

    if (!node || node->type != XML_ELEMENT_NODE)
        return;
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
        xmlChar *text = xmlNodeListGetString(node->doc, attr->children, 1);
        if (text) {
            sc2_parse_terrain_value((char const *)attr->name, (char const *)text);
            xmlFree(text);
        }
    }
    if (sc2_xml_content(node, value, sizeof(value))) {
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
    else if ((sc2_contains_i(key, "type") || sc2_contains_i(key, "unit") || sc2_contains_i(key, "doodad") ||
              sc2_streqi(key, "id") || sc2_streqi(key, "name")) && !object->name[0])
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
    } else if (sc2_contains_i(key, "player") || sc2_contains_i(key, "owner")) {
        object->player = (DWORD)strtoul(value, NULL, 10);
    }
}

static void sc2_parse_object_node(xmlNodePtr node) {
    sc2MapObject_t object;
    BOOL has_position = false;
    char value[256];

    if (!node || node->type != XML_ELEMENT_NODE) return;
    memset(&object, 0, sizeof(object));
    object.scale = 1.0f;
    object.type = sc2_contains_i((char const *)node->name, "doodad") ? SC2_OBJECT_DOODAD : SC2_OBJECT_UNIT;

    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
        xmlChar *text = xmlNodeListGetString(node->doc, attr->children, 1);
        if (text) {
            sc2_object_field(&object, (char const *)attr->name, (char const *)text, &has_position);
            xmlFree(text);
        }
    }
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && sc2_xml_content(child, value, sizeof(value)))
            sc2_object_field(&object, (char const *)child->name, value, &has_position);
    }

    if (has_position && (object.name[0] || object.model[0]) &&
        (sc2_contains_i((char const *)node->name, "unit") ||
         sc2_contains_i((char const *)node->name, "doodad") ||
         sc2_contains_i((char const *)node->name, "object"))) {
        if (sc2_map.num_objects < SC2_MAX_MAP_OBJECTS) {
            sc2_set_object_model(&object);
            sc2_map.objects[sc2_map.num_objects++] = object;
        }
    }
    for (xmlNodePtr child = node->children; child; child = child->next)
        sc2_parse_object_node(child);
}

static void sc2_parse_objects(sc2MapSource_t *source) {
    xmlDocPtr doc = sc2_read_xml(source, "Objects");
    if (!doc) doc = sc2_read_xml(source, "Objects.xml");
    if (!doc) return;
    sc2_parse_object_node(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
}

static DWORD sc2_read_le32(BYTE const *p) {
    return (DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}

static void sc2_parse_cell_flags(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3CellFlags", &size);
    if (data && size >= 16 && memcmp(data, "LFCT", 4) == 0) {
        DWORD w = sc2_read_le32(data + 24);
        DWORD h = sc2_read_le32(data + 28);
        if (w > 0 && h > 0 && w < 4096 && h < 4096) {
            sc2_map.width = w;
            sc2_map.height = h;
            if (size >= 32 + w * h) {
                sc2_map.cell_flags = sc2_alloc(w * h);
                if (sc2_map.cell_flags) {
                    memcpy(sc2_map.cell_flags, data + 32, w * h);
                    sc2_map.cell_flags_width = w;
                    sc2_map.cell_flags_height = h;
                }
            }
        }
    }
    sc2_free_file(data);
}

static void sc2_parse_sync_cliff_level(sc2MapSource_t *source) {
    DWORD size = 0;
    LPBYTE data = sc2_source_read(source, "t3SyncCliffLevel", &size);
    DWORD w, h;

    if (!data || size < 32 || memcmp(data, "CLIF", 4) != 0) {
        sc2_free_file(data);
        return;
    }
    w = sc2_read_le32(data + 8);
    h = sc2_read_le32(data + 12);
    if (!w || !h || w > 4096 || h > 4096 || size < 32 + w * h * sizeof(USHORT)) {
        sc2_free_file(data);
        return;
    }
    sc2_map.cliff_levels = sc2_alloc(w * h * sizeof(USHORT));
    if (sc2_map.cliff_levels) {
        memcpy(sc2_map.cliff_levels, data + 32, w * h * sizeof(USHORT));
        sc2_map.cliff_level_width = w;
        sc2_map.cliff_level_height = h;
    }
    sc2_free_file(data);
}

static BYTE sc2_texture_mask_nibble(BYTE byte, DWORD pixel) {
    return pixel & 1 ? byte & 0x0F : byte >> 4;
}

static void sc2_decode_texture_mask_block(LPBYTE out, LPBYTE src, DWORD width, DWORD height, DWORD block) {
    DWORD blocks_x = MAX(1, width / 64);
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
    DWORD w, h, layer_size, layers, blocks_per_layer;

    if (!data || size < 64 || memcmp(data, "MASK", 4) != 0) {
        sc2_free_file(data);
        return;
    }
    w = sc2_read_le32(data + 12);
    h = sc2_read_le32(data + 16);
    if (!w || !h || w > 4096 || h > 4096 || (w * h) / 2 == 0) {
        sc2_free_file(data);
        return;
    }
    layer_size = (w * h) / 2;
    blocks_per_layer = layer_size / (64 * 32);
    layers = MIN(SC2_MAX_TERRAIN_TEXTURES, (size - 64) / layer_size);
    sc2_map.texture_mask_width = w;
    sc2_map.texture_mask_height = h;
    sc2_map.num_texture_masks = layers;

    FOR_LOOP(layer, layers) {
        LPBYTE out = sc2_alloc(w * h);
        LPBYTE src = data + 64 + layer * layer_size;
        if (!out) {
            break;
        }
        memset(out, 0, w * h);
        if (blocks_per_layer > 0) {
            FOR_LOOP(block, blocks_per_layer) {
                sc2_decode_texture_mask_block(out, src + block * 64 * 32, w, h, block);
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
    sc2_add_default_object("CommandCenter", "Assets\\Units\\Terran\\CommandCenter\\CommandCenter.m3", 0, 0, 0, 1);
    sc2_add_default_object("SCV", "Assets\\Units\\Terran\\SCV\\SCV.m3", -6, -4, 0, 1);
    sc2_add_default_object("SCV", "Assets\\Units\\Terran\\SCV\\SCV.m3", -3, -6, 0, 1);
    sc2_add_default_object("Marine", "Assets\\Units\\Terran\\Marine\\Marine.m3", 7, 3, 0, 1);
    sc2_add_default_object("Marine", "Assets\\Units\\Terran\\Marine\\Marine.m3", 9, 5, 0, 1);
}

void SC2_MapSetHost(sc2MapHost_t const *host) {
    memset(&sc2_host, 0, sizeof(sc2_host));
    if (host) sc2_host = *host;
}

BOOL SC2_MapLoad(LPCSTR mapFilename) {
    sc2MapSource_t source;
    sc2_map_clear();
    sc2_map.cell_size = SC2_CELL_SIZE;
    sc2_map.origin.x = -(FLOAT)SC2_DEFAULT_MAP_WIDTH * SC2_CELL_SIZE * 0.5f;
    sc2_map.origin.y = -(FLOAT)SC2_DEFAULT_MAP_HEIGHT * SC2_CELL_SIZE * 0.5f;
    if (!sc2_source_open(&source, mapFilename)) {
        sc2_generate_default_map();
        return true;
    }
    sc2_parse_mapinfo(&source);
    sc2_parse_terrain(&source);
    sc2_parse_cell_flags(&source);
    sc2_parse_sync_cliff_level(&source);
    sc2_parse_texture_masks(&source);
    sc2_parse_objects(&source);
    sc2_source_close(&source);
    sc2_generate_default_map();
    sc2_map.origin.x = -(FLOAT)sc2_map.width * sc2_map.cell_size * 0.5f;
    sc2_map.origin.y = -(FLOAT)sc2_map.height * sc2_map.cell_size * 0.5f;
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
    DWORD cx, cy;

    if (!sc2_map.cliff_levels || !sc2_map.cliff_level_width || !sc2_map.cliff_level_height) {
        return 0.0f;
    }
    n = SC2_MapNormalizedPosition(x, y);
    cx = MIN(sc2_map.cliff_level_width - 1, (DWORD)floorf(n.x * sc2_map.cliff_level_width));
    cy = MIN(sc2_map.cliff_level_height - 1, (DWORD)floorf(n.y * sc2_map.cliff_level_height));
    return sc2_map.cliff_levels[cx + cy * sc2_map.cliff_level_width];
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
