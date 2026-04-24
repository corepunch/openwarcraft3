# MPQ      := /Users/igor/Documents/Warcraft3/war3.mpq
MPQ      := data/Warcraft\ III/War3.mpq
MAP      := Maps\\Campaign\\Human02.w3m

ZIP_URL  := https://archive.org/download/warcraft-iii-installer-enus/Warcraft-III-1.29.2-enUS.zip
ZIP_FILE := Warcraft-III-1.29.2-enUS.zip
DATA_DIR := data

CC       := gcc
BIN_DIR  := build/bin
LIB_DIR  := build/lib
OBJ_DIR  := build/obj
CFLAGS   := -Wall -fPIC -Isrc/cmath3/types
LDFLAGS  := -L$(LIB_DIR)

# Orion-UI submodule paths
ORION_DIR    := vendor/orion-ui
PLATFORM_DIR := $(ORION_DIR)/platform
ORION_LIB    := $(LIB_DIR)/liborion.a

CMATH3_OBJS   := $(patsubst src/cmath3/%.c,$(OBJ_DIR)/cmath3/%.o,$(shell find src/cmath3 -name '*.c'))
RENDERER_OBJS := $(patsubst src/renderer/%.c,$(OBJ_DIR)/renderer/%.o,$(shell find src/renderer -name '*.c'))
GAME_OBJS     := $(patsubst src/game/%.c,$(OBJ_DIR)/game/%.o,$(shell find src/game -name '*.c'))
APP_OBJS      := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(wildcard src/client/*.c) $(wildcard src/server/*.c) $(wildcard src/common/*.c) $(wildcard src/ui/*.c))

ifeq ($(shell uname -s),Linux)
	PLATFORM_LIB     := $(LIB_DIR)/libplatform.so
	PLATFORM_LDFLAGS := $(shell pkg-config --libs x11 egl gl 2>/dev/null || echo "-lX11 -lEGL -lGL")
	PLATFORM_CFLAGS  := $(shell pkg-config --cflags x11 egl gl 2>/dev/null)
	LDFLAGS          += -Wl,-z,defs -lm $(PLATFORM_LDFLAGS)
	PLATFORM_LIB_EXT := so
endif

ifeq ($(shell uname -s),Darwin)
	ifeq ($(shell uname -m),arm64)
		HOMEBREW_PREFIX := /opt/homebrew
	else
		HOMEBREW_PREFIX := /usr/local
	endif
	PLATFORM_LIB     := $(LIB_DIR)/libplatform.dylib
	PLATFORM_LDFLAGS := -framework AppKit -framework OpenGL
	PLATFORM_CFLAGS  := -I$(HOMEBREW_PREFIX)/include
	CFLAGS           += -DGL_SILENCE_DEPRECATION
	CFLAGS           += -I$(HOMEBREW_PREFIX)/include
	LDFLAGS          += -L$(HOMEBREW_PREFIX)/lib $(PLATFORM_LDFLAGS)
	PLATFORM_LIB_EXT := dylib
endif

# Compile flags used when building the orion-ui unity object
ORION_CFLAGS := -std=c11 -Wall -Wextra -fPIC \
                -I$(ORION_DIR) -I$(PLATFORM_DIR) \
                -DGL_SILENCE_DEPRECATION -D_DEFAULT_SOURCE \
                -DUI_WINDOW_SCALE=1 \
                -Wno-unused-parameter \
                $(PLATFORM_CFLAGS) \
                $(shell pkg-config --cflags cglm 2>/dev/null)

# Extra include paths for game source files that use orion-ui headers
CFLAGS += -I$(ORION_DIR) -I$(PLATFORM_DIR) -DUI_WINDOW_SCALE=1

default: build
build: cmath3 renderer game openwarcraft3
cmath3: $(LIB_DIR)/libcmath3.so
renderer: $(LIB_DIR)/librenderer.so
game: $(LIB_DIR)/libgame.so
openwarcraft3: $(BIN_DIR)/openwarcraft3
run: 
	build/bin/openwarcraft3 -mpq=$(MPQ) -map=$(MAP)

$(LIB_DIR):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

# ── Build platform shared library using its own Makefile ──────────────────
$(PLATFORM_LIB): | $(LIB_DIR)
	$(MAKE) -C $(PLATFORM_DIR) OUTDIR=$(abspath $(LIB_DIR))

# ── Build orion-ui static library (unity build) ───────────────────────────
$(ORION_LIB): $(PLATFORM_LIB) | $(LIB_DIR) $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)
	find $(ORION_DIR)/user $(ORION_DIR)/kernel $(ORION_DIR)/commctl \
	     -name "*.c" | sort | sed 's|.*|#include "&"|' | \
	$(CC) $(ORION_CFLAGS) -x c -c -o $(OBJ_DIR)/liborion_unity.o -
	$(AR) rcs $@ $(OBJ_DIR)/liborion_unity.o

$(LIB_DIR)/libcmath3.so: $(CMATH3_OBJS) $(LIB_DIR)
	$(CC) -shared -o $@ $(CMATH3_OBJS) $(LDFLAGS)

$(LIB_DIR)/libgame.so: $(GAME_OBJS) $(LIB_DIR)
	$(CC) -shared -o $@ $(GAME_OBJS) $(LDFLAGS) -lcmath3

$(LIB_DIR)/librenderer.so: cmath3 $(RENDERER_OBJS) $(LIB_DIR)
	$(CC) -shared -o $@ $(RENDERER_OBJS) $(LDFLAGS) -lcmath3 -lstorm -ljpeg

$(BIN_DIR)/openwarcraft3: cmath3 game renderer $(ORION_LIB) $(APP_OBJS) $(BIN_DIR)
	$(CC) -o $@ $(APP_OBJS) \
	      -Wl,-rpath,'$$ORIGIN/../lib' $(LDFLAGS) \
	      $(ORION_LIB) -lplatform \
	      -lcmath3 -lstorm -lgame -lrenderer

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

download: $(ZIP_FILE)
	mkdir -p $(DATA_DIR)
	unzip -o $(ZIP_FILE) -d $(DATA_DIR)
	
$(ZIP_FILE):
	curl -L -o $(ZIP_FILE) $(ZIP_URL)

clean:
	rm -rf build/obj build/lib

# ---------------------------------------------------------------------------
# Test target — builds and runs the unit test binary.
#
# The test binary compiles only the game modules needed by the tests
# (no renderer, no StormLib, no SDL2) together with the cmath3 objects.
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
	tests/test_net.c

TEST_CFLAGS := -Wall -Isrc/cmath3/types -Isrc/game -Isrc/server -Isrc/common -Isrc/game/skills

test: $(CMATH3_OBJS) $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -o $(BIN_DIR)/test_openwarcraft3 \
		$(TEST_SRCS) $(TEST_GAME_SRCS) $(CMATH3_OBJS) -lm
	$(BIN_DIR)/test_openwarcraft3

.PHONY: default cmath3 renderer game openwarcraft3 clean download test
