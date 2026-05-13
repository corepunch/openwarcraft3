# MPQ      := /Users/igor/Documents/Warcraft3/war3.mpq
MPQ      := data/Warcraft\ III/War3.mpq
DEMO     := data/Warcraft3demo/war3.mpq
MAP      := Maps\\Campaign\\Human02.w3m

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
	LIBS      := -lSDL2main -lSDL2 -ljpeg -lm -lopengl32 -lgdi32
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
				JPEG_PREFIX := $(shell brew --prefix jpeg 2>/dev/null)
        LIB_EXT   := .dylib
        LIB_FLAGS := -dynamiclib
        # Set the dylib install name to @rpath/<libname> so the executable
        # resolves it via the @executable_path/../lib rpath, not a build path.
        INSTALL_NAME = -Wl,-install_name,@rpath/$(notdir $@)
        RPATH     := -Wl,-rpath,@executable_path/../lib
		CFLAGS    += -DGL_SILENCE_DEPRECATION -I$(HOMEBREW_PREFIX)/include -arch $(ARCH)
		LDFLAGS   := -L$(LIB_DIR) -L$(HOMEBREW_PREFIX)/lib -arch $(ARCH)
		ifneq ($(JPEG_PREFIX),)
			CFLAGS  += -I$(JPEG_PREFIX)/include
			LDFLAGS += -L$(JPEG_PREFIX)/lib
		endif
		LIBS      := -lSDL2 -ljpeg -framework AppKit -framework OpenGL
    else
        # Linux
        LIB_EXT   := .so
        LIB_FLAGS := -shared -fPIC
        INSTALL_NAME =
        RPATH     := -Wl,-rpath,'$$ORIGIN/../lib'
        CFLAGS    += -fPIC
        LDFLAGS   := -L$(LIB_DIR) -Wl,-z,defs
		LIBS      := -lSDL2 -ljpeg -lEGL -lGL -lm
    endif
endif

SHARED_LIB   := $(LIB_DIR)/libshared$(LIB_EXT)
RENDERER_LIB := $(LIB_DIR)/librenderer$(LIB_EXT)
GAME_LIB     := $(LIB_DIR)/libgame$(LIB_EXT)
BINARY       := $(BIN_DIR)/openwarcraft3$(EXE_EXT)
MPQ_TEST         := $(BIN_DIR)/test_mpq_compat$(EXE_EXT)

TOOL_SRCS := $(shell find tools -maxdepth 1 -name '*.c' ! -name '*_common.c' | sort)
TOOL_NAMES := $(patsubst tools/%.c,%,$(TOOL_SRCS))
TOOL_BINS := $(addprefix $(BIN_DIR)/,$(addsuffix $(EXE_EXT),$(TOOL_NAMES)))
TOOL_HEADERS := tools/tool_common.h tools/tool_common.c tools/viewer_common.h tools/viewer_common.c

# Unity-build helper: pipe all .c files in a directory tree as #include
# directives to gcc's stdin so the whole module is one translation unit.
# Note: awk with octal \043 for '#' avoids Make treating '#' in a variable
# value as a comment character, which would truncate the sed expression.
UNITY = find $1 -name '*.c' $2 | sort | awk '{printf "\043include \"%s\"\n", $$0}'

default: build
build: shared renderer game openwarcraft3 tools
shared:      $(SHARED_LIB)
renderer:    $(RENDERER_LIB)
game:        $(GAME_LIB)
openwarcraft3: $(BINARY)
tools:       $(TOOL_BINS)
$(TOOL_NAMES): %: $(BIN_DIR)/%$(EXE_EXT)
run:
	$(BINARY) -mpq=$(MPQ)

run-demo:
	$(BINARY) -mpq=$(DEMO)

run-map:
	$(BINARY) -mpq=$(MPQ) -map=$(MAP)

diag: clean
	$(MAKE) DIAG_OUTPUT=1 build
	$(MAKE) DIAG_OUTPUT=1 run

$(BIN_DIR)/%$(EXE_EXT): tools/%.c $(TOOL_HEADERS) | $(BIN_DIR) $(SHARED_LIB) $(RENDERER_LIB) $(GAME_LIB)
	@echo "[$*]"
	$(CC) $(CFLAGS) -DTOOL_COMMON_IMPLEMENTATION -DVIEWER_COMMON_IMPLEMENTATION -o $@ $< \
		$(RPATH) $(LDFLAGS) -lshared -lrenderer -lgame $(LIBS) -lm -lz

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

# renderer — depends on shared
$(RENDERER_LIB): $(SHARED_LIB) $(shell find renderer -name '*.c') | $(LIB_DIR)
	@echo "[renderer]"
	@(echo '#define STB_TRUETYPE_IMPLEMENTATION'; \
	  echo '#include "renderer/stb/stb_truetype.h"'; \
	  echo '#undef STB_TRUETYPE_IMPLEMENTATION'; \
	  $(call UNITY,renderer,! -path '*/stb/*.c')) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - common/mpq.c $(LDFLAGS) -lshared $(LIBS) -lz

# game — depends on shared
$(GAME_LIB): $(SHARED_LIB) $(shell find game -name '*.c') | $(LIB_DIR)
	@echo "[game]"
	@$(call UNITY,game) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lshared -lm

# main binary — depends on all three libraries
APP_SRCS := $(shell find client server common -name '*.c')
$(BINARY): $(SHARED_LIB) $(GAME_LIB) $(RENDERER_LIB) $(APP_SRCS) | $(BIN_DIR)
	@echo "[openwarcraft3]"
	@$(call UNITY,client server common) | \
		$(CC) $(CFLAGS) -x c -o $@ - $(RPATH) $(LDFLAGS) \
		-lshared -lgame -lrenderer $(LIBS) -lz

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
	game/g_metadata.c \
	game/g_monster.c \
	game/g_pathing.c \
	game/g_phys.c \
	game/g_utils.c \
	game/m_unit.c \
	game/parser.c \
	game/jass/vm_main.c \
	game/jass/jass_parser.c \
	game/skills/s_attack.c \
	game/skills/s_move.c \
	game/skills/s_skills.c \
	game/skills/s_stop.c \
	game/skills/s_train.c \
	game/ui/ui_fdf.c \
	game/ui/ui_write.c \
	client/cl_scrn.c \
	server/sv_init.c \
	server/sv_send.c \
	common/net.c \
	common/msg.c

TEST_SRCS := \
	tests/test_main.c \
	tests/test_harness.c \
	tests/test_client_stubs.c \
	tests/test_slk.c \
	tests/test_tool_common.c \
	tests/test_unit.c \
	tests/test_movement.c \
	tests/test_collision.c \
	tests/test_net.c \
	tests/test_jass.c \
	tests/test_api.c \
	tests/test_game.c \
	tests/test_combat.c \
	tests/test_server_net.c \
	tests/test_ui_fdf.c \
	tests/test_ui_serialize.c \
	tests/test_ui_layout.c \
	tests/test_ui_e2e.c \
	tests/test_ui_oracle.c

TEST_CFLAGS := -Wall -DTOOL_COMMON_NO_MPQ -Itests/stubs -Ishared/types -Igame -Iserver -Icommon -Iclient -Igame/skills

TEST_UI_SRCS := \
	tests/test_main_ui.c \
	tests/test_harness.c \
	tests/test_client_stubs.c \
	tests/test_server_net.c \
	tests/test_jass.c \
	tests/test_tool_common.c \
	tests/test_ui_fdf.c \
	tests/test_ui_serialize.c \
	tests/test_ui_layout.c \
	tests/test_ui_e2e.c \
	tests/test_ui_oracle.c

test: test-assets | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -o $(BIN_DIR)/test_openwarcraft3$(EXE_EXT) \
		$(TEST_SRCS) $(TEST_GAME_SRCS) \
		$(shell find shared -name '*.c') tools/tool_common.c -lm
	$(BIN_DIR)/test_openwarcraft3$(EXE_EXT)

test-ui: test-assets | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -o $(BIN_DIR)/test_openwarcraft3_ui$(EXE_EXT) \
		$(TEST_UI_SRCS) $(TEST_GAME_SRCS) \
		$(shell find shared -name '*.c') tools/tool_common.c -lm
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

test-assets: blpgen mdxgen mpqtool | $(TESTS_DIR)
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

.PHONY: default build shared renderer game openwarcraft3 tools $(TOOL_NAMES) run run-map diag clean download test test-ui test-mpq-compat test-assets
