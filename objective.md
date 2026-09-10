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
- OpenRealm's corresponding target reference is approximately `459.000`,
  leaving a measured vertical discrepancy of about `4.894` units.
- OpenRealm's target XY, distance, angle-of-attack, rotation, FOV, and Z-offset
  progression otherwise closely follow the retail trace.
- Existing OpenRealm diagnostics showed the map terrain at the target as
  approximately `251` with a camera height offset of `-48`; this does not by
  itself explain retail's `454.106` reference height.
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

## What we are trying to find

We need to establish which retail quantity produces the approximately
`454.106` target reference:

1. Is it a smoothed/averaged terrain height sampled around the destination?
2. Is it a terrain/cliff-derived height with a fixed engine offset?
3. Is it authored camera/setup state independent of terrain?
4. Is it the previous camera target height carried into the setup?
5. Does retail apply a map/world coordinate conversion before exposing the
   camera getter values?

Only after this is known should OpenRealm's camera target-height calculation be
changed.

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

## Decision rule

Do not replace the direct terrain query with a hardcoded `454.106` value or a
Human02-specific correction. The retail terrain probe is now available and
shows that direct terrain does not account for the reference height. The next
diagnostic must compare candidate smoothed terrain calculations against the
exact `TowerLow` destination at the camera-setup boundary. Do not change
`target_height` or the camera composition formula until that calculation is
identified.

The eventual fix must:

- derive the height from authoritative map/camera state;
- preserve normal gameplay camera behavior;
- preserve the retail transition start state and interpolation;
- avoid a Human02-only branch or hardcoded `454.106` value;
- include a focused regression test for the corrected camera state.

## Completion evidence

This objective is complete when we have:

1. retail terrain and cliff measurements at the tower target;
2. a documented explanation for the `454.106` reference height;
3. an OpenRealm change based on that explanation;
4. matching retail/OpenRealm transition samples; and
5. passing focused and full test suites.
