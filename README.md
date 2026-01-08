# openwarcraft3

OpenWarcraft3 is an open-source implementation of Warcraft III that uses SDL2 and runs on Linux and macOS. 

It was developed using War3.mpq from Warcraft III v1.0 as reference, with ongoing support for version 1.29b.

<a href="https://youtu.be/vg7Jm046vcI">Check the video on YouTube</a> or see screenshots below.

<img width="1024" src="https://github.com/user-attachments/assets/c76a93af-1801-402e-83bc-b3e0a4462312" />

<img width="1024" src="https://github.com/corepunch/openwarcraft3/assets/83646194/643c7aa7-2b91-469c-857e-0f6910c939af">

<img width="1024" src="https://github.com/corepunch/openwarcraft3/assets/83646194/a79e447d-e42c-4468-b4ca-3d212efe346a">

## Cloning

```bash
git clone --recurse-submodules git@github.com:corepunch/openwarcraft3.git
```

## Create project using premake5

```bash
tools\bin\windows\premake5.exe vs2022
```

Or mac

```bash
tools/bin/darwin/premake5 xcode4
```

or

```bash
tools/bin/darwin/premake5 --cc=clang gmake
```

The project files will be created in the build folder.

---

## Or build using Makefile

You can also build and run the project using the included Makefile on Linux and macOS.

### Build

```bash
make build
```

This will compile all libraries (`cmath3`, `renderer`, `game`) and the `openwarcraft3` executable.

### Download Warcraft 3 v1.29b installation

```bash
make download
```

This will download 1.2Gb file from `archive.org` into the `data` folder.
This is optional, otherwise modify $(MPQ) in the Makefile to run desired `War3.mpq` file.

### Run

```bash
make run
```

This will run the built `openwarcraft3` executable from the `build/bin` folder.

---

### Dependencies

The Makefile build requires the following libraries installed on your system:

* **StormLib** (or **libmpq** on OpenBSD) — for MPQ archive support
* **SDL2** — for windowing and input
* **libjpeg** — for JPEG image decoding

On macOS, these can be installed via [Homebrew](https://brew.sh/):

```bash
brew install sdl2 libjpeg stormlib
```

On Linux, use your distribution package manager, e.g., for Ubuntu:

```bash
sudo apt-get install libsdl2-dev libjpeg-dev libstorm-dev
```

On OpenBSD, install the packages using pkg_add:

```bash
doas pkg_add sdl2 libmpq jpeg
```

**Note:** OpenBSD support uses libmpq instead of StormLib, as StormLib is not well-supported on OpenBSD. The code automatically detects OpenBSD and uses the appropriate library.

---

# Project Architecture & Code Structure

## Directory Structure

The project is organized into the following main directories:

```
src/
├── client/         - Client-side code (input handling, rendering interface)
├── server/         - Server-side code (game state, entity management)
├── game/           - Core game logic library (runs on server)
├── renderer/       - Rendering engine library (client-side only)
├── cmath3/         - Mathematics library (shared)
├── common/         - Shared code and utilities
└── lib/            - Third-party libraries
```

## Core Components

### 1. **cmath3** Library (`src/cmath3/`)
Mathematical operations and utilities used across the entire project.

**Key Components:**
- **Vector Operations**: `vector2.c`, `vector3.c`, `vector4.c` - 2D, 3D, and 4D vector mathematics
- **Matrix Operations**: `matrix3.c`, `matrix4.c` - Transformation matrices
- **Geometric Primitives**: 
  - `box2.c`, `box3.c` - Bounding boxes for collision detection
  - `plane3.c`, `triangle3.c`, `line3.c` - Geometric shapes
  - `rect.c` - Rectangle operations
- **Quaternion**: `quaternion.c` - Rotation representation
- **Frustum**: `frustum3.c` - View frustum for culling

**Usage:** Shared between client and server for consistent mathematical calculations.

---

### 2. **renderer** Library (`src/renderer/`)
Client-side graphics rendering engine that displays the game state.

**Core Rendering:**
- `r_main.c` - Main rendering loop and initialization
- `r_draw.c` - Drawing primitives and utilities
- `r_ents.c` - Entity rendering
- `r_buffer.c` - Buffer management
- `r_shader.c` - Shader system
- `r_particles.c` - Particle effects system

**Texture and Image Support:**
- `r_texture.c` - Texture management
- `r_blp1.c`, `r_blp2.c` - Blizzard BLP format loaders
- `r_dds.c` - DDS texture format support

**Text Rendering:**
- `r_font.c` - Font rendering system
- `r_sysfont.c` - System font support

**Model Formats:**
- `mdx/` - Warcraft III MDX model format
  - `r_mdx_load.c` - MDX model loading
  - `r_mdx_render.c` - MDX model rendering
- `m3/` - StarCraft II M3 model format
  - `r_m3_load.c` - M3 model loading

**Map Rendering:**
- `w3m/` - Warcraft III map (W3M) rendering
  - `r_war3map.c` - Main map loader
  - `r_war3map_ground.c` - Terrain ground rendering
  - `r_war3map_cliffs.c` - Cliff rendering
  - `r_war3map_water.c` - Water rendering
  - `r_war3map_utils.c` - Map utilities

**Additional Systems:**
- `r_fogofwar.c` - Fog of war rendering

---

### 3. **game** Library (`src/game/`)
Server-side game logic and mechanics. All gameplay runs here.

**Core Game Systems:**
- `g_main.c` - Game initialization and main loop
- `g_spawn.c` - Entity spawning system
- `g_utils.c` - Game utilities
- `g_events.c` - Game event system
- `g_commands.c` - Command handling
- `g_save.c` - Save/load system

**Entity Systems:**
- `g_monster.c` - Monster/unit behavior
- `m_unit.c` - Unit management
- `m_tree.c` - Tree/resource entities

**Gameplay Mechanics:**
- `g_ai.c` - AI system
- `g_pathing.c` - Pathfinding
- `g_phys.c` - Physics simulation
- `g_items.c` - Item system

**Metadata and Data:**
- `g_metadata.c`, `g_metadata.h` - Game metadata
- `g_unitdata.h` - Unit data definitions
- `parser.c`, `parser.h` - Data file parsing

**JASS Scripting System** (`jass/`):
- `jass_parser.c` - JASS script parser
- `vm_main.c` - Virtual machine
- `vm_compiler.c` - Script compiler

**User Interface** (`ui/`):
- `ui_init.c` - UI initialization
- `ui_write.c` - UI state writing
- `ui_fdf.c` - FDF (Frame Definition File) parser

**HUD System** (`hud/`):
- HUD components and displays

**Skills System** (`skills/`):
- Unit abilities and skills

**API Module** (`api/`):
- `api_module.c` - Game API implementation

---

### 4. **server** (`src/server/`)
Server-side infrastructure that manages game state and clients.

**Core Server:**
- `sv_main.c` - Main server loop and client message handling
- `sv_init.c` - Server initialization
- `sv_game.c` - Game library interface
- `game.h`, `server.h` - Server data structures

**Network and Communication:**
- `sv_send.c` - Sending data to clients
- `sv_parse.c` - Parsing client commands
- `sv_user.c` - User command processing

**Entity Management:**
- `sv_ents.c` - Server-side entity management
- `sv_world.c` - Game world state

---

### 5. **client** (`src/client/`)
Client-side code handling user input and display.

**Core Client:**
- `cl_main.c` - Main client loop and initialization
- `cl_parse.c` - Parsing server messages
- `client.h`, `renderer.h` - Client data structures

**Input System:**
- `cl_input.c` - Input processing
- `keys.c`, `keys.h` - Keyboard/mouse handling

**Visual Effects:**
- `cl_fx.c` - Client-side effects
- `cl_tent.c` - Temporary entities (explosions, particles)

**View and Display:**
- `cl_view.c` - Camera and viewport
- `cl_scrn.c` - Screen rendering

**User Interface:**
- `cl_console.c` - Console system

---

### 6. **common** (`src/common/`)
Shared code used by both client and server.

**Networking:**
- `net.c`, `net.h` - Network layer (currently using double-buffer)
- `msg.c` - Message serialization/deserialization

**Utilities:**
- `common.c`, `common.h` - Common utilities
- `shared.h` - Shared type definitions
- `cmd.c` - Command system
- `parser.c` - General parsing utilities
- `sheet.c` - Spreadsheet/data file parsing

**File System:**
- Main file I/O abstraction

**Map Data:**
- `mapinfo.h` - Map information structures
- `cmodel.h` - Collision models
- `world.c` - World/map utilities
- `routing.c` - Routing algorithms

**Platform-Specific:**
- `macos.c` - macOS-specific code
- `main.c` - Entry point

---

### 7. Main Executable
**openwarcraft3** - Combines client, server, and common code into a single executable that can run as a standalone game or host multiplayer sessions.

## Key Functionality

### Asset Management
- **MPQ Archive Support**: Uses StormLib to read Warcraft III's proprietary MPQ archive format
- **Resource Loading**: Handles loading of textures, models, sounds, and other game assets
- **Data Extraction**: Processes game data from original Warcraft III files
- **Model Formats**: Supports MDX (Warcraft III) and M3 (StarCraft II) model formats
- **Texture Formats**: BLP (Blizzard Picture), DDS, and JPEG image support

### Graphics & Rendering
- **SDL2 Integration**: Cross-platform windowing and input handling
- **3D Graphics Pipeline**: OpenGL-based rendering for models, terrain, and effects
- **Terrain Rendering**: Ground, cliffs, and water rendering from W3M map files
- **Particle Systems**: Client-side particle effects and temporary entities
- **Font Rendering**: Custom font system for UI and text display
- **Fog of War**: Visual fog of war rendering

### Game Systems
- **Map Loading**: Reads and processes Warcraft III W3M map files
- **Unit Management**: Server-authoritative unit creation, movement, and behavior
- **AI System**: Unit AI and pathfinding
- **Physics Simulation**: Server-side physics and collision detection
- **Item System**: Inventory and item management
- **JASS Scripting**: Custom JASS virtual machine for game scripting
- **Event System**: Game event handling and triggers
- **Save/Load**: Game state persistence

### Platform Support
- **Cross-Platform**: Designed to run on Linux and macOS
- **Build Systems**: 
  - Premake5 for Visual Studio 2022 (Windows), Xcode4 (macOS), and GMake
  - Simple Makefile for Linux/macOS builds

## Network Architecture

The project implements a client-server architecture where game logic is centralized on the server while clients handle only rendering and input.

### Current Networking Implementation

**⚠️ Important Note:** The networking layer currently uses a **double-buffer approach** for local client-server communication instead of proper network packets. This implementation can be found in `src/common/net.c`:

```c
struct loopback {
    char buffer[BUFFER_SIZE];  // 256 KiB (262,144 bytes) circular buffer
    int read;
    int write;
};

struct loopback bufs[2] = { 0 };  // One buffer per direction (client→server, server→client)
```

The system uses two circular buffers that act as loopback channels:
- **Buffer 0**: Client → Server communication
- **Buffer 1**: Server → Client communication

While this is a simplified approach suitable for single-process or local development, the **server-client architecture is properly implemented and functional**. The message serialization, entity synchronization, and game state updates all work correctly through this buffer system.

**Future Enhancement:** The double-buffer will eventually be replaced with proper UDP/TCP network packets for true networked multiplayer support. The architecture is already designed to support this transition.

### Server-Side Components

The **game library** runs exclusively on the server and contains all authoritative game logic:

- **Game State Management**: All game state is maintained server-side
- **Entity Management**: Units, buildings, projectiles, and resources
- **Game Logic**: Combat calculations, AI, pathfinding, physics
- **UI Generation**: Server generates UI layouts and state
- **Command Processing**: Validates and executes player commands
- **Anti-Cheat**: Server-side validation prevents client manipulation

**Key Server Files:**
- `src/server/sv_main.c` - Main server loop, packet processing
- `src/server/sv_ents.c` - Entity synchronization
- `src/server/sv_game.c` - Game library interface
- `src/game/` - Complete game logic implementation

### Client-Side Components

The **renderer library** runs on the client and handles only display and input:

- **Rendering Only**: Receives game state from server and renders it
- **Input Capture**: Captures user input and sends commands to server
- **Client Effects**: Generates local-only visual/audio effects
- **UI Display**: Renders UI layouts provided by server

**Key Client Files:**
- `src/client/cl_main.c` - Main client loop, input handling
- `src/client/cl_parse.c` - Parsing server messages
- `src/renderer/` - All rendering implementation

### Communication Protocol

**Server → Client:**
- Entity state synchronization (positions, animations, health)
- UI updates and layouts
- Game events and effects triggers
- Configuration strings and metadata

**Client → Server:**
- User input commands (mouse clicks, keyboard input)
- Unit commands (move, attack, build)
- UI interactions and menu selections

The message format uses delta compression to minimize bandwidth:
- `MSG_WriteDeltaEntity()` - Only sends changed entity fields
- `MSG_WriteDeltaUIFrame()` - Only sends changed UI state
- `MSG_WriteDeltaPlayerState()` - Only sends changed player data

**Network Message Types** (defined in `src/common/common.h`):
- `svc_configstring` - Server configuration data
- `svc_temp_entity` - Temporary visual effects
- `svc_playerinfo` - Player state updates
- `svc_layout` - UI layout updates
- `clc_stringcmd` - Client command strings

### Entity Synchronization

**Server-Authoritative Entities:** Synchronized across all clients
- Units with positions, health, and animations
- Buildings and construction progress
- Projectiles and trajectories
- Resources (gold mines, trees)
- Interactive map elements

**Client-Only Entities:** Generated locally for visual enhancement
- Particle effects (explosions, magic spells)
- Temporary animations and indicators
- Sound effects and ambient audio
- Camera effects

## Modding Architecture

The server/client separation creates a clean modding framework:

### Mod Development

**All mods target the server-side game library** (`src/game/`):
- Custom units, buildings, and abilities
- Modified game rules and balance changes
- Custom UI layouts through the UI system
- New AI behaviors via the AI system
- Custom victory conditions and game modes
- JASS scripting for complex behaviors

### Mod Distribution & Compatibility

- Server hosts the authoritative mod version
- Clients automatically receive game state from server
- No client-side mod installation required for logic changes
- Ensures all players have identical game behavior
- Prevents version mismatches and cheating

## Data Flow

### Asset Loading Pipeline
```
War3.mpq (StormLib) → Data Files → Server/Client Asset Loaders → Game/Renderer Libraries
```

### Game Loop Data Flow
```
1. Client: SDL2 Input → Commands → Network Buffer
2. Server: Read Commands → Game Logic (game library) → Update State
3. Server: Delta Encode State → Network Buffer → Client
4. Client: Read Updates → Renderer Library → OpenGL → Display
```

### JASS Script Execution
```
JASS Files → Parser → VM Compiler → Bytecode → VM Execution (server-side)
```

## Build System

### Build Tool Organization

The project uses **Premake5** to generate platform-specific project files and a **Makefile** for quick builds on Unix-like systems.

**Premake5** (`premake5.lua`):
- Generates Visual Studio 2022 projects for Windows
- Generates Xcode4 projects for macOS
- Generates GMake makefiles
- Configures platform-specific compiler flags and dependencies

**Makefile** (`Makefile`):
- Direct compilation for Linux and macOS
- Builds three shared libraries: `libcmath3.so`, `librenderer.so`, `libgame.so`
- Builds main executable: `openwarcraft3`
- Automatic dependency management
- Download command for Warcraft III 1.29b assets

### Library Dependencies

The build creates three core shared libraries:

1. **libcmath3.so** - Mathematics library (no dependencies)
2. **librenderer.so** - Rendering engine
   - Depends on: `libcmath3.so`, SDL2, StormLib, libjpeg
3. **libgame.so** - Game logic library
   - Depends on: `libcmath3.so`

The final executable links all three libraries plus SDL2 and StormLib.

## External Dependencies

- **StormLib**: MPQ archive format support for reading Warcraft III data files
- **SDL2**: Cross-platform windowing, input handling, and OpenGL context creation
- **libjpeg**: JPEG image format decoding for textures and UI elements
- **OpenGL**: 3D graphics rendering (system-provided)

## Development Approach

The project follows a modular architecture:
- **Separation of Concerns**: Math, rendering, and game logic in separate libraries
- **Code Reusability**: Libraries can be developed and tested independently
- **Cross-Platform**: Platform-specific code isolated to common/platform files
- **Client-Server**: Clean separation enables networked multiplayer architecture

## Current Status
- Functional game implementation with basic Warcraft III features
- Support for original v1.0 assets with ongoing work for v1.29b compatibility
- Active development focused on expanding game feature completeness
- Cross-platform support for Linux and macOS environments
