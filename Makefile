# MPQ      := /Users/igor/Documents/Warcraft3/war3.mpq
MPQ      := data/Warcraft\ III/War3.mpq
MAP      := Maps\\Campaign\\Human02.w3m

ZIP_URL  := https://archive.org/download/warcraft-iii-installer-enus/Warcraft-III-1.29.2-enUS.zip
ZIP_FILE := Warcraft-III-1.29.2-enUS.zip
DATA_DIR := data

CC      := gcc
BIN_DIR := build/bin
LIB_DIR := build/lib
CFLAGS  := -Wall -Isrc -Isrc/cmath3 -Isrc/cmath3/types

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
    LIBS      := -lSDL2main -lSDL2 -lstorm -ljpeg -lm -lopengl32 -lgdi32
else
    UNAME_S := $(shell uname -s)
    EXE_EXT :=
    ifeq ($(UNAME_S),Darwin)
        ifeq ($(shell uname -m),arm64)
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
        CFLAGS    += -DGL_SILENCE_DEPRECATION -I$(HOMEBREW_PREFIX)/include
        LDFLAGS   := -L$(LIB_DIR) -L$(HOMEBREW_PREFIX)/lib
        LIBS      := -lSDL2 -lstorm -ljpeg -framework AppKit -framework OpenGL
    else
        # Linux
        LIB_EXT   := .so
        LIB_FLAGS := -shared -fPIC
        INSTALL_NAME =
        RPATH     := -Wl,-rpath,'$$ORIGIN/../lib'
        CFLAGS    += -fPIC
        LDFLAGS   := -L$(LIB_DIR) -Wl,-z,defs
        LIBS      := -lSDL2 -lstorm -ljpeg -lEGL -lGL -lm
    endif
endif

CMATH3_LIB   := $(LIB_DIR)/libcmath3$(LIB_EXT)
RENDERER_LIB := $(LIB_DIR)/librenderer$(LIB_EXT)
GAME_LIB     := $(LIB_DIR)/libgame$(LIB_EXT)
BINARY       := $(BIN_DIR)/openwarcraft3$(EXE_EXT)

# Unity-build helper: pipe all .c files in a directory tree as #include
# directives to gcc's stdin so the whole module is one translation unit.
# Note: awk with octal \043 for '#' avoids Make treating '#' in a variable
# value as a comment character, which would truncate the sed expression.
UNITY = find $1 -name '*.c' $2 | sort | awk '{printf "\043include \"%s\"\n", $$0}'

default: build
build: cmath3 renderer game openwarcraft3
cmath3:      $(CMATH3_LIB)
renderer:    $(RENDERER_LIB)
game:        $(GAME_LIB)
openwarcraft3: $(BINARY)
run:
	$(BINARY) -mpq=$(MPQ) -map=$(MAP)

$(BIN_DIR) $(LIB_DIR):
	@mkdir -p $@

# cmath3 — math library
$(CMATH3_LIB): $(shell find src/cmath3 -name '*.c') | $(LIB_DIR)
	@echo "[cmath3]"
	@$(call UNITY,src/cmath3) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lm

# renderer — depends on cmath3
$(RENDERER_LIB): $(CMATH3_LIB) $(shell find src/renderer -name '*.c') | $(LIB_DIR)
	@echo "[renderer]"
	@(echo '#define STB_TRUETYPE_IMPLEMENTATION'; \
	  echo '#include "renderer/stb/stb_truetype.h"'; \
	  echo '#undef STB_TRUETYPE_IMPLEMENTATION'; \
	  $(call UNITY,src/renderer,! -path '*/stb/*.c')) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lcmath3 $(LIBS)

# game — depends on cmath3
$(GAME_LIB): $(CMATH3_LIB) $(shell find src/game -name '*.c') | $(LIB_DIR)
	@echo "[game]"
	@$(call UNITY,src/game) | \
		$(CC) $(CFLAGS) $(LIB_FLAGS) $(INSTALL_NAME) -x c -o $@ - $(LDFLAGS) -lcmath3 -lm

# main binary — depends on all three libraries
APP_SRCS := $(shell find src/client src/server src/common -name '*.c')
$(BINARY): $(CMATH3_LIB) $(GAME_LIB) $(RENDERER_LIB) $(APP_SRCS) | $(BIN_DIR)
	@echo "[openwarcraft3]"
	@$(call UNITY,src/client src/server src/common) | \
		$(CC) $(CFLAGS) -x c -o $@ - $(RPATH) $(LDFLAGS) \
		-lcmath3 -lgame -lrenderer $(LIBS)

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
# (no renderer, no StormLib, no SDL2) together with the cmath3 sources.
# Global game state and gi function-pointers are provided by the test
# harness (tests/test_harness.c) rather than by src/game/g_main.c.
# ---------------------------------------------------------------------------
TEST_GAME_SRCS := \
	src/game/g_ai.c \
	src/game/g_events.c \
	src/game/g_metadata.c \
	src/game/g_monster.c \
	src/game/g_pathing.c \
	src/game/g_phys.c \
	src/game/g_utils.c \
	src/game/m_unit.c \
	src/game/parser.c \
	src/game/jass/vm_main.c \
	src/game/jass/jass_parser.c \
	src/game/skills/s_move.c \
	src/game/skills/s_skills.c \
	src/game/skills/s_stop.c \
	src/common/net.c \
	src/common/msg.c

TEST_SRCS := \
	tests/test_main.c \
	tests/test_harness.c \
	tests/test_slk.c \
	tests/test_unit.c \
	tests/test_movement.c \
	tests/test_collision.c \
	tests/test_net.c \
	tests/test_jass.c

TEST_CFLAGS := -Wall -Isrc/cmath3/types -Isrc/game -Isrc/server -Isrc/common -Isrc/game/skills

test: | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -o $(BIN_DIR)/test_openwarcraft3$(EXE_EXT) \
		$(TEST_SRCS) $(TEST_GAME_SRCS) \
		$(shell find src/cmath3 -name '*.c') -lm
	$(BIN_DIR)/test_openwarcraft3$(EXE_EXT)

.PHONY: default build cmath3 renderer game openwarcraft3 run clean download test
