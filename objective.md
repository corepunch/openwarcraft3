# Current camera investigation objective

## Objective

Measure the retail Warcraft III terrain and camera reference height at the
Human02 opening-shot target, then use that evidence to make OpenRealm match
retail without adding a map-specific height constant.

The target point is:

```text
x = -4909.3
y = 2474.5
```

The immediate experiment is to compare retail's:

- `GetLocationZ(-4909.3, 2474.5)`;
- `GetTerrainCliffLevel(-4909.3, 2474.5)`;
- camera `targetZ`;
- camera `zoff`;
- `targetZ - zoff`.

## What we know

These are measurements from Warcraft III ROC 1.29.2 for
`Human02Interlude.w3m`, not universal Warcraft rules:

- Retail keeps the opening-shot target XY at approximately `(-4909.3, 2474.5)`.
- Retail keeps target distance at approximately `3348.6` during the tower
  transition.
- At the settled low shot, retail reports approximately:

  ```text
  targetZ = 831.507
  zoff    = 377.400
  targetZ - zoff = 454.107
  eye     = (-1656.662, 1729.546, 551.303)
  ```

- During the transition, target Z and Z offset change together while their
  difference remains approximately `454.106`.
- The newer boundary trace sampled the same low camera at `t=0.200`, immediately
  after the `TowerLow` setup, and again at `t=2.200`, after the two-second wait.
  Both samples reported `targetZ - zoff = 454.106`. The reference height is
  therefore established by the destination setup, not gradually corrected
  during the wait or introduced by the later `TowerHigh` setup.
- At the tower, the retail reference height is `198.106` units above the
  direct retail terrain query of `256.000`. This is evidence that the camera
  uses a separate terrain-derived height function near the cliff, rather than
  simply adding `CAMERA_FIELD_ZOFFSET` to `GetLocationZ(targetXY)`.
- Before the fix, OpenRealm retained a target reference of approximately
  `464.000`, leaving a measured vertical discrepancy of about `9.894` units.
- The W3E data around the tower contains direct vertex heights of `256`, but
  the 8x8 vertex neighborhood inside a 512x512 world-unit camera window
  averages to `456`. Applying the map's two-unit camera-sample correction
  produces `454.000`, matching retail's `454.106` within `0.106` units.
- OpenRealm's target XY, distance, angle-of-attack, rotation, FOV, and Z-offset
  progression otherwise closely follow the retail trace.
- Before the fix, OpenRealm used the retained runtime target reference rather
  than a destination-derived camera height; its direct terrain query and the
  legacy `-48` camera offset therefore did not explain retail's `454.106`.
- A previous targeted trace showed that changing the target reference to the
  destination terrain height would be incorrect.
- Retail's first two seconds are hidden by the cinematic filter. That black
  interval is independent of the camera-height measurement.
- The current OpenRealm camera trace has a separate trace-reconstruction fix
  for realized eye state during transitions. It must not be confused with the
  unresolved target-height behavior.
- `screenshots/onlineinfo.md` describes secondary reverse-engineering evidence
  for a retail camera-height map that averages terrain over approximately
  512-unit regions and interpolates between those values. That information is
  not authoritative, but it is now consistent with the boundary trace and is
  a useful algorithm hypothesis to test against the map data.

## What we found

The retail quantity is a camera-specific neighborhood height, not the direct
bilinear terrain height:

1. OpenRealm now samples authoritative W3E vertex heights in the 512x512
   world-unit neighborhood around a camera setup destination.
2. The sample applies the two-unit layer correction before becoming the
   retained camera target reference.
3. The retained value is selected when the destination position changes, so
   camera transitions preserve it while authored camera fields interpolate.
4. The bounded OpenRealm trace reports `targetbase=454.000` and
   `targetZ=831.400` for the tower setup, compared with retail
   `targetbase=454.106` and `targetZ=831.506`.

`screenshots/onlineinfo.md` was useful as a hypothesis, but the W3E
neighborhood calculation and the OpenRealm trace are the authoritative evidence
for this implementation.

## Current measurement map

The temporary retail probe is:

```text
build/retail-camera-trace/Human02Interlude-instrumented/Human02Interlude-CAMTRACE-terrain-retail.w3m
```

It preserves the known-good MPQ layout and replaces only `war3map.j`. The
script reports `terrain`, `towerterrain`, and `towercliff` in each `CAMTRACE`
line. It also writes the preload output to `camtrace-terrain.txt` when the
cinematic reaches its normal or skip stop path.

Launch it from a graphical Wine session:

```sh
map="$PWD/build/retail-camera-trace/Human02Interlude-instrumented/Human02Interlude-CAMTRACE-terrain-retail.w3m"
wine "$PWD/data/Warcraft III/Warcraft III.exe" \
  -window -graphicsapi OpenGL2 \
  -loadfile "$(winepath -w "$map")"
```

The generated file can be located with:

```sh
find "$HOME/.wine/drive_c/users" -type f -name 'camtrace-terrain.txt' -print
```

The useful samples are `towerlow-after-0.10`, `towerlow-after-2.00`,
`before-towerhigh`, and the first `towerlow-tick`. The boundary samples show
when the reference height is selected; the transition sample confirms that it
remains constant while Z offset and the other camera fields interpolate.
Extract the `CAMTRACE` lines from the generated preload wrapper before parsing
them as CSV. The on-screen `BJDebugMsg` output remains a fallback if the file
is not generated.

## Implementation and verification

The fix is in `CM_GetCameraHeightAtPoint()`. It is map-derived and does not
contain Human02 coordinates or retail measurements. `G_ApplyCameraSetup()` uses
it when a setup changes the camera destination; ordinary terrain queries remain
unchanged for units, destructables, pathing, and gameplay placement.

The verified bounded run used:

```sh
build/bin/openwarcraft3 -data 'data/Warcraft III' \
  +set vid_hidden 1 +set wc3_camera_trace 1 \
  +map 'Maps/Campaign/Human02Interlude.w3m' +com_frame_limit 300 \
  > /tmp/openrealm-height-fixed.log 2>&1
```

The diagnostic trace includes `terrain` and `targetbase` fields. Temporary
map-specific `CAMHEIGHT` logs were removed after the comparison.

## Completion evidence

This objective is complete when we have:

1. retail terrain and cliff measurements at the tower target;
2. a documented explanation for the `454.106` reference height;
3. an OpenRealm change based on that explanation;
4. matching retail/OpenRealm transition boundary samples; and
5. passing focused and full test suites.

All five items are complete: the focused and full project tests pass with
`23580/23580` assertions.
