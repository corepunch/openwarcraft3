ZIP_URL  := https://archive.org/download/warcraft-iii-installer-enus/Warcraft-III-1.29.2-enUS.zip
ZIP_FILE := Warcraft-III-1.29.2-enUS.zip
DATA_DIR := data

CC      := gcc
BIN_DIR := build/bin
LIB_DIR := build/lib
CFLAGS  := -Wall -I. -Ishared -Ishared/types
WOW_DIR := games/world-of-warcraft
WOW_TEST_DIR := $(WOW_DIR)/tests
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
RENDERER_WOW_LIB := $(LIB_DIR)/librenderer-wow$(LIB_EXT)
RENDERER_SC2_LIB := $(LIB_DIR)/librenderer-sc2$(LIB_EXT)
GAME_WOW_LIB     := $(LIB_DIR)/libgame-wow$(LIB_EXT)
GAME_SC2_LIB     := $(LIB_DIR)/libgame-sc2$(LIB_EXT)
UI_WOW_LIB       := $(LIB_DIR)/libui-wow$(LIB_EXT)
WOW_BINARY   := $(BIN_DIR)/openwow$(EXE_EXT)
SC2_BINARY   := $(BIN_DIR)/opensc2$(EXE_EXT)
WOW_CFLAGS   := $(CFLAGS) -I$(WOW_DIR) -DWOW -DOW3_LOAD_ALL_MPQS -Wno-unused-function
WOW_TEST_CFLAGS := $(WOW_CFLAGS) -DTOOL_COMMON_NO_MPQ -Itests
SC2_CFLAGS   := $(CFLAGS) -I$(SC2_DIR) -DSC2 -DOW3_LOAD_ALL_MPQS -Wno-unused-function

TOOL_SRCS := $(shell find tools -maxdepth 1 -name '*.c' ! -name 'jass.c' | sort)
TOOL_NAMES := $(patsubst tools/%.c,%,$(TOOL_SRCS))
TOOL_BINS := $(addprefix $(BIN_DIR)/,$(addsuffix $(EXE_EXT),$(TOOL_NAMES)))
TOOL_DEPS := $(shell find tools -maxdepth 1 -name '*.h' | sort)
CLIENT_HEADERS := $(shell find client -name '*.h' | sort)
COMMON_HEADERS := $(shell find common -name '*.h' | sort)
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
build:
shared:      $(SHARED_LIB)
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

run-wow: $(WOW_BINARY)
	$(WOW_BINARY) -data data/world-of-warcraft/installed/Data +map World/Maps/Azeroth/Azeroth.wdt

build-run-wow: openwow
	$(WOW_BINARY) -data data/world-of-warcraft/installed/Data +map World/Maps/Azeroth/Azeroth.wdt

diag: clean
	$(MAKE) DIAG_OUTPUT=1 build
	$(MAKE) DIAG_OUTPUT=1 run

$(BIN_DIR) $(LIB_DIR):
	@mkdir -p $@

APP_SRCS := $(shell find client server common -name '*.c')
RENDERER_BASE_DEPS := $(SHARED_LIB) $(CLIENT_HEADERS) $(COMMON_HEADERS) common/mpq.c common/mpq.h $(FONT_HEADER)
RENDERER_SHARED_LIBS := -lshared $(LIBS) -lz
GAME_BASE_DEPS := $(SHARED_LIB) $(COMMON_HEADERS) common/mpq.c common/mpq.h
UI_BASE_DEPS := $(SHARED_LIB) $(CLIENT_HEADERS) $(COMMON_HEADERS)

$(eval $(call unity_lib_schema,$(SHARED_LIB),$(call CSRC,shared),shared,shared,,$(CFLAGS),,-lm))

include games/warcraft-3/game.mk

$(BIN_DIR)/%$(EXE_EXT): tools/%.c $(TOOL_DEPS) $(CLIENT_HEADERS) $(COMMON_HEADERS) | $(BIN_DIR) $(SHARED_LIB) $(JASS_LIB) $(SHEET_LIB) $(RENDERER_LIB) $(GAME_LIB) $(UI_LIB)
	@echo "[$*]"
	@$(CC) $(CFLAGS) -o $@ $< \
		$(RPATH) $(LDFLAGS) -lsheet -lshared -ljass -lrenderer -lgame -lui $(LIBS) -lm -lz

$(BIN_DIR)/img2sysfont$(EXE_EXT): tools/img2sysfont.c | $(BIN_DIR)
	@echo "[img2sysfont]"
	@$(CC) $(CFLAGS) -o $@ tools/img2sysfont.c

$(FONT_HEADER): $(FONT_SRC) $(BIN_DIR)/img2sysfont$(EXE_EXT)
	@$(BIN_DIR)/img2sysfont$(EXE_EXT) $(FONT_SRC) $(FONT_HEADER) $(FONT_SYMBOL)

$(eval $(call unity_lib_schema,$(RENDERER_WOW_LIB),$(RENDERER_BASE_DEPS) $(call CSRC,renderer $(WOW_DIR)/renderer),renderer-wow,renderer $(WOW_DIR)/renderer,,$(WOW_CFLAGS),common/mpq.c,$(RENDERER_SHARED_LIBS)))
$(eval $(call unity_lib_schema,$(RENDERER_SC2_LIB),$(RENDERER_BASE_DEPS) $(call CSRC,renderer $(SC2_DIR)/renderer),renderer-sc2,renderer $(SC2_DIR)/renderer,,$(SC2_CFLAGS),common/mpq.c,$(RENDERER_SHARED_LIBS)))

$(eval $(call unity_lib_schema,$(GAME_WOW_LIB),$(GAME_BASE_DEPS) common/world.c $(WOW_COMMON_SRCS) $(call CSRC,$(WOW_DIR)/game),game-wow,$(WOW_DIR)/game,,$(WOW_CFLAGS),common/mpq.c,-lshared $(LIBS) -lm -lz))
$(eval $(call unity_lib_schema,$(GAME_SC2_LIB),$(GAME_BASE_DEPS) $(WORLD_CORE_SRCS) $(SC2_COMMON_SRCS) $(call CSRC,$(SC2_DIR)/game),game-sc2,$(SC2_DIR)/game,,$(SC2_CFLAGS),common/mpq.c,-lshared $(LIBS) -lm -lz))

$(eval $(call unity_lib_schema,$(UI_WOW_LIB),$(UI_BASE_DEPS) client/ui.h $(call CSRC,$(WOW_DIR)/ui),ui-wow,$(WOW_DIR)/ui,,$(WOW_CFLAGS),,-lshared -lm))

$(eval $(call app_schema,$(WOW_BINARY),$(SHARED_LIB) $(SHEET_LIB) $(GAME_WOW_LIB) $(RENDERER_WOW_LIB) $(UI_WOW_LIB) $(APP_SRCS) $(CLIENT_HEADERS) $(COMMON_HEADERS),openwow,$(WOW_CFLAGS),-lsheet -lshared -lgame-wow -lrenderer-wow -lui-wow $(LIBS) -lz))
$(eval $(call app_schema,$(SC2_BINARY),$(SHARED_LIB) $(SHEET_LIB) $(GAME_SC2_LIB) $(RENDERER_SC2_LIB) $(UI_LIB) $(APP_SRCS) $(CLIENT_HEADERS),opensc2,$(SC2_CFLAGS),-lsheet -lshared -lgame-sc2 -lrenderer-sc2 -lui $(LIBS) -lz))

download: $(ZIP_FILE)
	mkdir -p $(DATA_DIR)
	unzip -o $(ZIP_FILE) -d $(DATA_DIR)

$(ZIP_FILE):
	curl -L -o $(ZIP_FILE) $(ZIP_URL)

clean:
	rm -rf build

$(eval $(call test_schema,test-wow-appearance,,$(WOW_TEST_CFLAGS),$(BIN_DIR)/test_wow_appearance$(EXE_EXT),$(WOW_TEST_DIR)/test_wow_appearance.c common/msg.c common/net.c $(call CSRC,shared),-lm,))
$(eval $(call test_schema,test-wow-combat,,$(WOW_TEST_CFLAGS),$(BIN_DIR)/test_wow_combat$(EXE_EXT),$(WOW_TEST_DIR)/test_wow_combat.c $(WOW_DIR)/game/g_ai.c $(call CSRC,shared),-lm,))
$(eval $(call test_schema,test-wow-game,,$(WOW_TEST_CFLAGS),$(BIN_DIR)/test_wow_game$(EXE_EXT),$(WOW_TEST_DIR)/test_wow_game.c $(WOW_DIR)/game/g_wow.c $(WOW_DIR)/game/g_world.c $(WOW_DIR)/game/g_ai.c $(WOW_DIR)/game/m_creature.c common/mpq.c $(call CSRC,shared),-lm -lz,))

.PHONY: default build shared tools font $(TOOL_NAMES) diag clean download renderer-wow game-wow ui-wow openwow renderer-sc2 game-sc2 opensc2 test-wow-appearance test-wow-combat test-wow-game $(WC3_PHONY)
