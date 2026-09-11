# Sky Models and `CS_SKY`

## Contract

`CS_SKY` carries an optional registered model index for the current world. It is not a Quake II six-face sky name. The generic client
resolves the index through `cl.models[]` and publishes the handle as `viewDef.skyModel`; the shared renderer draws that model before
normal world geometry. A no-world/UI view clears the handle so a previous map cannot leak its sky into menus, loading views, or
other `RDF_NOWORLDMODEL` scenes.

The current producer is Warcraft III's JASS `SetSkyModel(string)`. An empty string publishes model index `0` and removes the sky.
Do not derive a sky path from W3I tileset fields or map names: the script-selected asset is authoritative.

## Warsmash Reference

The current supplied Warsmash checkout no longer contains this sky-model path. Historical Warsmash commit `60db46c8` implements
the reference lifecycle in `War3MapViewer.setSkyModel()` and `render()`:

- load the requested MDX;
- clear its model lights and force material layers unshaded;
- create one model instance and select sequence 0;
- on render, move that instance to `worldScene.camera.location`;
- advance its animation;
- render its opaque and translucent geometry before terrain/world geometry.

OpenRealm does not mutate the shared model asset to clear lights or material flags. Instead the transient sky entity uses
`RF_NO_LIGHTING`; OpenRealm MDX lights are entity-local shader inputs, so they do not illuminate the world scene. The sky is also
marked `RF_NO_FOGOFWAR` and `RF_NO_SHADOW`.

The historical Warsmash implementation is a renderer reference, not proof of every retail Warcraft III GL state detail. In
particular, do not infer unverified retail depth/fog equations from it.

## Data Flow

```text
JASS SetSkyModel(path)
        |
        v
gi.ModelIndex(path)
        |
        +--> CS_MODELS[index] = authored model path
        |
        `--> CS_SKY = decimal model index
                 |
                 v
          V_ConfigSkyModel()
                 |
                 v
          viewDef.skyModel
                 |
                 v
             R_DrawSky()
                 |
                 +--> origin = rendered camera eye
                 +--> sequence 0 at viewDef.time
                 +--> no lighting / FOW / shadow
                 `--> R_RenderModel() before R_DrawWorld()
```

`viewDef.target` is the rendered camera **focus**. It is not the sky origin. `Matrix4_getCameraMatrix()` derives the actual eye from
the same final orbit view used for projection and stores it in `viewDef.camerastate[0].eye`. This matters most for low or cinematic
cameras where the look-at target may be hundreds or thousands of world units from the eye. See [client camera samples](client.md).

## Animation

WC3 sky MDX models use sequence 0 in the Warsmash reference. `R_DrawSky()` asks the game renderer for indexed sequence `#0` every
frame; the WC3 MDX implementation maps `viewDef.time` into that sequence and writes the same value to `frame` and `oldframe`. This
advances node, geoset, material, texture-animation, and other ordinary sequence-driven tracks while preserving the existing global
sequence clock. Games that do not support the indexed selector leave the sky at its default pose.

OpenRealm currently uses the view render clock rather than retaining a separate per-`SetSkyModel` instance start timestamp. That
produces a continuously animated sequence 0, which is the compatibility behavior established here; exact phase-on-replacement is an
unverified retail detail and should not be changed without a reference trace.

## Render Ordering and State

The shared world pass is:

```text
R_SetupGL(false)
R_DrawSky()
R_DrawWorld()
R_DrawDecals()
R_DrawSplatRects()
R_DrawEntities()
...
```

`R_DrawSky()` temporarily disables frustum culling and requests no depth test/write around model submission, then restores those
shared states. Individual game model renderers may apply their own per-material state while submitting the model, so the exact retail
depth semantics remain a verification item rather than a documented guarantee. The durable compatibility requirements are the
camera-relative origin, unlit presentation, sequence-0 animation, and pre-world ordering.

## Quake II Distinction

Quake II also uses `CS_SKY`, but its value is a logical environment name. The renderer loads six `env/<name><suffix>` images and
draws an untranslated skybox. OpenRealm intentionally reuses only the configstring ownership pattern, not Quake II's asset format.
WC3 supplies a normal model through its own game module.

## Map Data Audit

- Warcraft III W3I stores `mainGroundType` and, in newer formats, `lightEnvironmentTileset`; neither is an authoritative sky-model
  path.
- StarCraft II fixtures and the current lighting parser expose no equivalent `CS_SKY` model contract.
- Classic WoW WDT/ADT data in this repository has no current per-map producer for `CS_SKY`.

Do not publish guessed paths for those games. Add a producer only when its game data establishes an authoritative asset mapping.

## Verification

Automated coverage includes:

- `wc3_api.set_sky_model_publishes_registered_model` for JASS -> model/configstring lifecycle;
- `client_environment.no_world_clears_sky_model` for no-world ownership;
- `client_camera.rendered_eye_tracks_orbit_distance` for camera-relative origin data;
- `renderer_model.mdx_sequence_zero_selector_advances_with_render_time` for the WC3 sequence-0 clock.

Suggested commands (not run automatically by documentation changes):

```sh
make test-client-camera
make test-renderer-model
make test-wc3-engine WC3_PATTERN='wc3_api.set_sky_model*'
```

Runtime verification should use a map that calls `SetSkyModel` with an animated stock sky and include a low-angle cinematic camera.
Check that translating/orbiting the camera produces no sky parallax, sequence-driven sky motion continues, and clearing the sky with
`SetSkyModel("")` immediately returns to the ordinary world background.

## Known Gaps

- Exact classic-retail depth state for sky-model layers has not been traced.
- Exact classic-retail interaction between `SetTerrainFogEx` and sky material layers has not been traced.
- Sequence phase when replacing one sky with another is not proven against retail; OpenRealm currently uses the scene render clock.
- WC3 environmental fog styles 1/2 remain a separate issue; see
  [environmental terrain fog](../games/warcraft-3/environmental-fog.md).

Keep these separate from DNC lighting: `SetDayNightModels` supplies time-of-day lighting samples, while `SetSkyModel` supplies visual
background geometry. See [environment lighting](environment-lighting.md).
