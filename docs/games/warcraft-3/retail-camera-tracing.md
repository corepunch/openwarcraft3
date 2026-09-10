# Retail Warcraft III camera tracing

This documents the repeatable retail-reference workflow used for the
`Human02Interlude.w3m` opening cinematic. The reference was Warcraft III ROC
1.29.2 running under Wine. The original campaign map was never modified.

## Workflow

1. Extract a copy of the map into `build/retail-camera-trace/`.
2. Edit only the copied `war3map.j`.
3. Add passive JASS snapshots around important cinematic events.
4. Repack the copied map as a legacy Warcraft III map.
5. Launch that copied map directly in retail.
6. Copy `CAMTRACE` messages from the game and parse them with
   `tools/retail_camera_trace.py`.

The generated working files are described in
`build/retail-camera-trace/README.md`. They are build artifacts, not source
assets.

## Exact extraction and repacking

Build the archive tool first, then use a workspace-local root variable rather
than hard-coding an installation path:

```sh
make mpqtool
export OPENREALM_ROOT=/path/to/open-realm
trace="$OPENREALM_ROOT/build/retail-camera-trace"
mkdir -p "$trace/Human02Interlude-original" "$trace/Human02Interlude-instrumented"
build/bin/mpqtool -mpq "$OPENREALM_ROOT/data/Warcraft III/War3Local.mpq" \
  cat 'Maps/Campaign/Human02Interlude.w3m' > "$trace/Human02Interlude-original/Human02Interlude.w3m"
build/bin/mpqtool -mpq "$trace/Human02Interlude-original/Human02Interlude.w3m" \
  cat 'war3map.j' > "$trace/Human02Interlude-original/war3map.j"
cp "$trace/Human02Interlude-original/Human02Interlude.w3m" \
   "$trace/Human02Interlude-instrumented/Human02Interlude.w3m"
cp "$trace/Human02Interlude-original/war3map.j" \
   "$trace/Human02Interlude-instrumented/war3map.j"
```

Campaign maps in this installation are in `War3Local.mpq`, not `War3.mpq`.
Extract the untouched control first and verify it in retail before editing or
repacking anything:

```sh
control="$trace/retail-original/Human02Interlude.w3m"
mkdir -p "$(dirname "$control")"
build/bin/mpqtool -mpq "$OPENREALM_ROOT/data/Warcraft III/War3Local.mpq" \
  cat 'Maps/Campaign/Human02Interlude.w3m' > "$control"
build/bin/mpqtool -mpq "$control" ls
```

For ROC 1.29, use `-loadfile` without `-launch`; this build otherwise returned
to the main menu. Use `-window -graphicsapi OpenGL2` under Wine when the
default renderer produces a black screen:

```sh
wine "$WAR3_EXE" -window -graphicsapi OpenGL2 \
  -loadfile "Z:$control"
```

Keep the Linux path and Wine path on one physical shell line. If the untouched
extracted control does not load, stop debugging JASS or repacking; the
installation, Wine launch, or map path is the problem. The successful control
run differed from the earlier `Human02Interlude-original` build artifact,
proving that artifact was not the untouched retail source.

`mpqtool pack-legacy` is not an in-place update operation. It creates a new
MPQ containing only the files passed to that command, so using it against a
copied payload would discard the rest of the map. Use `mpqtool` for inspection
and extraction, and use the StormLib-based `smpq` utility to build the complete
legacy payload:

```sh
command -v smpq                  # provided by StormLib; required for repacking
map="$trace/Human02Interlude-instrumented/Human02Interlude.w3m"
files="$trace/Human02Interlude-instrumented/map-files"
mkdir -p "$trace/Human02Interlude-instrumented/repack"
(cd "$files" && smpq -c -M 1 \
  "$trace/Human02Interlude-instrumented/repack/map-payload.mpq" *)
dd if="$map" of="$trace/Human02Interlude-instrumented/repack/map-header.bin" \
  bs=512 count=1 status=none
tail -c 260 "$map" > "$trace/Human02Interlude-instrumented/repack/map-footer.bin"
cat "$trace/Human02Interlude-instrumented/repack/map-header.bin" \
    "$trace/Human02Interlude-instrumented/repack/map-payload.mpq" \
    "$trace/Human02Interlude-instrumented/repack/map-footer.bin" \
  > "$trace/Human02Interlude-instrumented/Human02Interlude-CAMTRACE-legacy.w3m"
build/bin/mpqtool -mpq \
  "$trace/Human02Interlude-instrumented/Human02Interlude-CAMTRACE-legacy.w3m" ls
```

For this map the legacy wrapper is 512 bytes and the footer is 260 bytes. The
repacker used here is the apt-installed `/usr/bin/smpq` 1.6 using StormLib
9.30. The final `mpqtool ls` check must show the normal root-level
`war3map.*` names. Keep the MPQ-extracted control map and its wrapper as
comparison inputs; do not repack the campaign archive in place.

The JASS edits belong at these stable generated-script locations:

- add the trace globals and helper functions after the existing `globals` block;
- call `CameraTraceStart()` at the beginning of
  `Trig_CinematicStart_Actions`;
- call snapshots immediately before and after the existing
  `CameraSetupApplyForPlayer` calls in `Trig_Scene1_Actions`;
- call `CameraTraceStop()` in both the skip action and the normal end of Scene 1.

The generated `war3map-camera-trace.patch` records the exact Human02 edit and
can be applied with `patch -p0` when the generated build directory is present.
If it is absent, use the locations above and the helper contract below; do not
edit the source campaign archive.

## JASS instrumentation

The minimal helper body is:

```jass
globals
    timer udg_CameraTraceTimer = null
    integer udg_CameraTraceSample = 0
    real udg_CameraTraceTime = 0.0
    boolean udg_CameraTraceEnabled = false
endglobals

function CameraTraceSnapshot takes string label returns nothing
    if not udg_CameraTraceEnabled then
        return
    endif
    set udg_CameraTraceSample = udg_CameraTraceSample + 1
    set udg_CameraTraceTime = TimerGetElapsed(udg_CameraTraceTimer)
    call BJDebugMsg("CAMTRACE n=" + I2S(udg_CameraTraceSample) + " t=" + R2S(udg_CameraTraceTime) + " label=" + label +
        " tx=" + R2S(GetCameraTargetPositionX()) + " ty=" + R2S(GetCameraTargetPositionY()) + " tz=" + R2S(GetCameraTargetPositionZ()) +
        " ex=" + R2S(GetCameraEyePositionX()) + " ey=" + R2S(GetCameraEyePositionY()) + " ez=" + R2S(GetCameraEyePositionZ()) +
        " dist=" + R2S(GetCameraField(CAMERA_FIELD_TARGET_DISTANCE)) + " aoa=" + R2S(GetCameraField(CAMERA_FIELD_ANGLE_OF_ATTACK)) +
        " rot=" + R2S(GetCameraField(CAMERA_FIELD_ROTATION)) + " fov=" + R2S(GetCameraField(CAMERA_FIELD_FIELD_OF_VIEW)) +
        " roll=" + R2S(GetCameraField(CAMERA_FIELD_ROLL)) + " zoff=" + R2S(GetCameraField(CAMERA_FIELD_ZOFFSET)) +
        " farz=" + R2S(GetCameraField(CAMERA_FIELD_FARZ)))
endfunction

function CameraTraceStart takes nothing returns nothing
    set udg_CameraTraceSample = 0
    set udg_CameraTraceTime = 0.0
    set udg_CameraTraceEnabled = true
    set udg_CameraTraceTimer = CreateTimer()
    call TimerStart(udg_CameraTraceTimer, 3600.00, false, null)
    call CameraTraceSnapshot("start")
endfunction

function CameraTraceStop takes nothing returns nothing
    if not udg_CameraTraceEnabled then
        return
    endif
    call CameraTraceSnapshot("stop")
    set udg_CameraTraceEnabled = false
    call PauseTimer(udg_CameraTraceTimer)
endfunction
```

The timer is used only as an elapsed-time clock; it does not invoke a periodic
callback. Every snapshot increments the sample number and records the elapsed
time at the named event.

The copied script added three helpers:

- `CameraTraceSnapshot(label)` prints one machine-readable line containing the
  target XYZ, eye XYZ, distance, angle of attack, rotation, FOV, roll, Z offset,
  and far Z.
- `CameraTraceStart()` creates an elapsed-time clock and samples the local camera.
- `CameraTraceStop()` records the final state and pauses the clock.

The trace records the cinematic start, the state before and after `DummyStart`,
the states immediately before and after the `TowerHigh` and `TowerLow`
applications, and the Scene 1 end/skip paths. It does not change camera calls,
waits, transmissions, units, terrain, fog, game speed, or trigger order.

`BJDebugMsg` was the only initially reliable output channel. Warcraft JASS has
no general filesystem-write native, so the first trace had to be copied from
the game UI. The values are raw retail getter values: angular fields and FOV
appeared as radians in the trace, while distance and positions remained world
units.

## Retail launch

For the installed 1.29 executable, this was the working form:

```sh
export OPENREALM_ROOT=/path/to/open-realm
map="$OPENREALM_ROOT/build/retail-camera-trace/Human02Interlude-instrumented/Human02Interlude-CAMTRACE-legacy.w3m"
wine "$OPENREALM_ROOT/data/Warcraft III/Warcraft III.exe" \
  -window -graphicsapi OpenGL2 \
  -loadfile "$(winepath -w "$map")"
```

`Z:` is Wine's mapping of the Linux filesystem. Verify the Linux source first:

```sh
map="$OPENREALM_ROOT/build/retail-camera-trace/Human02Interlude-instrumented/Human02Interlude-CAMTRACE-legacy.w3m"
test -f "$map" && echo "Linux path exists" || echo "Missing"
winepath -w "$map"
```

Keep the Wine path on one shell line. A newline embedded in a quoted path was
passed through to `winepath` during the investigation and produced an invalid
Windows path. Shell parentheses also caused `wine cmd /c` syntax errors; a
simple `test -f` plus `winepath -w` check was less error-prone.

## Packing lessons

The 1.29 client was sensitive to the map package shape. The reliable package
was the legacy 512-byte Warcraft map wrapper followed by a complete MPQ
payload. The compressed development package was rejected by 1.29, and several
other wrapper/payload variants loaded the loading screen and then crashed
before the map became playable. Always test the untouched map extracted from
`War3Local.mpq` first; a derived copy going to the menu does not prove that
the retail installation or launch command is wrong.

Do not overwrite the campaign map in `data/Warcraft III`. Pack into a separate
file with a distinct name and preserve the original extracted files for binary
comparison. A repacked uninstrumented map is a useful control: if it crashes,
the packer/package is the problem, not JASS instrumentation.

## What did not work

- Launching an invalid or unsupported package without the compatible legacy
  wrapper led to the menu, an empty error dialog, or an access-violation report.
- Repacking with compression produced a map that 1.29 could reject or crash
  after the loading screen.
- The original retail executable sometimes showed a black screen under Wine;
  `-window -graphicsapi OpenGL2` made the client usable for this test.
- Wine's NTLM warnings (`ntlm_auth`/winbind) were startup noise, not the cause
  of the map crash.
- `BJDebugMsg` did not create a trace file. It only displayed messages in the
  game UI, and the long camera line wrapped visually.
- Starting OpenRealm with a temporary data directory containing only selected
  archives did not load the map reliably. The complete Warcraft data layout and
  the expected map path are safer for OpenRealm runs.

## Reducing output noise

The default trace is event-only, so it avoids filling the debug message area
with repeated identical states. Use one of these modes depending on the
question being answered:

| Mode | Sampling | Use |
| --- | ---: | --- |
| Transition | snapshots immediately before, during, and after a camera call | destination/start-state bugs |
| Event-only | named events | settled camera values and trigger ordering |
| Fine | 0.05 s timer | short interpolation or angle-wrap investigations |

For a transition investigation, temporarily add a timer callback to the copied
script; do not change the cinematic's authored waits or camera durations. For
event-only tracing, keep `CameraTraceSnapshot()` and call it only at named
points:

```jass
call CameraTraceSnapshot("before-tower-high")
call CameraSetupApply(towerHigh, true, false)
call CameraTraceSnapshot("after-tower-high")
```

The sampler should remain optional and local-player-only. Do not add permanent
per-frame `BJDebugMsg` calls to production engine code. A future file-output
experiment could investigate the 1.29 `PreloadGen*` natives, but that was not
validated here and must not be treated as a reliable logging backend without a
controlled test.

## Parsing

Save copied messages as `retail-camtrace.txt` and run:

```sh
python3 tools/retail_camera_trace.py retail-camtrace.txt -o retail-camtrace.csv
```

The parser retains the raw fields and adds `dx`, `dy`, `dz`, horizontal
distance, and Euclidean eye-to-target distance. These derived values are
diagnostics only; they do not replace the retail camera fields.

## OpenRealm engine-side trace

OpenRealm can emit the same event-only format without modifying or repacking
the map. Enable the opt-in cvar before loading the original map:

```sh
build/bin/openwarcraft3 -data 'data/Warcraft III' \
  +set vid_hidden 1 +set wc3_camera_trace 1 \
  +map 'Maps/Campaign/Human02Interlude.w3m' +com_frame_limit 300 \
  > openrealm-camtrace.log 2>&1
```

The trace is emitted by the `CameraSetupApply*` camera natives after they
assign runtime state. It is event-only, disabled by default, and reports the
same logical fields and reconstructed eye/target geometry as the JASS getters.
It does not sample every frame, and it does not depend on `BJDebugMsg` or the
instrumented retail map.

## Verified reference observations

For the Human02 opening shot, retail kept target X/Y at approximately
`(-4909.3, 2474.5)` and distance at `3348.6`. Target Z and Z offset changed
together, while `targetZ - zOffset` stayed approximately `454.106`. The first
and final low shot reported approximately:

```text
targetZ=831.507 eye=(-1656.662,1729.546,551.303)
aoa=0.084 rotation=2.916 fov=1.571 zoff=377.400 farz=4000.000
```

Near the tower, target Z, angle of attack, FOV, and Z offset interpolated while
distance remained fixed. These are retail measurements from this map/version,
not universal assumptions about every Warcraft camera setup.

## See also

- [WC3 Cinematic / Cutscene System](cinematics.md)
- [Client camera architecture](../../architecture/client.md)
- [Camera viewport and cinematic state](cinematics.md)
