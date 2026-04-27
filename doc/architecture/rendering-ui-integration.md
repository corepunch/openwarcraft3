# Rendering and Orion-UI Integration

This note documents how the game renderer and Orion-UI share OpenGL state, and records a drag repaint bug analysis that is useful for future debugging.

## High-level Flow

Per tick, the main loop runs:

1. SV_Frame
2. CL_Frame
3. UI_ProcessEvents

Inside UI_ProcessEvents, Orion events are dispatched and repost_messages performs the draw pass and swap.

## Who Draws What

- Game renderer: renders 3D world into off-screen RT_GAME FBO.
- Orion-UI: draws desktop windows into the default framebuffer.
- Game window: in src/ui/ui_main.c evPaint, calls R_GetGameTexture and blits RT_GAME into the client rect with draw_sprite_region.

## Important API Contracts

- draw_sprite_region now takes frect_t const *uv for normalized float UVs.
- UV packing is x=u0, y=v0, w=u1, h=v1.
- Game FBO blit uses flipped V UVs because GL texture origin is bottom-left while UI coordinates are top-left.
- DRAW_SPRITE_NO_ALPHA is used when blitting opaque game output.

## Orion Stencil and Paint Model

Orion uses stencil ownership per window.

- evRefreshStencil triggers repaint_stencil.
- repaint_stencil clears stencil and draws each window shape into stencil with that window id.
- evNCPaint and evPaint then render with GL_EQUAL on the current window id.

This model requires stencil clear and stencil repaint work to cover the full intended target area.

## Bug Case: Non-client Area Not Redrawing During Drag

Observed behavior:

- While dragging the game window, only the game view appears to move.
- Title bar and other non-client pixels do not repaint until drag stops or the window crosses another window.

### Root Cause

The game renderer was leaving GL_SCISSOR_TEST enabled with a restricted rectangle derived from game render dimensions.

Known values in this project:

- src/client/client.h defines WINDOW_WIDTH=1024 and WINDOW_HEIGHT=768.
- ui_init_graphics creates a 1400x900 desktop surface (UI_WINDOW_SCALE=1 in current setup).

If scissor remains 1024x768 on the default framebuffer:

- Non-client title bar regions at higher Y on the 1400x900 desktop can be clipped.
- repaint_stencil clears stencil only within the scissor box, leaving stale stencil data outside it.
- During drag, this can make chrome appear frozen while game content updates.

### Why It Sometimes Catches Up

- Passing over another window can route paint work through paths that update scissor, temporarily breaking the stale-state loop.
- Releasing drag changes event timing/order, allowing repaint to recover in a later frame.

## Fix Applied

In src/renderer/r_main.c, R_EndFrame now disables GL_SCISSOR_TEST after unbinding the game FBO.

Rationale:

- Orion-UI starts from an unrestricted baseline.
- Orion-UI still enables and sets scissor where needed through its own clipping helpers.
- Prevents renderer-to-UI state leakage across frame boundaries.

## Debugging Checklist for Future Issues

1. Verify frame order and ownership boundaries (game pass vs UI pass).
2. Confirm renderer init dimensions and desktop dimensions.
3. Inspect GL state crossing the boundary: scissor, viewport, depth test, blend, framebuffer binding.
4. Check repaint_stencil coverage and whether stencil clear is clipped.
5. Compare behavior during drag, overlap with another window, and after mouse release.
6. Add explicit state reset at pass boundary if ownership is ambiguous.
