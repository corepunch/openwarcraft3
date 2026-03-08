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

CMATH3_OBJS := $(patsubst src/cmath3/%.c,$(OBJ_DIR)/cmath3/%.o,$(shell find src/cmath3 -name '*.c'))
RENDERER_OBJS := $(patsubst src/renderer/%.c,$(OBJ_DIR)/renderer/%.o,$(shell find src/renderer -name '*.c'))
GAME_OBJS := $(patsubst src/game/%.c,$(OBJ_DIR)/game/%.o,$(shell find src/game -name '*.c'))
APP_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(wildcard src/client/*.c) $(wildcard src/server/*.c) $(wildcard src/common/*.c))

ifeq ($(shell uname -s),Linux)
	LDFLAGS += -Wl,-z,defs -lm -lEGL -lGL
endif

ifeq ($(shell uname -s),Darwin)
	ifeq ($(shell uname -m),arm64)
		HOMEBREW_PREFIX := /opt/homebrew
	else
		HOMEBREW_PREFIX := /usr/local
	endif
	CFLAGS += -DGL_SILENCE_DEPRECATION
	CFLAGS += -I$(HOMEBREW_PREFIX)/include
	LDFLAGS += -L$(HOMEBREW_PREFIX)/lib
	LDFLAGS += -framework AppKit -framework OpenGL
endif

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

$(LIB_DIR)/libcmath3.so: $(CMATH3_OBJS) $(LIB_DIR)
	$(CC) -shared -o $@ $(CMATH3_OBJS) $(LDFLAGS)

$(LIB_DIR)/libgame.so: $(GAME_OBJS) $(LIB_DIR)
	$(CC) -shared -o $@ $(GAME_OBJS) $(LDFLAGS) -lcmath3

$(LIB_DIR)/librenderer.so: cmath3 $(RENDERER_OBJS) $(LIB_DIR)
	$(CC) -shared -o $@ $(RENDERER_OBJS) $(LDFLAGS) -lcmath3 -lSDL2 -lstorm -ljpeg

$(BIN_DIR)/openwarcraft3: cmath3 game renderer $(APP_OBJS) $(BIN_DIR)
	$(CC) -o $@ $(APP_OBJS) -Wl,-rpath,'$$ORIGIN/../lib' $(LDFLAGS) -lcmath3 -lSDL2 -lstorm -lgame -lrenderer

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

# -----------------------------------------------------------------------
# Tests — compiled directly from source, no OpenGL/SDL2 dependency.
# -----------------------------------------------------------------------
CMATH3_SRCS := $(shell find src/cmath3/source -name '*.c')
TEST_CFLAGS  := -Wall -Isrc/cmath3/types -Isrc/cmath3

$(BIN_DIR)/test_cmath3: tests/test_cmath3.c $(CMATH3_SRCS) | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -o $@ tests/test_cmath3.c $(CMATH3_SRCS) -lm

$(BIN_DIR)/test_game_parser: tests/test_game_parser.c | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) -Isrc/common -Isrc/game -Itests/stubs \
	    -o $@ tests/test_game_parser.c -lm

test: $(BIN_DIR)/test_cmath3 $(BIN_DIR)/test_game_parser
	@echo "Running tests..."
	@$(BIN_DIR)/test_cmath3
	@$(BIN_DIR)/test_game_parser

.PHONY: default cmath3 renderer game openwarcraft3 clean download test
