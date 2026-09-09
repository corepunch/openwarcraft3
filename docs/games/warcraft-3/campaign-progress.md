# Warcraft III Campaign Progress

## Retail persistence split

Warcraft III keeps campaign-menu progression separate from the JASS `gamecache`
used to carry Heroes, variables, and items between maps.

Classic/profile-era installs store both files in the active Warcraft III profile
save directory. Later classic installs commonly use a path such as:

```text
%USERPROFILE%\Saved Games\Warcraft III\Profile1\Campaigns.w3p
%USERPROFILE%\Saved Games\Warcraft III\Profile1\Campaigns.w3v
```

Reforged uses the Battle.net campaign directory under Documents and separates
Classic/Reforged progression, for example:

```text
Documents\Warcraft III\BattleNet\<account>\Campaigns\Classic\Classic.w3p
Documents\Warcraft III\BattleNet\<account>\Campaigns\Reforged\Reforged.w3p
```

The `.w3p` file is the frontend campaign-progress record: unlocked campaigns and
mission progression. The `.w3v` file is the game-cache container used by map
scripts for state carried between campaign maps. See
[campaign-game-cache.md](campaign-game-cache.md) for OpenRealm's `.w3v`-equivalent
runtime and Hero persistence.

OpenRealm does not claim binary compatibility with retail `.w3p` or `.w3v`
files. Both are represented by private, versioned sidecars.

## OpenRealm ownership and file

Campaign/menu availability is persisted in:

```text
campaign-progress.orcp
```

The filename is resolved through the engine-owned `FS_UserPath()` policy. The
game receives that boundary as `game_import.UserPath`; the menu receives the
same resolver as `menuImport_t.UserPath`. This follows the same ownership rule
as campaign game-cache persistence: DLLs request a writable per-game path, but
do not know the platform-specific user-data directory themselves.

The common serializer lives in:

- `games/warcraft-3/common/campaign_progress.c`
- `games/warcraft-3/common/campaign_progress.h`

Game-side native integration lives in:

- `games/warcraft-3/game/g_campaign_progress.c`
- `games/warcraft-3/game/api/api_misc.h`

The campaign selector consumes the same file in:

- `games/warcraft-3/menu/screens/single_player.c`

The private file uses the FOURCC `ORCP` magic and version `1`. Its integer header is
explicit little-endian and the payload stores bounded known/value bits for:

- tutorial-cleared state;
- campaign availability;
- mission availability.

The separate known bits matter because authored `false` writes must override
`DefaultOpen`; an explicit lock is not the same state as a fresh profile with no
saved value.

Writes go to a temporary file and are renamed into place after a complete write.
The previous file is preserved as a temporary backup during replacement so a
failed install does not silently leave a partially written progress record.

## Campaign indices

The native indices are the values Blizzard.j ultimately passes to
`SetCampaignAvailable` / `SetMissionAvailable`, not always the public BJ campaign
constants.

Reign of Chaos uses:

| Native campaign | Campaign key |
| ---: | --- |
| 0 | `Tutorial` |
| 1 | `Human` |
| 2 | `Undead` |
| 3 | `Orc` |
| 4 | `NightElf` |

The Frozen Throne wrappers convert the expansion BJ constants to offsets before
calling the natives:

| Native campaign | Campaign key |
| ---: | --- |
| 0 | `NightElf` |
| 1 | `Human` |
| 2 | `Undead` |
| 3 | `Orc` |

RoC and TFT progress are therefore stored as separate editions inside the same
OpenRealm sidecar.

## JASS update boundary

These natives immediately update and commit the campaign-progress sidecar:

```text
SetTutorialCleared(cleared)
SetCampaignAvailable(campaignNumber, available)
SetMissionAvailable(campaignNumber, missionNumber, available)
```

This is intentionally different from JASS `gamecache`, where `Store*()` remains
handle-local until an explicit `SaveGameCache()` call. Campaign availability is
frontend/profile state authored directly by these availability natives.

The Prologue02 ending is the concrete compatibility case that established this
contract. Its `Trig_NextLevelPrep_Actions` calls Blizzard's wrappers to make the
Human campaign and Human mission 0 available. Those wrappers reach:

```text
SetCampaignAvailable(1, true)
SetMissionAvailable(1, 0, true)
```

before the long ending cinematic finishes. OpenRealm writes both unlocks at that
point. `ForceCampaignSelectScreen()` only selects the destination for the later
`EndGame`, so the cinematic can continue while the newly unlocked state is
already durable.

## Campaign selector visibility

The game-owned archived CVar is initialized in `games/warcraft-3/share/config.cfg`:

```text
wc3_campaign_visibility all
wc3_campaign_visibility unlocked
```

Default: `all`.

`all` keeps the developer-friendly behavior of showing every parsed campaign and
map-backed mission regardless of profile progress.

`unlocked` uses authored/persisted availability:

- an explicit `SetCampaignAvailable` value wins when one has been written;
- otherwise a campaign with `DefaultOpen=1` is shown;
- an explicit `SetMissionAvailable` value wins when one has been written;
- otherwise mission 0 is shown when its campaign is available.

`DefaultOpen` is parsed from the active `CampaignStrings` data rather than being
hard-coded by race. Campaign and mission rows are rebuilt/reloaded when entering
the campaign selector, so progress written by the preceding map is visible after
the session returns to the frontend.

The older `wc3_campaign_mission_visibility=played` and
`wc3_campaign_played_<campaign>_<mission>` frontend bridge is removed. Merely
launching a map is not campaign progression; authored JASS availability natives
are authoritative.

## Verification

Regression coverage verifies:

- the JASS availability natives create/update the progress file;
- the persisted RoC Human campaign/mission bits survive a file reload;
- RoC/TFT campaign string keys map to Blizzard's native campaign indices;
- the menu defaults to showing all campaign content;
- `unlocked` mode shows `DefaultOpen` content plus campaigns/missions present in
  the progress sidecar;
- selecting a mission no longer creates a synthetic `played` CVar.

## Retail evidence used for this implementation

The retail file/path conclusions above are version-specific rather than a claim
that every Warcraft III release used one directory:

- Blizzard's Warcraft III: Reforged forum documents Classic progress under
  `Documents\Warcraft III\BattleNet\<account>\Campaigns\Classic` and identifies
  `Classic.w3p` as the file containing unlocked campaigns and completed missions:
  <https://us.forums.blizzard.com/en/warcraft3/t/psa-backing-up-campaign-progress/22160>
- A Reforged support thread identifies the corresponding Reforged directory and
  the `Reforged.w3p` / `Campaigns.w3v` files needed when moving campaign state:
  <https://us.forums.blizzard.com/en/warcraft3/t/solvedwhere-are-the-campaigns-saved-games-stored/17387>
- The documented Warcraft III game-cache format identifies `Campaigns.w3v` as
  per-profile variables/units carried between campaign maps, not the selector's
  `.w3p` availability record:
  <https://alanfox2000software.github.io/war3-diy/doc/w3x/index.html>
- Later pre-Reforged classic installs have been observed with both
  `Campaigns.w3p` and `Campaigns.w3v` under
  `%USERPROFILE%\Saved Games\Warcraft III\ProfileN`:
  <https://forum.3ice.hu/viewtopic.php?p=6018>
