# UI System Quick Reference (Phase 8+)

A quick guide to understanding the **client-side UI architecture** introduced in Phase 8 (May 2026).

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
     │ ui/ui_router.c                    │
     │ UI_HandleInput() - Route to       │
     │ Active Screen                     │
     └────────────┬──────────────────────┘
                  │
                  ↓
     ┌───────────────────────────────────┐
     │ ui/screens/*.c                    │
     │ ConsoleUI_HandleInput()           │
     │ MainMenu_HandleInput()            │
     └────────────┬──────────────────────┘
                  │
                  ↓
     ┌───────────────────────────────────┐
     │ ui/ui_render.c                    │
     │ UI_Render() - Draw Frame Tree     │
     └────────────┬──────────────────────┘
                  │
                  ↓
┌─────────────────────────────────────────────────────────────┐
│ SCREEN OUTPUT                                               │
│ (Rendered UI on Client Display)                             │
└─────────────────────────────────────────────────────────────┘
```

### Unit Selection Flow (Client \u2194 Server)

```
┌─────────────────────────────────────────────────────────────┐
│ USER SELECTS UNITS                                          │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ↓
     ┌───────────────────────────────────┐
     │ client/cl_input.c                 │
     │ IN_SelectUp() - Store Selection   │
     │ CL_RequestUnitUI()                │
     └────────────┬──────────────────────┘
                  │
         ┌────────┴────────┐
         │ clc_request_    │  [num_units] [entity_nums...]
         │ unit_ui         │
         └────────┬────────┘
                  │ Network
                  ↓
     ┌───────────────────────────────────┐
     │ server/sv_unit_ui.c               │
     │ SV_HandleUnitUIRequest()          │
     └────────────┬──────────────────────┘
                  │
                  ↓
     ┌───────────────────────────────────┐
     │ game/g_unit_ui.c                  │
     │ G_GetCommandButtons()             │
     │ G_GetInventory()                  │
     │ G_GetBuildQueue()                 │
     └────────────┬──────────────────────┘
                  │
         ┌────────┴────────┐
         │ svc_unit_ui     │  [buttons] [inventory] [queue]
         └────────┬────────┘
                  │ Network
                  ↓
     ┌───────────────────────────────────┐
     │ client/cl_unit_ui.c               │
     │ CL_ParseUnitUI() - Store Data     │
     └────────────┬──────────────────────┘
                  │
                  ↓
     ┌───────────────────────────────────┐
     │ ui/screens/console_ui.c           │
     │ ConsoleUI_UpdateUnitUI()          │
     │ Cache unit data                   │
     └────────────┬──────────────────────┘
                  │
                  ↓
┌─────────────────────────────────────────────────────────────┐
│ COMMAND CARD RENDERED                                       │
│ (Buttons, Inventory, Build Queue)                           │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Files by Function

### Client-Side UI Library

| File | Key Functions | Purpose |
|------|---|---|
| `ui/ui_main.c` | `UI_GetAPI()`, `UI_Init()`, `UI_Render()` | Library entry point, screen management |
| `ui/ui_fdf.c` | `UI_ParseFDF_Buffer()`, `UI_InitFrame()`, `UI_SetPoint()`, `UI_SetText()` | Parse FDF files, programmatic frame API |
| `ui/ui_render.c` | `UI_Render()`, type-specific renderers | Frame tree traversal and rendering |
| `ui/ui_router.c` | `UI_HandleInput()`, `UI_SetScreen()` | Input routing, screen transitions |
| `ui/screens/console_ui.c` | `ConsoleUI_Init()`, `ConsoleUI_Render()`, `ConsoleUI_UpdateUnitUI()` | In-game HUD (command card, resource bar) |
| `ui/screens/main_menu.c` | `MainMenu_Init()`, `MainMenu_Render()` | Main menu, single player menu |

### Client Integration

| File | Key Functions | Purpose |
|------|---|---|
| `client/cl_input.c` | `CL_Input()`, `IN_SelectUp()`, `CL_RequestUnitUI()` | Input handling, selection tracking |
| `client/cl_unit_ui.c` | `CL_ParseUnitUI()` | Parse `svc_unit_ui` messages |
| `client/cl_main.c` | `CL_Init()`, `CL_Frame()` | Initialize UI library, main loop |

### Server-Side Data Providers

| File | Key Functions | Purpose |
|------|---|---|
| `server/sv_unit_ui.c` | `SV_HandleUnitUIRequest()` | Handle `clc_request_unit_ui`, query game DLL |
| `game/g_unit_ui.c` | `G_GetCommandButtons()`, `G_GetInventory()`, `G_GetBuildQueue()` | Provide unit data to server |
| `game/g_ui_stubs.c` | `Get_Commands_f()`, `UI_Init()`, etc. | No-op stubs for legacy server-side UI functions |

### Shared

| File | Key Data | Purpose |
|------|---|---|
| `ui/ui.h` | `uiImport_t`, `uiExport_t`, `uiCommandButton_t`, `uiUnitData_t` | UI library interface |
| `server/game.h` | `game_export` with `GetCommandButtons`, `GetInventory`, `GetBuildQueue` | Game DLL unit data callbacks |
| `common/common.h` | `clc_request_unit_ui`, `svc_unit_ui` | Network protocol opcodes |

---

## Data Flow: Selection to Render

### 1. Client Input \u2192 Unit Selection

```c
// client/cl_input.c — Mouse drag complete
void IN_SelectUp(void) {
    // Collect entities in selection rectangle
    DWORD entity_nums[MAX_SELECTED_ENTITIES];
    DWORD num = CollectSelectedEntities(entity_nums);
    
    // Store selection locally
    cl.selection.num_selected = num;
    memcpy(cl.selection.entity_nums, entity_nums, sizeof(DWORD) * num);
    
    // Request unit data from server
    CL_RequestUnitUI(num, entity_nums);
}
```

### 2. Client \u2192 Server: Query Unit Data

```c
// client/cl_main.c — Send request
void CL_RequestUnitUI(DWORD num_selected, DWORD *entity_nums) {
    MSG_WriteByte(&cls.netchan.message, clc_request_unit_ui);
    MSG_WriteByte(&cls.netchan.message, num_selected);
    for (i = 0; i < num_selected; i++) {
        MSG_WriteShort(&cls.netchan.message, entity_nums[i]);
    }
}
```

### 3. Server: Query Game DLL

```c
// server/sv_unit_ui.c — Handle request
void SV_HandleUnitUIRequest(LPCLIENT client, LPSIZEBUF msg) {
    BYTE num = MSG_ReadByte(msg);
    
    for (i = 0; i < num; i++) {
        WORD ent_num = MSG_ReadShort(msg);
        LPEDICT ent = &ge->edicts[ent_num];
        
        // Query game DLL
        gameCommandButton_t buttons[12];
        BYTE count = ge->GetCommandButtons(ent, buttons, 12);
        
        // Write response (svc_unit_ui)
        MSG_WriteByte(response, count);
        for (j = 0; j < count; j++) {
            MSG_WriteString(response, buttons[j].art);
            MSG_WriteString(response, buttons[j].tooltip);
            // ...
        }
    }
}
```

### 4. Server \u2192 Client: Send Data

```c
// Game DLL provides data
// game/g_unit_ui.c
BYTE G_GetCommandButtons(LPEDICT ent, gameCommandButton_t *buttons, BYTE max) {
    LPCSTR abilList = FindConfigValue(GetClassName(ent->class_id), \"abilList\");
    // Parse abilities, populate buttons array
    return count;
}
```

### 5. Client: Store and Render

```c
// client/cl_unit_ui.c — Parse response
void CL_ParseUnitUI(LPSIZEBUF msg) {
    DWORD num_units = MSG_ReadByte(msg);
    uiUnitData_t units[MAX_CACHED_UNITS];
    
    for (i = 0; i < num_units; i++) {
        units[i].entity = MSG_ReadShort(msg);
        units[i].num_buttons = MSG_ReadByte(msg);
        for (j = 0; j < units[i].num_buttons; j++) {
            MSG_ReadString(msg, units[i].buttons[j].art, 256);
            MSG_ReadString(msg, units[i].buttons[j].tooltip, 256);
            // ...
        }
    }
    
    // Forward to UI library
    ui.UpdateUnitUI(num_units, units);
}

// ui/screens/console_ui.c — Cache and render
static uiUnitData_t cached_units[MAX_CACHED_UNITS];
static DWORD cached_unit_count = 0;

void ConsoleUI_UpdateUnitUI(DWORD num_units, uiUnitData_t *units) {
    cached_unit_count = num_units;
    memcpy(cached_units, units, sizeof(uiUnitData_t) * num_units);
}

void ConsoleUI_Render(void) {
    // Draw command buttons from cached_units[0].buttons[]
    for (i = 0; i < cached_units[0].num_buttons; i++) {
        DrawCommandButton(&cached_units[0].buttons[i]);
    }
}
```

---

## Phase 8 Migration Notes

### Removed Files

- `game/ui/` directory (ui_fdf.c, ui_init.c, ui_write.c) — 🗑️ Deleted
- `game/hud/` directory (ui_console.c, ui_log.c, ui_quests.c) — 🗑️ Deleted
- `tools/fdftool.c` — 🗑️ Disabled (depended on deleted game/ui/)

### New Files

- `ui/` directory — ✅ Client-side UI library
- `ui/screens/console_ui.c` — ✅ In-game HUD controller
- `client/cl_unit_ui.c` — ✅ Unit data parser
- `server/sv_unit_ui.c` — ✅ Unit data handler
- `game/g_unit_ui.c` — ✅ Unit data providers
- `game/g_ui_stubs.c` — ✅ Legacy function stubs

### Protocol Changes

- Added `clc_request_unit_ui` — Client requests unit data
- Added `svc_unit_ui` — Server provides unit data
- Removed `svc_layout` (obsolete, was for server-side UI)

### Binary Size Changes

- `libgame.dylib`: 406K → 299K (-107KB / -26%)
- `libui.dylib`: New library, 108K
- `openwarcraft3`: Unchanged, 208K

---

## Common Patterns

### Adding a Screen

1. Create `ui/screens/my_screen.c`
2. Implement `MyScreen_Init()`, `MyScreen_Render()`, `MyScreen_HandleInput()`
3. Register in `ui/ui_router.c`

### Adding a Command Button

Unit data comes from server via query protocol. To add a button:

1. Add ability to unit's `abilList` in config file
2. `G_GetCommandButtons()` will automatically include it
3. Client renders from received data

### Debugging UI Issues

- Use `mdxtool --info` to verify model/texture assets
- Check `ui/ui_fdf.c` FDF parser for layout issues
- Trace `CL_ParseUnitUI()` for data reception issues
- Check `cached_units[]` in `console_ui.c` for render issues
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
