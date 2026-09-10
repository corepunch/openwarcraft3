# Warcraft III Environmental Terrain Fog

## Contract

Environmental terrain fog (distance mist / Z fog) is a presentation system, not gameplay fog of war. `SetTerrainFogEx` changes one
map-lifetime fog state owned by the WC3 game module. The game publishes that state through the generic `CS_SCENE_FOG` configstring,
the client copies enabled linear parameters into `viewDef_t`, and the WC3 renderer applies them to terrain, cliffs, water, and eligible
MDX material layers. It does not mutate exploration, current vision, entity visibility, minimap fog, camera state, or far clipping.

The generic wire value is:

```text
style start end density red green blue
```

`style == 0` disables scene fog. Positive styles are enabled. The client deliberately does not know Warcraft style names; it currently
feeds every positive style through the renderer's existing linear start/end path.

## JASS Mapping And State

The supplied Warsmash reference maps the integer passed to `SetTerrainFogEx` through `style + 1` into its internal `FogStyle` enum:

| JASS style | Stored `wc3EnvironmentFogState_t.style` | Meaning |
| ---: | ---: | --- |
| `-1` | `0` | none |
| `0` | `1` | linear |
| `1` | `2` | exponential 1 identifier |
| `2` | `3` | exponential 2 identifier |
| other | `0` | disabled |

`SetTerrainFogEx(style, start, end, density, r, g, b)` stores all seven values and republishes `CS_SCENE_FOG`. RGB inputs stay
normalized floats; they are not interpreted as 0..255 values. The style and density remain on the wire even though the generic
renderer does not yet consume density or distinguish the two exponential identifiers.

`level.environment_fog` contains `active`, `defaults`, and a `defaults_valid` latch. On map initialization, `active` starts disabled. This matches the
supplied Warsmash tree: loading `DefaultZFog` does not automatically copy it into `worldScene.fogSettings`. `ResetTerrainFog()` copies
the saved default state into the active state and republishes it when that row was present; if no default row was loaded, reset is a no-op, matching the supplied Warsmash guard.

The default reset target comes from the merged MiscData cache's `[DefaultZFog]` row. `fs_expansion == 0` selects the RoC entry and a
non-zero value selects the TFT entry. `Style`, `Start`, `End`, and `Density` use that version index. `Color` is stored as four values
per version in A,R,G,B order; OpenRealm retains RGB after dividing the authored 0..255 values by 255. `war3mapMisc.txt` participates in
the same existing merged MiscData cache, so its row can override earlier data through the normal loader.

The W3I parser also retains `fogStyle`, `fogStartZ`, `fogEndZ`, `fogDensity`, and `fogColor`. This patch intentionally does not make
those parsed fields a second runtime authority: the supplied Warsmash tree does not directly copy its W3I fog fields into the scene
fog state, and establishing retail precedence between W3I, map script calls, and `DefaultZFog` needs separate evidence.

## Renderer Path

The ownership path is:

```text
JASS SetTerrainFogEx / ResetTerrainFog
    -> level.environment_fog
    -> CS_SCENE_FOG
    -> client V_UpdateSceneFog()
    -> viewDef.fogEnable / fogStart / fogEnd / fogColor
    -> WC3 terrain + MDX renderers
```

The WC3 W3M renderer copies the view fog state into `shader_default` before its ground/cliff pass and again before its alpha water
pass. The default shader then applies the same linear blend convention already used by the model shader:

```text
clarity = clamp((end - depth) / (end - start), 0, 1)
result  = mix(fogColor, shadedColor, clarity)
```

A zero-length range is guarded in the shader so malformed `start == end` input cannot produce a division-by-zero NaN/Inf. The existing
OpenRealm depth expression remains unchanged; changing depth-space semantics requires a separate retail/rendering comparison rather
than being bundled into the JASS wiring change.

MDX layers apply environmental fog only when all of these are true:

- scene fog is enabled;
- the layer does not carry `MODEL_GEO_UNFOGGED`;
- blend mode is `NONE`, `ALPHAKEY`, or `BLEND`.

Additive/modulate-style layers retain the pre-existing exclusion because blending those transparent effects toward an opaque fog
colour produces solid fog-coloured quads. `MODEL_GEO_UNSHADED` controls lighting only; environmental fog is applied after that lighting
branch, so `Unshaded` no longer accidentally means `Unfogged`.

## Save / Load

Save format version 16 serializes both the active environmental fog state, its `DefaultZFog` reset target, and the validity latch through the level field
schema. After a successful load, `G_EnvironmentFogPublish()` republishes the restored active state so connected clients do not retain
the freshly reloaded map's pre-save presentation.

Adding fields to `wc3EnvironmentFog_t` requires a matching `g_save.c` field-schema update and, when layout/meaning changes, another save
format version bump.

## Known Gaps

- `SetTerrainFog` remains a placeholder. The supplied Warsmash implementation does not implement that legacy native, so this patch does
  not infer its contract.
- `SetUnitFog` remains unrelated and unimplemented.
- `EXPONENTIAL_1`, `EXPONENTIAL_2`, and `density` are stored faithfully but currently render with the same linear start/end equation.
  This deliberately matches the supplied Warsmash shader path, which also uploads density/style but uses one linear fog calculation for
  every enabled style. True retail exponential equations require separate verification.
- Generic `cparticle_t` particles/ribbons do not yet carry an authored MDX `Unfogged` bit or environmental-fog state. Extending that
  path requires a deliberate generic particle contract rather than WC3-specific fields in engine particle structures.
- W3I environmental-fog fields remain parsed metadata rather than an independent runtime source until precedence is established.

## Verification

Automated WC3 API tests cover `SetTerrainFogEx` state/wire publication, `ResetTerrainFog` restoration, and invalid style disabling. The
patch was designed so renderer behavior can be checked visually without debug logging.

Useful runtime checks are:

1. Load a map whose script calls `SetTerrainFogEx(0, ...)`; verify ground, cliffs, water, units, buildings, and ordinary doodads blend
   toward the same colour with distance.
2. Move and rotate the gameplay/cinematic camera; the fog band must follow fragment depth without changing gameplay visibility.
3. Exercise an MDX model containing both `Unshaded` and `Unfogged` material flags. `Unshaded` layers should still mist; `Unfogged`
   layers should not.
4. Check additive glows and spell-like transparent layers for fog-coloured rectangles.
5. Call `ResetTerrainFog()` after a scripted override and verify the `[DefaultZFog]` values return.
6. Save while scripted fog is active, change/reload state, then load the save; the saved active fog should be republished.

See [Fog And Cinematic Visibility](fog-and-cinematics.md) for the independent gameplay fog-of-war system.
