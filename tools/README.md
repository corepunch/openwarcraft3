# Tools

This directory contains the standalone command-line tools used to inspect
Warcraft III assets and game data.

## `mpqtool`

Archive utility for MPQ files.

Use it to list contents, dump files, inspect image headers, or create and
pack test archives.

Syntax:

```bash
build/bin/mpqtool -mpq "<archive.mpq>" <command> [args...]
```

Commands:

- `ls [path]` list archive entries under a path
- `cat <file>` print a file to stdout
- `imginfo <file>` inspect texture metadata
- `create [max-files]` create a new archive
- `pack <src> <archive-file> [<src> <archive-file> ...]` add files to an archive

Examples:

```bash
build/bin/mpqtool -mpq "data/Warcraft III/War3.mpq" ls UI/FrameDef
build/bin/mpqtool -mpq "data/Warcraft III/War3.mpq" cat UI/FrameDef/Glue/MainMenu.fdf
build/bin/mpqtool -mpq "data/Warcraft III/War3.mpq" imginfo UI/Widgets/Glues/GlueScreen-Button1-Border.blp
```

## `fdftool`

FDF scene parser and viewer.

Use it to inspect parsed UI frame trees, check layout behavior, and preview
the decoded scene graph from one or more FDF files.

Syntax:

```bash
build/bin/fdftool -mpq "<archive.mpq>" -fdf "<file.fdf>" [-fdf "<file.fdf>" ...] -root "<FrameName>" [options]
```

Useful options:

- `--info` print parsed frame/tree information and exit
- `-width <px>` and `-height <px>` set the preview window size

Examples:

```bash
build/bin/fdftool -mpq "data/Warcraft III/War3.mpq" -fdf "UI\\FrameDef\\Glue\\MainMenu.fdf" -root MainMenuFrame
build/bin/fdftool -mpq "data/Warcraft III/War3.mpq" -fdf "UI\\FrameDef\\UI\\ConsoleUI.fdf" -fdf "UI\\FrameDef\\UI\\ResourceBar.fdf" -root ConsoleUI
build/bin/fdftool -mpq "data/Warcraft III/War3.mpq" -fdf "UI\\FrameDef\\Glue\\MainMenu.fdf" --info
```

## `mdxtool`

MDX model viewer and inspector.

Use it to inspect model metadata, preview sequences, compare UI models against
game models, and render models with different camera modes.

Syntax:

```bash
build/bin/mdxtool -mpq "<archive.mpq>" -model "<file.mdx>" [options]
```

Useful options:

- `--anim "<sequence>"` choose a sequence to preview
- `--use-model-camera` use the model's own camera if present
- `--front-ortho` force the front-facing UI preview camera
- `--info` print model metadata and exit
- `--dump-all` print detailed model contents
- `--once` render one frame and exit

Examples:

```bash
build/bin/mdxtool -mpq "data/Warcraft III/War3.mpq" -model "UI\\Glues\\SpriteLayers\\TopRightPanel.mdx" --front-ortho
build/bin/mdxtool -mpq "data/Warcraft III/War3.mpq" -model "UI\\Glues\\MainMenu\\WarCraftIIILogo\\WarCraftIIILogo.mdx" --info
build/bin/mdxtool -mpq "data/Warcraft III/War3.mpq" -model "units\\orc\\Peon\\Peon.mdx" --use-model-camera
```

## `maptool`

Map viewer for Warcraft III world maps.

Use it to open a map and inspect the world renderer with the current camera.

Syntax:

```bash
build/bin/maptool -mpq "<archive.mpq>" -map "<file.w3m>"
```

Example:

```bash
build/bin/maptool -mpq "data/Warcraft III/War3.mpq" -map "Maps\\Campaign\\Human02.w3m"
```

## `mpqnc`

Terminal browser for MPQ archives.

Use it to browse archive contents interactively and launch the other tools on
the selected files.

Syntax:

```bash
build/bin/mpqnc -mpq "<archive.mpq>" [-path "<archive/path>"] [-mpqtool "<path>"] [-mdxtool "<path>"] [-fdftool "<path>"]
```

Example:

```bash
build/bin/mpqnc -mpq "data/Warcraft III/War3.mpq" -path "UI/FrameDef/Glue"
```

## Notes

- Paths inside MPQs usually use forward slashes, but most tools accept either
  `\` or `/` on the command line.
- The `build/bin/...` paths are the default build outputs used by this repo.
