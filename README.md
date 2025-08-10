# openwarcraft3
Open source re-implementation of Warcraft III. Uses SDL2 and currently runs on Linux and Mac.

Was developed using War3.mpq from Warcraft III v1.0 as a reference, but 1.29b support is in progress.

<a href="https://youtu.be/m3mgmPJLqbo">Check the video on YouTube</a> or see screenshots below.

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

Let me know if you want me to add a snippet for installing prerequisites on Windows or instructions for other platforms!
