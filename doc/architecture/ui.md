# UI System Architecture

All UI logic in OpenWarcraft3 runs on the **server** inside the game library (`src/game/ui/`). The client is a dumb renderer: it stores a serialised UI blob and repaints it each frame without knowing anything about the frame hierarchy or layout rules.

## Concepts

| Term | Meaning |
|------|---------|
| `frameDef_t` | A template parsed from an FDF file and stored in the registry |
| `uiFrame_t` | A concrete frame instance with resolved position and current state |
| `FRAMEDEF` | The alias used by the C API for a frame definition being constructed |
| `svc_layout` | Network message opcode used to deliver UI data to clients |

## Initialisation

`UI_Init` (`src/game/ui/ui_init.c`) is called once from `G_ClientBegin` when the first client connects:

1. Loads Warcraft III `.fdf` files from the MPQ archive via `UI_ParseFDF` (`ui_fdf.c`).
2. Builds built-in frame definitions for the HUD (resource display, command card, minimap backdrop, chat area) using the programmatic C API.
3. Calls `UI_WriteLayout` to serialise the complete initial frame tree and send it to the client as an `svc_layout` message.

## Frame Definition Files (FDF)

FDF files declare frame templates hierarchically. `UI_ParseFDF_Buffer` (`ui_fdf.c`) accepts a writable C string, tokenises it, and registers `frameDef_t` entries in a global lookup table. Frames can inherit from a base template with `InheritsParts`.

See [FDF File Format](../file-formats/fdf.md) for the full syntax reference.

## Frame Tree Layout

The frame tree is a depth-first hierarchy. Each `uiFrame_t` node stores:

- **Anchor point** — which of the nine anchor points (`TOPLEFT` … `BOTTOMRIGHT`) of this frame is attached to which point of which parent frame, and at what X/Y offset (stored as integers scaled by `UI_FRAMEPOINT_SCALE` = 32767).
- **Size** — explicit width and height if set.
- **Texture** — `tex.index` into a server-side texture registry.
- **Text** — optional UTF-8 string for labels.
- **Stat** — optional `PLAYERSTATE_*` tag that makes the text field track a live player stat (gold, lumber, supply, etc.) and update automatically every frame.
- **Type-specific data** — backdrop edge insets, button up/down/hover states, label font index.
- **Children** — singly-linked list of child frames.

## Serialisation (`UI_WriteLayout`)

`UI_WriteLayout` (`src/game/ui/ui_write.c`) serialises the frame tree to a client by traversing it depth-first and calling `UI_WriteFrame` for each node.

`UI_WriteFrame` copies the frame's properties into a `uiFrame_t` struct and calls `gi.WriteUIFrame`, which delta-encodes the struct against a known baseline and writes it as a compact binary message.

The fields used for delta encoding are declared in `uiFrameFields[]` in `src/common/msg.c`:

```c
// Conceptual field list (not exact code):
{ "x",    FT_SHORT,  offsetof(uiFrame_t, x)        },
{ "y",    FT_SHORT,  offsetof(uiFrame_t, y)        },
{ "w",    FT_SHORT,  offsetof(uiFrame_t, w)        },
{ "h",    FT_SHORT,  offsetof(uiFrame_t, h)        },
{ "pic",  FT_BYTE,   offsetof(uiFrame_t, tex.index) },
{ "stat", FT_BYTE,   offsetof(uiFrame_t, stat)     },
{ "text", FT_STRING, offsetof(uiFrame_t, text)     },
...
```

Only fields that differ from the baseline are transmitted, minimising per-update bandwidth.

## Client-Side Rendering

The client stores the raw `svc_layout` message blob in `cl.layout`. The renderer (`src/renderer/`) reads this blob each frame in `R_DrawUI`. It walks the serialised frame list and for each frame:

1. Resolves the anchor position by accumulating parent offsets (the server has already computed absolute coordinates by the time the layout is sent, so no layout recalculation is required on the client).
2. Draws a textured quad for backdrop/button frames using the texture index.
3. Rasterises any text string using the font system (`src/renderer/r_font.c`).
4. Replaces any `stat`-tagged text fields with the current live player stat value stored in `cl.playerstate`.

The client never parses FDF files and has no knowledge of frame types — it only interprets the compact binary format emitted by `UI_WriteFrame`.

## Dynamic Updates

The server can push incremental UI changes at any time, for example:

- **Command card** — when the player selects different units, `SV_UpdateCommandCard` clears the command button layer and writes new button frames for the selected unit's abilities.
- **Resource display** — frames that reference a `stat` field update automatically because the client polls `cl.playerstate` every render frame; no re-transmission is needed.
- **Chat messages** — new chat lines are appended as child frames of the `ChatDisplay` frame and sent as a partial `svc_layout` update.

All these updates go through the same `svc_layout` / delta-encoding path.

## Adding a New UI Element

1. In `src/game/ui/ui_init.c` (or any server-side `.c` file), declare a `FRAMEDEF`, configure it with the helper functions, and emit it:

```c
FRAMEDEF btn;
UI_InitFrame(&btn, FT_COMMANDBUTTON);
UI_SetPoint(&btn, FRAMEPOINT_BOTTOMLEFT, NULL, FRAMEPOINT_BOTTOMLEFT, 0.40, 0.10);
UI_SetSize(&btn, 0.04, 0.04);
UI_SetTexture(&btn, "CommandButtonNormal", true);
UI_WriteFrame(&btn);
```

2. To update the frame later (e.g. change its texture), call `UI_WriteFrame` again with the same frame contents modified — the delta encoder will transmit only the changed fields.

## Key Files

| File | Purpose |
|------|---------|
| `src/game/ui/ui_init.c` | `UI_Init`, built-in HUD layout |
| `src/game/ui/ui_fdf.c` | FDF parser and programmatic frame API |
| `src/game/ui/ui_write.c` | `UI_WriteLayout`, `UI_WriteFrame` — serialisation |
| `src/game/hud/` | In-game HUD frames (resource bar, command card, etc.) |
| `src/client/cl_parse.c` | `CL_ParseLayout` — receive and store UI blob |
| `src/renderer/r_main.c` | `R_DrawUI` — render UI from stored blob |
| `src/renderer/r_font.c` | Bitmap font rasteriser |
| `src/common/msg.c` | `uiFrameFields[]`, delta encode/decode helpers |
