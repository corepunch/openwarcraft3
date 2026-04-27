---
description: Use when editing or debugging Orion-UI and game renderer integration, especially FBO blits, stencil and scissor behavior, drag repaint issues, and frame ordering between CL_Frame and UI_ProcessEvents.
---

# Rendering Integration Guidance

## Frame Ownership

- The game renderer draws the 3D scene into RT_GAME during CL_Frame.
- Orion-UI presents that texture during evPaint in src/ui/ui_main.c via draw_sprite_region.
- Present order is SV_Frame -> CL_Frame -> UI_ProcessEvents -> repost_messages.

## Known State Hazard

- The game renderer may leave GL state configured for its own pass.
- Orion-UI assumes a clean baseline for fullscreen stencil and non-client paint paths.
- If scissor is left enabled with a restricted rectangle, title bars and non-client regions can fail to repaint while dragging.

## Required Invariant

- Before Orion-UI pass begins, default framebuffer scissor must not clip fullscreen repaint work.
- Current project fix is in src/renderer/r_main.c: R_EndFrame disables GL_SCISSOR_TEST after unbinding the game FBO.

## UI Blit Rules

- R_GetGameTexture returns the RT_GAME color texture used by game window evPaint.
- UVs are float rects: frect_t with x=u0, y=v0, w=u1, h=v1.
- Game texture origin is bottom-left, UI coordinates are top-left, so V is flipped in ui_main.c.
- Use DRAW_SPRITE_NO_ALPHA when blitting the opaque game FBO to avoid unwanted blending.

## Debug Workflow

1. Confirm dimensions and coordinate spaces.
2. Verify renderer init dimensions in src/client/cl_main.c and src/client/client.h.
3. Check the last GL scissor state after R_EndFrame.
4. Inspect Orion repaint path: repost_messages -> evRefreshStencil -> evNCPaint -> evPaint.
5. Validate that stencil clear and title bar painting are not clipped by stale scissor state.
6. Test drag behavior while moving over another window and while stopping drag to detect stale-state symptoms.

## Quick Checks

- If game content moves but title bar lags: suspect scissor or stencil refresh clipping.
- If repaint catches up only after overlap with another window: suspect state reset occurs only in other paint paths.
- If non-client redraw fails only during drag: inspect immediate repost during drag handlers and frame-to-frame GL state ownership.
