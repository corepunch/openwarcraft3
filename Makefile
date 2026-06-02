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
WC3_DIR := games/warcraft-3
WC3_JASS_DIR := $(WC3_DIR)/jass
WC3_SHEET_DIR := $(WC3_DIR)/sheet
WOW_DIR := games/world-of-warcraft
SC2_DIR := games/starcraft-2

ifeq ($(DIAG_OUTPUT),1)
	CFLAGS += -DDIAG_OUTPUT
endif
ifeq ($(NO_NETWORK),1)
	CFLAGS += -DBZ_NO_NETWORK
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
RENDERER_SC2_LIB := $(LIB_DIR)/librenderer-sc2$(LIB_EXT)
GAME_WOW_LIB     := $(LIB_DIR)/libgame-wow$(LIB_EXT)
GAME_SC2_LIB     := $(LIB_DIR)/libgame-sc2$(LIB_EXT)
UI_WOW_LIB       := $(LIB_DIR)/libui-wow$(LIB_EXT)
BINARY       := $(BIN_DIR)/openwarcraft3$(EXE_EXT)
WOW_BINARY   := $(BIN_DIR)/openwow$(EXE_EXT)
SC2_BINARY   := $(BIN_DIR)/opensc2$(EXE_EXT)
MPQ_TEST         := $(BIN_DIR)/test_mpq_compat$(EXE_EXT)
WC3_CFLAGS   := $(CFLAGS) -I$(WC3_DIR)
WOW_CFLAGS   := $(CFLAGS) -I$(WOW_DIR) -DWOW -DOW3_LOAD_ALL_MPQS -Wno-unused-function
SC2_CFLAGS   := $(CFLAGS) -I$(SC2_DIR) -DSC2 -DOW3_LOAD_ALL_MPQS -Wno-unused-function

TOOL_SRCS := $(shell find tools -maxdepth 1 -name '*.c' ! -name 'jass.c' | sort)
TOOL_NAMES := $(patsubst tools/%.c,%,$(TOOL_SRCS))
TOOL_BINS := $(addprefix $(BIN_DIR)/,$(addsuffix $(EXE_EXT),$(TOOL_NAMES)))
JASS_BIN := $(BIN_DIR)/jass$(EXE_EXT)
TOOL_DEPS := $(shell find tools -maxdepth 1 -name '*.h' | sort)
CLIENT_HEADERS := $(shell find client -name '*.h' | sort)
UI_HEADERS := $(shell find $(WC3_DIR)/ui -name '*.h' | sort) client/ui.h
COMMON_HEADERS := $(shell find common -name '*.h' | sort)
WC3_COMMON_SRCS := $(shell find $(WC3_DIR)/common -name '*.c' 2>/dev/null | sort)
WOW_COMMON_SRCS := $(shell find $(WOW_DIR)/common -name '*.c' 2>/dev/null | sort)
SC2_COMMON_SRCS := $(shell find $(SC2_DIR)/common -name '*.c' 2>/dev/null | sort)
WORLD_CORE_SRCS := common/world.c common/routing.c
FONT_SRC := renderer/conchars.pcx
FONT_HEADER := renderer/conchars_sysfont.h
FONT_SYMBOL := conchars_sysfont_pcx

# Unity-build helper: pipe all .c files in a directory tree as #include
# directives to gcc's stdin so the whole module is one translation unit.
# Note: awk with octal \043 for '#' avoids Make treating '#' in a variable
# value as a comment character, which would truncate the sed expression.
UNITY = find $1 -name '*.c' $2 | sort | awk '{printf "\043include \"%s\"\n", $$0}'
CSRC = $(shell find $(1) -name '*.c' $(2) | sort)

define unity_lib_schema
$(1): $(2) | $$(LIB_DIR)
	@echo "[$(3)]"
	@$$(call UNITY,$(4),$(5)) | \
		$$(CC) $(6) $$(LIB_FLAGS) $$(INSTALL_NAME) -x c -o $$@ - $(7) $$(LDFLAGS) $(8)
endef

define src_lib_schema
$(1): $(2) | $$(LIB_DIR)
	@echo "[$(3)]"
	@$$(CC) $(4) $$(LIB_FLAGS) $$(INSTALL_NAME) -x c -o $$@ $(5) $$(LDFLAGS) $(6)
endef

define app_schema
$(1): $(2) | $$(BIN_DIR)
	@echo "[$(3)]"
	@$$(call UNITY,client server common,) | \
		$$(CC) $(4) -x c -o $$@ - $$(RPATH) $$(LDFLAGS) $(5)
endef

define test_schema
$(1): $(2) | $$(BIN_DIR)
	@$$(CC) $(3) -o $(4) $(5) $$(RPATH) $$(LDFLAGS) $(6)
	@$(4) $(7)
endef

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
renderer-sc2: $(RENDERER_SC2_LIB)
game-wow:     $(GAME_WOW_LIB)
ui-wow:       $(UI_WOW_LIB)
openwow:      $(WOW_BINARY)
game-sc2:     $(GAME_SC2_LIB)
opensc2:      $(SC2_BINARY)
tools:       $(TOOL_BINS)
font:       $(FONT_HEADER)
$(TOOL_NAMES): %: $(BIN_DIR)/%$(EXE_EXT)
run: $(BINARY)
	$(BINARY) -data $(WC3DATA)

run-demo: $(BINARY)
	$(BINARY) -data $(DEMODATA)

run-map: $(BINARY)
	$(BINARY) -data $(WC3DATA) +map "$(MAP)"

run-wow: $(WOW_BINARY)
	$(WOW_BINARY) -data data/world-of-warcraft/installed/Data +map World/Maps/Azeroth/Azeroth.wdt

build-run-wow: openwow
	$(WOW_BINARY) -data data/world-of-warcraft/installed/Data +map World/Maps/Azeroth/Azeroth.wdt

run-ui-text: $(BINARY)
	$(BINARY) -data $(WC3DATA) +r_module stdout +com_frame_limit 1 +$(UI_CMD)

diag: clean
	$(MAKE) DIAG_OUTPUT=1 build
	$(MAKE) DIAG_OUTPUT=1 run

$(BIN_DIR)/%$(EXE_EXT): tools/%.c $(TOOL_DEPS) $(CLIENT_HEADERS) $(COMMON_HEADERS) | $(BIN_DIR) $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) $(RENDERER_LIB) $(GAME_LIB) $(UI_LIB)
	@echo "[$*]"
	@$(CC) $(CFLAGS) -o $@ $< \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lrenderer -lgame -lui $(LIBS) -lm -lz

$(BIN_DIR)/img2sysfont$(EXE_EXT): tools/img2sysfont.c | $(BIN_DIR)
	@echo "[img2sysfont]"
	@$(CC) $(CFLAGS) -o $@ tools/img2sysfont.c

$(FONT_HEADER): $(FONT_SRC) $(BIN_DIR)/img2sysfont$(EXE_EXT)
	@$(BIN_DIR)/img2sysfont$(EXE_EXT) $(FONT_SRC) $(FONT_HEADER) $(FONT_SYMBOL)

# jass — standalone JASS interpreter (no renderer/game/SDL2 needed)
$(JASS_BIN): tools/jass.c $(TOOL_DEPS) | $(BIN_DIR) $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB)
	@echo "[jass-tool]"
	@$(CC) $(WC3_CFLAGS) -DTOOL_COMMON_NO_MPQ -o $@ tools/jass.c \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lm

$(MPQ_TEST): tests/test_mpq_compat.c common/mpq.c common/mpq.h | $(BIN_DIR)
	@echo "[mpq-compat-test]"
	@$(CC) $(CFLAGS) -o $@ tests/test_mpq_compat.c common/mpq.c -lm -lz

$(BIN_DIR) $(LIB_DIR):
	@mkdir -p $@

APP_SRCS := $(shell find client server common -name '*.c')
RENDERER_BASE_DEPS := $(SHARED_LIB) $(CLIENT_HEADERS) $(COMMON_HEADERS) common/mpq.c common/mpq.h $(FONT_HEADER)
RENDERER_SHARED_LIBS := -lshared $(LIBS) -lz
GAME_BASE_DEPS := $(SHARED_LIB) $(COMMON_HEADERS) common/mpq.c common/mpq.h
UI_BASE_DEPS := $(SHARED_LIB) $(CLIENT_HEADERS) $(COMMON_HEADERS)

$(eval $(call unity_lib_schema,$(SHARED_LIB),$(call CSRC,shared),shared,shared,,$(CFLAGS),,-lm))
$(eval $(call unity_lib_schema,$(JASS_LIB),$(SHARED_LIB) $(shell find $(WC3_JASS_DIR) -name '*.c' -o -name '*.h'),jass,$(WC3_JASS_DIR),,$(WC3_CFLAGS),,-lshared -lm))
$(eval $(call src_lib_schema,$(SHEET_LIB),$(WC3_SHEET_DIR)/parser.c $(WC3_SHEET_DIR)/sheet.c common/common.h,sheet,$(CFLAGS),$(WC3_SHEET_DIR)/parser.c $(WC3_SHEET_DIR)/sheet.c,))

$(eval $(call unity_lib_schema,$(RENDERER_LIB),$(RENDERER_BASE_DEPS) $(call CSRC,renderer $(WC3_DIR)/renderer),renderer,renderer $(WC3_DIR)/renderer,,$(WC3_CFLAGS),common/mpq.c,$(RENDERER_SHARED_LIBS)))
$(eval $(call unity_lib_schema,$(RENDERER_WOW_LIB),$(RENDERER_BASE_DEPS) $(call CSRC,renderer $(WOW_DIR)/renderer),renderer-wow,renderer $(WOW_DIR)/renderer,,$(WOW_CFLAGS),common/mpq.c,$(RENDERER_SHARED_LIBS)))
$(eval $(call unity_lib_schema,$(RENDERER_SC2_LIB),$(RENDERER_BASE_DEPS) $(call CSRC,renderer $(SC2_DIR)/renderer),renderer-sc2,renderer $(SC2_DIR)/renderer,,$(SC2_CFLAGS),common/mpq.c,$(RENDERER_SHARED_LIBS)))

$(eval $(call unity_lib_schema,$(GAME_LIB),$(GAME_BASE_DEPS) $(JASS_LIB) $(SHEET_LIB) $(WORLD_CORE_SRCS) $(WC3_COMMON_SRCS) $(call CSRC,$(WC3_DIR)/game),game,$(WC3_DIR)/game,,$(WC3_CFLAGS),common/mpq.c,-lsheet -lshared -ljass $(LIBS) -lm -lz))
$(eval $(call unity_lib_schema,$(GAME_WOW_LIB),$(GAME_BASE_DEPS) common/world.c $(WOW_COMMON_SRCS) $(call CSRC,$(WOW_DIR)/game),game-wow,$(WOW_DIR)/game,,$(WOW_CFLAGS),common/mpq.c,-lshared $(LIBS) -lm -lz))
$(eval $(call unity_lib_schema,$(GAME_SC2_LIB),$(GAME_BASE_DEPS) $(WORLD_CORE_SRCS) $(SC2_COMMON_SRCS) $(call CSRC,$(SC2_DIR)/game),game-sc2,$(SC2_DIR)/game,,$(SC2_CFLAGS),common/mpq.c,-lshared $(LIBS) -lm -lz))

$(eval $(call unity_lib_schema,$(UI_LIB),$(UI_BASE_DEPS) $(UI_HEADERS) $(call CSRC,$(WC3_DIR)/ui),ui,$(WC3_DIR)/ui,,$(WC3_CFLAGS),,-lshared -lm))
$(eval $(call unity_lib_schema,$(UI_WOW_LIB),$(UI_BASE_DEPS) client/ui.h $(call CSRC,$(WOW_DIR)/ui),ui-wow,$(WOW_DIR)/ui,,$(WOW_CFLAGS),,-lshared -lm))

$(eval $(call app_schema,$(BINARY),$(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) $(GAME_LIB) $(RENDERER_LIB) $(UI_LIB) $(APP_SRCS) $(CLIENT_HEADERS) $(COMMON_HEADERS),openwarcraft3,$(WC3_CFLAGS),-lsheet -lshared -ljass -lgame -lrenderer -lui $(LIBS) -lz))
$(eval $(call app_schema,$(WOW_BINARY),$(SHARED_LIB) $(SHEET_LIB) $(GAME_WOW_LIB) $(RENDERER_WOW_LIB) $(UI_WOW_LIB) $(APP_SRCS) $(CLIENT_HEADERS) $(COMMON_HEADERS),openwow,$(WOW_CFLAGS),-lsheet -lshared -lgame-wow -lrenderer-wow -lui-wow $(LIBS) -lz))
$(eval $(call app_schema,$(SC2_BINARY),$(SHARED_LIB) $(SHEET_LIB) $(GAME_SC2_LIB) $(RENDERER_SC2_LIB) $(UI_LIB) $(APP_SRCS) $(CLIENT_HEADERS),opensc2,$(SC2_CFLAGS),-lsheet -lshared -lgame-sc2 -lrenderer-sc2 -lui $(LIBS) -lz))

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
# harness ($(WC3_DIR)/tests/test_harness.c) rather than by games/warcraft-3/game/g_main.c.
# ---------------------------------------------------------------------------
WC3_TEST_DIR := $(WC3_DIR)/tests

TEST_GAME_SRCS := \
	common/routing.c \
	$(WC3_DIR)/game/g_ai.c \
	$(WC3_DIR)/game/g_events.c \
	$(WC3_DIR)/game/g_fow.c \
	$(WC3_DIR)/game/g_metadata.c \
	$(WC3_DIR)/game/g_model.c \
	$(WC3_DIR)/game/g_monster.c \
	$(WC3_DIR)/game/g_pathing.c \
	$(WC3_DIR)/game/g_phys.c \
	$(WC3_DIR)/game/g_utils.c \
	$(WC3_DIR)/game/m_unit.c \
	$(WC3_DIR)/game/skills/s_attack.c \
	$(WC3_DIR)/game/skills/s_area_spell.c \
	$(WC3_DIR)/game/skills/s_move.c \
	$(WC3_DIR)/game/skills/s_item.c \
	$(WC3_DIR)/game/skills/s_skills.c \
	$(WC3_DIR)/game/skills/s_spell.c \
	$(WC3_DIR)/game/skills/s_stop.c \
	$(WC3_DIR)/game/skills/s_summon.c \
	$(WC3_DIR)/game/skills/s_thunderbolt.c \
	$(WC3_DIR)/game/skills/s_train.c \
	$(WC3_DIR)/game/skills/s_utility_abilities.c \
	$(WC3_DIR)/game/skills/s_ability_stubs.c \
	client/cl_layout.c \
	client/cl_parse.c \
	client/cl_scrn.c \
	server/sv_init.c \
	server/sv_lan.c \
	server/sv_lobby.c \
	server/sv_send.c \
	server/sv_main.c \
	common/net.c \
	common/msg.c

TEST_SRCS := \
	$(shell find $(WC3_TEST_DIR) -maxdepth 1 -name 'test_*.c' \
	! -name 'test_commands.c' \
	! -name 'test_commands_main.c' \
	! -name 'test_jass_main.c' \
	! -name 'test_main_ui.c' \
	! -name 'test_ui_*.c' | sort) \
	tests/test_net.c \
	tests/test_tool_common.c

TEST_CFLAGS := $(WC3_CFLAGS) -DTOOL_COMMON_NO_MPQ -Itests -I$(WC3_TEST_DIR) -Ishared/types -I$(WC3_DIR)/game -Iserver -Icommon -Iclient -I$(WC3_DIR)/game/skills
TEST_UI_CFLAGS := $(TEST_CFLAGS)

TEST_UI_SRCS := \
	$(WC3_TEST_DIR)/test_main_ui.c \
	$(WC3_TEST_DIR)/test_harness.c \
	$(WC3_TEST_DIR)/test_client_stubs.c \
	$(WC3_TEST_DIR)/test_server_net.c \
	$(WC3_TEST_DIR)/test_jass.c \
	tests/test_tool_common.c \
	$(shell find $(WC3_TEST_DIR) -maxdepth 1 -name 'test_ui_*.c' \
		! -name 'test_ui_e2e.c' \
		! -name 'test_ui_oracle.c' \
		! -name 'test_ui_serialize.c' | sort)

test: test-assets $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) | $(BIN_DIR)
	@$(CC) $(TEST_CFLAGS) -o $(BIN_DIR)/test_openwarcraft3$(EXE_EXT) \
		$(TEST_SRCS) $(TEST_GAME_SRCS) \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lm
	@$(BIN_DIR)/test_openwarcraft3$(EXE_EXT)
	@$(MAKE) test-commands
	@$(MAKE) test-wow-appearance
	@$(MAKE) test-wow-combat
	@$(MAKE) test-ui

$(eval $(call test_schema,test-commands,test-assets $(SHARED_LIB) $(SHEET_LIB),$(TEST_CFLAGS),$(BIN_DIR)/test_commands$(EXE_EXT),$(WC3_TEST_DIR)/test_commands_main.c $(WC3_TEST_DIR)/test_commands.c common/common.c common/cmd.c common/cvar.c common/msg.c common/net.c common/mpq.c,-lsheet -lshared -lm -lz,))
$(eval $(call test_schema,test-jass,$(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB),$(TEST_CFLAGS),$(BIN_DIR)/test_jass$(EXE_EXT),$(WC3_TEST_DIR)/test_jass_main.c $(WC3_TEST_DIR)/test_jass.c $(WC3_TEST_DIR)/test_harness.c $(WC3_TEST_DIR)/test_client_stubs.c $(WC3_DIR)/game/g_metadata.c common/msg.c,-lsheet -lshared -ljass -lm,))
$(eval $(call test_schema,test-wow-appearance,,$(TEST_CFLAGS) -DWOW,$(BIN_DIR)/test_wow_appearance$(EXE_EXT),tests/test_wow_appearance.c common/msg.c common/net.c $(call CSRC,shared),-lm,))
$(eval $(call test_schema,test-wow-combat,,$(TEST_CFLAGS) -DWOW,$(BIN_DIR)/test_wow_combat$(EXE_EXT),tests/test_wow_combat.c $(WOW_DIR)/game/g_ai.c $(call CSRC,shared),-lm,))
$(eval $(call test_schema,test-ui,test-assets $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB),$(TEST_UI_CFLAGS),$(BIN_DIR)/test_openwarcraft3_ui$(EXE_EXT),$(TEST_UI_SRCS) $(TEST_GAME_SRCS) common/mpq.c $(call CSRC,$(WC3_DIR)/ui),-lsheet -lshared -ljass -lm -lz,))

test-mpq-compat: mpqtool $(MPQ_TEST)
	@$(MPQ_TEST) -mpq=$(MPQ)

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
	@for tex in \
		"solid 1 1 ffffffff $(TESTS_RES_DIR)/TestUI/Textures/solid_white.blp" \
		"checker 8 8 2 $(TESTS_RES_DIR)/TestUI/Textures/checker_8x8.blp" \
		"alpha_ring 16 16 $(TESTS_RES_DIR)/TestUI/Textures/alpha_ring_16x16.blp" \
		"panel_border 32 32 2 $(TESTS_RES_DIR)/TestUI/Textures/panel_border_32x32.blp" \
		"paletted 8 8 2 $(TESTS_RES_DIR)/TestUI/Textures/paletted_checker_8x8.blp"; do \
		$(BIN_DIR)/blpgen$(EXE_EXT) $$tex; \
	done
	@echo "[test-assets] generating models"
	@mkdir -p $(TESTS_RES_DIR)/TestUI/Models
	@for model in \
		"quad_sprite TestUI/Textures/checker_8x8.blp $(TESTS_RES_DIR)/TestUI/Models/quad_sprite.mdx" \
		"panel_sprite TestUI/Textures/solid_white.blp $(TESTS_RES_DIR)/TestUI/Models/panel_sprite.mdx" \
		"ui_panel TestUI/Textures/solid_white.blp $(TESTS_RES_DIR)/TestUI/Models/ui_panel.mdx" \
		"anim_pulse TestUI/Textures/alpha_ring_16x16.blp $(TESTS_RES_DIR)/TestUI/Models/anim_pulse.mdx"; do \
		$(BIN_DIR)/mdxgen$(EXE_EXT) $$model; \
	done
	@echo "[test-assets] packing tests.mpq"
	@set --; \
	for f in $$(find $(TESTS_RES_DIR) -type f | sort); do \
		rel=$${f#$(TESTS_RES_DIR)/}; set -- "$$@" "$$f" "$$rel"; \
	done; \
	for f in $$(find $(TESTS_SRC_DIR) -type f | sort); do \
		rel=$${f#$(TESTS_SRC_DIR)/}; arc=$$rel; \
		case "$$rel" in TestUI/FrameDef/*|TestUI/CampaignStrings*) arc=UI/$${rel#TestUI/};; esac; \
		set -- "$$@" "$$f" "$$arc"; \
	done; \
	$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) pack "$$@"
	@echo "[test-assets] verifying archive"
	@$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) ls | grep -q "TestUI/" && echo "  ls OK"
	@$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) cat Maps/Campaign/Human02.w3m | grep -q "Human02" && echo "  cat map OK"
	@$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) cat TestUI/Frames/basic_layout.fdf | grep -q "TestRootFrame" && echo "  cat FDF OK"
	@$(BIN_DIR)/mpqtool$(EXE_EXT) -mpq $(TESTS_MPQ) cat TestUI/Textures/solid_white.blp | head -c4 | grep -q "BLP2" && echo "  cat BLP OK"
	@echo "[test-assets] done — $(TESTS_MPQ)"

$(TESTS_DIR):
	@mkdir -p $@

.PHONY: default build shared jass renderer game ui openwarcraft3 tools font $(TOOL_NAMES) run run-demo run-map run-ui-text diag clean download test test-jass test-wow-appearance test-wow-combat test-ui test-mpq-compat test-assets renderer-sc2 game-sc2 opensc2
