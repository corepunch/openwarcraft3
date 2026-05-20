# Complete UI Flow: From Client Input to Rendering (Phase 8+)

This document traces the complete lifecycle of UI interaction in the **client-side UI architecture** introduced in Phase 8 (May 2026).

## Table of Contents

1. [Overview](#overview)
2. [Phase 8 Migration](#phase-8-migration)
3. [Menu Navigation Flow](#menu-navigation-flow)
4. [Unit Selection and Command Card Flow](#unit-selection-and-command-card-flow)
5. [Key Architectural Decisions](#key-architectural-decisions)

## Overview

The UI system follows a **client-authoritative** model for rendering and layout:

```
Client Input Event (Mouse/Keyboard)
    ↓
UI Library Input Handler (ui/ui_router.c)
    ↓
Screen Controller (ui/screens/*.c)
    ↓
Client Rendering (ui/ui_render.c)
    ↓
Screen Output
```

For game state data (unit abilities, inventory, etc.), the client queries the server:

```
Client: Unit Selection
    ↓
Network Message (clc_request_unit_ui)
    ↓
Server Handler (sv_unit_ui.c)
    ↓
Game DLL Query (ge->GetCommandButtons)
    ↓
Network Response (svc_unit_ui)
    ↓
Client Parser (cl_unit_ui.c)
    ↓
UI Update (console_ui.c)
    ↓
Client Rendering
    ↓
Screen Output
```

All UI logic, FDF parsing, layout calculation, and rendering happen **client-side**. The server is game-agnostic and only provides data.

---

## Phase 8 Migration

### Before (Phase 1-7): Server-Side UI

- Game DLL (`game/ui/`, `game/hud/`) generated complete frame trees
- Server serialized frames via `svc_layout` messages
- Client was a "dumb renderer" that only displayed received blobs
- **Problem**: Violated game-agnostic principle, bloated game.dll by ~107KB

### After (Phase 8): Client-Side UI

- UI library (`ui/`) handles all rendering and layout
- Client parses FDF files locally
- Server provides only **data** (unit stats, abilities) via query protocol
- Game DLL shrunk from 406K to 299K (-107KB)

---

## Menu Navigation Flow

### 1. Client Input \u2192 Screen Controller

```c
// ui/ui_router.c — Route input to active screen
void UI_HandleInput(mouseEvent_t *event) {
    if (active_screen && active_screen->handle_input) {
        active_screen->handle_input(event);
    }
}

// ui/screens/main_menu.c — Handle button click
void MainMenu_HandleInput(mouseEvent_t *event) {
    if (event->type == UI_LEFT_MOUSE_UP) {
        LPFRAMEDEF btn = UI_HitTest(event->position);
        if (btn && btn->onclick) {
            // Execute onclick command (e.g., "show SinglePlayerMenu")
            UI_ExecuteCommand(btn->onclick);
        }
    }
}
```

### 2. Screen Transition

```c
// ui/ui_router.c — Switch to new screen
void UI_SetScreen(LPCSTR screenName) {
    if (!strcmp(screenName, "SinglePlayerMenu")) {
        active_screen = &singleplayer_menu_screen;
        active_screen->init();  // Build frame tree
    }
}
```

### 3. Rendering

```c
// ui/ui_render.c — Render active screen
void UI_Render(void) {
    if (active_screen && active_screen->render) {
        active_screen->render();
    }
}
```

No network traffic for menu navigation — everything is client-side.

---

## Unit Selection and Command Card Flow

### 1. Client: Selection Input

```c
// client/cl_input.c — Mouse drag complete
void IN_SelectUp(void) {
    // Collect entities in selection rect
    DWORD entity_nums[MAX_SELECTED_ENTITIES];
    DWORD num = 0;
    
    for (i = 0; i < cl.frame.num_entities; i++) {
        if (EntityInSelectionRect(&cl.frame.entities[i])) {
            entity_nums[num++] = cl.frame.entities[i].number;
        }
    }
    
    // Store selection
    cl.selection.num_selected = num;
    memcpy(cl.selection.entity_nums, entity_nums, sizeof(DWORD) * num);
    
    // Request unit data from server
    CL_RequestUnitUI(num, entity_nums);
}
```

### 2. Client \u2192 Server: Request Unit Data

```c
// client/cl_main.c — Send query
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
    BYTE num_requested = MSG_ReadByte(msg);
    
    for (i = 0; i < num_requested; i++) {
        WORD entity_num = MSG_ReadShort(msg);
        LPEDICT ent = &ge->edicts[entity_num];
        
        // Query game DLL for unit data
        gameCommandButton_t buttons[12];
        BYTE num_buttons = ge->GetCommandButtons(ent, buttons, 12);
        
        // ... repeat for inventory and build queue
    }
}
```

### 4. Game DLL: Provide Data

```c
// game/g_unit_ui.c — Populate button data
BYTE G_GetCommandButtons(LPEDICT ent, gameCommandButton_t *buttons, BYTE max_buttons) {
    LPCSTR class_name = GetClassName(ent->class_id);
    LPCSTR abilList = FindConfigValue(class_name, \"abilList\");
    
    BYTE count = 0;
    PARSE_LIST(abilList, abil, parse_segment) {
        if (count >= max_buttons) break;
        
        // Populate button data from config
        strncpy(buttons[count].art, FindConfigValue(abil, \"Art\"), 255);
        strncpy(buttons[count].tooltip, FindConfigValue(abil, \"Tip\"), 255);
        // ...
        count++;
    }
    
    return count;
}
```

### 5. Server \u2192 Client: Send Unit Data

```c
// server/sv_unit_ui.c — Write response
MSG_WriteByte(response, svc_unit_ui);
MSG_WriteByte(response, num_units);

for (i = 0; i < num_units; i++) {
    MSG_WriteShort(response, entity_nums[i]);
    MSG_WriteByte(response, num_buttons);
    
    for (j = 0; j < num_buttons; j++) {
        MSG_WriteString(response, buttons[j].art);
        MSG_WriteString(response, buttons[j].tooltip);
        MSG_WriteString(response, buttons[j].ubertip);
        MSG_WriteString(response, buttons[j].command);
        MSG_WriteByte(response, buttons[j].hotkey);
    }
    // ... repeat for inventory and build queue
}
```

### 6. Client: Store Unit Data

```c
// client/cl_unit_ui.c — Parse response
void CL_ParseUnitUI(LPSIZEBUF msg) {
    DWORD num_units = MSG_ReadByte(msg);
    uiUnitData_t *units = malloc(sizeof(uiUnitData_t) * num_units);
    
    for (i = 0; i < num_units; i++) {
        units[i].entity = MSG_ReadShort(msg);
        units[i].num_buttons = MSG_ReadByte(msg);
        
        for (j = 0; j < units[i].num_buttons; j++) {
            MSG_ReadString(msg, units[i].buttons[j].art, 256);
            MSG_ReadString(msg, units[i].buttons[j].tooltip, 256);
            MSG_ReadString(msg, units[i].buttons[j].ubertip, 512);
            MSG_ReadString(msg, units[i].buttons[j].command, 256);
            units[i].buttons[j].hotkey = MSG_ReadByte(msg);
        }
        // ... repeat for inventory and build queue
    }
    
    // Forward to UI library
    ui.UpdateUnitUI(num_units, units);
    free(units);
}
```

### 7. UI: Cache and Render

```c
// ui/screens/console_ui.c — Store unit data
static uiUnitData_t cached_units[MAX_CACHED_UNITS];
static DWORD cached_unit_count = 0;

void ConsoleUI_UpdateUnitUI(DWORD num_units, uiUnitData_t *units) {
    cached_unit_count = num_units;
    memcpy(cached_units, units, sizeof(uiUnitData_t) * num_units);
}

// Render loop uses cached_units[] to draw command buttons
void ConsoleUI_Render(void) {
    if (cached_unit_count == 0) return;
    
    uiUnitData_t *unit = &cached_units[0];  // Primary selection
    
    for (i = 0; i < unit->num_buttons; i++) {
        LPFRAMEDEF btn = &command_buttons[i];
        UI_SetTexture(btn, unit->buttons[i].art, true);
        UI_SetTooltip(btn, unit->buttons[i].tooltip);
        // ... render button
    }
}
```

---

## Key Architectural Decisions

### 1. Client-Side Rendering
- **Benefit**: Instant UI updates, no network latency for menus
- **Trade-off**: Client must load FDF files and maintain frame trees

### 2. Server Query Protocol
- **Benefit**: Server stays game-agnostic, no UI logic in game DLL
- **Trade-off**: Network round-trip for unit data (mitigated by caching)

### 3. Data-Only Server
- **Benefit**: Clean separation, smaller game DLL, follows Quake 2/3 pattern
- **Trade-off**: Cannot do server-side UI validation (not needed for RTS)

### 4. Stub Migration
- **Benefit**: Preserved API compatibility for legacy code
- **Implementation**: `game/g_ui_stubs.c` provides no-op implementations

---

## Phase 4: Server-Side UI Generation

### File: `game/ui/ui_init.c`

After processing a menu command, the server must generate the new UI state and send it to the client. This happens via `UI_ShowMenu()` or similar functions:

```c
// From game/ui/ui_init.c (conceptual; actual may vary)
void UI_ShowMenu(LPEDICT ent, LPCSTR menu_name) {
    if (!strcmp(menu_name, "MainMenu")) {
        UI_ShowMainMenu(ent);
    } else if (!strcmp(menu_name, "SinglePlayerMenu")) {
        UI_ShowSinglePlayerMenu(ent);
    }
}
```

### Building the Frame Tree

```c
void UI_ShowSinglePlayerMenu(LPEDICT ent) {
    // Layer 0 (LAYER_BACKGROUND): Menu background + glue models
    UI_WriteStart(LAYER_BACKGROUND);
    UI_WriteMainMenuGlueBackground(ent->client);
    gi.WriteLong(0);  // End-of-list sentinel
    gi.unicast(ent);  // Send to this client
    
    // Layer 3 (LAYER_CONSOLE): Menu buttons + frames
    UI_WriteStart(LAYER_CONSOLE);
    UI_WriteMenuWithMainFrame(ent->client, UI_FindFrame("SinglePlayerMenu"));
    gi.WriteLong(0);
    gi.unicast(ent);
}
```

### Building Frames Programmatically

Each UI element (button, text, model, etc.) is created using the frame API:

```c
void UI_WriteMenuWithMainFrame(LPGAMECLIENT client, LPCFRAMEDEF menu_root) {
    // Write the frame hierarchy starting from menu_root
    UI_WriteFrameWithChildren(menu_root, NULL);
}

// UI_WriteFrameWithChildren traverses the FDF-parsed hierarchy depth-first
void UI_WriteFrameWithChildren(LPCFRAMEDEF frame, LPCFRAMEDEF parent) {
    if (!frame)
        return;
    
    // Serialize this frame
    UI_WriteFrame(frame);
    
    // Recursively serialize all children
    // (children are linked in the FRAMEDEF)
    LPCFRAMEDEF child = frame->FirstChild;
    while (child) {
        UI_WriteFrameWithChildren(child, frame);
        child = child->NextSibling;
    }
}
```

### Key Data Structure: FRAMEDEF

```c
// From game/g_local.h
typedef struct uiFrameDef_s {
    LPCFRAMEDEF Parent;          // Parent frame
    FRAMETYPE Type;              // FT_TEXT, FT_BACKDROP, FT_COMMANDBUTTON, etc.
    UINAME Name;                 // Frame name (max 80 chars)
    UINAME TextStorage;          // Storage for frame's text
    UINAME OnClick;              // Command sent on click: "menu X", "button Y"
    LPCSTR Text;                 // Pointer to text or NULL
    LPCSTR Tip;                  // Tooltip text
    LPCSTR Ubertip;              // Extended tooltip
    FLOAT Width, Height;         // Frame dimensions
    COLOR32 Color;               // RGBA color
    BLEND_MODE AlphaMode;        // Alpha blending mode
    FRAMEPOINT x[FPP_COUNT];     // X-axis anchor points (3 slots: min/center/max)
    FRAMEPOINT y[FPP_COUNT];     // Y-axis anchor points (3 slots: min/center/max)
    struct {
        DWORD Image;             // Texture ID
        BOX2 TexCoord;           // UV coordinates
    } Texture;
    // ... more fields for backdrop edges, button states, etc.
} FRAMEDEF;
```

---

## Phase 5: Serialization for Network

### File: `game/ui/ui_write.c`

Once the frame tree is constructed, each frame must be serialized into a compact binary format for network transmission.

### UI_WriteFrame: Serializing One Frame

```c
void UI_WriteFrame(LPCFRAMEDEF frame) {
    UINAME buffer;
    uiFrame_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    
    BYTE typedata[256] = { 0 };
    sizeBuf_t buf = {
        .data = typedata,
        .maxsize = sizeof(typedata),
    };
    
    // Copy base properties from FRAMEDEF to network struct
    UI_CopyFrameBase(&tmp, frame);
    
    // Serialize type-specific data based on frame type
    switch (frame->Type) {
        case FT_BACKDROP:
            WriteBackdrop(frame, &buf, buffer);
            break;
        case FT_TEXT:
        case FT_STRING:
            WriteLabel(frame, &buf, &tmp);
            break;
        case FT_COMMANDBUTTON:
            WriteCommandButton(frame, &buf, buffer);
            tmp.text = buffer;
            break;
        case FT_GLUEBUTTON:
        case FT_GLUETEXTBUTTON:
            WriteGlueTextButton(frame, &buf, buffer);
            tmp.text = buffer;
            break;
        case FT_PORTRAIT:
            tmp.tex.index = frame->Portrait.model;
            break;
        // ... other frame types
    }
    
    // Store the type-specific buffer
    tmp.buffer.size = buf.cursize;
    tmp.buffer.data = buf.data;
    tmp.flags.type = frame->Type;
    
    // Delta-encode and write to the network buffer
    gi.WriteUIFrame(&tmp);
}
```

### UI_CopyFrameBase: Copy Common Properties

```c
static void UI_CopyFrameBase(LPUIFRAME dest, LPCFRAMEDEF src) {
    if (!src)
        return;
    
    // Copy position anchors (9 points: TOPLEFT, TOP, TOPRIGHT, LEFT, etc.)
    for (int i = 0; i < FPP_COUNT; i++) {
        dest->points.x[i] = ConvertPoint(&src->Points.x[i]);
        dest->points.y[i] = ConvertPoint(&src->Points.y[i]);
    }
    
    // Copy size
    dest->w = src->Width * UI_FRAMEPOINT_SCALE;
    dest->h = src->Height * UI_FRAMEPOINT_SCALE;
    
    // Copy texture info
    dest->tex.index = src->Texture.Image;
    CONVERT_UV(dest->tex.coord, src->Texture.TexCoord);
    
    // Copy color
    dest->color = src->Color;
    
    // Copy text (pointer, not value — text buffer sent separately)
    dest->text = src->Text;
    dest->tooltip = src->Tip;
    dest->onclick = src->OnClick;
    
    // Copy stat flag (for live stat tracking)
    dest->stat = src->Stat;
    
    // ... more fields
}
```

### Wire Format: uiFrame_t

```c
// From common/shared.h
typedef struct {
    WORD parent;                 // Parent frame index (or UI_PARENT=255 for root)
    WORD flagsvalue;             // Type + alpha mode packed
    uiFramePoint_t points[2][3]; // X and Y: min/center/max anchor points
    COLOR32 color;               // Frame color
    struct {
        BYTE index;              // Texture registry index
        BYTE coord[4];           // UV coordinates: min_x, max_x, min_y, max_y
    } tex;
    struct {
        LPSTR data;              // Type-specific buffer
        BYTE size;               // Buffer size
    } buffer;
    LPCSTR text;                 // Text string (or NULL)
    LPCSTR tooltip;              // Tooltip (or NULL)
    LPCSTR onclick;              // Click command (or NULL)
    BYTE stat;                   // Live stat tag (PLAYERSTATE_*, or 0 for none)
    BYTE alpha_mode;
} uiFrame_t;
```

### Delta Encoding

The `gi.WriteUIFrame` function (defined in `server/sv_game.c`) delta-encodes the frame against a baseline:

```c
// Conceptual delta encoding process
BYTE bits = 0;
if (frame->x != baseline->x) bits |= (1 << UF_X);
if (frame->y != baseline->y) bits |= (1 << UF_Y);
if (frame->tex.index != baseline->tex.index) bits |= (1 << UF_TEXTURE);
// ... check all fields

MSG_WriteShort(msg, bits);           // Write which fields changed
if (bits & (1 << UF_X)) {
    MSG_WriteShort(msg, frame->x);   // Write changed fields only
}
if (bits & (1 << UF_Y)) {
    MSG_WriteShort(msg, frame->y);
}
// ... write all changed fields
```

This drastically reduces bandwidth: instead of sending ~100 bytes per frame update, often only ~10 bytes are needed.

---

## Phase 6: Client Reception and Storage

### File: `client/cl_parse.c`

The client's main loop calls `CL_ReadPackets()` to process incoming server messages. When an `svc_layout` message arrives, it's dispatched to `CL_ParseLayout`:

```c
void CL_ParseServerMessage(LPSIZEBUF msg) {
    while (true) {
        BYTE cmd = MSG_ReadByte(msg);
        if (cmd == -1)
            break;
        
        switch (cmd) {
            case svc_layout:
                CL_ParseLayout(msg);
                break;
            case svc_packetentities:
                CL_ReadPacketEntities(msg);
                break;
            // ... other message types
        }
    }
}
```

### CL_ParseLayout: Deserialize Frame Tree

```c
void CL_ParseLayout(LPSIZEBUF msg) {
    // Read the layer ID (LAYER_BACKGROUND, LAYER_CONSOLE, etc.)
    DWORD layer = MSG_ReadByte(msg);
    
    // Initialize the layer's frame array
    LPUIFRAME frames = SCR_Clear(layer);
    
    // Read all frames in this layer
    DWORD nument = 0;
    while (true) {
        DWORD bits = 0;
        nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;  // End of list
        
        LPUIFRAME ent = &frames[nument];
        
        // Delta-decode the frame
        MSG_ReadDeltaUIFrame(msg, ent, nument, bits);
        
        // Read type-specific buffer
        ent->buffer.size = MSG_ReadByte(msg);
        ent->buffer.data = msg->data + msg->readcount;
        msg->readcount += ent->buffer.size;
        
        num_frames = MAX(num_frames, nument + 1);
    }
}
```

### Storage: cl.layout

```c
// From client/client.h
typedef struct {
    // ... other fields
    BYTE layout[MAX_LAYOUT_LAYERS][MAX_LAYOUT_BYTES];
    DWORD layout_size[MAX_LAYOUT_LAYERS];
} client_state_t;
```

The raw serialized frame blob is stored verbatim in `cl.layout[layer]`. The client never parses or reconstructs the frame hierarchy—it just renders the binary data directly.

---

## Phase 7: Client-Side Rendering

### File: `client/cl_scrn.c` and `renderer/r_main.c`

Every frame, after receiving new UI data, the client renders it. This happens in two stages:

#### Stage 1: Deserialize Layout into Frame Array

```c
// Called each frame from SCR_Clear
LPUIFRAME SCR_Clear(DWORD layer) {
    static UIFRAME frames[MAX_LAYOUT_OBJECTS];
    DWORD num_frames = 0;
    
    LPSIZEBUF msg = SZ_Init(cl.layout[layer], cl.layout_size[layer]);
    
    // Deserialize frames from the raw blob
    while (true) {
        DWORD bits = 0;
        int nument = MSG_ReadEntityBits(&msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        
        LPUIFRAME ent = &frames[nument];
        ent->tex.coord[1] = 0xff;  // Default UV
        ent->tex.coord[3] = 0xff;
        
        // Delta-decode against the empty baseline
        MSG_ReadDeltaUIFrame(&msg, ent, nument, bits);
        
        // Read type-specific buffer
        ent->buffer.size = MSG_ReadByte(&msg);
        ent->buffer.data = msg.data + msg.readcount;
        msg.readcount += ent->buffer.size;
        
        num_frames = MAX(num_frames, nument + 1);
    }
    
    return frames;
}
```

#### Stage 2: Draw Each Frame

```c
// Called each frame
void SCR_UpdateScreen(void) {
    // ... render game world
    
    // Draw all UI layers
    for (DWORD layer = 0; layer < MAX_LAYOUT_LAYERS; layer++) {
        if (!(cl.playerstate.uiflags & (1 << layer)))
            continue;  // Layer is hidden
        
        LPUIFRAME frames = SCR_Clear(layer);
        
        for (DWORD i = 0; i < num_frames; i++) {
            LPUIFRAME frame = &frames[i];
            
            // Compute absolute screen position from anchor offsets
            LPCRECT screen = SCR_LayoutRect(frame);
            
            // Check for mouse over (for tooltips and clicks)
            VECTOR2 mouse_fdf = SCR_MouseToFdf();
            if (Rect_contains(screen, &mouse_fdf)) {
                active_tooltip = frame->tooltip;
                
                // Handle click on this frame
                if (mouse.event == UI_LEFT_MOUSE_UP) {
                    // Send the frame's onclick command to the server
                    if (frame->onclick) {
                        MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
                        MSG_WriteString(&cls.netchan.message, frame->onclick);
                    }
                    
                    // Consume the event to prevent other frames processing it
                    mouse.event = UI_EVENT_NONE;
                }
            }
            
            // Render the frame based on its type
            SCR_DrawFrame(frame, screen);
        }
    }
}
```

#### Drawing Frame Types

```c
void SCR_DrawFrame(LPCUIFRAME frame, LPCRECT screen) {
    FRAMETYPE type = frame->flags.type;
    
    switch (type) {
        case FT_TEXT:
        case FT_STRING:
            SCR_DrawLabel(frame, screen);
            break;
        case FT_BACKDROP:
            SCR_DrawBackdrop(frame, screen);
            break;
        case FT_SPRITE:
        case FT_COMMANDBUTTON:
        case FT_GLUEBUTTON:
            SCR_DrawSprite(frame, screen);
            break;
        case FT_MODEL:
        case FT_PORTRAIT:
            SCR_DrawModel(frame, screen);
            break;
        // ... other types
    }
}

void SCR_DrawLabel(LPCUIFRAME frame, LPCRECT screen) {
    LPCSTR text = frame->text;
    
    // Replace stat-tagged fields with live player stats
    if (frame->stat) {
        int value = GetPlayerStat(frame->stat, cl.playerstate);
        static char buffer[1024];
        snprintf(buffer, sizeof(buffer), text, value);
        text = buffer;
    }
    
    // Render text using the bitmap font
    R_DrawText(screen, text, frame->color);
}

void SCR_DrawSprite(LPCUIFRAME frame, LPCRECT screen) {
    // Lookup texture from server registry
    LPCSTR texture = GetTextureByIndex(frame->tex.index);
    
    // Draw textured quad
    re.DrawQuad(screen->x, screen->y, screen->w, screen->h,
                frame->tex.coord[0] / 255.0f,
                frame->tex.coord[1] / 255.0f,
                frame->tex.coord[2] / 255.0f,
                frame->tex.coord[3] / 255.0f,
                texture, frame->color);
}

void SCR_DrawModel(LPCUIFRAME frame, LPCRECT screen) {
    // Render MDX model (e.g., unit portrait)
    LPCSTR model_path = GetModelPath(frame->tex.index);
    re.DrawModel(screen, model_path, NULL);
}
```

---

## Complete Example: SinglePlayer Menu Click

Here's the full sequence when a user clicks the SinglePlayer button:

### 1. User Clicks in Window (Client)

```
User clicks at pixel (800, 300) on the SinglePlayer button
```

**In `client/cl_input.c`:**
```c
mouse.origin.x = 800;
mouse.origin.y = 300;
mouse.event = UI_LEFT_MOUSE_UP;
```

### 2. Client Renders UI (Client)

**In `client/cl_scrn.c`, `SCR_UpdateScreen`:**

- Deserialize `cl.layout[LAYER_CONSOLE]` into frame array
- For each frame, compute screen rect and test if mouse is over it
- Find frame with name "SinglePlayerMenuButton" (for example)
- Frame has `onclick = "menu SinglePlayerMenu"`
- Mouse is over frame and `mouse.event == UI_LEFT_MOUSE_UP`

```c
if (Rect_contains(screen, &mouse_fdf) && mouse.event == UI_LEFT_MOUSE_UP) {
    // Send command to server
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, "menu SinglePlayerMenu");
    
    // Consume the event
    mouse.event = UI_EVENT_NONE;
}
```

### 3. Message Reaches Server (Network)

The `clc_stringcmd` message flows through:
- Client's netchan buffer → network socket
- Server's netchan buffer ← network socket
- `SV_ReadPackets()` dispatches to `G_ClientCommand`

### 4. Server Processes Command (Server)

**In `game/g_commands.c`, `G_ClientCommand`:**

```c
void G_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    if (!strcmp(argv[0], "menu")) {
        CMD_Menu(ent, argc, argv);  // argv[1] = "SinglePlayerMenu"
    }
}

void CMD_Menu(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    LPCSTR menu_name = argv[1];  // "SinglePlayerMenu"
    
    // Update game state
    ent->client->menu.current_menu = FindMenuByName(menu_name);
    
    // Generate new UI
    UI_ShowMenu(ent, menu_name);
}
```

### 5. Server Generates UI (Server)

**In `game/ui/ui_init.c`, `UI_ShowMenu` and `UI_ShowSinglePlayerMenu`:**

The server loads the SinglePlayerMenu frame from the parsed FDF hierarchy and serializes it:

```c
void UI_ShowSinglePlayerMenu(LPEDICT ent) {
    // Layer 0: Background
    UI_WriteStart(LAYER_BACKGROUND);
    UI_WriteMainMenuGlueBackground(ent->client);
    gi.WriteLong(0);
    gi.unicast(ent);
    
    // Layer 3: Console (menu buttons)
    UI_WriteStart(LAYER_CONSOLE);
    UI_WriteMenuWithMainFrame(ent->client, UI_FindFrame("SinglePlayerMenu"));
    gi.WriteLong(0);
    gi.unicast(ent);
}
```

### 6. Serialization (Server)

**In `game/ui/ui_write.c`, `UI_WriteFrame`:**

For each frame in the SinglePlayerMenu hierarchy (MainMenu, SinglePlayerMenu, CancelButton, LoadGameButton, etc.):

```c
// Example: SinglePlayerMenu button frame
FRAMEDEF button = {
    .Name = "CancelButton",
    .Type = FT_GLUETEXTBUTTON,
    .Text = "Cancel",
    .OnClick = "menu MainMenu",
    .Texture.Image = TextureIndex("CancelButtonUp"),
    // ... position, size, color, etc.
};

UI_WriteFrame(&button);
// This calls gi.WriteUIFrame, which delta-encodes and writes to the message buffer
```

### 7. Message Reaches Client (Network)

The `svc_layout` message flows through:
- Server's netchan buffer → network socket
- Client's netchan buffer ← network socket
- `CL_ReadPackets()` dispatches to `CL_ParseLayout`

### 8. Client Stores Layout (Client)

**In `client/cl_parse.c`, `CL_ParseLayout`:**

```c
void CL_ParseLayout(LPSIZEBUF msg) {
    DWORD layer = MSG_ReadByte(msg);  // LAYER_CONSOLE
    
    // Deserialize frames and store in cl.layout[layer]
    while (true) {
        int nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        
        LPUIFRAME ent = &frames[nument];
        MSG_ReadDeltaUIFrame(msg, ent, nument, bits);
        ent->buffer.size = MSG_ReadByte(msg);
        ent->buffer.data = msg->data + msg->readcount;
        msg->readcount += ent->buffer.size;
    }
    
    // Store the complete serialized blob
    memcpy(cl.layout[layer], frames_blob, blob_size);
    cl.layout_size[layer] = blob_size;
}
```

### 9. Client Renders New UI (Client)

Next frame, `SCR_UpdateScreen` is called:

```c
void SCR_UpdateScreen(void) {
    // ... render game world
    
    for (DWORD layer = 0; layer < MAX_LAYOUT_LAYERS; layer++) {
        if (!(cl.playerstate.uiflags & (1 << layer)))
            continue;
        
        // Deserialize and render frames
        LPUIFRAME frames = SCR_Clear(layer);
        
        for (DWORD i = 0; i < num_frames; i++) {
            LPCRECT screen = SCR_LayoutRect(&frames[i]);
            SCR_DrawFrame(&frames[i], screen);  // Draws buttons, text, etc.
        }
    }
}
```

### 10. User Sees New Menu

The SinglePlayer menu is now displayed with its background, buttons (Resume Game, New Game, Load Game, Cancel), and tooltips.

---

## Key Architectural Decisions

### 1. Server-Authoritative UI

**Decision**: All UI state and logic runs on the server.

**Why**: Prevents client-side exploits (e.g., spawning fake ability buttons), ensures all players see consistent UI, simplifies state management.

**Trade-off**: Network round-trip latency (100–200ms) is perceptible for menu clicks, but acceptable for RTS games.

### 2. Dumb Client Renderer

**Decision**: Client only stores and renders serialized frame blobs; it never parses FDF or understands frame hierarchy.

**Why**: Minimal client complexity, prevents client-side UI mods, easy hot-reloading of UI on the server.

**Trade-off**: All UI logic complexity lives on the server (game library).

### 3. Delta Encoding

**Decision**: Only changed frame fields are transmitted; baseline is implicit.

**Why**: Drastically reduces network bandwidth. A full frame serialization might be 100 bytes; a delta update often ~10 bytes.

**Trade-off**: More complex serialization code, must maintain frame number consistency.

### 4. Centralized FDF Parsing

**Decision**: Server parses `.fdf` files at startup; clients never see them.

**Why**: Single source of truth for UI definitions, no need to ship FDF files to clients, reduces attack surface.

**Trade-off**: UI changes require server restart (mitigated by reloading during development).

### 5. Layer-Based Organization

**Decision**: UI is split into logical layers (LAYER_BACKGROUND, LAYER_CONSOLE, etc.), each sent as a separate `svc_layout` message.

**Why**: Allows partial UI updates without re-transmitting the entire tree, simplifies organization (e.g., always hide cinematic layer during gameplay).

**Trade-off**: Slight increase in message count, but bandwidth savings outweigh it.

### 6. Live Stat Tracking

**Decision**: Frames with a `stat` tag update automatically by polling `cl.playerstate` each render frame.

**Why**: Eliminates need for re-serialization when stats (gold, lumber, supply) change; the client just substitutes the current value.

**Trade-off**: Client must know how to format each stat type; adds per-frame string formatting overhead (negligible).

---

## Performance Considerations

### Bandwidth
- Full UI tree at startup: ~10–50 KB (depending on menu complexity)
- Per-frame stats updates: 0 bytes (client-side substitution)
- Command card update (unit selection): ~1–5 KB (re-serialize LAYER_COMMANDBAR)
- Menu transition: ~5–10 KB

### Latency
- Menu click → new menu visible: ~100–200ms (network round-trip + server processing)
- Stat change (gold, lumber) → visible: 0ms (client-side substitution)

### Memory
- Client UI storage: ~256 KB (MAX_LAYOUT_BYTES per layer × 9 layers)
- Server frame registry: ~50 KB (depends on FDF complexity)
- Deserialized frame array: ~100 KB (uiFrame_t × MAX_LAYOUT_OBJECTS)

---

## Related Files

| File | Purpose |
|------|---------|
| `game/ui/ui_init.c` | UI initialization, menu definitions, HUD layout |
| `game/ui/ui_fdf.c` | FDF parser, programmatic frame API |
| `game/ui/ui_write.c` | Frame serialization, delta encoding |
| `game/g_commands.c` | Server-side command handlers (menu, button clicks) |
| `game/hud/ui_console.c` | In-game HUD frames (command card, resource bar) |
| `client/cl_input.c` | Mouse/keyboard input, event generation |
| `client/cl_main.c` | Client main loop, command sending |
| `client/cl_parse.c` | Network message parsing, layout reception |
| `client/cl_scrn.c` | UI deserialization, hit-testing, rendering |
| `renderer/r_main.c` | Renderer integration, frame type dispatch |
| `renderer/r_font.c` | Bitmap font rasterization |
| `common/msg.c` | Delta encoding helpers, field definitions |
| `server/sv_main.c` | Server main loop, message flushing |
| `server/sv_game.c` | Server-to-client message writers |

---

## Debugging Tips

### Trace a Click Through the System

1. Add a breakpoint in `CL_Input` to catch the mouse event
2. Check `mouse.event` and `mouse.origin` values
3. Add logging in `SCR_DrawFrame` to see which frames are hit-tested
4. Add logging in `G_ClientCommand` to see what command was received
5. Add logging in `UI_WriteFrame` to see what's being serialized
6. Add logging in `CL_ParseLayout` to see what was received

### Verify UI State

```bash
# Check client-side: what's currently displayed?
$ lldb
(lldb) p num_frames
(lldb) p frames[0]  // First frame in current layer
(lldb) p frames[0].text
```

### Check Serialization

Use the diagnostics functions to dump frame trees:
- Server side: `UI_DumpFrameTree()` to print the FRAMEDEF hierarchy
- Client side: `SCR_DumpLayout()` to print deserialized frames
- Network side: log `CL_ParseLayout` and `UI_WriteFrame` to see bandwidth usage

---

## Next Steps

- Review [ui.md](./ui.md) for the serialization format details
- See [FDF File Format](../file-formats/fdf.md) for frame definition syntax
- Check `game/hud/ui_console.c` for examples of programmatic UI creation
