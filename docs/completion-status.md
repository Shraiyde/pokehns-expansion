# Heart & Soul completion status

Code-reading audit of `Shraiyde/pokehns-expansion` (HnS maps/scripts).
No emulator / runtime playthrough was used. Co-op rules follow
[`docs/coop-flag-audit.md`](coop-flag-audit.md): shared world/story vs
per-player rewards.

---

## 1. Summary

**Main story Johto → Elite Four → Kanto → Red is complete in code.**
All gyms, HMs, Rocket / Radio Tower / Power Plant / Magnet Train /
SS Aqua / League / Mt. Silver / Red are wired. Passes 1–2 removed
debug softlock warps, hardened Copycat / HOF scripts, closed NULL
gate-officer dialogs on Cycling Road, and replaced an Indigo League
“check back later” placeholder.

Remaining gaps are postgame / optional polish only.

---

## 2. Done (major story arcs present in code)

- [x] Johto gyms 1–8 (Violet → Blackthorn), including Clair’s Dragon’s Den Rising Badge (`FLAG_BADGE08_GET`)
- [x] Required HMs: Cut, Flash, Rock Smash, Strength, Surf, Whirlpool, Waterfall, Fly
- [x] Sudowoodo / Squirtbottle, Farfetch’d Cut quest, Ampharos / Olivine lighthouse
- [x] Team Rocket: Slowpoke Well, Radio Tower, Mahogany / Lake of Rage, Rocket Hideout + Whirlpool HM
- [x] Kimono Girls / Theater / Tin Tower / Whirl Islands (`VAR_ECRUTEAK_CITY_THEATER` → 8) + eight badges unlock Indigo Reception Gate
- [x] Rival Silver battles: Cherrygrove (1), Azalea (2), Burned Tower (3), Goldenrod Underground (4), Victory Road (5), Mt. Moon (6), Indigo rematch (7); other Silver objects are cutscene props
- [x] SS Aqua Johto ↔ Kanto; Indigo Plateau Elite Four + Lance + Hall of Fame (`FLAG_IS_CHAMPION`, `FLAG_SYS_GAME_CLEAR` via `GameClear`)
- [x] Kanto gyms 9–16; Power Plant Machine Part → Copycat Lost Item → Magnet Train Pass
- [x] Mt. Silver access + Red battle (`FLAG_DEFEATED_RED`, SHARED for co-op)
- [x] Postgame Sinjoh storyline vars 1–13 (plates → Arceus) present in scripts
- [x] Alola island maps (Melemele / Akala / Ula’ula / Poni) with totem / Tapu scripts
- [x] All HnS trainer parties in `trainers_hns.party` have Level entries (no empty parties)
- [x] Map warp targets: every `dest_map` / connection resolves to an existing map id
- [x] No undefined `FLAG_*` references from HnS scripts; key story vars advance through full ranges
- [x] Cycling Road gate officers (Route 16 / Route 18) have talk scripts

---

## 3. Fixes by pass

### Pass 1 (softlocks / Magnets / script hygiene)

| Priority | Issue | Fix |
| --- | --- | --- |
| P0 | New Bark warps to `MAP_TEST_MAP1_HNS` / Tin Tower roof | Removed; Elm Lab side exit warp `6` → `4` |
| P0 | Ecruteak PC-adjacent warp to test map | Removed |
| P1 | Copycat Magnet Train cutscenes (`MSGBOX_NPC` mid-script / missing `release`) | `MSGBOX_DEFAULT` + `release` |
| P2 | Missing commas on Elm police / HOF Arceus `call_if_*` | Commas added |

### Pass 2 (deeper sweep / P3 closeout)

| Priority | Issue | Fix |
| --- | --- | --- |
| P3 | `Gate_CeladonCity_Route16_hns` officer `script: NULL` | Added cycling-road officer dialog |
| P3 | `Gate_FuchsiaCity_Route18_hns` officer `script: NULL` | Added cycling-road officer dialog |
| P3 | Indigo Plateau Blackbelt “not taking challengers / check back later” | Replaced with Elite Four flavor |
| P3 | Fighting Dojo VIP mart attendant `NULL` | Short attendant dialog |
| — | Stale `@ CHANGE THIS TO 1 LATER` on Cherrygrove guide decline | Comment removed (value was already 1) |

### Pass 2 checks with no story-blocking findings

- No remaining warps from HnS overworld into test maps (false positives were `CONTEST` matching `TEST`)
- Indigo unlock complete: `VAR_ECRUTEAK_CITY_THEATER >= 8` (Tin Tower / Lugia) **and** `FLAG_BADGE08_GET`
- Silver battles 1–7 present with full starter branches; Olivine / Ecruteak / Sprout Tower / Route 22 / etc. are cutscene-only by design
- No undefined flags; story state vars for Goldenrod / Mahogany / Olivine / SS Aqua / Kanto Rocket / Blackthorn advance through expected values
- Main-path “under construction” lines (Cianwood Safari engineer, Route 19) are intentional GSC blocks, not unfinished content

---

## 4. Remaining (postgame / optional only)

| Item | Notes |
| --- | --- |
| Cutscene-only NPCs with `NULL` interact scripts (Silver / Lance / Clair props) | Expected; triggered via map scripts |
| `NewSinjoh_hns/scripts.pory` empty | `scripts.inc` is complete |
| Challenge menu “skeleton / placeholder” source comments | Feature implemented; comments stale |
| Battle Tower double/multi “under construction” strings | Emerald leftovers; singles supported |
| Safari “under construction” flavor NPCs | Baoba quest itself is scripted |
| `TestMap1_hns` / `TestMap2_hns` still in tree | Unlinked from New Bark / Ecruteak; engine stress maps |
| Emerald/FRLG leftover maps in repo | Outside HnS overworld progression |
| New Bark Lab policeman `NULL` script | Cutscene-only (`FLAG_HIDE_LAB_POLICEMAN`) |

---

## 5. Co-op notes

Neither pass added new story flags/vars. No updates to
`IsCoopSharedStoryFlag`, `sCoopSharedStoryVars`, or
`docs/coop-flag-audit.md` were required.

| Class | Examples |
| --- | --- |
| **SHARED** | Gym/badge flags, story hide/show, Rocket/quest flags, `FLAG_DEFEATED_RED`, `VAR_SINJOH_STORYLINE`, `VAR_ALOLA_STATE`, HMs / progress key items |
| **PER-PLAYER** | Legendaries/statics, gift ownership, normal item balls, phone gift pending flags |

Do not share personal inventory, party, or money.

---

## Audit method

- Grep TODO/FIXME/stub/placeholder/NYI; near-empty scripts; warp integrity
- `script: NULL` scan focused on officers / gates / talkable humans
- Silver trainerbattle inventory vs expected story points
- Indigo Reception Gate conditions; flag/var set-vs-check; main-path placeholder text
