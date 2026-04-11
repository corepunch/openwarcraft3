<img width="647" height="88" alt="OpenWarcraft3 logo" src="https://github.com/user-attachments/assets/67313005-70d7-4e68-97a3-3e0ff1e788ed" />

**OpenWarcraft3** is an open-source implementation of Warcraft III that uses SDL2 and runs on Linux and macOS.

It was developed using War3.mpq from Warcraft III v1.0 as reference, with ongoing support for version 1.29b.

<a href="https://youtu.be/vg7Jm046vcI">▶ Watch the demo on YouTube</a> · see screenshots below

## Download

Pre-built binaries for Linux and macOS are available on the [Releases page](https://github.com/corepunch/openwarcraft3/releases/latest).

You can also download the latest build artifact from the [CI workflow runs](https://github.com/corepunch/openwarcraft3/actions/workflows/c-cpp.yml) (click the most recent successful run and download `openwarcraft3-linux-x64`).

<p align="center">
  <img src="https://github.com/user-attachments/assets/c76a93af-1801-402e-83bc-b3e0a4462312" width="31%" style="margin-right:2%;" />
  <img src="https://github.com/corepunch/openwarcraft3/assets/83646194/643c7aa7-2b91-469c-857e-0f6910c939af" width="31%" style="margin-right:2%;" />
  <img src="https://github.com/corepunch/openwarcraft3/assets/83646194/a79e447d-e42c-4468-b4ca-3d212efe346a" width="31%" />
</p>

## Getting Started

### 1. Clone

```bash
git clone --recurse-submodules git@github.com:corepunch/openwarcraft3.git
cd openwarcraft3
```

### 2. Install Dependencies

The build requires **StormLib**, **SDL2**, and **libjpeg**.

**macOS** (via [Homebrew](https://brew.sh/)):

```bash
brew install sdl2 libjpeg stormlib
```

**Linux** (Ubuntu/Debian):

```bash
sudo apt-get install libsdl2-dev libjpeg-dev libstorm-dev
```

### 3. Build

```bash
make build
```

Compiles all libraries (`cmath3`, `renderer`, `game`) and the `openwarcraft3` executable into `build/`.

### 4. Run

```bash
make run
```

Runs `openwarcraft3` from `build/bin/` using the MPQ path configured in the Makefile.

### (Optional) Download Warcraft III 1.29b assets

```bash
make download
```

Downloads a ~1.2 GB installer from `archive.org` into the `data/` folder. Skip this step if you already have a `War3.mpq` and update the `MPQ` variable in the Makefile to point to it.

---

## Advanced: Building with Premake5

Premake5 can generate native project files for Visual Studio, Xcode, or GMake. Bundled binaries are in `tools/bin/`.

| Platform | Command |
|---|---|
| Windows (VS 2022) | `tools\bin\windows\premake5.exe vs2022` |
| macOS (Xcode) | `tools/bin/darwin/premake5 xcode4` |
| macOS (GMake) | `tools/bin/darwin/premake5 --cc=clang gmake` |
| Linux (GMake) | `premake5 --cc=clang gmake` *(install [premake5](https://premake.github.io/download) separately)* |

Project files are generated in the `build/` folder.

---

## Architecture

### Client-Server Architecture

OpenWarcraft3 uses a strict client-server separation where all game logic runs exclusively on the server and clients are responsible only for rendering and input.

The **server** hosts the game library (`src/game/`), which is a shared library loaded at runtime. It maintains the authoritative game state: all entities, their positions, health, current animations, and AI state. The server processes player commands, runs the game simulation each frame, and sends the resulting state to clients.

The **client** (`src/client/`) captures user input via SDL2, forwards commands to the server, receives the updated game state, and renders it using the renderer library (`src/renderer/`). The client never runs game logic directly — it is purely a display and input layer.

Communication between the client and server happens through the network layer (`src/common/net.c`), which follows the Quake 2 runtime-dispatch model.  The routing decision is made at runtime on `netadr_t.type`:

- **Loopback** (`NA_LOOPBACK`) — when both server and client run in the same process (started with `-map=`), two 256 KiB ring buffers carry traffic in each direction with zero latency and no socket overhead.
- **UDP** (`NA_IP`) — when the executable is started with `-connect=<host>`, it binds a non-blocking UDP socket and communicates with a remote listen server over the network.

```
# Listen server (loopback, single process)
SDL2 Input  →  Client (cl_main.c)  →  net.c ring buffer  →  Server (sv_main.c)
                    ↑                                               |
                    └──────────── net.c ring buffer ←──────────────┘

# Remote client (UDP)
SDL2 Input  →  Client (cl_main.c)  →  UDP socket  →  Server (sv_main.c)
                    ↑                                      |
                    └─────────── UDP socket ←──────────────┘
```

See the [Network Architecture](architecture/network.md) page for the full design, wire format, and CLI reference.

## Build System

The project builds three shared libraries and one executable:

1. **libcmath3** — mathematics (vectors, matrices, quaternions, geometric primitives); no external dependencies
2. **librenderer** — OpenGL rendering engine; depends on `libcmath3`, SDL2, StormLib, libjpeg
3. **libgame** — server-side game logic; depends on `libcmath3`
4. **openwarcraft3** — main executable linking all three libraries plus SDL2 and StormLib

The build is driven by a `Makefile` for Linux/macOS and a `premake5.lua` that can generate Visual Studio 2022, Xcode, or GMake project files.

## External Dependencies

- **StormLib**: reads Warcraft III MPQ archives
- **SDL2**: windowing, input, and OpenGL context
- **libjpeg**: JPEG texture decoding
- **OpenGL**: 3D rendering (system-provided)

## Current Status

- Functional game implementation with basic Warcraft III features
- Support for original v1.0 assets with ongoing work for v1.29b compatibility
- Active development focused on expanding game feature completeness
- Cross-platform support for Linux and macOS environments

## License

This project is licensed under the [MIT License](https://github.com/corepunch/openwarcraft3/blob/main/LICENSE). Contributions are welcome!
