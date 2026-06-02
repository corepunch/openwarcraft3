#include "client/ui.h"

static uiImport_t uiimport;

typedef struct {
    LPRENDERER renderer;
    LPTEXTURE background;
    LPTEXTURE bar_background;
    LPTEXTURE bar_border;
    LPTEXTURE bar_fill;
    LPTEXTURE bar_glass;
    LPTEXTURE bar_glow;
    LPCFONT font_title;
    LPCFONT font_status;
    PATHSTR active_map;
    FLOAT displayed_progress;
} uiWowState_t;

static uiWowState_t wow_ui;

static LPCSTR UIWow_DefaultLoadingBackground(void) {
    return "Interface\\Glues\\LoadingScreens\\LoadScreenEnviroment.blp";
}

static void UIWow_LoadStaticAssets(void) {
    if (!wow_ui.renderer) {
        return;
    }

    if (!wow_ui.bar_background) {
        wow_ui.bar_background = wow_ui.renderer->LoadTexture("Interface\\Glues\\LoadingBar\\Loading-BarBackground.blp");
    }
    if (!wow_ui.bar_border) {
        wow_ui.bar_border = wow_ui.renderer->LoadTexture("Interface\\Glues\\LoadingBar\\Loading-BarBorder.blp");
    }
    if (!wow_ui.bar_fill) {
        wow_ui.bar_fill = wow_ui.renderer->LoadTexture("Interface\\Glues\\LoadingBar\\Loading-BarFill.blp");
    }
    if (!wow_ui.bar_glass) {
        wow_ui.bar_glass = wow_ui.renderer->LoadTexture("Interface\\Glues\\LoadingBar\\Loading-BarGlass.blp");
    }
    if (!wow_ui.bar_glow) {
        wow_ui.bar_glow = wow_ui.renderer->LoadTexture("Interface\\Glues\\LoadingBar\\Loading-BarGlow.blp");
    }
    if (!wow_ui.font_title) {
        wow_ui.font_title = wow_ui.renderer->LoadFont("Fonts\\FRIZQT__.TTF", 22);
    }
    if (!wow_ui.font_status) {
        wow_ui.font_status = wow_ui.renderer->LoadFont("Fonts\\FRIZQT__.TTF", 16);
    }
}

static void UIWow_UpdateMapBackground(LPCPLAYER ps) {
    LPCSTR map_path;
    LPCSTR screen_path;

    if (!wow_ui.renderer) {
        return;
    }

    map_path = uiimport.GetLoadingMap ? uiimport.GetLoadingMap() : "";
    if (!map_path) {
        map_path = "";
    }
    screen_path = (ps && ps->texts[PLAYERTEXT_MAP_PREVIEW] && *ps->texts[PLAYERTEXT_MAP_PREVIEW])
        ? ps->texts[PLAYERTEXT_MAP_PREVIEW]
        : UIWow_DefaultLoadingBackground();

    if (!strcmp(wow_ui.active_map, map_path)) {
        return;
    }

    snprintf(wow_ui.active_map, sizeof(wow_ui.active_map), "%s", map_path);

    SAFE_DELETE(wow_ui.background, wow_ui.renderer->ReleaseTexture);
    wow_ui.background = wow_ui.renderer->LoadTexture(screen_path);
    if (!wow_ui.background) {
        wow_ui.background = wow_ui.renderer->LoadTexture(UIWow_DefaultLoadingBackground());
    }

    if (uiimport.Printf) {
        uiimport.Printf("UIWow: loading screen map=%s background=%s\n",
                       wow_ui.active_map[0] ? wow_ui.active_map : "<none>",
                       screen_path);
    }
}

static FLOAT UIWow_GetLoadingProgress(void) {
    FLOAT target = uiimport.GetLoadingProgress ? uiimport.GetLoadingProgress() : 0.0f;

    if (target < 0.0f) {
        target = 0.0f;
    } else if (target > 1.0f) {
        target = 1.0f;
    }

    if (target < wow_ui.displayed_progress) {
        wow_ui.displayed_progress = target;
    } else {
        wow_ui.displayed_progress = wow_ui.displayed_progress * 0.82f + target * 0.18f;
        if (target - wow_ui.displayed_progress < 0.002f) {
            wow_ui.displayed_progress = target;
        }
    }
    return wow_ui.displayed_progress;
}

static void UIWow_DrawLoadingScreen(void) {
    RECT full = MAKE(RECT, 0, 0, 1, 1);
    RECT uv = MAKE(RECT, 0, 0, 1, 1);
    RECT bar = MAKE(RECT, 0.305f, 0.865f, 0.39f, 0.032f);
    RECT bar_border = MAKE(RECT, 0.285f, 0.848f, 0.43f, 0.067f);
    RECT bar_fill = bar;
    RECT title_rect = MAKE(RECT, 0.16f, 0.77f, 0.68f, 0.05f);
    RECT status_rect = MAKE(RECT, 0.16f, 0.818f, 0.68f, 0.04f);
    FLOAT progress = UIWow_GetLoadingProgress();
    LPCPLAYER ps = uiimport.GetPlayerState ? uiimport.GetPlayerState() : NULL;
    LPCSTR map_path = uiimport.GetLoadingMap ? uiimport.GetLoadingMap() : "";
    LPCSTR map_title = ps ? ps->texts[PLAYERTEXT_MAP_TITLE] : NULL;
    LPCSTR status = uiimport.GetLoadingStatus ? uiimport.GetLoadingStatus() : "";

    if (!wow_ui.renderer) {
        return;
    }

    if (wow_ui.background) {
        wow_ui.renderer->DrawImage(wow_ui.background, &full, &uv, COLOR32_WHITE);
    }

    if (wow_ui.bar_background) {
        wow_ui.renderer->DrawImage(wow_ui.bar_background, &bar_border, &uv, COLOR32_WHITE);
    }
    if (wow_ui.bar_fill) {
        bar_fill.w *= progress;
        if (bar_fill.w > 0.0f) {
            RECT fill_uv = uv;
            fill_uv.w = progress;
            wow_ui.renderer->DrawImage(wow_ui.bar_fill, &bar_fill, &fill_uv, COLOR32_WHITE);
        }
    }
    if (wow_ui.bar_glow) {
        wow_ui.renderer->DrawImage(wow_ui.bar_glow, &bar_border, &uv, MAKE(COLOR32, 255, 255, 255, 200));
    }
    if (wow_ui.bar_glass) {
        wow_ui.renderer->DrawImage(wow_ui.bar_glass, &bar_border, &uv, COLOR32_WHITE);
    }
    if (wow_ui.bar_border) {
        wow_ui.renderer->DrawImage(wow_ui.bar_border, &bar_border, &uv, COLOR32_WHITE);
    }

    if (!map_title || !*map_title) {
        map_title = map_path;
    }

    if (wow_ui.font_title && map_title && *map_title) {
        wow_ui.renderer->DrawText(&MAKE(drawText_t,
                                        .font = wow_ui.font_title,
                                        .text = map_title,
                                        .rect = title_rect,
                                        .color = MAKE(COLOR32, 235, 210, 160, 255),
                                        .textWidth = title_rect.w,
                                        .lineHeight = title_rect.h,
                                        .wordWrap = false,
                                        .halign = FONT_JUSTIFYCENTER,
                                        .valign = FONT_JUSTIFYMIDDLE));
    }
    if (wow_ui.font_status && status && *status) {
        wow_ui.renderer->DrawText(&MAKE(drawText_t,
                                        .font = wow_ui.font_status,
                                        .text = status,
                                        .rect = status_rect,
                                        .color = MAKE(COLOR32, 220, 220, 220, 255),
                                        .textWidth = status_rect.w,
                                        .lineHeight = status_rect.h,
                                        .wordWrap = false,
                                        .halign = FONT_JUSTIFYCENTER,
                                        .valign = FONT_JUSTIFYMIDDLE));
    }
}

static void UIWow_Init(void) {
    memset(&wow_ui, 0, sizeof(wow_ui));
    wow_ui.renderer = uiimport.GetRenderer ? uiimport.GetRenderer() : NULL;
    UIWow_LoadStaticAssets();
}

static void UIWow_Shutdown(void) {
    if (!wow_ui.renderer) {
        return;
    }
    SAFE_DELETE(wow_ui.background, wow_ui.renderer->ReleaseTexture);
    SAFE_DELETE(wow_ui.bar_background, wow_ui.renderer->ReleaseTexture);
    SAFE_DELETE(wow_ui.bar_border, wow_ui.renderer->ReleaseTexture);
    SAFE_DELETE(wow_ui.bar_fill, wow_ui.renderer->ReleaseTexture);
    SAFE_DELETE(wow_ui.bar_glass, wow_ui.renderer->ReleaseTexture);
    SAFE_DELETE(wow_ui.bar_glow, wow_ui.renderer->ReleaseTexture);
    memset(&wow_ui, 0, sizeof(wow_ui));
}

static void UIWow_Refresh(DWORD msec) {
    (void)msec;
}

static void UIWow_DrawFrame(void) {
    LPCPLAYER ps = uiimport.GetPlayerState ? uiimport.GetPlayerState() : NULL;

    if (!ps || ps->client_ui_state != CLIENT_UI_LOADING) {
        return;
    }

    if (!wow_ui.renderer) {
        wow_ui.renderer = uiimport.GetRenderer ? uiimport.GetRenderer() : NULL;
        UIWow_LoadStaticAssets();
    }

    UIWow_UpdateMapBackground(ps);
    UIWow_DrawLoadingScreen();
}

static void UIWow_KeyEvent(int key, BOOL down, DWORD time) {
    (void)key;
    (void)down;
    (void)time;
}

static void UIWow_TextInput(LPCSTR text) {
    (void)text;
}

static void UIWow_MouseEvent(int x, int y, int button, BOOL down) {
    (void)x;
    (void)y;
    (void)button;
    (void)down;
}

static void UIWow_MenuCommand(LPCSTR route) {
    (void)route;
}

static void UIWow_UpdateUnitUI(DWORD num_units, uiUnitData_t *units) {
    (void)num_units;
    (void)units;
}

static void UIWow_SetLayoutLayer(DWORD layer, HANDLE data) {
    (void)layer;
    (void)data;
}

static void UIWow_ClearLayoutLayer(DWORD layer) {
    (void)layer;
}

static BOOL UIWow_HitTestLayout(int x, int y) {
    (void)x;
    (void)y;
    return false;
}

uiExport_t UI_GetAPI(uiImport_t import) {
    uiimport = import;
    (void)uiimport;

    return (uiExport_t) {
        .Init = UIWow_Init,
        .Shutdown = UIWow_Shutdown,
        .Refresh = UIWow_Refresh,
        .DrawFrame = UIWow_DrawFrame,
        .KeyEvent = UIWow_KeyEvent,
        .TextInput = UIWow_TextInput,
        .MouseEvent = UIWow_MouseEvent,
        .MenuCommand = UIWow_MenuCommand,
        .UpdateUnitUI = UIWow_UpdateUnitUI,
        .SetLayoutLayer = UIWow_SetLayoutLayer,
        .ClearLayoutLayer = UIWow_ClearLayoutLayer,
        .HitTestLayout = UIWow_HitTestLayout,
    };
}
