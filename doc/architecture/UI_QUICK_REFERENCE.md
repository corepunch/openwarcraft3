# UI System Quick Reference

A quick guide to understanding the UI codebase structure and key files.

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│ USER INTERACTION                                            │
│ (Mouse/Keyboard Events on Client)                           │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ↓
     ┌───────────────────────────────────┐
     │ client/cl_input.c                 │
     │ CL_Input() - Mouse/Keyboard       │
     └────────────┬──────────────────────┘
                  │
                  ↓
     ┌───────────────────────────────────┐
     │ client/cl_main.c                  │
     │ CL_SendCommand() - Serialize      │
     └────────────┬──────────────────────┘
                  │
         ┌────────┴────────┐
         │ clc_stringcmd   │  (e.g., "menu SinglePlayerMenu")
         └────────┬────────┘
                  │ Network
                  ↓
     ┌───────────────────────────────────┐
     │ game/g_commands.c                 │
     │ G_ClientCommand() - Dispatch      │
     └────────────┬──────────────────────┘
                  │
                  ↓
     ┌───────────────────────────────────┐
     │ game/ui/ui_init.c                 │
     │ UI_ShowMenu() - Generate UI       │
     └────────────┬──────────────────────┘
                  │
                  ↓
     ┌───────────────────────────────────┐
     │ game/ui/ui_write.c                │
     │ UI_WriteLayout() - Serialize      │
     └────────────┬──────────────────────┘
                  │
         ┌────────┴────────┐
         │ svc_layout      │  (Serialized frame tree)
         └────────┬────────┘
                  │ Network
                  ↓
     ┌───────────────────────────────────┐
     │ client/cl_parse.c                 │
     │ CL_ParseLayout() - Store Blob     │
     └────────────┬──────────────────────┘
                  │
                  ↓
     ┌───────────────────────────────────┐
     │ client/cl_scrn.c                  │
     │ SCR_DrawFrame() - Render & Detect │
     └────────────┬──────────────────────┘
                  │
                  ↓
┌─────────────────────────────────────────────────────────────┐
│ SCREEN OUTPUT                                               │
│ (Rendered UI on Client Display)                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Files by Function

### Server-Side UI Generation

| File | Key Functions | Purpose |
|------|---|---|
| `game/ui/ui_init.c` | `UI_Init()`, `UI_ShowMainMenu()`, `UI_ShowSinglePlayerMenu()` | Initialize UI system, define menus, manage menu transitions |
| `game/ui/ui_fdf.c` | `UI_ParseFDF_Buffer()`, `UI_InitFrame()`, `UI_SetPoint()`, `UI_SetText()` | Parse FDF files, programmatic frame API |
| `game/ui/ui_write.c` | `UI_WriteLayout()`, `UI_WriteFrame()`, `UI_WriteFrameWithChildren()` | Serialize frame trees, emit svc_layout messages |
| `game/hud/ui_console.c` | `ui_unit_commands()`, `ui_unit_info()` | In-game HUD definitions (command card, resource bar) |
| `game/g_commands.c` | `G_ClientCommand()`, `CMD_Menu()`, `CMD_Button()` | Dispatch incoming client commands |

### Client-Side Rendering

| File | Key Functions | Purpose |
|------|---|---|
| `client/cl_input.c` | `CL_Input()` | Poll keyboard/mouse, record events |
| `client/cl_main.c` | `CL_Frame()`, `CL_SendCommand()` | Main client loop, send commands to server |
| `client/cl_parse.c` | `CL_ParseLayout()`, `MSG_ReadDeltaUIFrame()` | Receive and store serialized layouts |
| `client/cl_scrn.c` | `SCR_DrawFrame()`, `SCR_LayoutRect()`, `SCR_Clear()` | Deserialize, render, hit-test frames |
| `renderer/r_main.c` | `R_DrawUI()` | Frame dispatch to type-specific renderers |
| `renderer/r_font.c` | `R_DrawText()` | Bitmap font rendering |

### Network & Serialization

| File | Key Functions | Purpose |
|------|---|---|
| `common/msg.c` | `MSG_WriteDeltaUIFrame()`, `MSG_ReadDeltaUIFrame()` | Delta encoding/decoding |
| `server/sv_game.c` | `gi.WriteUIFrame()` | Server-side delta write |
| `common/shared.h` | `uiFrame_t`, `FRAMEDEF` | Wire format definitions |

---

## Data Flow: Click to Render

### 1. Client Input → Server Command

```c
// client/cl_input.c — Record mouse event
mouse.event = UI_LEFT_MOUSE_UP;
mouse.origin.x = 800;
mouse.origin.y = 300;

// client/cl_main.c — Send to server
MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
MSG_WriteString(&cls.netchan.message, "menu SinglePlayerMenu");
```

### 2. Server Command → UI Generation

```c
// game/g_commands.c — Dispatch command
void G_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    if (!strcmp(argv[0], "menu")) {
        CMD_Menu(ent, argc, argv);  // argv[1] = "SinglePlayerMenu"
    }
}

// game/ui/ui_init.c — Generate UI
void UI_ShowSinglePlayerMenu(LPEDICT ent) {
    UI_WriteStart(LAYER_CONSOLE);
    UI_WriteMenuWithMainFrame(ent->client, UI_FindFrame("SinglePlayerMenu"));
    gi.WriteLong(0);  // End-of-list sentinel
    gi.unicast(ent);  // Send to client
}
```

### 3. Serialization → Network Message

```c
// game/ui/ui_write.c — Serialize frames
void UI_WriteFrame(LPCFRAMEDEF frame) {
    uiFrame_t tmp;
    UI_CopyFrameBase(&tmp, frame);
    
    // Type-specific buffer
    switch (frame->Type) {
        case FT_GLUETEXTBUTTON:
            WriteGlueTextButton(frame, &buf, buffer);
            break;
        // ...
    }
    
    // Delta-encode and write
    gi.WriteUIFrame(&tmp);
}

// server/sv_game.c — Delta-encode
gi.WriteUIFrame(&frame);  // Writes: bits | changed_fields
```

### 4. Client Reception → Storage

```c
// client/cl_parse.c — Receive layout
void CL_ParseLayout(LPSIZEBUF msg) {
    DWORD layer = MSG_ReadByte(msg);  // LAYER_CONSOLE
    
    while (true) {
        int nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0) break;
        
        LPUIFRAME ent = &frames[nument];
        MSG_ReadDeltaUIFrame(msg, ent, nument, bits);
        // ... read type-specific buffer
    }
    
    // Store serialized blob
    memcpy(cl.layout[layer], serialized_blob, size);
}
```

### 5. Rendering → Screen

```c
// client/cl_scrn.c — Render loop
void SCR_UpdateScreen(void) {
    for (DWORD layer = 0; layer < MAX_LAYOUT_LAYERS; layer++) {
        LPUIFRAME frames = SCR_Clear(layer);  // Deserialize blob
        
        for (DWORD i = 0; i < num_frames; i++) {
            LPCRECT screen = SCR_LayoutRect(&frames[i]);
            
            // Hit-test for clicks
            if (Rect_contains(screen, &mouse_fdf) && 
                mouse.event == UI_LEFT_MOUSE_UP) {
                // Send onclick command
                if (frames[i].onclick) {
                    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
                    MSG_WriteString(&cls.netchan.message, frames[i].onclick);
                }
                mouse.event = UI_EVENT_NONE;
            }
            
            // Render frame
            SCR_DrawFrame(&frames[i], screen);
        }
    }
}
```

---

## Key Structs

### FRAMEDEF — Server-Side Template

```c
typedef struct uiFrameDef_s {
    FRAMETYPE Type;              // FT_TEXT, FT_BACKDROP, FT_COMMANDBUTTON, etc.
    UINAME Name;                 // "SinglePlayerMenu", "CancelButton", etc.
    LPCSTR Text, Tip, Ubertip;   // Display text
    UINAME OnClick;              // Command sent on click
    FLOAT Width, Height;         // Dimensions
    COLOR32 Color;               // RGBA color
    FRAMEPOINT x[3], y[3];       // Anchor points (min/center/max)
    struct {
        DWORD Image;             // Texture registry index
        BOX2 TexCoord;           // UV coordinates
    } Texture;
    BYTE Stat;                   // Live player stat tag (PLAYERSTATE_GOLD, etc.)
} FRAMEDEF;
```

### uiFrame_t — Wire Format

```c
typedef struct {
    WORD parent;                 // Parent frame index
    WORD flagsvalue;             // Type + alpha mode
    uiFramePoint_t points[2][3]; // X and Y anchors
    COLOR32 color;               // Color
    struct {
        BYTE index;              // Texture index
        BYTE coord[4];           // UV: min_x, max_x, min_y, max_y
    } tex;
    struct {
        LPBYTE data;             // Type-specific buffer
        BYTE size;               // Buffer size
    } buffer;
    LPCSTR text;                 // Text string
    LPCSTR tooltip;              // Tooltip
    LPCSTR onclick;              // Click command
    BYTE stat;                   // Live stat tag
} uiFrame_t;
```

---

## Common Tasks

### Add a New UI Button

1. **Define in FDF** (`data/MainMenu.fdf` or similar):
   ```
   Frame "NewButtonName" INHERITS GLUEBUTTON
       SetPoint BOTTOMRIGHT, NULL, BOTTOMRIGHT, -0.05, 0.05
       SetSize 0.04, 0.04
       TextureName "ButtonNormalImage"
       OnClick "menu TargetMenuName"
   EndFrame
   ```

2. **Or programmatically in C** (`game/ui/ui_init.c`):
   ```c
   FRAMEDEF btn;
   UI_InitFrame(&btn, FT_GLUEBUTTON);
   UI_SetPoint(&btn, FRAMEPOINT_BOTTOMRIGHT, NULL, FRAMEPOINT_BOTTOMRIGHT, -0.05, 0.05);
   UI_SetSize(&btn, 0.04, 0.04);
   UI_SetTexture(&btn, "ButtonNormalImage", true);
   UI_SetOnClick(&btn, "menu TargetMenuName");
   UI_WriteFrame(&btn);
   ```

3. **Handle the command** (`game/g_commands.c`):
   ```c
   void CMD_Menu(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
       if (!strcmp(argv[1], "TargetMenuName")) {
           UI_ShowTargetMenu(ent);
       }
   }
   ```

### Update UI Dynamically

```c
// Server: Modify frame and re-serialize
FRAMEDEF button;
UI_InitFrame(&button, FT_TEXT);
UI_SetText(&button, "Gold: %d", player_gold);
button.Stat = PLAYERSTATE_RESOURCE_GOLD;  // Auto-update on client
UI_WriteFrame(&button);

// The delta encoder will only send changed fields
gi.WriteUIFrame(&button);
```

### Render a Custom Frame Type

1. **Define type** (`game/g_local.h`):
   ```c
   typedef enum {
       FT_CUSTOMFRAME = 42,
       // ...
   } FRAMETYPE;
   ```

2. **Serialize type** (`game/ui/ui_write.c`):
   ```c
   case FT_CUSTOMFRAME:
       WriteCustomFrame(frame, &buf);  // Write type-specific data
       break;
   ```

3. **Render type** (`client/cl_scrn.c`):
   ```c
   void SCR_DrawCustomFrame(LPCUIFRAME frame, LPCRECT screen) {
       // Custom rendering logic
   }
   ```

---

## Debug Workflows

### Trace a Frame Through Serialization

```bash
# Add logging to ui_write.c
printf("UI_WriteFrame: %s (type=%d)\n", frame->Name, frame->Type);

# Build and reproduce
make clean && make
./build/bin/openwarcraft3 -- -mpq='data/Warcraft III/War3.mpq'
# [Click to trigger the frame]
```

### Verify Frame Arrives on Client

```c
// In client/cl_scrn.c, SCR_DrawFrame
printf("Rendering frame %d: type=%d, x=%d, y=%d\n", 
       i, frame->flags.type, frame->points[0][0].x, frame->points[1][0].y);
```

### Check Hit-Testing

```c
// In client/cl_scrn.c, near Rect_contains
VECTOR2 mouse_fdf = SCR_MouseToFdf();
printf("Mouse at FDF coords: (%f, %f)\n", mouse_fdf.x, mouse_fdf.y);
printf("Frame rect: (%f, %f) — (%f, %f)\n", 
       screen->x, screen->y, screen->x + screen->w, screen->y + screen->h);
```

---

## Architecture Principles

1. **Server-Authoritative**: All UI state and logic on the server
2. **Client Dumb**: Client only renders; no FDF parsing, no frame hierarchy knowledge
3. **Delta Encoding**: Only changed fields transmitted each frame
4. **Layer-Based**: UI split into independent layers (LAYER_CONSOLE, LAYER_COMMANDBAR, etc.)
5. **Live Stats**: Stat-tagged fields update client-side without re-serialization

---

## Performance Notes

| Metric | Value | Notes |
|--------|-------|-------|
| Startup UI blob | 10–50 KB | Depends on FDF complexity |
| Menu transition | 5–10 KB | Re-serialize one layer |
| Command card update | 1–5 KB | Buttons only |
| Latency (menu click) | 100–200 ms | Network round-trip |
| Memory (client) | 256 KB | MAX_LAYOUT_BYTES × layers |

---

## See Also

- [UI System Architecture](./ui.md) — Detailed serialization and rendering
- [UI Flow (Complete End-to-End)](./UI_FLOW.md) — Full click-to-render walkthrough
- [FDF File Format](../file-formats/fdf.md) — Frame definition syntax
- [File Formats Index](../file-formats/README.md) — Other format references
