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
succeeded more often. Fix: steal the farthest non-critical NPC slot when full.

## Fixes

| Side | Change |
| --- | --- |
| ROM | Remove `remoteMapId != 0` check |
| ROM | Spawn at `remoteX/Y` (fallback to local only if unpublished 0,0) |
| ROM | Cardinal **step queue** (size 24); snap only if gap ≥ 8 tiles / map change |
| ROM | Force a step if collision refuses, so the queue stays authoritative |
| ROM | Steal farthest NPC slot when object table is full |
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
