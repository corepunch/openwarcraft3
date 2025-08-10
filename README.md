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

* **StormLib** — for MPQ archive support
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

---

# Project Architecture & Functionality Outline

## Core Architecture Components

### 1. Core Libraries Structure
The project is organized into three main library components:

#### **cmath3 Library**
- Mathematical operations and utilities
- Vector/matrix calculations for 3D graphics
- Geometric transformations
- Physics calculations for game mechanics

#### **renderer Library** 
- Graphics rendering engine
- 3D model rendering and display
- Texture management and loading
- Scene rendering pipeline
- Camera and viewport management
- Shader management (if applicable)

#### **game Library**
- Core game logic and mechanics
- Game state management
- Unit behavior and AI
- Resource management
- Map handling and terrain
- Game rules and victory conditions

### 2. Main Executable
**openwarcraft3** - The main executable that ties all components together and provides the game interface.

## Key Functionality Areas

### Asset Management
- **MPQ Archive Support**: Uses StormLib to read Warcraft III's proprietary MPQ archive format
- **Resource Loading**: Handles loading of textures, models, sounds, and other game assets
- **Data Extraction**: Processes game data from original Warcraft III files

### Graphics & Rendering
- **SDL2 Integration**: Cross-platform windowing and input handling
- **3D Graphics Pipeline**: Renders 3D models, terrain, and effects
- **JPEG Support**: Handles JPEG image decoding via libjpeg
- **Model Loading**: Processes Warcraft III's 3D models and animations

### Game Systems
- **Map Loading**: Reads and processes Warcraft III map files
- **Unit Management**: Handles unit creation, movement, and behavior
- **Resource System**: Manages gold, wood, and other resources
- **Building System**: Construction and upgrade mechanics
- **Combat System**: Unit combat, damage calculations, and effects

### Platform Support
- **Cross-Platform**: Designed to run on Linux and macOS
- **Build Systems**: 
  - Premake5 for Visual Studio 2022 (Windows)
  - Xcode4 support for macOS
  - GMake with Clang for Unix-like systems
  - Makefile for simplified building

## Server/Client Architecture

### Network Architecture Design
The project implements a distributed architecture where game logic is centralized on the server while clients handle only rendering and input:

#### **Server-Side Components**
- **game library**: Runs exclusively on the server
  - Authoritative game state management
  - All game logic calculations (combat, AI, resource management)
  - UI generation and interface logic
  - Physics simulation and collision detection
  - Map state and unit positioning
  - Game rules enforcement and validation
  - Anti-cheat protection through server-side validation

#### **Client-Side Components**
- **renderer library**: Client-side rendering only
  - Receives game state updates from server
  - Renders 3D graphics based on server data
  - Displays UI elements generated by server
  - Manages local graphics settings and performance
- **cmath3 library**: Used on both client and server
  - Shared mathematical operations
  - Consistent calculations across network

#### **Communication Layer**
- **Server → Client**: Game state synchronization
  - Unit positions, animations, and states
  - Resource updates and UI data
  - UI layout and interface elements generated by server
  - Map changes and environmental effects
  - Combat results and damage calculations
  - **Entity synchronization**: Server-authoritative entities (units, buildings, projectiles)
  - **Effect triggers**: Events that spawn client-only entities (explosions, particles, sound effects)

#### **Entity Management System**
- **Server-Authoritative Entities**: Synchronized across all clients
  - Units (workers, soldiers, heroes) with positions, health, and states
  - Buildings and structures with construction progress and functionality
  - Projectiles with trajectories and collision detection
  - Resources (gold mines, trees) with depletion states
  - Interactive map elements (doors, switches, destructible objects)
  
- **Client-Only Entities**: Generated locally for visual/audio enhancement
  - Particle effects (fire, smoke, magic spells)
  - Explosion animations and debris
  - UI animation effects (button highlights, selection indicators)
  - Sound effects and ambient audio
  - Camera shake and screen effects
  - Temporary visual indicators (damage numbers, range circles)
- **Client → Server**: Input commands only
  - User input events (mouse clicks, keyboard)
  - Unit movement commands
  - Building placement requests
  - Menu selections and game actions

### Modding Architecture
The server/client separation creates a clean modding framework:

#### **Mod Development Focus**
- **game library modifications only**: All mods target the server-side game library
  - Custom units, buildings, and abilities
  - Modified game rules and balance changes
  - Custom UI layouts and interface elements
  - New AI behaviors and strategies
  - Custom victory conditions and game modes
  
#### **Mod Distribution & Compatibility**
- Server hosts the authoritative mod version
- Clients automatically receive necessary assets and data
- No client-side mod installation required
- Ensures all players have identical game logic
- Prevents version mismatches and cheating

## Data Flow Architecture

### Networked Asset Pipeline
```
Original War3.mpq Files → StormLib → Server Asset Loaders → game library (server) → Network Protocol → Client renderer library
```

### Distributed Rendering Pipeline
```
Server: Game State Calculations → Network Updates → Client: renderer library → SDL2 → Display Output
Server: Authoritative Entities → Client: Game Objects + Client-Generated Effects → Rendering Pipeline
```

### Input Processing Flow
```
Client: SDL2 Input Events → Network Commands → Server: game library → Game State Updates → Network Broadcast → All Clients
```

### Modding Data Flow
```
Mod Files → Server game library → Modified Game Logic + Entity Definitions → Network Protocol → Client Rendering Updates + Effect Triggers
```

## Dependencies & Integration

### External Dependencies
- **StormLib**: MPQ archive format support for reading Warcraft III data files
- **SDL2**: Cross-platform windowing, input handling, and basic graphics
- **libjpeg**: JPEG image format decoding for textures and UI elements

### Build Tools
- **Premake5**: Project file generation for different IDEs/build systems
- **Make**: Standard Unix build automation
- **Platform-specific compilers**: Support for various C/C++ toolchains

## Development Approach
The project follows a modular architecture where each major system (math, rendering, game logic) is separated into its own library, promoting:
- **Code Reusability**: Libraries can be used independently
- **Maintainability**: Clear separation of concerns
- **Cross-Platform Compatibility**: Platform-specific code is isolated
- **Incremental Development**: Each component can be developed and tested separately

## Current Status
- Functional game implementation with basic Warcraft III features
- Support for original v1.0 assets with ongoing work for v1.29b compatibility
- Active development focused on expanding game feature completeness
- Cross-platform support for Linux and macOS environments
