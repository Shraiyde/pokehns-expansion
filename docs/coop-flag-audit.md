# Co-op flag & var classification audit

Heart & Soul two-player co-op aims for a **shared world** with **personal rewards**.
This document classifies `flags_hns.h` blocks and story `VAR_*` values, and records
what `src/overworld.c` (`IsCoopSharedStoryFlag`, `sCoopSharedStoryVars`, initial sync)
actually propagates.

Runtime filter: only flags/vars accepted by those helpers are queued, applied from
the peer, or included in the bounded initial sync scan.

**Clearflag sync:** `FlagSet` and `FlagClear` both queue shared changes when the
local bit actually toggles. Packets carry `flagId | ((value?1:0) << 16)`. Incoming
apply calls `FlagSet` (+ bag grant if newly set / `!wasSet`, so reconnect grants
do not duplicate items) or `FlagClear` (no grant), under `sApplyingRemoteStoryFlag`.
Initial sync stays **SET-only** for progress (badges/trainers/quest/received/
registration/sys unlocks). **Bidirectional hide** ranges also queue **CLEAR** when
locally unset: NPC_HIDE (except Celebi), shared story HIDE_POKEMON (incl.
electrodes), Ice Path boulders, and shared extended story hides — so reconnect
mirrors object show state without wiping badges.

**Peer reconnect (`peerLinkReady`):** a rising edge `0→1` while transport is still
connected re-runs `Coop_BeginInitialSharedFlagSync()` even if an initial sync had
already completed, requeuing shared SET/CLEAR + story vars after presence expiry /
soft return. A falling edge clears `localStorySyncReady` / `remoteStorySyncReady`
(and local `sCoopStorySyncReady`) so WORLD_READY must re-handshake.

---

## 1. Classification rules

| Class | Meaning | Examples |
| --- | --- | --- |
| **SHARED** | Both players need it for a consistent overworld | Story progress, gyms/badges, trainers defeated, key items/HMs, Match Call unlocks + rematch registration, NPC hide/show from story, path obstacles (Sudowoodo, Snorlax, Ice Path boulders, …), town/gym/quest state vars, Rocket passwords, Red defeated, Alola totem gates / boat mapsetup |
| **PER-PLAYER** | Personal rewards / encounters | Legendaries & statics (hide + defeated + caught), gift Pokémon ownership, normal item balls / hidden items, TMs / berries / held-item gifts, phone gift pending (`FLAG_*_HAS_ITEM_HNS`, `VAR_*_ITEM_HNS`), daily rewards, starter balls, personal inventory, roaming legendaries |
| **MIXED** | Block contains both; named exceptions listed below | Hide-Pokémon, RECEIVED, Battle Frontier, Safari, Feature, EXTENDED, SYS_FLAGS, MATCH_CALL |

**Do not share:** personal inventory contents, party/PC, money, and anything that only
marks “this console already collected a reward.”

**Do share:** anything that moves NPCs, unlocks doors/paths, or changes town/quest
appearance so both players experience the same story map.

---

## 2. Flag blocks (`include/constants/flags_hns.h`)

### HIDE_POKEMON — `HNS_HIDE_POKEMON_START` (0x22) · count 40 — **MIXED**

| Subset | Class | Notes |
| --- | --- | --- |
| Starter balls (`FLAG_HIDE_CHIKORITA/CYNDAQUIL/TOTODILEBALL`) | PER-PLAYER | Each player picks their own starter |
| Farfetch’d / Azalea Slowpoke / Ariados / Ilex Farfetch’d trail | **SHARED** | Cut quest + Azalea gym obstacles |
| `FLAG_HIDE_LAKE_OF_RAGE_GYARADOS` | **SHARED** | Lake of Rage story |
| `FLAG_HIDE_SUDOWOODO`, `FLAG_HIDE_VERMILION_SNORLAX` | **SHARED** | Path gates |
| `FLAG_HIDE_CERULEAN_GYM_POKEMON` | **SHARED** | Kanto Rocket / Misty gym story |
| `FLAG_HIDE_WHIRL_ISLANDS_TENTACRUEL` | **SHARED** | Kimono / Whirl Islands story object |
| `FLAG_HIDE_BATTLE_FRONTIER_SUDOWOODO` | **SHARED** | Pairs with shared `FLAG_DEFEATED_SUDOWOODO` |
| Rocket Hideout electrodes (1–3) | **SHARED** | With `VAR_ELECTRODES_FAINTED`; bidirectional clear sync |
| `FLAG_HIDE_UNION_CAVE_LAPRAS` | PER-PLAYER | Static gift encounter |
| Beasts, Suicune routes, Ho-Oh, Lugia, Celebi, birds, Mewtwo, weather trio, Lati@s, Jirachi | PER-PLAYER | Legendary / static encounter objects |

### NPC_HIDE — `HNS_NPC_HIDE_START` (0x4A) · count 160 — **SHARED** (with exceptions)

| Exception | Class | Reason |
| --- | --- | --- |
| `FLAG_HIDE_ILEX_FOREST_SECOND_CELEBI` | PER-PLAYER | Legendary Celebi NPC |
| *(no other Celebi/legendary NPC hides in this block)* | — | Beasts/Suicune objects live in HIDE_POKEMON |

Everything else in this range (Silver, Rockets, Kurt/well, Radio Tower, Lance,
gym-leader dojo placement, copycat doll, SS Aqua, Indigo guards, …) is **SHARED**
so both players see the same story NPCs. Bidirectional clear sync on reconnect.

Battle Frontier multi-partner / opponent / reporter hides fall in this range and
are shared by the range rule (cosmetic BF lobby consistency).

Museum fossils (`FLAG_HIDE_PEWTER_CLAW/ROOT_FOSSIL`) are in this range → **SHARED**
via the range (intentional leftover / optional carve-out; see §6).

### ITEM_BALL — `HNS_ITEM_BALL_START` (0xEA) · count 234 — **PER-PLAYER** (exception)

| Exception | Class |
| --- | --- |
| `FLAG_ITEM_ICEPATH1_HM_WATERFALL` | **SHARED** (+ bag grant on apply) |

All other overworld balls / `FLAG_HIDDEN_ITEM_MACHINE_PART` stay local.

### QUEST — `HNS_QUEST_START` / `FLAG_ADVENTURE_STARTED`..`FLAG_UNLOCKED_GOLDENROD_UNDERGROUND_DOOR` — **SHARED** (exception)

| Exception | Class |
| --- | --- |
| `FLAG_ROUTE25_GOT_NUGGET` | PER-PLAYER (personal nugget gift) |

Includes adventure start, Ampharos healed, Rocket hideout switches, Ruins puzzles,
Goldenrod underground door, etc.

### RECEIVED — `HNS_RECEIVED_START` · count 53 — **MIXED**

**SHARED (progress unlocks):** bike, Match Call, HMs Cut/Flash/Rock Smash/Strength/Surf,
`FLAG_RECEIVED_HM_WHIRLPOOL` (Lance, Rocket Hideout B2F), PokéNav, running shoes,
Squirtbottle, Headbutt, Super Rod, Rocket passwords
(`FLAG_GOT_PASSWORD_FROM_ETO/GRUNTF`).

**PER-PLAYER:** first potion, Odd Egg / Togepi egg, Hoenn starter / Dratini / Eevee /
Tyrogue gifts, TM gifts, berry / held-item / nugget / shield gifts, fossil revive
receipt, Frustation/Hidden Power/Return gift moves.

### GYM — `HNS_GYM_DEFEATED_START` · 16 flags — **SHARED**

All Johto + Kanto gym defeated flags.

### LEGEND_DEFEATED — `HNS_LEGEND_DEFEATED_START` · 5 flags — **MIXED**

| Flag | Class |
| --- | --- |
| `FLAG_DEFEATED_HO_OH` / `LUGIA` / `MEW` / `DEOXYS` | PER-PLAYER |
| `FLAG_DEFEATED_RED` | **SHARED** (story boss, not a static) |

### LEGEND_CAUGHT — `HNS_LEGEND_CAUGHT_START` · 6 flags — **PER-PLAYER**

Ho-Oh, Lugia, Entei, Raikou, Celebi, Mew — never shared.

### NPC_TRADE — `HNS_NPC_TRADE_START` · 4 flags — **PER-PLAYER**

Trade completion is personal (each player can complete the trade).

### MATCH_CALL — `HNS_MATCH_CALL_START` · 4 flags — **MIXED**

| Flag | Class |
| --- | --- |
| `FLAG_ADDED_MATCH_CALL_TO_POKENAV` | **SHARED** |
| `FLAG_ENABLE_BAOBA_MATCH_CALL` | **SHARED** (Baoba / Safari quest gate) |
| `FLAG_REGISTER_RIVAL_POKENAV` | **SHARED** |
| `FLAG_ENABLE_PROF_BIRCH_MATCH_CALL` | PER-PLAYER / unused Emerald leftover |

### Trainer rematch registration — `TRAINER_REGISTERED_FLAGS_START` (0x310)..`FLAG_REGISTERED_REENA_HNS` — **SHARED**

Both phones share the same Match Call rematch contacts so the phone book matches.

| Related | Class | Reason |
| --- | --- | --- |
| `FLAG_*_HAS_ITEM_HNS` | **PER-PLAYER** | Pending phone gift for this console only |
| `VAR_*_ITEM_HNS` | **PER-PLAYER** | Which gift item that contact is offering locally |

### MOVE_TUTOR — `HNS_MOVE_TUTOR_START` · 4 flags — **PER-PLAYER**

“Met tutor” / taught Headbutt met flags are personal.

### BATTLE_FRONTIER — `HNS_BATTLE_FRONTIER_START` · 10 flags — **MIXED**

| Flag | Class |
| --- | --- |
| `FLAG_DEFEATED_SUDOWOODO` | **SHARED** (path / BF exterior story) |
| Trade done, challenge cancel, multi partner chosen, gold/silver symbols, met NPCs, Scott BP | PER-PLAYER |

BF lobby NPC hides are **SHARED** via the NPC_HIDE range (cosmetic consistency).

### SAFARI — `HNS_SAFARI_ZONE_START` · 4 flags — **MIXED**

| Flag | Class |
| --- | --- |
| `FLAG_SAFARI_ZONE_EAST/WEST_EXPANSION` | **SHARED** (world layout) |
| `FLAG_GOOD_LUCK_SAFARI_ZONE`, `FLAG_IN_KANTO_SAFARI_ZONE` | PER-PLAYER (session) |

### FEATURE — `HNS_FEATURE_START` · 10 flags — **PER-PLAYER**

Day/night encounter toggles, radio unlocks (`FLAG_ENABLE_RADIO`,
`FLAG_KANTO_RADIO_GOT`), Exp Share, finite TMs, mints, no-shiny — console options /
personal unlocks. Kanto radio intentionally kept local in prior co-op work.

### MISC — `HNS_MISC_START` · 3 flags — **PER-PLAYER**

Mom visited, pending daycare egg, wall clock set.

### ITEMS_2 — `HNS_ITEMS_2_START` · count 120 — **PER-PLAYER**

Extra TM balls and hidden items.

### Unused / puzzle hides (0x2F3–0x2FA, 0x304)

| Flag | Class |
| --- | --- |
| `FLAG_HIDE_ICEPATH_B1F_BOULDER1`..`B2F_BOULDER4` | **SHARED** (bidirectional) |
| `FLAG_RECEIVED_HM_WHIRLPOOL` (was `FLAG_UNUSED_36`) | **SHARED** (+ bag grant) |
| `FLAG_LATIOS_OR_LATIAS_ROAMING` | PER-PLAYER |

### EXTENDED — `HNS_EXTENDED_CONTENT_START` (0x36A) — **MIXED**

**SHARED (implemented):** Slowpoke well tailed/tailless hides, Route 13 boat,
`FLAG_HIDE_FLYOVER` / `FLAG_HIDE_PHANT_OUTSIDE` / `FLAG_HIDE_PHANT_INSIDE`
(Alola travel mapsetup), `FLAG_VISITED_NEW_SINJOH` / `FLAG_VISITED_ALOLA`,
Sinjoh/New Sinjoh Steven hides, Vermilion Samson / Route 28 scientist /
Samson Oak intro hides, `FLAG_ROUTE_49_KIMONOGIRL`,
`FLAG_TRIGGERED_*_TOTEM`, `FLAG_DEFEATED_*_TOTEM`.

**PER-PLAYER:** noble Pokémon hides, Regi / Arceus encounter hides, Tapu
`FLAG_BATTLED_TAPU_*` + Tapu/Alolan form static hides, totem mon hides
(`FLAG_HIDE_*_TOTEM`), totem day toggles, `FLAG_FLYOVER_TAPU_*`,
`FLAG_CAUGHT_ALL_*`, `FLAG_DEFEATED_SAMSON_OAK`, item/hidden-item flags,
NPC trade completions, move-tutor paid, decorations, Azure Flute hide, gifts.

**`FLAG_BATTLED_GOLEM/SANDSLASH/NINETALES/MUK/DUGTRIO/MAROWAK/RAICHU/EXEGGUTOR`:**
PER-PLAYER. Map scripts only `setflag` them on static encounter start; nothing
reads them to gate shared NPC/path state (hides use `FLAG_HIDE_*_ALOLA`).

### TRAINER_FLAGS — `TRAINER_FLAGS_START` (0x500)..`TRAINER_FLAGS_END` (0x85F) — **SHARED**

Defeated-trainer flags for world consistency (re-battles / blocking trainers).
Rematch “open” scheduling is engine/personal; no extra shared rematch-open flag
beyond defeated trainers + Match Call registration.

### SYS_FLAGS — `SYS_FLAGS` (0x860+) — **MIXED**

| Subset | Class |
| --- | --- |
| `FLAG_BADGE01_GET`..`FLAG_BADGE16_GET` | **SHARED** |
| `FLAG_SYS_GAME_CLEAR`, `FLAG_SYS_NATIONAL_DEX`, `FLAG_SYS_POKENAV_GET` | **SHARED** |
| Safari/bug-contest mode, Strength/Flash in-map, cycling road, BF symbols, ribbon, mystery gift, B-Dash, Pokédex get, Pokémon get, clock, Lake of Rage tide | PER-PLAYER / session / time-of-day |

### DAILY_FLAGS — `DAILY_FLAGS_START` (0x910) — **PER-PLAYER**

Haircuts, Bug Contest, Buena’s Password, flower shop berry, etc.

### Berry trees / Bug Contest / Pokéathlon / Frontier records

| System | Class | Notes |
| --- | --- | --- |
| Berry trees | PER-PLAYER | Daily plant/pick counters + tree state are personal |
| Bug Contest | PER-PLAYER / session | `VAR_BUG_CONTEST_STATE`, `FLAG_SYS_BUG_CONTEST_MODE`, daily completion |
| Safari in-zone session | PER-PLAYER | Expansions already **SHARED** |
| Battle Frontier / Tower records | PER-PLAYER | Win streaks / BP / symbols stay personal |
| Battle Tower co-op Link Multi invite | **SHARED** (transient) | `FLAG_COOP_TOWER_LINK_MULTI_INVITE`, `FLAG_COOP_TOWER_LINK_MULTI_LVL_OPEN`, `FLAG_COOP_TOWER_LINK_MULTI_PARTNER_READY` — set+clear bidirectional; cleared on cancel / peer-drop / lobby return. Not story progress. |
| Pokéathlon | PER-PLAYER | Music/assets only in tree; no shared story flags |
| Ship / Magnet Train | **SHARED** | `VAR_SSAQUA_STATE`, `VAR_TRAIN` already in `sCoopSharedStoryVars` |
| Roaming legendaries | PER-PLAYER | e.g. `FLAG_LATIOS_OR_LATIAS_ROAMING` |

---

## 3. Story `VAR_*` list

Cross-check: `sCoopSharedStoryVars` in `src/overworld.c` vs HnS vars in
`include/constants/vars_hns.h`.

### SHARED (in `sCoopSharedStoryVars`)

Town / gym / quest arcs: `VAR_*_STATE` for New Bark, Cherrygrove, Violet, Azalea,
Goldenrod, Ecruteak, Olivine, Cianwood, Mahogany, Blackthorn, League, Mt Silver,
Pallet, Viridian, Pewter, Cerulean, Vermilion, Lavender, Celadon, Fuchsia, Saffron,
Cinnabar; plus `VAR_BLACKTHORN_GYM_STATE`, `VAR_NUM_BADGES`, `VAR_ILEX_FOREST_FARFETCHD`,
`VAR_SPROUT_TOWER`, `VAR_RUINSOFALPH_STATE`, `VAR_ICE_PATH_STATE`, `VAR_SSAQUA_STATE`,
`VAR_TRAIN`, `VAR_ELEVATOR`, `VAR_KENYA`, `VAR_ROUTE27_STATE`, `VAR_ROUTE39_BARN`,
`VAR_LAKE_OF_RAGE_FISHERMAN`, `VAR_DRAGONS_DEN_QUIZ`, `VAR_ECRUTEAK_CITY_THEATER`,
`VAR_VIOLET_CITY_KIMONO_GIRL`, `VAR_CLEFAIRY_DOLL_STATE`, `VAR_FAN_CLUB_*`,
`VAR_BILLS_GRANDPA`, `VAR_BAOBA_QUEST_STATE`, `VAR_KANTO_ROCKET_STORY_STATE`,
`VAR_KANTO_SAFARI_ZONE_PROGRESS`, `VAR_SAFARI_ZONE_GATE_STATE`, `VAR_GETFLY`,
`VAR_ROCKET_PASSWORD`, `VAR_ROCKET_HIDEOUT_STATUE_1`..`5`,
`VAR_TOHJO_FALLS_GIOVANNI_STATE`, `VAR_ROUTE22_GIOVANNI_STATE`,
`VAR_TRIGGER_ELM_ROCKET_CALL`, `VAR_ROUTE27_BAOBA_CALL`, `VAR_OAK_MT_SILVER_CALL`,
`VAR_ROUTE28_SCIENTIST`, `VAR_VERMILION_CITY_SAMSON/STEVEN`,
`VAR_VIRIDIAN_FOREST_PICHU`, `VAR_NEWBARKTOWN_LABSTATE`, `VAR_PALLETTOWN_LABSTATE`,
`VAR_ALOLA_STATE`, `VAR_ALOLA_TRAVEL`, `VAR_SLOWPOKE_TAIL`,
`VAR_ELECTRODES_FAINTED`, `VAR_SINJOH_STORYLINE`, `VAR_PLATECOUNTER`.

Var sync remains **monotonic increase only** (no zero/reset propagation). Transient
travel resets rely on each console’s local map scripts after the shared peak value
has been applied.

### PER-PLAYER (intentionally omitted)

| Var | Reason |
| --- | --- |
| `VAR_COMPLETED_HO_OH`, `VAR_COMPLETED_LUGIA` | Legendary completion |
| `VAR_SUICUNE_ENCOUNTERS` | Personal Suicune chase |
| `VAR_LUGIA_OR_HOOH` | Personal beast choice |
| `VAR_REGIS_SCANNED` | Personal Regi scan progress |
| `VAR_NOBLE_*` | Personal noble battles |
| `VAR_SHUCKIE` | Loaned Pokémon ownership |
| `VAR_STARTER_MON` | Personal starter choice |
| Phone item vars (`VAR_*_ITEM_HNS`), Mom gifts, radio IDs | Personal / daily |
| `VAR_BUG_CONTEST_STATE`, daily counters | Session / daily |
| Frontier / contest engine vars | Personal facility state |

---

## 4. Script / map notes

**Must rely on shared flags/vars**

- Map scripts that `setflag`/`clearflag` NPC or obstacle objects (Kurt/well Rockets,
  Radio Tower occupation, Sudowoodo, Snorlax, Farfetch’d trail, gym Ariados,
  Ice Path boulders, Rocket electrodes).
- Door / statue / password gates (`VAR_ROCKET_PASSWORD`, hideout statues,
  Goldenrod underground door).
- Town state machines (`VAR_*_STATE`) that change which NPCs spawn on enter.
- Gym / badge / trainer defeat checks that permanently clear a blocker.
- Alola boat/travel (`VAR_ALOLA_TRAVEL`, Route 13 boat / flyover / Phantonomy
  mapsetup) and totem trigger/defeat gates.

**Stay local**

- `giveitem` for consumables / TMs / berries without a shared progress flag
  (Whirlpool now sets `FLAG_RECEIVED_HM_WHIRLPOOL` and grants on peer apply).
- Legendary `setflag FLAG_HIDE_*` / `FLAG_CAUGHT_*` / `FLAG_DEFEATED_*` (except Red).
- Item ball and hidden item scripts.
- NPC trade and move-tutor “already did this” flags.
- Daily event scripts; phone **gift** pending flags/vars (registration is shared).

**Peer apply behavior**

- Shared progress HMs/keys still use `Coop_GrantSharedProgressItem` (bike, bottle,
  Cut/Flash/Rock Smash/Strength/Surf/Whirlpool, Waterfall ball, Super Rod).
- Hide flags set or clear with no invented item grants.

---

## 5. Gaps closed

**Already shared before the classification audit**

- Adventure/quest block (minus Route 25 nugget)
- Gyms, badges, trainer flags
- Progress key/HM RECEIVED flags + Ice Path Waterfall ball (+ grants)
- Rocket passwords, `FLAG_DEFEATED_RED`, `FLAG_DEFEATED_SUDOWOODO`
- Broad Johto/Kanto/`VAR_ALOLA_STATE`/`VAR_SINJOH_STORYLINE` story vars
- Legendaries kept per-player; presence-only story sync path

**Closed in the classification audit commit**

- Entire story **NPC_HIDE** range (except Ilex second Celebi)
- Story **HIDE_POKEMON** path/quest objects (Farfetch’d, Slowpoke, Ariados, Sudowoodo,
  Snorlax, Lake Gyarados, Cerulean gym Pokémon, Whirl Tentacruel, BF Sudowoodo)
- Match Call story unlocks (added-to-PokéNav, Baoba, rival)
- Safari East/West expansion flags
- `FLAG_SYS_GAME_CLEAR` / `NATIONAL_DEX` / `POKENAV_GET`
- Extended story hides/visits (Slowpoke tails, Route 13 boat, Sinjoh Steven cast,
  Alola/New Sinjoh visited, Samson/scientist intros, Route 49 Kimono)
- Initial sync phases for hide-Pokémon + NPC-hide ranges and an extra named-flag list

**Closed in this gap-close pass**

1. **Clearflag peer mirror** — `FlagClear` queues shared clears; queue/apply pack bit16; bidirectional hide initial sync.
2. **HM Whirlpool** — `FLAG_RECEIVED_HM_WHIRLPOOL` set in `RocketHideout_B2F_hns` after Lance `giveitem`; shared + grant.
3. **Ice Path boulders** — `FLAG_HIDE_ICEPATH_B1F_BOULDER1`..`B2F_BOULDER4` shared (set + clear).
4. **Story vars** — `VAR_ALOLA_TRAVEL`, `VAR_SLOWPOKE_TAIL`, `VAR_ELECTRODES_FAINTED` added.
5. **Electrode hides** — electrode 1–3 shared with the var.
6. **Phone gift timing** — rematch **registration** shared; `FLAG_*_HAS_ITEM_HNS` / `VAR_*_ITEM_HNS` stay personal.
7. **Alola second pass** — totem triggered/defeated + boat/flyover/Phantonomy mapsetup shared; totem/static/Tapu battle hides and `FLAG_BATTLED_*` Alolan form bookkeeping stay personal.
8. **Second sweep** — berry/Bug Contest/Safari session/BF records/Pokéathlon/roamers classified PER-PLAYER; ship/train vars confirmed SHARED; rematch world state via trainer defeat + registration only.

**Closed in Tower Link Multi co-op pass**

9. **Tower Link Multi invite handshake** — three extended-content flags shared +
   bidirectional clear so both phones warp into the multi room together without a
   mailbox layout bump. Facility peer-drop uses `B_OUTCOME_PLAYER_TELEPORTED` /
   party snapshot restore and does **not** set `CHALLENGE_STATUS_LOST`.

**Closed in mailbox v12 follower presence**

10. **Partner follower OW** — mailbox v12 appends `localFollower` /
    `remoteFollower` (sizeof 2292). ROM publishes packed local follower gfx;
    Android mirrors peer → `remoteFollower`; remote object
    `OBJ_EVENT_ID_REMOTE_FOLLOWER` (`0xFC`). See `docs/coop-mailbox.md`.

---

## 6. True leftovers only

1. **Museum fossils** — still **SHARED** via NPC_HIDE range; optional carve-out to
   PER-PLAYER if one-time claw/root pickups should not hide for the peer.
2. **BF lobby NPC hides** — still **SHARED** via NPC_HIDE; optional carve-out if
   lobby cosmetics should stay personal.
3. **Daily events** — remain per-player by design (haircuts, Bug Contest, Buena,
   flower shop berries); not a sync gap.
4. **Monotonic vars** — shared story vars never propagate resets to 0; local map
   scripts still clear transient values (`VAR_ALOLA_TRAVEL`, electrode init heaps).
