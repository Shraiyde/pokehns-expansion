# Co-op mailbox (HnS)

Shared EWRAM struct `gHnsCoopMailbox` in `src/overworld.c`. Android reads/writes
the same layout; **bump `version` and `size` together** whenever fields change.

## v12 (follower presence)

| Field | Offset | Size | Writer | Notes |
| --- | --- | --- | --- | --- |
| `localFollower` | 2284 | 4 | ROM | Packed local follower graphics + visible |
| `remoteFollower` | 2288 | 4 | Android | Copy of peer’s `localFollower` |
| **sizeof** | — | **2292** | — | was 2284 in v11 |

`HNS_COOP_MAILBOX_VERSION` = **12**. Earlier fields (through `remoteConfigBlob`)
are unchanged so Android config-blob offsets stay valid.

### Packing (`localFollower` / `remoteFollower`)

- **Bit 31** — `visible` (`HNS_COOP_FOLLOWER_VISIBLE`). Clear / whole word `0` =
  no follower (in ball, surf/bike hide, indoors-hidden, none).
- **Bits 0–15** — `graphicsId` from the same packing as local followers
  (`species + OBJ_EVENT_MON` [+ `OBJ_EVENT_MON_SHINY`] [+ `OBJ_EVENT_MON_FEMALE`]).
- Facing is **not** packed; the remote OW faces from `remoteFacing` / partner
  avatar facing. Position is one tile behind the partner avatar.

### Android

Must exchange the new u32s each presence frame (send local → peer’s
`remoteFollower`). Older clients that only know sizeof 2284 will fail the
version/size check and re-init.

### ROM render

Special object `OBJ_EVENT_ID_REMOTE_FOLLOWER` (`0xFC`). Same-map only
(`Coop_IsPeerVisible`). Walk/snap like `Coop_SnapRemoteLinkPlayers`; destroyed
on hide, map change, or disconnect.


## v13 (presence field-return resync)

| Field | Offset | Size | Writer | Notes |
| --- | --- | --- | --- | --- |
| `localPresenceResync` | 2292 | 4 | ROM | Monotonic counter; bumped on return-to-field / warp |
| **sizeof** | — | **2296** | — | was 2292 in v12 |

`HNS_COOP_MAILBOX_VERSION` = **13**. Android watches this counter: on change it
force-publishes local POSITION and asks the peer to republish (`POSITION_RESYNC`).

Return-to-field previously skipped link-player sprite rebuild, leaving the partner
avatar invisible after battles/menus; ROM now invalidates and respawns presence.

## v14 (avatar locomotion / doors)

| Field | Offset | Size | Writer | Notes |
| --- | --- | --- | --- | --- |
| `localAvatarState` | 2296 | 4 | ROM | `action:8 \| avatarState:8 \| doorPhase:8 \| claimSeq:8` |
| `remoteAvatarState` | 2300 | 4 | Android | Host-confirmed peer copy |
| **sizeof** | — | **2304** | — | was 2296 in v13 |

`HNS_COOP_MAILBOX_VERSION` = **14**. POSITION wire payload is 8 words (adds
follower + avatarState). World Host drops duplicate/late claimSeq values.

## Presence movement

See [`docs/coop-presence-movement.md`](coop-presence-movement.md) for outdoor
visibility (`remoteMapId == 0`), step-queue vs snap, and Android POSITION queue.
