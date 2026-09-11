# Docker Sandbox Dependencies

This document records the Debian/Ubuntu packages installed in the development
[sandbox (docker sbx)](https://docs.docker.com/ai/sandboxes/) used to build and inspect OpenRealm. The recorded package names are the
explicit dependencies requested from APT; APT also installed their normal
runtime and development transitive dependencies.

## Environment

- Distribution: Ubuntu 26.04 LTS (`resolute`)
- Architecture: `amd64`
- Installation method: `apt-get` with `sudo`
- Sandbox reference: [Docker AI Sandboxes](https://docs.docker.com/ai/sandboxes/)

## OpenRealm build and debug tools

```sh
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential \
  gdb \
  libsdl2-dev \
  libgl-dev \
  libegl1-mesa-dev \
  zlib1g-dev \
  pkg-config \
  ca-certificates \
  git
```

These provide GCC/binutils and `make`, GDB, SDL2 headers and libraries, Linux
desktop OpenGL/EGL development files, zlib headers and libraries, and the
basic package/source-control utilities used by the project.

The verified toolchain versions in this sandbox were:

| Tool | Version |
| --- | --- |
| GCC | 15.2.0 |
| GNU Make | 4.4.1 |
| GDB | 17.1 |
| SDL2 (`pkg-config`) | 2.32.10 |

## Radare2 inspection tools

```sh
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y radare2
```

The Ubuntu package supplies both commands used by the Warcraft III binary
inspection workflow:

```sh
r2 -v
rabin2 -v
```

The installed version was radare2 6.0.7. APT additionally installed the
radare2 libraries and supporting packages such as Capstone, libmagic, libzip,
liblz4, and xxHash development files.

## Project build verification

After installation, the main Warcraft III target built successfully:

```sh
make -j2 openwarcraft3
```

## Headless Warcraft III campaign runs

The SDL client still needs an X11 display even when no physical desktop is
available. The sandbox uses Xvfb with software OpenGL:

```sh
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y xvfb xauth
```

Run the client through `xvfb-run`, hide the window, and bound execution with
`+com_frame_limit`:

```sh
xvfb-run -a env SDL_VIDEODRIVER=x11 LIBGL_ALWAYS_SOFTWARE=1 \
  build/bin/openwarcraft3 -data 'data/warcraft-3' -roc \
  +set vid_hidden 1 +set sv_cheats 1 \
  +map 'Maps/Campaign/Prologue01.w3m' \
  +com_frame_limit 1800
```

Commands that must run after the campaign intro are placed in a temporary
config and executed after the map command. The config contains `camera edge 0`,
approximately 900 `wait` commands for the intro, then the diagnostic actions:

```text
camera edge 0
hero select
smartpoint -5056 -1344
camera selected
```

`camera edge 0` prevents local mouse-edge scrolling from moving the viewport;
`camera selected` places the view on the selected Hero after the move. This
sequence was used to verify the Prologue01 Circle of Power transition. The
bounded log showed the Circle receiving team 15, reaching the renderer as
`team=15`, and loading the team-15 texture. `hero select` requires
`sv_cheats 1` and selects the first live controllable Hero for headless/GDB
diagnostics.

## Optional FFmpeg support

The default build does not link FFmpeg. To enable Warcraft III pre-rendered
movie playback and background music decoding, install the five development
packages named by `games/warcraft-3/game.mk`:

```sh
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  libavformat-dev \
  libavcodec-dev \
  libavutil-dev \
  libswscale-dev \
  libswresample-dev
```

`pkg-config` is already included in the base dependency set and is used to
resolve these libraries. Build the optional configuration with:

```sh
make FFMPEG=1 openwarcraft3
```

The `ffmpeg` command-line package is not required for compilation; install it
only when command-line media conversion tools are also needed.

The repository's `tools/r2_ability.sh` script uses base `r2` for string,
FourCC, and disassembly analysis. Its optional `pdg` path requires the
`r2ghidra` plugin; Ubuntu 26.04 does not provide an `r2ghidra` APT package,
so that plugin is not part of this sandbox setup.

## Not included

- `r2ghidra`: optional, not available as an Ubuntu 26.04 package.
- FFmpeg development packages: only needed for the optional `FFMPEG=1`
  Warcraft III movie/audio build path, not the default build; see above.
