# Thin Pipeline / Root Struct Research

Tracking: [#89](https://github.com/corepunch/open-realm/issues/89) (implement),
[#93](https://github.com/corepunch/open-realm/issues/93) (research).

## Sources

| Source | Role |
|--------|------|
| [SebAaltonen, NoGraphicsAPI](https://github.com/sebbbi/NoGraphicsAPI) (MIT, Vulkan 1.4) | Living reference. Root structs via `vkCmdPushDataEXT`, descriptor heaps, no descriptor sets/layouts. |
| [SIGGRAPH 2026 talk](https://www.sebastianaaltonen.com/blog/no-graphics-api) (~1h44) | Best single narrative + hardware justification for thin pipelines. |
| [X: NoGraphicsLibrary](https://x.com/SebAaltonen/status/2096314523980349853) (2026-09-05) | Size of real implementation: 1 header + 1 source core, 6+4 utilities. |
| Doom 3 `R_*` / `RB_*` split | GL state cache XOR pattern, frontend/backend ownership model. |
| dhewm3 `GL_State(uint64)` | Bitmask packing for blend/depth/color-mask, XOR delta, sort-by-state. |

## What we steal (mapped to GL)

| Aaltonen concept | Open-Realm GL implementation |
|------------------|------------------------------|
| Root struct + one push | `RB_UploadRoot`: per-program UBO, `glBufferSubData` one call, binding point 0. |
| Thin pipelines | `pipeline_t`: progid + `pipelineRasterState_t` (blend, depth, cull, color mask). |
| Blend/depth as small objects | `RB_State(DWORD)` packed bitmask, XOR diff against `backEnd.glStateBits`. |
| Texture/sampler heap | `textureHeap_t`: 32-bit indices, `RB_HeapAllocTexture`/`RB_HeapBindTexture`. |
| Explicit vs stateful | Frontend fills root struct, backend diffs state. No raw GL from draw functions. |

## What we do NOT steal

- Full fixed-function emulation (lighting, fog, texture-env modes).
- 256 uber-shader permutations from FF state bits.
- `VK_EXT_device_generated_commands` or buffer device address for GL.
- Large pipeline-create paths or retained descriptor sets.

## Packed state bit layout

```
bits 0-3:   blend src  (GL_SRC_ALPHA, GL_ONE, etc.)
bits 4-7:   blend dst  (GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, etc.)
bit  8:     depth write (1 = on)
bits 9-11:  depth func  (5 = LEQUAL, 7 = ALWAYS)
bits 12-15: color mask  (4 bits: R G B A)
```

XOR delta between old and new state determines which GL calls to emit.
`RB_State()` is the single entry point; helpers (`RB_Cull`, `RB_PolygonOffset`,
etc.) handle non-packed state.

## Platform constraints

| Platform | Transport | Notes |
|----------|-----------|-------|
| macOS GL 4.1 | UBO (std140) | No bindless. Texture heap binds to units. |
| Desktop GL 3.1 | UBO (std140) | `GL_ARB_bindless_texture` optional for texture heap. |
| GLES3 Mali-G31 | UBO (std140) | No bindless. Explicit per-unit bind with logged strategy. |
| Future Vulkan/Metal | Push constants / `setBytes` | Same root struct, different transport. |

## Implementation phases

| Phase | Status | Description |
|-------|--------|-------------|
| 1 | ✅ | Backend state cache (`r_backend.h`/`.c`): RB_State, RB_Cull, RB_PolygonOffset, RB_Scissor, RB_Viewport, RB_BindVAO, RB_BindFBO. |
| 2 | ✅ | Migrate all engine + game draw functions from `R_Call(gl...)` to `RB_*`. |
| 3 | ✅ | Thin pipeline objects: `pipelineDesc_t` → `pipeline_t` → `RB_BindPipeline`. |
| 4 | ✅ | Root struct UBO: `RB_UploadRoot` per-program UBO, one `glBufferSubData` call. |
| 5 | ✅ | Texture/sampler heap: 32-bit indices, `RB_HeapAllocTexture`/`RB_HeapBindTexture`. |

## Remaining raw GL state calls (intentionally kept)

- `GL_SAMPLE_ALPHA_TO_COVERAGE` in `R_SetAlphaKeyState` — MSAA-specific, not general state cache.
- `#ifdef USE_SHADOWMAPS` shadow-map depth setup.
- `#ifdef SC2` color-mask overrides in `R_BeginFrame`/`R_EndFrame`.
- `R_InitRenderer` initial state.
