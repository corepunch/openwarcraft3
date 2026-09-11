# Coordinates and camera angles

## World and asset spaces

All three current game backends expose **Z-up world coordinates** to the engine. World positions, camera targets,
collision, picking, lighting and culling must agree before projection. OpenGL eye space looks down -Z with +Y screen-up;
that is a view-matrix convention, not a reason to swap world Y/Z.

WoW has several on-disk spaces, not one global Y-up world:

- Gameplay/SQL positions, WMO vertices and M2 vertices are Z-up.
- ADT object placements (MODF/MDDF) encode height in Y and horizontal coordinates relative to the map corner.
  `CM_WowObjectPoint(x,y,z)` maps them to `(center-z, center-x, y)`, where `center = 32 * WOW_ADT_SIZE`.
- MODF rotations are placement angles, not camera angles. `Wow_PlacementMatrix` in
  `games/world-of-warcraft/common/wow_coords.h` owns their conversion for both WMO rendering and collision.
  It uses the equivalent rotation `Rz(rot.y+180) * Ry(rot.x) * Rx(rot.z)` instead of the old permutation basis followed
  by three separate rotations. The old chain was `B * Ry(y-270) * Rz(-x) * Rx(z-90)`, with `B(x,y,z)=(z,x,y)`.
  Scale 1024 is unity; an absent/zero MODF scale means unity.
- WMO child doodads retain their local MODD quaternion/translation and compose with the parent placement matrix.

Keep model buffers in their native space and apply placement at the model-to-world boundary. Deferring this to the
final camera matrix would also require migrating collision, terrain queries, bounds, normals, lights and every other
world-space consumer. It cannot correct just the WMO draw path. The shared placement helper avoids renderer/collision
drift while leaving local vertex data intact.

## Camera wire contract

`playerState.viewangles` and `viewCamera_t.viewangles` are three **view rotation Euler components in degrees**.
They are not world positions, nor three interchangeable gameplay yaw/pitch/roll values. `ROTATE_ZYX` builds
`Rx(x) * Ry(y) * Rz(z)`. The shared client converts these samples to quaternions, slerps them, and builds
`T(0,0,-distance) * R * T(-target)`.

Identity therefore looks straight down world -Z. X is orbit tilt from that direction, Z is the world-to-view yaw
rotation, and Y is the middle Euler rotation (not an independent roll about the final sight line). Existing comments
calling the tuple `{pitch, roll, yaw}` are historical shorthand. Rename/retype only with a full consumer and serializer
audit; a named struct alone cannot reconcile different reference directions or rotation orders.

| Producer | Native angles | Conversion to the existing view Euler tuple |
|---|---|---|
| WC3 | Existing JASS orbit values | Existing WC3 conversion, unchanged |
| SC2 | Map/Galaxy downward pitch `p`, camera azimuth `y` | `{p-90, 0, y-180}` |
| WoW | Downward pitch `p`, actor heading `y` from +X toward +Y | `{p-90, 0, 90-y}` |

`SC2_EulerFromCamera` / `SC2_CameraFromEuler` and `Wow_EulerFromCamera` / `Wow_CameraFromEuler` are the game-boundary
adapters. Both directions matter: manual view input returns through the inverse adapter before updating native game
camera state or actor movement. SC2 height offset remains a target-height value, never an angle. WoW's old wrapped
negative pitch remains internal to `wow_move`; input limits use canonical tilt (-85..-35 for downward pitch 5..55).
Do not add game conditionals or a second orientation representation to the shared client or wire protocol.

## Confirmed camera regressions

The September 2026 Euler-snapshot unification retained WC3's view contract but did not fully convert the other games:

- SC2's authored yaw was copied directly to the view rotation. The TRaynor01 bridge appeared from the opposite side
  compared with the retail reference. At native pitch 34.878 and yaw 193.947, runtime tracing recovered an eye offset
  `(5.971,24.043,17.268)`; the corrected azimuth places the eye at `(-5.971,-24.043,17.268)`.
  Apply this convention in the camera adapter, not to map coordinates or light directions.
- WoW published downward pitch 32 as orbit tilt 32 and heading 0 as view yaw 0. The measured eye offset at distance
  8.5 was `(0,4.504,7.209)`: sideways and steeply overhead. The correct view tuple is `{-58,0,90}`, giving offset
  `(-7.209,0,4.504)` behind the actor's +X heading.

History: `ea66b69ad` introduced shared Euler snapshots; `f9083e76c` introduced SC2 pitch conversion, leaving native yaw
unchanged. WMO renderer/collision placement code was duplicated; consolidating it is independent of these camera fixes.

## Verification

```sh
make -j8 opensc2 openwow openwarcraft3 install-share
make test-sc2 test-wow-appearance test-wow-game test-client-camera
build/bin/opensc2 -data data/StarCraft2 +vid_hidden 1 +com_maxfps 60 +map Maps/Campaign/TRaynor01.SC2Map +screenshot 10 +com_frame_limit 300
build/bin/openwow -data data/world-of-warcraft +vid_hidden 1 +com_maxfps 60 +set wow_playerinfo '\race\Orc\sex\Male\class\1\appearance\0' +map playercreate +screenshot 10 +com_frame_limit 300
build/bin/openwarcraft3 -data 'data/Warcraft III' +vid_hidden 1 +map Maps/Campaign/Human02.w3m +screenshot 10 +com_frame_limit 40
```

Use `com_maxfps` for bounded render captures: an uncapped hidden client can exhaust even 1000 iterations before the
server's first post-load snapshot. Do not use fast-forward for rendering. Temporary logs at `Matrix4_fromViewQuat`
can recover the eye from the inverse view matrix; remove them after diagnosis.

Tests check cardinal camera headings through the actual Euler/quaternion matrices, upright screen projection, WoW
controller input/movement, terrain-relative camera interpolation, and the reduced WMO matrix against its previous
basis chain for translated, scaled and tilted placements. Compare engine screenshots with the reference as well.
The per-game Makefiles explicitly depend on common headers: inline adapter edits must rebuild the game library,
renderer and client together. Without that dependency, unit tests could pass while the live game still used old angles.

See also: [client camera samples](docs/architecture/client.md), [shared input](docs/architecture/shared-input.md),
[SC2 rendering](docs/games/starcraft-2/terrain-and-world-rendering.md).
