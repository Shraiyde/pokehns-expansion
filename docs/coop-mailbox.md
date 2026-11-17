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

## Presence movement

See [`docs/coop-presence-movement.md`](coop-presence-movement.md) for outdoor
visibility (`remoteMapId == 0`), step-queue vs snap, and Android POSITION queue.
