# MPQ      := /Users/igor/Documents/Warcraft3/war3.mpq
MPQ      := data/Warcraft\ III/War3.mpq
MAP      := Maps\\Campaign\\Human02.w3m

ZIP_URL  := https://archive.org/download/warcraft-iii-installer-enus/Warcraft-III-1.29.2-enUS.zip
ZIP_FILE := Warcraft-III-1.29.2-enUS.zip
DATA_DIR := data

CC      := gcc
BIN_DIR := build/bin
LIB_DIR := build/lib
CFLAGS  := -Wall -I. -Ishared -Ishared/types

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
        LIB_EXT   := .dylib
        LIB_FLAGS := -dynamiclib
        # Set the dylib install name to @rpath/<libname> so the executable
        # resolves it via the @executable_path/../lib rpath, not a build path.
        INSTALL_NAME = -Wl,-install_name,@rpath/$(notdir $@)
        RPATH     := -Wl,-rpath,@executable_path/../lib
		CFLAGS    += -DGL_SILENCE_DEPRECATION -I$(HOMEBREW_PREFIX)/include -arch $(ARCH)
		LDFLAGS   := -L$(LIB_DIR) -L$(HOMEBREW_PREFIX)/lib -arch $(ARCH)
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
MPQ_TOOL     := $(BIN_DIR)/mpqtool$(EXE_EXT)
MDX_TOOL     := $(BIN_DIR)/mdxtool$(EXE_EXT)
MPQ_TEST     := $(BIN_DIR)/test_mpq_compat$(EXE_EXT)

# Unity-build helper: pipe all .c files in a directory tree as #include
# directives to gcc's stdin so the whole module is one translation unit.
# Note: awk with octal \043 for '#' avoids Make treating '#' in a variable
# value as a comment character, which would truncate the sed expression.
UNITY = find $1 -name '*.c' $2 | sort | awk '{printf "\043include \"%s\"\n", $$0}'

default: build
build: shared renderer game openwarcraft3 mpqtool mdxtool
shared:      $(SHARED_LIB)
renderer:    $(RENDERER_LIB)
game:        $(GAME_LIB)
openwarcraft3: $(BINARY)
mpqtool:     $(MPQ_TOOL)
mdxtool:     $(MDX_TOOL)
run:
	$(BINARY) -mpq=$(MPQ)

run-map:
	$(BINARY) -mpq=$(MPQ) -map=$(MAP)

$(MPQ_TOOL): tools/mpqtool.c common/mpq.c common/mpq.h | $(BIN_DIR)
	@echo "[mpqtool]"
	$(CC) $(CFLAGS) -o $@ $< common/mpq.c $(LDFLAGS) -lm -lz

$(MDX_TOOL): tools/mdxtool.c common/mpq.c common/sheet.c common/parser.c | $(BIN_DIR) $(SHARED_LIB) $(RENDERER_LIB)
	@echo "[mdxtool]"
	$(CC) $(CFLAGS) -o $@ $< common/mpq.c common/sheet.c common/parser.c \
		$(RPATH) $(LDFLAGS) -lshared -lrenderer $(LIBS) -lm -lz

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
	server/sv_init.c \
	server/sv_send.c \
	common/net.c \
	common/msg.c

TEST_SRCS := \
	tests/test_main.c \
	tests/test_harness.c \
	tests/test_slk.c \
	tests/test_unit.c \
	tests/test_movement.c \
	tests/test_collision.c \
	tests/test_net.c \
	tests/test_jass.c \
	tests/test_api.c \
	tests/test_game.c \
	tests/test_combat.c \
	tests/test_server_net.c

TEST_CFLAGS := -Wall -Itests/stubs -Ishared/types -Igame -Iserver -Icommon -Igame/skills

test: | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -o $(BIN_DIR)/test_openwarcraft3$(EXE_EXT) \
		$(TEST_SRCS) $(TEST_GAME_SRCS) \
		$(shell find shared -name '*.c') -lm
	$(BIN_DIR)/test_openwarcraft3$(EXE_EXT)

test-mpq-compat: mpqtool $(MPQ_TEST)
	$(MPQ_TEST) -mpq=$(MPQ)

.PHONY: default build shared renderer game openwarcraft3 mpqtool mdxtool run run-map clean download test test-mpq-compat
