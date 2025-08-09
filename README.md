# openwarcraft3
Open source re-implementation of Warcraft III

Currently support War3.mpq from Warcraft III v1.0 or similar

<img width="1025" alt="Screenshot 2023-05-25 at 09 57 57" src="https://github.com/corepunch/openwarcraft3/assets/83646194/643c7aa7-2b91-469c-857e-0f6910c939af">

<img width="1026" alt="Screenshot 2023-05-25 at 09 58 15" src="https://github.com/corepunch/openwarcraft3/assets/83646194/a79e447d-e42c-4468-b4ca-3d212efe346a">

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

Let me know if you want me to add a snippet for installing prerequisites on Windows or instructions for other platforms!
