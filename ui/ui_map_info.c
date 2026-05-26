/*
 * ui_map_info.c - Warcraft III map info pane runtime layout.
 */

#include "ui_local.h"

static void UI_SetSizeIfPresent(LPFRAMEDEF frame, FLOAT width, FLOAT height) {
    if (frame) {
        UI_SetSize(frame, width, height);
    }
}

static void UI_SetPointIfPresent(LPFRAMEDEF frame,
                                 UIFRAMEPOINT point,
                                 LPCFRAMEDEF relative,
                                 UIFRAMEPOINT relative_point,
                                 FLOAT x,
                                 FLOAT y)
{
    if (frame && relative) {
        UI_SetPoint(frame, point, relative, relative_point, x, y);
    }
}

static void UI_SetHiddenIfPresent(LPFRAMEDEF frame, BOOL hidden) {
    if (frame) {
        UI_SetHidden(frame, hidden);
    }
}

void UI_LayoutMapInfoPane(LPFRAMEDEF frame) {
    if (!frame) {
        return;
    }

    UI_FRAME(frame, MaxPlayersIcon);
    UI_FRAME(frame, MapNameValue);
    UI_FRAME(frame, AuthIcon);
    UI_FRAME(frame, MinimapImage);
    UI_FRAME(frame, MinimapImageBackdrop);
    UI_FRAME(frame, SuggestedPlayersLabel);
    UI_FRAME(frame, SuggestedPlayersValue);
    UI_FRAME(frame, MapSizeLabel);
    UI_FRAME(frame, MapSizeValue);
    UI_FRAME(frame, MapTilesetLabel);
    UI_FRAME(frame, MapTilesetValue);
    UI_FRAME(frame, MapDescLabel);
    UI_FRAME(frame, MapDescValue);

    UI_SetPointIfPresent(MaxPlayersIcon, FRAMEPOINT_TOPLEFT, frame, FRAMEPOINT_TOPLEFT, 0.012f, -0.004f);
    UI_SetSizeIfPresent(MapNameValue, 0.218f, 0.020f);
    UI_SetPointIfPresent(MapNameValue, FRAMEPOINT_LEFT, MaxPlayersIcon, FRAMEPOINT_RIGHT, 0.0025f, 0.0f);
    UI_SetHiddenIfPresent(AuthIcon, true);

    UI_SetSizeIfPresent(MinimapImage, 0.13125f, 0.13125f);
    UI_SetPointIfPresent(MinimapImage, FRAMEPOINT_TOP, frame, FRAMEPOINT_TOP, 0.0f, -0.035f);
    UI_SetSizeIfPresent(MinimapImageBackdrop, 0.183125f, 0.183125f);
    UI_SetPointIfPresent(MinimapImageBackdrop, FRAMEPOINT_CENTER, MinimapImage, FRAMEPOINT_CENTER, 0.0f, 0.002275f);

    UI_SetSizeIfPresent(SuggestedPlayersLabel, 0.170f, 0.016f);
    UI_SetPointIfPresent(SuggestedPlayersLabel, FRAMEPOINT_TOPLEFT, frame, FRAMEPOINT_TOPLEFT, 0.012f, -0.195f);
    UI_SetSizeIfPresent(SuggestedPlayersValue, 0.126f, 0.016f);
    UI_SetPointIfPresent(SuggestedPlayersValue, FRAMEPOINT_TOPRIGHT, frame, FRAMEPOINT_TOPRIGHT, -0.012f, -0.195f);

    UI_SetSizeIfPresent(MapSizeLabel, 0.112f, 0.016f);
    UI_SetPointIfPresent(MapSizeLabel, FRAMEPOINT_TOPLEFT, SuggestedPlayersLabel, FRAMEPOINT_BOTTOMLEFT, 0.0f, -0.002f);
    UI_SetSizeIfPresent(MapSizeValue, 0.126f, 0.016f);
    UI_SetPointIfPresent(MapSizeValue, FRAMEPOINT_TOPRIGHT, SuggestedPlayersValue, FRAMEPOINT_BOTTOMRIGHT, 0.0f, -0.002f);

    UI_SetSizeIfPresent(MapTilesetLabel, 0.112f, 0.016f);
    UI_SetPointIfPresent(MapTilesetLabel, FRAMEPOINT_TOPLEFT, MapSizeLabel, FRAMEPOINT_BOTTOMLEFT, 0.0f, -0.002f);
    UI_SetSizeIfPresent(MapTilesetValue, 0.126f, 0.016f);
    UI_SetPointIfPresent(MapTilesetValue, FRAMEPOINT_TOPRIGHT, MapSizeValue, FRAMEPOINT_BOTTOMRIGHT, 0.0f, -0.002f);

    UI_SetSizeIfPresent(MapDescLabel, 0.200f, 0.016f);
    UI_SetPointIfPresent(MapDescLabel, FRAMEPOINT_TOPLEFT, MapTilesetLabel, FRAMEPOINT_BOTTOMLEFT, 0.0f, -0.012f);
    UI_SetSizeIfPresent(MapDescValue, 0.245f, 0.080f);
    UI_SetPointIfPresent(MapDescValue, FRAMEPOINT_TOPLEFT, MapDescLabel, FRAMEPOINT_BOTTOMLEFT, 0.0f, -0.002f);
}
