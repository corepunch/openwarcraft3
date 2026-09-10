# Retail Warcraft III camera tracing

This documents the repeatable retail-reference workflow used for the
`Human02Interlude.w3m` opening cinematic. The reference was Warcraft III ROC
1.29.2 running under Wine. The original campaign map was never modified.

## Workflow

1. Extract a copy of the map into `build/retail-camera-trace/`.
2. Edit only the copied `war3map.j`.
3. Add a passive JASS sampler around the existing cinematic.
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
build/bin/mpqtool -mpq "$OPENREALM_ROOT/data/Warcraft III/War3.mpq" \
  cat 'Maps/Campaign/Human02Interlude.w3m' > "$trace/Human02Interlude-original/Human02Interlude.w3m"
build/bin/mpqtool -mpq "$trace/Human02Interlude-original/Human02Interlude.w3m" \
  cat 'war3map.j' > "$trace/Human02Interlude-original/war3map.j"
cp "$trace/Human02Interlude-original/Human02Interlude.w3m" \
   "$trace/Human02Interlude-instrumented/Human02Interlude.w3m"
cp "$trace/Human02Interlude-original/war3map.j" \
   "$trace/Human02Interlude-instrumented/war3map.j"
```

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

For this map the legacy wrapper is 512 bytes, the footer is 260 bytes, and
the payload created by `smpq -M 1` contains the map files plus the generated
`(listfile)` and `(attributes)` entries. The final `mpqtool ls` check must show
the normal root-level `war3map.*` names. Keep the original map and its wrapper
as comparison inputs; do not repack the campaign archive in place.

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
    call BJDebugMsg("CAMTRACE n=" + I2S(udg_CameraTraceSample) + " t=" + R2S(udg_CameraTraceTime) + " label=" + label +
        " tx=" + R2S(GetCameraTargetPositionX()) + " ty=" + R2S(GetCameraTargetPositionY()) + " tz=" + R2S(GetCameraTargetPositionZ()) +
        " ex=" + R2S(GetCameraEyePositionX()) + " ey=" + R2S(GetCameraEyePositionY()) + " ez=" + R2S(GetCameraEyePositionZ()) +
        " dist=" + R2S(GetCameraField(CAMERA_FIELD_TARGET_DISTANCE)) + " aoa=" + R2S(GetCameraField(CAMERA_FIELD_ANGLE_OF_ATTACK)) +
        " rot=" + R2S(GetCameraField(CAMERA_FIELD_ROTATION)) + " fov=" + R2S(GetCameraField(CAMERA_FIELD_FIELD_OF_VIEW)) +
        " roll=" + R2S(GetCameraField(CAMERA_FIELD_ROLL)) + " zoff=" + R2S(GetCameraField(CAMERA_FIELD_ZOFFSET)) +
        " farz=" + R2S(GetCameraField(CAMERA_FIELD_FARZ)))
endfunction

function CameraTraceTick takes nothing returns nothing
    if not udg_CameraTraceEnabled then
        return
    endif
    set udg_CameraTraceSample = udg_CameraTraceSample + 1
    set udg_CameraTraceTime = udg_CameraTraceTime + 0.25
    call CameraTraceSnapshot("periodic")
endfunction

function CameraTraceStart takes nothing returns nothing
    set udg_CameraTraceEnabled = true
    set udg_CameraTraceSample = 0
    set udg_CameraTraceTime = 0.0
    if udg_CameraTraceTimer == null then
        set udg_CameraTraceTimer = CreateTimer()
    endif
    call CameraTraceSnapshot("start")
    call TimerStart(udg_CameraTraceTimer, 0.25, true, function CameraTraceTick)
endfunction

function CameraTraceStop takes nothing returns nothing
    if not udg_CameraTraceEnabled then
        return
    endif
    call CameraTraceSnapshot("stop")
    set udg_CameraTraceEnabled = false
    if udg_CameraTraceTimer != null then
        call PauseTimer(udg_CameraTraceTimer)
    endif
endfunction
```

Use `0.05` in both places for a fine-grained transition trace. The elapsed
time is an accumulated sampler time, not a claim about wall-clock precision.

The copied script added three helpers:

- `CameraTraceSnapshot(label)` prints one machine-readable line containing the
  target XYZ, eye XYZ, distance, angle of attack, rotation, FOV, roll, Z offset,
  and far Z.
- `CameraTraceStart()` creates a timer and samples the local camera.
- `CameraTraceStop()` destroys the timer.

The sampler was started immediately before the existing opening cinematic and
stopped after the existing Scene 1 sequence. It did not change camera calls,
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
was the legacy 512-byte Warcraft map wrapper followed by an MPQ payload with
the expected generated `(attributes)` and `(listfile)` entries. The compressed
development package was rejected by 1.29, and several other wrapper/payload
variants loaded the loading screen and then crashed before the map became
playable.

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

The original 0.05-second timer emits 20 samples per second. Each sample has
many fields, so it quickly fills the debug message area and wraps on screen.
Use one of these modes depending on the question being answered:

| Mode | Sampling | Use |
| --- | ---: | --- |
| Transition | snapshots immediately before, during, and after a camera call | destination/start-state bugs |
| Normal | 0.25 s | ordinary cinematic comparison |
| Fine | 0.05 s | short interpolation or angle-wrap investigations |
| Event-only | no periodic timer | settled camera values and trigger ordering |

For normal comparison, change the copied script's timer period from `0.05` to
`0.25`; do not change the cinematic's authored waits or camera durations. For
the least spam, keep `CameraTraceSnapshot()` but call it only at named points:

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
