#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FDF_NODES 4096
#define MAX_ROOTS 256
#define MAX_TOKEN 1024
#define MAX_NAME 128
#define MAX_IDENT 160
#define MAX_TYPE 192
#define MAX_FILES 256

typedef struct {
    char *data;
    size_t size;
    size_t pos;
    const char *name;
    int line;
} lexer_t;

typedef struct {
    char name[MAX_NAME];
    char ident[MAX_IDENT];
    char binding_ident[MAX_IDENT];
    char frame_type[MAX_NAME];
    int parent;
    int first_child;
    int last_child;
    int next_sibling;
    bool selected_root;
    bool needs_struct;
    bool type_emitted;
} fdf_node_t;

static fdf_node_t nodes[MAX_FDF_NODES];
static int node_count = 0;
static int selected_roots[MAX_ROOTS];
static int selected_root_count = 0;
static const char *root_filters[MAX_ROOTS];
static int root_filter_count = 0;
static char prefix[MAX_IDENT] = "";
static char include_path[256] = "ui/ui_local.h";
static bool emit_include = true;

static void usage(void) {
    fprintf(stderr,
            "Usage:\n"
            "  fdfbindgen [-prefix Name] [-root FrameName] [-include path] [-no-include] <file.fdf|->...\n"
            "\n"
            "Generates a header-only C binding struct for FDF frame names on stdout.\n"
            "Use -root more than once to bind selected root frames; without -root,\n"
            "all top-level frames found in the input are emitted.\n"
            "Redirect stdout to choose the output file. Name generated headers after\n"
            "the consuming .c file, e.g. ui/screens/main_menu.c -> ui/generated/main_menu.h.\n"
            "\n"
            "Examples:\n"
            "  fdfbindgen -prefix MainMenu -root MainMenuFrame MainMenu.fdf > ui/generated/main_menu.h\n"
            "  mpqtool -mpq War3.mpq cat UI/FrameDef/Glue/MainMenu.fdf | fdfbindgen -prefix MainMenu -root MainMenuFrame -\n");
}

static bool read_all(FILE *fp, const char *name, char **out_data, size_t *out_size) {
    size_t cap = 8192;
    size_t len = 0;
    char *data = malloc(cap + 1);
    if (!data) {
        fprintf(stderr, "fdfbindgen: out of memory reading %s\n", name);
        return false;
    }
    for (;;) {
        size_t got;
        if (len == cap) {
            char *grown;
            cap *= 2;
            grown = realloc(data, cap + 1);
            if (!grown) {
                free(data);
                fprintf(stderr, "fdfbindgen: out of memory reading %s\n", name);
                return false;
            }
            data = grown;
        }
        got = fread(data + len, 1, cap - len, fp);
        len += got;
        if (got == 0) {
            break;
        }
    }
    if (ferror(fp)) {
        free(data);
        fprintf(stderr, "fdfbindgen: failed reading %s\n", name);
        return false;
    }
    data[len] = '\0';
    *out_data = data;
    *out_size = len;
    return true;
}

static void lexer_skip_space_and_comments(lexer_t *lx) {
    for (;;) {
        while (lx->pos < lx->size && isspace((unsigned char)lx->data[lx->pos])) {
            if (lx->data[lx->pos] == '\n') {
                lx->line++;
            }
            lx->pos++;
        }
        if (lx->pos + 1 < lx->size && lx->data[lx->pos] == '/' && lx->data[lx->pos + 1] == '/') {
            lx->pos += 2;
            while (lx->pos < lx->size && lx->data[lx->pos] != '\n') {
                lx->pos++;
            }
            continue;
        }
        if (lx->pos + 1 < lx->size && lx->data[lx->pos] == '/' && lx->data[lx->pos + 1] == '*') {
            lx->pos += 2;
            while (lx->pos + 1 < lx->size &&
                   !(lx->data[lx->pos] == '*' && lx->data[lx->pos + 1] == '/')) {
                if (lx->data[lx->pos] == '\n') {
                    lx->line++;
                }
                lx->pos++;
            }
            if (lx->pos + 1 < lx->size) {
                lx->pos += 2;
            }
            continue;
        }
        return;
    }
}

static bool lexer_next(lexer_t *lx, char *out, size_t out_size, bool *quoted) {
    size_t len = 0;
    lexer_skip_space_and_comments(lx);
    if (quoted) {
        *quoted = false;
    }
    if (lx->pos >= lx->size) {
        return false;
    }
    if (lx->data[lx->pos] == '"' || lx->data[lx->pos] == '\'') {
        char quote = lx->data[lx->pos++];
        if (quoted) {
            *quoted = true;
        }
        while (lx->pos < lx->size && lx->data[lx->pos] != quote) {
            char c = lx->data[lx->pos++];
            if (c == '\\' && lx->pos < lx->size) {
                if (len + 1 < out_size) {
                    out[len++] = c;
                }
                c = lx->data[lx->pos++];
            }
            if (c == '\n') {
                lx->line++;
            }
            if (len + 1 < out_size) {
                out[len++] = c;
            }
        }
        if (lx->pos < lx->size) {
            lx->pos++;
        }
        out[len] = '\0';
        return true;
    }
    if (lx->data[lx->pos] == '{' || lx->data[lx->pos] == '}' || lx->data[lx->pos] == ',') {
        out[0] = lx->data[lx->pos++];
        out[1] = '\0';
        return true;
    }
    while (lx->pos < lx->size) {
        char c = lx->data[lx->pos];
        if (isspace((unsigned char)c) || c == '{' || c == '}' || c == ',' || c == '"' || c == '\'') {
            break;
        }
        if (c == '/' && lx->pos + 1 < lx->size &&
            (lx->data[lx->pos + 1] == '/' || lx->data[lx->pos + 1] == '*')) {
            break;
        }
        lx->pos++;
        if (len + 1 < out_size) {
            out[len++] = c;
        }
    }
    out[len] = '\0';
    return true;
}

static bool is_declaration_token(const char *tok) {
    return !strcmp(tok, "Frame") ||
           !strcmp(tok, "Texture") ||
           !strcmp(tok, "String") ||
           !strcmp(tok, "Layer");
}

static bool is_c_keyword(const char *ident) {
    static const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while", NULL
    };
    for (int i = 0; keywords[i]; i++) {
        if (!strcmp(ident, keywords[i])) {
            return true;
        }
    }
    return false;
}

static void make_ident(const char *name, char *out, size_t out_size) {
    size_t len = 0;
    if (!name || !*name) {
        name = "frame";
    }
    if (!(isalpha((unsigned char)name[0]) || name[0] == '_')) {
        if (len + 1 < out_size) {
            out[len++] = '_';
        }
    }
    for (size_t i = 0; name[i]; i++) {
        unsigned char c = (unsigned char)name[i];
        if (isalnum(c) || c == '_') {
            if (len + 1 < out_size) {
                out[len++] = (char)c;
            }
        } else {
            if (len == 0 || out[len - 1] != '_') {
                if (len + 1 < out_size) {
                    out[len++] = '_';
                }
            }
        }
    }
    while (len > 1 && out[len - 1] == '_') {
        len--;
    }
    out[len] = '\0';
    if (is_c_keyword(out)) {
        strncat(out, "_", out_size - strlen(out) - 1);
    }
}

static bool ident_used_by_sibling(int parent, const char *ident, int before_node) {
    int child = parent >= 0 ? nodes[parent].first_child : -1;
    if (parent < 0) {
        for (int i = 0; i < before_node; i++) {
            if (nodes[i].parent < 0 && !strcmp(nodes[i].ident, ident)) {
                return true;
            }
        }
        return false;
    }
    while (child >= 0 && child != before_node) {
        if (!strcmp(nodes[child].ident, ident)) {
            return true;
        }
        child = nodes[child].next_sibling;
    }
    return false;
}

static void uniquify_node_ident(int node_index) {
    char base[MAX_IDENT];
    int suffix = 2;
    snprintf(base, sizeof(base), "%s", nodes[node_index].ident);
    while (ident_used_by_sibling(nodes[node_index].parent, nodes[node_index].ident, node_index)) {
        snprintf(nodes[node_index].ident, sizeof(nodes[node_index].ident), "%s_%d", base, suffix++);
    }
}

static int add_node(int parent, const char *name, const char *frame_type) {
    int index;
    if (node_count >= MAX_FDF_NODES) {
        fprintf(stderr, "fdfbindgen: too many FDF frame declarations (max %d)\n", MAX_FDF_NODES);
        exit(1);
    }
    index = node_count++;
    memset(&nodes[index], 0, sizeof(nodes[index]));
    snprintf(nodes[index].name, sizeof(nodes[index].name), "%s", name ? name : "");
    snprintf(nodes[index].frame_type, sizeof(nodes[index].frame_type), "%s", frame_type ? frame_type : "");
    make_ident(nodes[index].name, nodes[index].ident, sizeof(nodes[index].ident));
    nodes[index].parent = parent;
    nodes[index].first_child = -1;
    nodes[index].last_child = -1;
    nodes[index].next_sibling = -1;
    if (parent >= 0) {
        if (nodes[parent].last_child >= 0) {
            nodes[nodes[parent].last_child].next_sibling = index;
        } else {
            nodes[parent].first_child = index;
        }
        nodes[parent].last_child = index;
    }
    uniquify_node_ident(index);
    return index;
}

static void skip_scope(lexer_t *lx) {
    char tok[MAX_TOKEN];
    bool quoted;
    int depth = 1;
    while (depth > 0 && lexer_next(lx, tok, sizeof(tok), &quoted)) {
        if (quoted) {
            continue;
        }
        if (!strcmp(tok, "{")) {
            depth++;
        } else if (!strcmp(tok, "}")) {
            depth--;
        }
    }
}

static void parse_scope(lexer_t *lx, int parent) {
    char tok[MAX_TOKEN];
    bool quoted;
    while (lexer_next(lx, tok, sizeof(tok), &quoted)) {
        char frame_type[MAX_NAME] = "";
        char frame_name[MAX_NAME] = "";
        int child;
        if (!quoted && !strcmp(tok, "}")) {
            return;
        }
        if (!quoted && !strcmp(tok, "{")) {
            skip_scope(lx);
            continue;
        }
        if (quoted || !is_declaration_token(tok)) {
            continue;
        }
        if (!strcmp(tok, "Frame")) {
            if (!lexer_next(lx, frame_type, sizeof(frame_type), &quoted)) {
                return;
            }
            if (!lexer_next(lx, frame_name, sizeof(frame_name), &quoted)) {
                return;
            }
        } else {
            snprintf(frame_type, sizeof(frame_type), "%s", tok);
            if (!lexer_next(lx, frame_name, sizeof(frame_name), &quoted)) {
                return;
            }
        }
        while (lexer_next(lx, tok, sizeof(tok), &quoted)) {
            if (!quoted && !strcmp(tok, "{")) {
                child = add_node(parent, frame_name, frame_type);
                parse_scope(lx, child);
                break;
            }
            if (!quoted && !strcmp(tok, "}")) {
                return;
            }
        }
    }
}

static bool parse_file(const char *path) {
    FILE *fp = NULL;
    char *data = NULL;
    size_t size = 0;
    lexer_t lx;
    bool ok;
    if (!strcmp(path, "-")) {
        fp = stdin;
    } else {
        fp = fopen(path, "rb");
        if (!fp) {
            fprintf(stderr, "fdfbindgen: cannot open %s\n", path);
            return false;
        }
    }
    ok = read_all(fp, path, &data, &size);
    if (fp != stdin) {
        fclose(fp);
    }
    if (!ok) {
        return false;
    }
    lx.data = data;
    lx.size = size;
    lx.pos = 0;
    lx.name = path;
    lx.line = 1;
    parse_scope(&lx, -1);
    free(data);
    return true;
}

static void select_roots(void) {
    if (root_filter_count == 0) {
        for (int i = 0; i < node_count; i++) {
            if (nodes[i].parent < 0 && selected_root_count < MAX_ROOTS) {
                nodes[i].selected_root = true;
                selected_roots[selected_root_count++] = i;
            }
        }
    } else {
        for (int r = 0; r < root_filter_count; r++) {
            bool found = false;
            for (int i = 0; i < node_count; i++) {
                if (!strcmp(nodes[i].name, root_filters[r])) {
                    if (!nodes[i].selected_root && selected_root_count < MAX_ROOTS) {
                        nodes[i].selected_root = true;
                        selected_roots[selected_root_count++] = i;
                    }
                    found = true;
                }
            }
            if (!found) {
                fprintf(stderr, "fdfbindgen: root frame not found in input: %s\n", root_filters[r]);
            }
        }
    }
}

static bool binding_ident_used(const char *ident, int self_index) {
    for (int i = 0; i < node_count; i++) {
        if (i == self_index) {
            continue;
        }
        if (nodes[i].binding_ident[0] && !strcmp(nodes[i].binding_ident, ident)) {
            return true;
        }
    }
    return false;
}

static void assign_binding_idents(int node_index) {
    char base[MAX_IDENT];
    int suffix = 2;

    snprintf(base, sizeof(base), "%s", nodes[node_index].ident);
    snprintf(nodes[node_index].binding_ident, sizeof(nodes[node_index].binding_ident), "%s", base);
    while (binding_ident_used(nodes[node_index].binding_ident, node_index)) {
        snprintf(nodes[node_index].binding_ident,
                 sizeof(nodes[node_index].binding_ident),
                 "%s_%d",
                 base,
                 suffix++);
    }
    for (int child = nodes[node_index].first_child; child >= 0; child = nodes[child].next_sibling) {
        assign_binding_idents(child);
    }
}

static void assign_selected_binding_idents(void) {
    for (int i = 0; i < selected_root_count; i++) {
        assign_binding_idents(selected_roots[i]);
    }
}

static void emit_binding_fields(int node_index) {
    printf("    LPFRAMEDEF %s;\n", nodes[node_index].binding_ident);
    for (int child = nodes[node_index].first_child; child >= 0; child = nodes[child].next_sibling) {
        emit_binding_fields(child);
    }
}

static void emit_binding_type(void) {
    printf("typedef struct %sFdfBindings_s {\n", prefix);
    for (int i = 0; i < selected_root_count; i++) {
        emit_binding_fields(selected_roots[i]);
    }
    printf("} %sFdfBindings_t;\n\n", prefix);
}

static void emit_bind_children(int node_index, const char *node_expr) {
    for (int child = nodes[node_index].first_child; child >= 0; child = nodes[child].next_sibling) {
        char child_expr[1024];
        snprintf(child_expr, sizeof(child_expr), "out->%s", nodes[child].binding_ident);
        printf("    OW3_FDF_BIND_CHILD(out, %s, %s, \"%s\");\n",
               nodes[child].binding_ident, node_expr, nodes[child].name);
        emit_bind_children(child, child_expr);
    }
}

static void emit_bind_function(void) {
    printf("static inline BOOL %sFdfBindings_Bind(%sFdfBindings_t *out) {\n", prefix, prefix);
    printf("    BOOL ok = true;\n");
    printf("    if (!out) {\n");
    printf("        return false;\n");
    printf("    }\n");
    printf("    memset(out, 0, sizeof(*out));\n");
    for (int i = 0; i < selected_root_count; i++) {
        int root = selected_roots[i];
        char root_expr[1024];
        snprintf(root_expr, sizeof(root_expr), "out->%s", nodes[root].binding_ident);
        printf("    OW3_FDF_BIND_ROOT(out, %s, \"%s\");\n",
               nodes[root].binding_ident, nodes[root].name);
        emit_bind_children(root, root_expr);
    }
    printf("    return ok;\n");
    printf("}\n");
}

static void emit_bind_from_root_function(int root) {
    const char *root_ident = nodes[root].binding_ident;

    printf("\n");
    printf("static inline BOOL %sFdfBindings_BindFrom%s(%sFdfBindings_t *out, LPFRAMEDEF root) {\n",
           prefix, root_ident, prefix);
    printf("    BOOL ok = true;\n");
    printf("    if (!out || !root) {\n");
    printf("        return false;\n");
    printf("    }\n");
    printf("    memset(out, 0, sizeof(*out));\n");
    printf("    OW3_FDF_BIND_EXISTING_ROOT(out, %s, root, \"%s\");\n",
           root_ident, nodes[root].name);
    emit_bind_children(root, "root");
    printf("    return ok;\n");
    printf("}\n");
}

static void emit_bind_from_root_functions(void) {
    for (int i = 0; i < selected_root_count; i++) {
        emit_bind_from_root_function(selected_roots[i]);
    }
}

static void emit_header(void) {
    char guard[MAX_IDENT * 2];
    char guard_prefix[MAX_IDENT];
    snprintf(guard_prefix, sizeof(guard_prefix), "%s_FDF_BINDINGS_H", prefix);
    make_ident(guard_prefix, guard, sizeof(guard));
    for (size_t i = 0; guard[i]; i++) {
        guard[i] = (char)toupper((unsigned char)guard[i]);
    }
    printf("/* Generated by fdfbindgen. Do not edit by hand. */\n");
    printf("#ifndef %s\n", guard);
    printf("#define %s\n\n", guard);
    if (emit_include) {
        printf("#include \"%s\"\n\n", include_path);
    }
    printf("#ifndef OW3_FDF_BINDINGS_REPORT_MISSING\n");
    printf("#define OW3_FDF_BINDINGS_REPORT_MISSING(NAME) \\\n");
    printf("    do { if (uiimport.Printf) uiimport.Printf(\"ERROR: missing FDF binding: %%s\\n\", (NAME)); } while (0)\n");
    printf("#endif\n\n");
    printf("#ifndef OW3_FDF_BIND_ROOT\n");
    printf("#define OW3_FDF_BIND_ROOT(OUT, FIELD, NAME) \\\n");
    printf("    do { (OUT)->FIELD = UI_FindFrame((NAME)); if (!(OUT)->FIELD) { OW3_FDF_BINDINGS_REPORT_MISSING((NAME)); ok = false; } } while (0)\n");
    printf("#endif\n\n");
    printf("#ifndef OW3_FDF_BIND_EXISTING_ROOT\n");
    printf("#define OW3_FDF_BIND_EXISTING_ROOT(OUT, FIELD, ROOT, NAME) \\\n");
    printf("    do { (OUT)->FIELD = (ROOT); if (!(OUT)->FIELD) { OW3_FDF_BINDINGS_REPORT_MISSING((NAME)); ok = false; } } while (0)\n");
    printf("#endif\n\n");
    printf("#ifndef OW3_FDF_BIND_CHILD\n");
    printf("#define OW3_FDF_BIND_CHILD(OUT, FIELD, PARENT, NAME) \\\n");
    printf("    do { (OUT)->FIELD = (PARENT) ? UI_FindChildFrame((PARENT), (NAME)) : NULL; if (!(OUT)->FIELD) { OW3_FDF_BINDINGS_REPORT_MISSING((NAME)); ok = false; } } while (0)\n");
    printf("#endif\n\n");
    emit_binding_type();
    emit_bind_function();
    emit_bind_from_root_functions();
    printf("\n#endif /* %s */\n", guard);
}

static void derive_prefix_from_path(const char *path) {
    const char *base = strrchr(path, '/');
    char tmp[MAX_IDENT];
    if (!base) {
        base = strrchr(path, '\\');
    }
    base = base ? base + 1 : path;
    snprintf(tmp, sizeof(tmp), "%s", base);
    for (size_t i = 0; tmp[i]; i++) {
        if (tmp[i] == '.') {
            tmp[i] = '\0';
            break;
        }
    }
    make_ident(tmp, prefix, sizeof(prefix));
    if (!prefix[0]) {
        snprintf(prefix, sizeof(prefix), "Generated");
    }
}

int main(int argc, char **argv) {
    const char *files[MAX_FILES];
    int file_count = 0;
    bool ok = true;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage();
            return 0;
        } else if (!strcmp(argv[i], "-prefix")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            make_ident(argv[i], prefix, sizeof(prefix));
        } else if (!strcmp(argv[i], "-root")) {
            if (++i >= argc || root_filter_count >= MAX_ROOTS) {
                usage();
                return 1;
            }
            root_filters[root_filter_count++] = argv[i];
        } else if (!strcmp(argv[i], "-include")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            snprintf(include_path, sizeof(include_path), "%s", argv[i]);
            emit_include = true;
        } else if (!strcmp(argv[i], "-no-include")) {
            emit_include = false;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0' && strcmp(argv[i], "-")) {
            fprintf(stderr, "fdfbindgen: unknown option: %s\n", argv[i]);
            usage();
            return 1;
        } else {
            if (file_count >= MAX_FILES) {
                fprintf(stderr, "fdfbindgen: too many input files\n");
                return 1;
            }
            files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        usage();
        return 1;
    }
    if (!prefix[0]) {
        derive_prefix_from_path(files[0]);
    }
    for (int i = 0; i < file_count; i++) {
        if (!parse_file(files[i])) {
            ok = false;
        }
    }
    if (!ok) {
        return 1;
    }
    select_roots();
    if (selected_root_count == 0) {
        fprintf(stderr, "fdfbindgen: no root frames selected\n");
        return 1;
    }
    assign_selected_binding_idents();
    emit_header();
    return 0;
}
