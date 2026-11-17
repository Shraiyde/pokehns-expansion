# Co-op presence movement (causes + fixes)

Phone co-op shows the partner via mailbox POSITION → `remoteX/Y/facing/mapId`
and a presence link-player object (`COOP_PRESENCE_LINK_ID`). This note records
the APK 238 + ROM 128 field report and the fixes that followed.

## Symptoms (APK 238 / ROM 128)

1. Partner avatar skipped / teleported.
2. Partner position felt “guessed” / inaccurate.
3. Partner invisible outdoors; visible inside buildings.

## Root causes

### Outdoor invisibility (New Bark)

`Coop_IsPeerVisible` required `remoteMapId != 0`. Packed map ids are
`(mapGroup) | (mapNum << 16)`. **New Bark Town is group 0 / num 0 → packed 0**,
so the peer was treated as “no map” outdoors while indoor maps (nonzero group)
passed. Fix: gate on `remoteMapReady` + map equality only.

### Skipping / teleporting

- Receiver used **walk-or-snap**: Manhattan gap ≥ 2 → hard teleport. With ~70 ms
  RTT and run movement, gaps of 2+ are normal → constant snaps.
- Android POSITION sender used a **2-deep `DiscardOldest`** queue, so intermediate
  tile publishes were silently dropped under load.
- Collision (`LinkPlayerGetCollision`) could refuse a one-tile walk; the next
  frame then snapped.

### “Guessed” position

- Presence spawn used the **local** avatar coords, then snapped to remote —
  wrong place until the first correction.
- No step queue: the receiver invented a single chase step toward the latest
  point instead of replaying the peer’s tile path.

### Outdoor spawn pressure (secondary)

`OBJECT_EVENTS_COUNT == 16`. Dense outdoor views can fill every slot;
`SpawnLinkPlayerObjectEvent` bailed silently. Indoor maps are sparse, so spawn
succeeded more often. Fix: **reserve** `OBJECT_EVENTS_COUNT-1` for the partner
and `COUNT-2` for the remote follower so map NPCs never take those ids.

## Fixes

| Side | Change |
| --- | --- |
| ROM | Remove `remoteMapId != 0` check |
| ROM | Spawn at `remoteX/Y` (fallback to local only if unpublished 0,0) |
| ROM | Cardinal **step queue** (size 24); snap only if gap ≥ 8 tiles / map change |
| ROM | Force a step if collision refuses, so the queue stays authoritative |
| ROM | Reserved object-event slots for partner + remote follower |
| Android | POSITION queue depth 32 + **AbortPolicy** (retry instead of drop) |

Elevation / facing still come from the peer’s published mailbox words. Follower
OW uses the same large-desync snap threshold while tracking the partner avatar.

## Non-causes checked

- Camera / 15×10 view culling does **not** remove link-player avatars (they are
  exempt in `event_object_movement.c`).
- `MAP_OFFSET` (7) is already included in published `currentCoords`; Android
  relays those words unchanged.
- No dead-reckoning / extrapolation remains on the receiver after this change.

## Return-to-field invisibility (battle / menu)

`SpawnObjectEventsOnReturnToField` recreates sprites for map objects but
**skips link-player slots**. After a wild/trainer battle (or start menu), the
presence `gLinkPlayerObjectEvents` entry stayed `active` while its sprite was
destroyed — `Coop_UpdateRemotePresence` thought the partner was still spawned
and never recreated them.

Fix: invalidate presence link players (+ remote follower) in
`InitObjectEventsReturnToField`, validate sprite `inUse` before trusting an
active entry, and bump `localPresenceResync` so Android forces a POSITION
exchange.

## Always-visible partner (battle / menu / warp / cutscene)

Hard requirement: on the same map, both players must see each other (and the
partner follower) with no gaps after battles, bag/party/PC/save, warps, HM
field moves, cutscenes that hide/reload objects, fades, or reconnects.

| Mechanism | Behavior |
| --- | --- |
| Reserved slots | Partner = `OBJECT_EVENTS_COUNT-1`, remote follower = `COUNT-2`. `GetFirstInactive` / `GetAvailableObjectEventId` skip them so outdoor NPC density cannot steal them. |
| Eviction block | `RemoveObjectEvent` no-ops on reserved ids; `SetObjectInvisibility` refuses to hide the remote follower / reserved slots; OOB cull skips reserved ids. Only `Coop_ForceClearObjectEventSlot` zeroes them for a controlled re-spawn. |
| Every-frame watchdog | `CB1_Overworld` → `Coop_UpdateRemotePresence` / `Coop_UpdateRemoteFollower`: if peer is same-map but the avatar is missing, wrong slot, dead sprite, or hidden → ForceClear + spawn into the reserved slot immediately (same tick). |
| Field return | Still invalidates stale link entries after battles/menus; watchdog re-spawns on the next CB1. |

## Local player invisible / cannot move (APK 252 / ROM 132)

`Coop_ForceClearObjectEventSlot` destroyed `gSprites[spriteId]` whenever
`spriteId < MAX_SPRITES && inUse`. Unused object-event slots are zeroed
(`ClearObjectEvent` / `memset`), so **`spriteId == 0`**. `CreateSprite` allocates
from index 0, so the local player avatar is almost always `gSprites[0]`.

Triggered on Join PC Host (including **one phone**): every CB1 tick,
`Coop_DestroyRemoteFollower` ForceCleared the empty reserved follower slot
(`spriteId == 0`) and **destroyed the local player sprite**. Same path fires
when a same-map peer makes the presence watchdog Ensure/ForceClear the partner
slot. The camera still tracked sprite id 0 (now dead/reused), so the player
looked missing and unable to move while partner/NPC sprites (higher ids, or
re-spawned by the watchdog) still drew. Offline/unconnected play is unchanged:
local input still drives the avatar; the PC host remains authoritative for
relaying each phone's POSITION claim to the partner.

Fix: only `DestroySprite` when the object event is **`active`** and owns that
sprite; never ForceClear `gPlayerAvatar.objectEventId`; skip follower
ForceClear when the reserved slot is already empty.
