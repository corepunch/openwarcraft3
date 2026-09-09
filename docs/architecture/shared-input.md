# Shared client input profiles

## Contract

Every game build includes `client/cl_input_rts.c` and `client/cl_input_orbit.c`. `CL_InputModeInit` selects the
interaction profile from `cl_input_mode` after shipped config, user config, autoexec, and command-line overrides
have loaded. The profile is fixed until process restart; editing the cvar during play does not switch live input state.
There are no game-name conditionals in either implementation or the mode dispatcher.

The client owns interaction mechanisms, transient input state, selection hints, and numbered groups. The game
server owns selection legality, unit orders, movement, and scripted cameras. No import/export callbacks were added.
The orbit profile retains the existing `move flags yaw pitch distance` command contract; choosing a profile does
not add that command's gameplay implementation to a server which does not support it.

`cl_input.c` dispatches SDL events and shared commands; `cl_selection.c` owns selection/group commands;
`cl_control_groups.c` retains the separately testable append helper and reset. The RTS profile supplies pan,
smart-click, scrolling, and hover-health behavior. The orbit profile supplies click targeting, relative mouse look,
movement-button state, and native context cursors. The generic gameplay-ready check honors server-authored modals
for every game. A menu library does not own these gameplay features.

## Configuration

Defaults live in `games/<game>/share/config.cfg`, installed with `make install-share`.

| Setting | WC3 | SC2 | WoW |
|---|---|---|---|
| `cl_input_mode` | `rts` | `rts` | `orbit` |
| `cl_start_menu` | `menu_main` | `menu_main` | `menu_login` |
| `cl_camera_scroll_speed` | 1400 world units/s | 350 world units/s | Not used by orbit |
| `cl_camera_edge_scroll` | 1 | 0 | Not used by orbit |
| `cl_camera_pan_plane` | 0: terrain trace | 1: camera-plane trace | Not used by orbit |
| `cl_camera_pitch` / `cl_camera_distance` | Not used by RTS | Not used by RTS | 342 degrees / 8 world units |
| `cl_camera_min_pitch` / `cl_camera_max_pitch` | Not used by RTS | Not used by RTS | 305 / 355 degrees |
| `cl_mouse_speed` | Not used by RTS | Not used by RTS | 0.18 degrees/pixel |
| `cl_click_threshold` | Not used by RTS | Not used by RTS | 10 pixels |

`cl_camera_edge_margin` defaults to 6 pixels. `camera edge 0|1` writes `cl_camera_edge_scroll`; other supported
camera subcommands still go to the server. Existing `camera_min_distance`, `camera_max_distance`, and `zoom_speed`
remain unchanged. Native game layouts and server camera limits remain authoritative.

SC2's `menu_main` setting preserves its previous startup command; its current menu stub does not implement that
command. This change does not add an SC2 main menu.

### Saved-setting compatibility

The generic command `cvar_alias old canonical` declares a compatibility name for an already defined cvar. Game
configs declare aliases for `wc3_camera_edge_scroll`, `wow_mouse_speed`, `wow_camera_min_pitch`,
`wow_camera_max_pitch`, and `wow_click_threshold`. The engine contains no table of game-specific aliases.

Both names resolve to one cvar, so later user/autoexec/command-line settings win in their ordinary execution order,
and archived output uses only the canonical name. Existing values/flags created by early command-line settings are
migrated. Declarations are allowed only during startup config processing, before `Cvar_EndConfig`: migration can
retire an old cvar allocation, so it must precede modules retaining cvar pointers. Existing aliases remain readable
and writable during gameplay. Repeating an identical declaration is harmless; retargeting or declaring a new alias
after startup is rejected with a diagnostic. Alias declarations belong in shipped defaults, not generated user config.

## Groups and target reconciliation

RTS profiles retain assign/add/recall and double-tap camera focus. Orbit profiles store one target per group:
assign replaces it, add fills an empty group while retaining an existing target, and recall selects it. Orbit recall
does not pan the character-follow camera. WoW keeps its action-bar number bindings; users can opt into group bindings:

```cfg
bind CTRL+1 "group assign 1"
bind ALT+1 "group 1"
```

Orbit clicks and recalls use `CL_ApplySelection` and the common `cl.selection` hint. `Wow_SelectEntity` emits the
existing `svc_set_selection` message, so explicit selection, target cycling, interaction, and rejected selections
reconcile the same cache. No new selection opcode or multi-unit WoW behavior was introduced. Group memberships
still reset at map boundaries and are not pruned merely because a snapshot cannot currently see a member.

## Order-marker media

`CS_ORDER_MARKER` (slot 11) carries the server-authored point-order model path. WC3 resolves `TargetPointConfirm`
from the active `[Default]` skin; the stock War3.mpq value is `UI\Feedback\Confirmation\Confirmation.mdl`.
The client precaches this resource in `CL_PrepRefresh`, replaces/releases it on late configstring changes, and releases
it in `CL_ClearState`. An empty slot explicitly disables marker drawing. A missing WC3 skin field or failed model
load is diagnosed; the client does not substitute a hardcoded game asset. WC3 test skin data includes the native key.
This adds meaning to a previously unused configstring slot without changing message framing; old clients do not
consume this new slot, so overriding marker art requires an updated client.

## Verification and remaining boundaries

Coverage includes config alias precedence/migration/lifecycle, order-marker precache/replacement/clear through
`svc_configstring`, both input profiles in one build, pan surface dispatch, single-target group recall without camera
messages, and WoW authoritative target reconciliation. Existing group/key/network tests retain RTS coverage.

```sh
make -j4 openwarcraft3 openwow opensc2 install-share
make -j4 test TEST_JOBS=4
build/bin/openwarcraft3-tests -data build/tests +dedicated 1 +test 'client_input.*'
build/bin/openwarcraft3 -data 'data/Warcraft III' +set vid_hidden 1 +map 'Maps/Campaign/Human02.w3m' +com_frame_limit 5
```

This is shared input source, not runtime game-module switching. Game-dependent view/render structs, native game
camera translation in `cl_view.c`, server-to-local-client session callbacks, legacy menu HUD data, and bindable
queue-modifier work remain separate changes. The existing Shift queue behavior is preserved.

See also: [architecture review](multi-game-review.md), [runtime config](runtime.md), and
[control groups](../games/warcraft-3/control-groups.md).
