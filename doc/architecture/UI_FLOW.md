# Complete UI Flow: From Client Request to Rendering

This document traces the complete lifecycle of a UI interaction: from a mouse click on the client, through server-side command processing and UI generation, to final rendering.

## Table of Contents

1. [Overview](#overview)
2. [Phase 1: Client-Side Input](#phase-1-client-side-input)
3. [Phase 2: Network Transmission to Server](#phase-2-network-transmission-to-server)
4. [Phase 3: Server Command Processing](#phase-3-server-command-processing)
5. [Phase 4: Server-Side UI Generation](#phase-4-server-side-ui-generation)
6. [Phase 5: Serialization for Network](#phase-5-serialization-for-network)
7. [Phase 6: Client Reception and Storage](#phase-6-client-reception-and-storage)
8. [Phase 7: Client-Side Rendering](#phase-7-client-side-rendering)
9. [Complete Example: SinglePlayer Menu Click](#complete-example-singleplayer-menu-click)
10. [Key Architectural Decisions](#key-architectural-decisions)

## Overview

The UI system follows a **server-authoritative** model:

```
Client Input Event
    ↓
Network Message (clc_stringcmd)
    ↓
Server Command Handler (G_ClientCommand)
    ↓
Game Logic Updates + UI Generation
    ↓
Serialization (UI_WriteLayout)
    ↓
Network Message (svc_layout)
    ↓
Client Storage (cl.layout)
    ↓
Client Rendering (SCR_DrawFrame)
    ↓
Screen Output
```

All UI logic and state live on the **server**. The client is a thin renderer that receives serialized frame trees and paints them without understanding the frame hierarchy or game context.

---

## Phase 1: Client-Side Input

### File: `client/cl_input.c`

The client's input system runs every frame in `CL_Input()`:

```c
void CL_Input(void) {
    SDL_Event event;
    mouse.event = UI_EVENT_NONE;
    
    // Poll SDL2 for keyboard and mouse events
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_MOUSEBUTTONDOWN:
                mouse.origin.x = event.button.x;
                mouse.origin.y = event.button.y;
                
                // Translate SDL mouse button to engine constant
                DWORD mousevt = K_MOUSE1 + event.button.button - 1;
                
                // Record the event for later processing
                // This will be handled in SCR_DrawFrame's hit-testing
                break;
                
            case SDL_KEYDOWN:
                // Handle keyboard input...
                break;
        }
    }
}
```

### Data: Mouse Events

```c
typedef struct {
    VECTOR2 origin;      // Window-space pixel coordinates
    DWORD event;         // UI_EVENT_NONE, UI_LEFT_MOUSE_UP, etc.
} mouseEvent_t;
```

The mouse position is converted from window pixel coordinates to FDF normalized coordinates (0.0–1.0) by `SCR_MouseToFdf()`:

```c
VECTOR2 SCR_MouseToFdf(void) {
    size2_t window = re.GetWindowSize();
    RECT scene = SCR_GetUISceneRect();  // Compute aspect-ratio adjusted scene bounds
    
    FLOAT nx = (FLOAT)mouse.origin.x / (FLOAT)window.width;
    FLOAT ny = (FLOAT)mouse.origin.y / (FLOAT)window.height;
    
    return MAKE(VECTOR2,
                scene.x + nx * scene.w,
                scene.y + ny * scene.h);
}
```

---

## Phase 2: Network Transmission to Server

### File: `client/cl_main.c`

Every frame, after collecting input, `CL_SendCommand()` is called to forward user commands to the server:

```c
void CL_SendCommand(void) {
    CL_SendCmd();    // Send usercmd_t (camera movement)
    Cbuf_Execute();  // Execute queued console commands
}
```

### Sending UI Commands

When the user clicks a UI button, the command is issued as a **console command** (not part of the regular movement packet). For menu buttons, this happens in the client's frame rendering loop (see Phase 7), where the command is extracted from the frame's `onclick` field.

For example, clicking the "Single Player" button sends:

```c
MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
MSG_WriteString(&cls.netchan.message, "menu SinglePlayerMenu");
```

The message is appended to `cls.netchan.message` (the outgoing network buffer) and will be flushed by the next `SV_ReadPackets` call on the server.

### Network Message Format

```c
typedef enum {
    clc_move,           // usercmd_t — camera pan, unit movement
    clc_stringcmd,      // string — "menu X", "button Y", etc.
    clc_ack,            // acknowledgement
    // ...
} clc_ops_e;
```

---

## Phase 3: Server Command Processing

### File: `game/g_commands.c` and `game/g_main.c`

The server's main loop calls `SV_ReadPackets()` to drain incoming client messages. Each `clc_stringcmd` is dispatched to `G_ClientCommand`:

```c
// From game/g_main.c
void G_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    if (argc < 1)
        return;
    
    LPCSTR cmd = argv[0];
    
    if (!strcmp(cmd, "menu")) {
        // Handle menu state change
        CMD_Menu(ent, argc, argv);
    } else if (!strcmp(cmd, "button")) {
        // Handle ability button click
        CMD_Button(ent, argc, argv);
    }
    // ... more command handlers
}
```

### Menu Command Handler

```c
void CMD_Menu(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    if (argc < 2)
        return;
    
    LPCSTR menu_name = argv[1];
    
    // In the "menu" command context:
    // "menu MainMenu" → show the main menu
    // "menu SinglePlayerMenu" → show single player menu with main menu background
    // etc.
    
    // The game state records which menu is active
    ent->client->menu.current_menu = FindMenuByName(menu_name);
    
    // Trigger UI update (see next phase)
    UI_ShowMenu(ent, menu_name);
}
```

### Example: SinglePlayer Menu Command

When the user clicks the SinglePlayer button in the main menu:

1. Client receives frame with `onclick = "menu SinglePlayerMenu"`
2. Client sends: `clc_stringcmd "menu SinglePlayerMenu"`
3. Server's `G_ClientCommand` calls `CMD_Menu(ent, 2, ["menu", "SinglePlayerMenu"])`
4. `CMD_Menu` updates game state and calls `UI_ShowMenu`

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
