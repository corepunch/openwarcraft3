WC3DATA  := data/Warcraft\ III
DEMODATA := data/Warcraft3demo
MPQ      := $(WC3DATA)/War3.mpq
MAP      := Maps/Campaign/Human02.w3m
UI_CMD   := menu_main

ZIP_URL  := https://archive.org/download/warcraft-iii-installer-enus/Warcraft-III-1.29.2-enUS.zip
ZIP_FILE := Warcraft-III-1.29.2-enUS.zip
DATA_DIR := data

CC      := gcc
BIN_DIR := build/bin
LIB_DIR := build/lib
CFLAGS  := -Wall -I. -Ishared -Ishared/types

ifeq ($(DIAG_OUTPUT),1)
	CFLAGS += -DDIAG_OUTPUT
endif
ifeq ($(NO_NETWORK),1)
	CFLAGS += -DOW3_NO_NETWORK
endif
ifeq ($(ALL_MPQS),1)
	CFLAGS += -DOW3_LOAD_ALL_MPQS
endif

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    LIB_EXT   := .dll
    LIB_FLAGS := -shared
    EXE_EXT   := .exe
    INSTALL_NAME =
    RPATH     :=
    LDFLAGS   := -L$(LIB_DIR)
	LIBS      := -lSDL2main -lSDL2 -lm -lopengl32 -lgdi32
else
    UNAME_S := $(shell uname -s)
    EXE_EXT :=
    ifeq ($(UNAME_S),Darwin)
		ARCH ?= arm64
		ifeq ($(ARCH),arm64)
            HOMEBREW_PREFIX := /opt/homebrew
        else
            HOMEBREW_PREFIX := /usr/local
        endif
        LIB_EXT   := .dylib
        LIB_FLAGS := -dynamiclib
        # Set the dylib install name to @rpath/<libname> so the executable
        # resolves it via the @executable_path/../lib rpath, not a build path.
        INSTALL_NAME = -Wl,-install_name,@rpath/$(notdir $@)
        RPATH     := -Wl,-rpath,@executable_path/../lib
		CFLAGS    += -DGL_SILENCE_DEPRECATION -I$(HOMEBREW_PREFIX)/include -arch $(ARCH)
		LDFLAGS   := -L$(LIB_DIR) -L$(HOMEBREW_PREFIX)/lib -arch $(ARCH)
		LIBS      := -lSDL2 -framework AppKit -framework OpenGL
    else
        # Linux
        LIB_EXT   := .so
        LIB_FLAGS := -shared -fPIC
        INSTALL_NAME =
        RPATH     := -Wl,-rpath,'$$ORIGIN/../lib'
        CFLAGS    += -fPIC
        LDFLAGS   := -L$(LIB_DIR) -Wl,-z,defs
		LIBS      := -lSDL2 -lEGL -lGL -lm
    endif
endif

SHARED_LIB   := $(LIB_DIR)/libshared$(LIB_EXT)
JASS_LIB     := $(LIB_DIR)/libjass$(LIB_EXT)
SHEET_LIB    := $(LIB_DIR)/libsheet$(LIB_EXT)
RENDERER_LIB := $(LIB_DIR)/librenderer$(LIB_EXT)
GAME_LIB     := $(LIB_DIR)/libgame$(LIB_EXT)
UI_LIB       := $(LIB_DIR)/libui$(LIB_EXT)
RENDERER_WOW_LIB := $(LIB_DIR)/librenderer-wow$(LIB_EXT)
GAME_WOW_LIB     := $(LIB_DIR)/libgame-wow$(LIB_EXT)
UI_WOW_LIB       := $(LIB_DIR)/libui-wow$(LIB_EXT)
BINARY       := $(BIN_DIR)/openwarcraft3$(EXE_EXT)
WOW_BINARY   := $(BIN_DIR)/openwow$(EXE_EXT)
MPQ_TEST         := $(BIN_DIR)/test_mpq_compat$(EXE_EXT)
WOW_CFLAGS   := $(CFLAGS) -DWOW -DOW3_LOAD_ALL_MPQS -Wno-unused-function

TOOL_SRCS := $(shell find tools -maxdepth 1 -name '*.c' ! -name 'jass.c' | sort)
TOOL_NAMES := $(patsubst tools/%.c,%,$(TOOL_SRCS))
TOOL_BINS := $(addprefix $(BIN_DIR)/,$(addsuffix $(EXE_EXT),$(TOOL_NAMES)))
JASS_BIN := $(BIN_DIR)/jass$(EXE_EXT)
TOOL_DEPS := $(shell find tools -maxdepth 1 -name '*.h' | sort)
CLIENT_HEADERS := $(shell find client -name '*.h' | sort)
UI_HEADERS := $(shell find ui -name '*.h' | sort)
FONT_SRC := renderer/conchars.pcx
FONT_HEADER := renderer/conchars_sysfont.h
FONT_SYMBOL := conchars_sysfont_pcx

# Unity-build helper: pipe all .c files in a directory tree as #include
# directives to gcc's stdin so the whole module is one translation unit.
# Note: awk with octal \043 for '#' avoids Make treating '#' in a variable
# value as a comment character, which would truncate the sed expression.
UNITY = find $1 -name '*.c' $2 | sort | awk '{printf "\043include \"%s\"\n", $$0}'

default: build
build: shared jass sheet renderer game ui openwarcraft3 tools jass-tool
jass-tool: $(JASS_BIN)
shared:      $(SHARED_LIB)
jass:        $(JASS_LIB)
sheet:       $(SHEET_LIB)
renderer:    $(RENDERER_LIB)
game:        $(GAME_LIB)
ui:          $(UI_LIB)
openwarcraft3: $(BINARY)
renderer-wow: $(RENDERER_WOW_LIB)
game-wow:     $(GAME_WOW_LIB)
ui-wow:       $(UI_WOW_LIB)
openwow:      $(WOW_BINARY)
tools:       $(TOOL_BINS)
font:       $(FONT_HEADER)
$(TOOL_NAMES): %: $(BIN_DIR)/%$(EXE_EXT)
run: $(BINARY)
	$(BINARY) -data=$(WC3DATA)

run-demo: $(BINARY)
	$(BINARY) -data=$(DEMODATA)

run-map: $(BINARY)
	$(BINARY) -data=$(WC3DATA) -net_enabled=1 +map $(MAP)

run-wow: $(WOW_BINARY)
	$(WOW_BINARY) -data=data/world-of-warcraft/installed/Data -net_enabled=1 +map World/Maps/Azeroth/Azeroth.wdt

build-run-wow: openwow
	$(WOW_BINARY) -data=data/world-of-warcraft/installed/Data -net_enabled=1 +map World/Maps/Azeroth/Azeroth.wdt

run-ui-text: $(BINARY)
	$(BINARY) -data=$(WC3DATA) -net_enabled=0 -r_module=stdout -com_frame_limit=1 +$(UI_CMD)

diag: clean
	$(MAKE) DIAG_OUTPUT=1 build
	$(MAKE) DIAG_OUTPUT=1 run

$(BIN_DIR)/%$(EXE_EXT): tools/%.c $(TOOL_DEPS) $(CLIENT_HEADERS) | $(BIN_DIR) $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) $(RENDERER_LIB) $(GAME_LIB) $(UI_LIB)
	@echo "[$*]"
	$(CC) $(CFLAGS) -o $@ $< \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lrenderer -lgame -lui $(LIBS) -lm -lz

$(BIN_DIR)/img2sysfont$(EXE_EXT): tools/img2sysfont.c | $(BIN_DIR)
	@echo "[img2sysfont]"
	$(CC) $(CFLAGS) -o $@ tools/img2sysfont.c

$(FONT_HEADER): $(FONT_SRC) $(BIN_DIR)/img2sysfont$(EXE_EXT)
	$(BIN_DIR)/img2sysfont$(EXE_EXT) $(FONT_SRC) $(FONT_HEADER) $(FONT_SYMBOL)

# jass — standalone JASS interpreter (no renderer/game/SDL2 needed)
$(JASS_BIN): tools/jass.c $(TOOL_DEPS) | $(BIN_DIR) $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB)
	@echo "[jass-tool]"
	$(CC) $(CFLAGS) -DTOOL_COMMON_NO_MPQ -o $@ tools/jass.c \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lm

$(MPQ_TEST): tests/test_mpq_compat.c common/mpq.c common/mpq.h | $(BIN_DIR)
	@echo "[mpq-compat-test]"
	$(CC) $(CFLAGS) -o $@ tests/test_mpq_compat.c common/mpq.c -lm -lz

$(BIN_DIR) $(LIB_DIR):
	@mkdir -p $@

# shared — math library
$(SHARED_LIB): $(shell find shared -name '*.c') | $(LIB_DIR)
	@echo "[shared]"
	@$(call UNITY,shared) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lm

# jass — Warcraft III script VM
$(JASS_LIB): $(SHARED_LIB) $(shell find jass -name '*.c' -o -name '*.h') | $(LIB_DIR)
	@echo "[jass]"
	@$(call UNITY,jass) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lshared -lm

$(SHEET_LIB): sheet/parser.c sheet/sheet.c common/common.h | $(LIB_DIR)
	@echo "[sheet]"
	$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ sheet/parser.c sheet/sheet.c $(LDFLAGS)

# renderer — depends on shared
# Uses FS_ReadFile (archive-agnostic) for initial file loads, but includes
# common/mpq.c for nested .w3m archive handling (maps are MPQ archives containing
# internal files like war3map.w3e and war3map.shd).
$(RENDERER_LIB): $(SHARED_LIB) $(CLIENT_HEADERS) common/mpq.c common/mpq.h $(FONT_HEADER) $(shell find renderer -name '*.c') | $(LIB_DIR)
	@echo "[renderer]"
	@$(call UNITY,renderer,! -path 'renderer/wow/*') | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - common/mpq.c $(LDFLAGS) -lshared $(LIBS) -lz

$(RENDERER_WOW_LIB): $(SHARED_LIB) $(CLIENT_HEADERS) common/mpq.c common/mpq.h $(FONT_HEADER) $(shell find renderer -name '*.c') | $(LIB_DIR)
	@echo "[renderer-wow]"
	@$(call UNITY,renderer,! -path 'renderer/w3m/*') | \
		$(CC) $(WOW_CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - common/mpq.c $(LDFLAGS) -lshared $(LIBS) -lz

# game — depends on shared and jass
$(GAME_LIB): $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) $(shell find game -name '*.c') | $(LIB_DIR)
	@echo "[game]"
	@$(call UNITY,game) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lsheet -lshared -ljass -lm

$(GAME_WOW_LIB): $(SHARED_LIB) $(shell find game-wow -name '*.c') | $(LIB_DIR)
	@echo "[game-wow]"
	@$(call UNITY,game-wow) | \
		$(CC) $(WOW_CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lshared -lm

# ui — depends on shared
$(UI_LIB): $(SHARED_LIB) $(CLIENT_HEADERS) $(UI_HEADERS) $(shell find ui -name '*.c') | $(LIB_DIR)
	@echo "[ui]"
	@$(call UNITY,ui) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lshared -lm

$(UI_WOW_LIB): $(SHARED_LIB) $(CLIENT_HEADERS) $(UI_HEADERS) $(shell find ui-wow -name '*.c') | $(LIB_DIR)
	@echo "[ui-wow]"
	@$(call UNITY,ui-wow) | \
		$(CC) $(WOW_CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lshared -lm

# main binary — depends on all libraries
APP_SRCS := $(shell find client server common -name '*.c')
$(BINARY): $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) $(GAME_LIB) $(RENDERER_LIB) $(UI_LIB) $(APP_SRCS) $(CLIENT_HEADERS) | $(BIN_DIR)
	@echo "[openwarcraft3]"
	@$(call UNITY,client server common) | \
		$(CC) $(CFLAGS) -x c -o $@ - $(RPATH) $(LDFLAGS) \
		-lsheet -lshared -ljass -lgame -lrenderer -lui $(LIBS) -lz

$(WOW_BINARY): $(SHARED_LIB) $(SHEET_LIB) $(GAME_WOW_LIB) $(RENDERER_WOW_LIB) $(UI_WOW_LIB) $(APP_SRCS) $(CLIENT_HEADERS) | $(BIN_DIR)
	@echo "[openwow]"
	@$(call UNITY,client server common) | \
		$(CC) $(WOW_CFLAGS) -x c -o $@ - $(RPATH) $(LDFLAGS) \
		-lsheet -lshared -lgame-wow -lrenderer-wow -lui-wow $(LIBS) -lz

download: $(ZIP_FILE)
	mkdir -p $(DATA_DIR)
	unzip -o $(ZIP_FILE) -d $(DATA_DIR)

$(ZIP_FILE):
	curl -L -o $(ZIP_FILE) $(ZIP_URL)

clean:
	rm -rf build

# ---------------------------------------------------------------------------
# Test target — builds and runs the unit test binary.
#
# The test binary compiles only the game modules needed by the tests
# (no renderer, no archive backend, no SDL2) together with the shared sources.
# Global game state and gi function-pointers are provided by the test
# harness (tests/test_harness.c) rather than by game/g_main.c.
# ---------------------------------------------------------------------------
TEST_GAME_SRCS := \
	game/g_ai.c \
	game/g_events.c \
	game/g_fow.c \
	game/g_metadata.c \
	game/g_monster.c \
	game/g_pathing.c \
	game/g_phys.c \
	game/g_utils.c \
	game/m_unit.c \
	game/skills/s_attack.c \
	game/skills/s_area_spell.c \
	game/skills/s_move.c \
	game/skills/s_item.c \
	game/skills/s_skills.c \
	game/skills/s_spell.c \
	game/skills/s_stop.c \
	game/skills/s_summon.c \
	game/skills/s_thunderbolt.c \
	game/skills/s_train.c \
	game/skills/s_utility_abilities.c \
	game/skills/s_ability_stubs.c \
	client/cl_layout.c \
	client/cl_parse.c \
	client/cl_scrn.c \
	server/sv_init.c \
	server/sv_lan.c \
	server/sv_send.c \
	server/sv_main.c \
	common/net.c \
	common/msg.c

TEST_SRCS := $(shell find tests -maxdepth 1 -name 'test_*.c' \
	! -name 'test_jass_main.c' \
	! -name 'test_main_ui.c' \
	! -name 'test_mpq_compat.c' \
	! -name 'test_ui_*.c' \
	! -name 'test_wow_appearance.c' | sort)

TEST_CFLAGS := -Wall -DTOOL_COMMON_NO_MPQ -Itests/stubs -Ishared/types -Igame -Iserver -Icommon -Iclient -Igame/skills
TEST_UI_CFLAGS := $(TEST_CFLAGS)

TEST_UI_SRCS := \
	tests/test_main_ui.c \
	tests/test_harness.c \
	tests/test_client_stubs.c \
	tests/test_server_net.c \
	tests/test_jass.c \
	tests/test_tool_common.c \
	$(shell find tests -maxdepth 1 -name 'test_ui_*.c' | sort)

test: test-assets $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -o $(BIN_DIR)/test_openwarcraft3$(EXE_EXT) \
		$(TEST_SRCS) $(TEST_GAME_SRCS) \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lm
	$(BIN_DIR)/test_openwarcraft3$(EXE_EXT)
	$(MAKE) test-wow-appearance
	$(MAKE) test-ui

test-jass: $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -o $(BIN_DIR)/test_jass$(EXE_EXT) \
		tests/test_jass_main.c tests/test_jass.c tests/test_harness.c tests/test_client_stubs.c \
		game/g_metadata.c common/msg.c \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lm
	$(BIN_DIR)/test_jass$(EXE_EXT)

test-wow-appearance: | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -DWOW -o $(BIN_DIR)/test_wow_appearance$(EXE_EXT) \
		tests/test_wow_appearance.c common/msg.c common/net.c \
		$(shell find shared -name '*.c') -lm
	$(BIN_DIR)/test_wow_appearance$(EXE_EXT)

test-ui: test-assets $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) | $(BIN_DIR)
	$(CC) $(TEST_UI_CFLAGS) -o $(BIN_DIR)/test_openwarcraft3_ui$(EXE_EXT) \
		$(TEST_UI_SRCS) $(TEST_GAME_SRCS) \
		$(shell find ui -name '*.c' | sort) \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lm
	$(BIN_DIR)/test_openwarcraft3_ui$(EXE_EXT)

test-mpq-compat: mpqtool $(MPQ_TEST)
	$(MPQ_TEST) -mpq=$(MPQ)

# ---------------------------------------------------------------------------
# test-assets — generate fixture resources and pack them into build/tests/tests.mpq
#
# Generated files (never committed):
#   build/tests/resources/TestUI/Textures/*.blp  — via blpgen
#   build/tests/resources/TestUI/Models/*.mdx    — via mdxgen
# Source-controlled FDF fixtures (tests/resources-src/**/*.fdf) are packed directly.
# The archive is recreated from scratch on every run (deterministic).
# ---------------------------------------------------------------------------
TESTS_DIR     := build/tests
TESTS_MPQ     := $(TESTS_DIR)/tests.mpq
TESTS_RES_DIR := $(TESTS_DIR)/resources
TESTS_SRC_DIR := tests/resources-src

test-assets: blpgen mdxgen mpqtool mdxtool | $(TESTS_DIR)
	@echo "[test-assets] generating textures"
	@mkdir -p $(TESTS_RES_DIR)/TestUI/Textures
	$(BIN_DIR)/blpgen$(EXE_EXT) solid        1  1  ffffffff  $(TESTS_RES_DIR)/TestUI/Textures/solid_white.blp
	$(BIN_DIR)/blpgen$(EXE_EXT) checker      8  8  2         $(TESTS_RES_DIR)/TestUI/Textures/checker_8x8.blp
	$(BIN_DIR)/blpgen$(EXE_EXT) alpha_ring   16 16           $(TESTS_RES_DIR)/TestUI/Textures/alpha_ring_16x16.blp
	$(BIN_DIR)/blpgen$(EXE_EXT) panel_border 32 32 2         $(TESTS_RES_DIR)/TestUI/Textures/panel_border_32x32.blp
	$(BIN_DIR)/blpgen$(EXE_EXT) paletted     8  8  2         $(TESTS_RES_DIR)/TestUI/Textures/paletted_checker_8x8.blp
	@echo "[test-assets] generating models"
	@mkdir -p $(TESTS_RES_DIR)/TestUI/Models
	$(BIN_DIR)/mdxgen$(EXE_EXT) quad_sprite   TestUI/Textures/checker_8x8.blp       $(TESTS_RES_DIR)/TestUI/Models/quad_sprite.mdx
	$(BIN_DIR)/mdxgen$(EXE_EXT) panel_sprite  TestUI/Textures/solid_white.blp        $(TESTS_RES_DIR)/TestUI/Models/panel_sprite.mdx
	$(BIN_DIR)/mdxgen$(EXE_EXT) ui_panel      TestUI/Textures/solid_white.blp        $(TESTS_RES_DIR)/TestUI/Models/ui_panel.mdx
	$(BIN_DIR)/mdxgen$(EXE_EXT) anim_pulse    TestUI/Textures/alpha_ring_16x16.blp   $(TESTS_RES_DIR)/TestUI/Models/anim_pulse.mdx
	@echo "[test-assets] packing tests.mpq"
	$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) pack \
		$(TESTS_RES_DIR)/TestUI/Textures/solid_white.blp        TestUI/Textures/solid_white.blp \
		$(TESTS_RES_DIR)/TestUI/Textures/checker_8x8.blp        TestUI/Textures/checker_8x8.blp \
		$(TESTS_RES_DIR)/TestUI/Textures/alpha_ring_16x16.blp   TestUI/Textures/alpha_ring_16x16.blp \
		$(TESTS_RES_DIR)/TestUI/Textures/panel_border_32x32.blp TestUI/Textures/panel_border_32x32.blp \
		$(TESTS_RES_DIR)/TestUI/Textures/paletted_checker_8x8.blp TestUI/Textures/paletted_checker_8x8.blp \
		$(TESTS_RES_DIR)/TestUI/Models/quad_sprite.mdx           TestUI/Models/quad_sprite.mdx \
		$(TESTS_RES_DIR)/TestUI/Models/panel_sprite.mdx          TestUI/Models/panel_sprite.mdx \
		$(TESTS_RES_DIR)/TestUI/Models/ui_panel.mdx              TestUI/Models/ui_panel.mdx \
		$(TESTS_RES_DIR)/TestUI/Models/anim_pulse.mdx            TestUI/Models/anim_pulse.mdx \
		$(TESTS_SRC_DIR)/TestUI/Frames/basic_layout.fdf          TestUI/Frames/basic_layout.fdf \
		$(TESTS_SRC_DIR)/TestUI/Frames/backdrop_variants.fdf     TestUI/Frames/backdrop_variants.fdf \
		$(TESTS_SRC_DIR)/TestUI/Frames/simple_sprite.fdf         TestUI/Frames/simple_sprite.fdf \
		$(TESTS_SRC_DIR)/TestUI/Frames/animated_sprite.fdf       TestUI/Frames/animated_sprite.fdf
	@echo "[test-assets] verifying archive"
	@$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) ls | grep -q "TestUI/" && echo "  ls OK"
	@$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) cat TestUI/Frames/basic_layout.fdf | grep -q "TestRootFrame" && echo "  cat FDF OK"
	@$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) cat TestUI/Textures/solid_white.blp | head -c4 | grep -q "BLP2" && echo "  cat BLP OK"
	@echo "[test-assets] done — $(TESTS_MPQ)"

$(TESTS_DIR):
	@mkdir -p $@

.PHONY: default build shared jass renderer game ui openwarcraft3 tools font $(TOOL_NAMES) run run-demo run-map run-ui-text diag clean download test test-jass test-wow-appearance test-ui test-mpq-compat test-assets
