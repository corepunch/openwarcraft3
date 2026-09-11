# Galaxy Objectives, Actors, and Conversation Metadata

Findings from the September 11, 2026 TRaynor01 investigation. This describes the current implementation and the inspected local
client scripts, not complete retail SC2 conformance. See [Galaxy scripting](galaxy-scripting.md) for execution/error boundaries and
[native coverage](galaxy-native-coverage.md) for the remaining work.

## Ownership and Data Flow

| Concern | Current owner | Boundary |
|---|---|---|
| Mission conditions, objective progression, dialogue sequencing | Authored `MapScript.galaxy` and TriggerLibs | Galaxy trigger callbacks |
| Objective identity, text, state, visibility, primary flag | `game/galaxy/galaxy_ui.h` | Integer native IDs; no HUD output yet |
| Actor identity and scope relationships | `game/galaxy/galaxy_actor.h` | Opaque handles; game callback hooks for presentation |
| Layered conversation catalog strings | `common/sc2_map.c` | `SC2_MapConversationField`, installed as `sc2_galaxy_conversation_field` |
| Future objective/help UI | SC2 game HUD code | Existing `svc_layout` and generic client rendering |

Use WC3 `games/warcraft-3/game/api/api_quest.h` as the state-ownership reference: native wrappers mutate server-owned quest records
created/removed by `G_MakeQuest` / `G_RemoveQuest`; scripts choose mission policy. SC2 integer objective IDs are a different native
contract from WC3 quest handles. Do not share IDs or cast one domain's objects into the other.

Presentation should extend the [SC2 HUD pipeline](hud-layout-pipeline.md) or game-authored entity output. The menu module must not
become the owner of in-game objectives, help tips, or actor effects. Completing a native's script-state behavior does not establish
that its presentation is rendered.

## Objective Contract

The mounted map uses legacy `ObjectiveCreate(name, description, state, primary)`. The mounted Core `natives.galaxy` instead declares
`ObjectiveCreate3(name, description, state, visible, primary)`. The host supports both; its legacy adapter sets visibility true.
Do not rename the map's calls or assume its generated script and the mounted native declarations have the same API generation.

| Native/data | Behavior |
|---|---|
| Create / LastCreated | IDs begin at 1; 0 means invalid; last-created ID updates on creation |
| GetState | -1 unknown/invalid/destroyed; 0 hidden; 1 active; 2 completed; 3 failed |
| GetPrimary | Boolean; the old placeholder returned an objective handle |
| GetName / GetDescription | Retained string contents; invalid IDs currently return empty strings |
| SetName | Copies replacement text before VM arguments are released |
| SetState | Updates the referenced record; invalid IDs raise a protected script error |
| Destroy | Releases name/description and invalidates that record; IDs are not recycled |
| Capacity / reset | 256 creations per map; capacity exhaustion is a script error; `galaxy_reset` frees records |

`ObjectiveLastCreated` is creation history, not a search for the last live objective: destruction does not rewind it. Visibility and
primary are retained fields; player-group visibility, setters not in the binding table, objective HUD output, and save/load support
are not implemented by this change.

`libCamp_gf_RegisterMissionObjective` stores the objective integer in a campaign array and reads `ObjectiveGetPrimary` into a boolean
array. This is why a zero-ID or wrong-type placeholder is not a working implementation.

TRaynor01 starts by creating the logistical-HQ and Raynor-survival objectives, both active and primary. The optional holoboard
objective builds a progress name with `IntToText`. Galaxy `text` is mapped to VM `string`; `IntToText` must return decimal text.
Its former integer-zero placeholder failed the concatenation regression. The map initializes the holoboard target count to 6.

## Actor ABI and Scope Lifecycle

Authoritative declaration in `Mods/Core.SC2Mod/Base.SC2Data`, `TriggerLibs/natives.galaxy`:

```c
native actor ActorCreate(actorscope as, string actorName, string content1Name, string content2Name, string content3Name);
```

`libNtve_gf_AttachActorToUnit` calls it in this order:

1. Get the unit scope with `ActorScopeFromUnit`.
2. Create `SiteHosted`, passing the attachment name as content1.
3. Retrieve it with `ActorFrom("::LastCreated")` and send `RefSet ::scope.hostsite ::Self`.
4. Create the requested actor (for the opening dialogue, `TalkIcon`) under the unit scope.
5. Retrieve that actor and send `RefSet ::Host ::scope.hostsite`.

The old host read argument one as the actor-name string and argument two as the scope. A live unit scope therefore hit
`jass_checkstring`'s assertion. The crash was independent of the preceding missing `ObjectiveCreate` message.

Current state behavior:

- `sc2_unit_scope` caches a root scope for each valid Galaxy unit handle.
- Each `ActorCreate` allocates a child scope and inherits its parent scope's unit identity.
- `ActorFrom("::LastCreated")` returns the last actor if still live, otherwise null.
- `ActorScopeFrom("::LastCreated")` returns the last created scope; `ActorScopeFromActor` retrieves an actor's scope.
- After the opening transmission waits, the script calls `ActorScopeKill(libNtve_gf_ActorScopeLastCreated())`.
- Killing a scope marks its descendants dead and clears affected actor records. Killing the icon's scope leaves its parent unit
  scope and sibling site scope intact. Repeating the same kill does not emit a second actor-destroy callback.
- Actor IDs and scopes are monotonic until reset: 1,024 actor creations; scope capacity is `MAX_GALAXY_UNITS + MAX_GALAXY_ACTORS`.

The native ABI and scope bookkeeping are implemented; the full actor engine is not. Content1/2/3 are not yet retained/applied,
`RefSet` attachment semantics are not implemented locally, and `SC2_InitGalaxyHost` does not install actor create/send/destroy
presentation callbacks. Thus resolving `TalkIcon` identity does not prove a visible attached icon. Named actor/scope references
other than `::LastCreated` raise errors; `ActorFromScope` and actor-region operations remain placeholders. Reusing destroyed slots,
recreating a killed unit root scope, and complete actor-message semantics still need lifecycle work.

See [Actors and Models](file-formats/actors-and-models.md) for catalog terminology. A Galaxy actor name is not automatically an M3
file path; follow the actor/model catalog and attachment data when wiring presentation.

## Conversation Catalog Schema

The inspected campaign metadata comes from
`Campaigns/LibertyStory.SC2Campaign/Base.SC2Data` → `GameData/ConversationStateData.xml`.
The normal catalog loader applies dependency and map-local layers; it follows `GameData.xml` manifests and includes the conversation
file in its existing known-file path for layers without a manifest.

```text
CConversationState id + Indices Id
    -> group|index lookup key
    -> SC2CONVERSATION plus repeated SC2CONVTEXT records
    -> SC2_MapConversationField
    -> sc2_galaxy_conversation_field
    -> ConversationDataStateName / Text / ImagePath
```

| XML production | Internal lookup | Parsing contract |
|---|---|---|
| `CConversationState id="StoryTips"` + `Indices Id="Marine"` | `StoryTips|Marine` | Case-sensitive query key |
| `Indices Name="..."` or child `<Name value="..."/>` | `Name` | Schema-table scalar field |
| `Indices ImagePath="..."` or child `<ImagePath value="..."/>` | `ImagePath` | Schema-table scalar field |
| `<InfoText Id="Description" Text="..."/>` | `Text:Description` | Repeated keyed production |
| `InfoText` with child `Id` / `Text` value tags | `Text:<ID>` | Same schema as compact attributes |
| `<InfoText Id="Loading Screen Restart 2"/>` | Present empty text value | Distinct from an unresolved field |

InfoText IDs may contain spaces. `Name` and `Text:Name` are separate concepts. The first implementation attempt only kept
`Description`; the bounded run then stopped at `Maps|TRaynor01`, `Loading Screen Restart` during initialization. Preserve all
InfoText IDs, not a hardcoded description slot.

Current layering preserves earlier scalar name/image values unless a later nonempty value replaces them. InfoText entries merge by
ID and retain explicitly empty text. Consequently an explicit empty scalar override does not currently clear an earlier name/image;
this is a limitation, not complete XML override semantics. Records use bounded buffers (combined key 128 bytes, text ID 64 bytes,
name/image/text 256 bytes). Numeric index patches and general conversation-state inheritance are not implemented.

The local archive contains `Indices index="0"` with `InfoText index="8" removed="1"` in groups such as `Missions`, `MapLast`, and
`StoryNews`. These are ordinal/removal patches, not malformed named tutorial records. The current parser logs missing index IDs
for them; future support must resolve inherited ordinal identity and removal semantics, not invent replacement IDs.

A missing row/field logs its key and field and returns NULL; the Galaxy wrapper raises a protected script error. A present empty
field returns an empty string. Catalog strings live until catalog replacement/map shutdown, with cleanup in `sc2_free_catalog`.
Name and InfoText values are usually localization keys. `StringExternal` is still a placeholder passthrough, and help-panel natives
remain placeholders, so catalog lookup does not establish localized or visible tutorial UI. Conversation state get/set/persistence
is separate from this read-only metadata lookup.

## Regression Evidence

| Test | What it verifies |
|---|---|
| `galaxy.vm_objective_lifecycle` | Distinct IDs, primary/state/text, progress concatenation, destruction, map reset |
| `galaxy.vm_actor_scope_first` | Live scope in arg 1, last-created identity, unequal opaque handles, idempotent icon cleanup |
| `sc2_map.sc2_map_loads_xml_objects_and_terrain` | Core/map fixture layering, both XML forms, arbitrary InfoText ID, unresolved inverse |

Fixture metadata lives under `games/starcraft-2/tests/resources-src/Mods/Core.SC2Mod/Base.SC2Data/` and
`Maps/Test/Tiny.SC2Map/Base.SC2Data/`; both manifests include `GameData/ConversationStateData.xml`. Tests use the generated fixture
archive and do not depend on locally installed SC2 assets. The [bounded runtime workflow](galaxy-scripting.md#diagnostic-workflow)
verifies script progression separately from these state tests.
