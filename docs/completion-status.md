# Heart & Soul completion status

Code-reading audit of `Shraiyde/pokehns-expansion` (HnS maps/scripts).
No emulator / runtime playthrough was used. Co-op rules follow
[`docs/coop-flag-audit.md`](coop-flag-audit.md): shared world/story vs
per-player rewards.

---

## 1. Summary

The Johto → Elite Four → Kanto → Red main path is largely present and
wired in scripts: all 16 gyms grant badges, HMs exist, Rocket / Radio
Tower / Power Plant / Magnet Train / SS Aqua / League / Mt. Silver /
Red are implemented. Extended postgame (Sinjoh, Alola isles, Frontier,
Safari Baoba) also has substantial script coverage.

Leftover **debug warps** into `MAP_TEST_MAP1_HNS` (dead-end test map)
were the clearest main-story softlock risk found in this audit. Those
are fixed in this run. Remaining incompleteness is mostly postgame /
optional polish, cosmetic stubs, or expansion leftovers—not missing
core gym→Red beats.

---

## 2. Done (major story arcs present in code)

- [x] Johto gyms 1–8 (Violet → Blackthorn), including Clair’s Dragon’s Den Rising Badge (`FLAG_BADGE08_GET`)
- [x] Required HMs: Cut, Flash, Rock Smash, Strength, Surf, Whirlpool, Waterfall, Fly
- [x] Sudowoodo / Squirtbottle, Farfetch’d Cut quest, Ampharos / Olivine lighthouse
- [x] Team Rocket: Slowpoke Well, Radio Tower, Mahogany / Lake of Rage, Rocket Hideout + Whirlpool HM
- [x] Kimono Girls / Theater / Tin Tower / Whirl Islands legendary gate (`VAR_ECRUTEAK_CITY_THEATER` → 8 unlocks Indigo Reception Gate)
- [x] SS Aqua Johto ↔ Kanto; Indigo Plateau Elite Four + Lance + Hall of Fame (`FLAG_IS_CHAMPION`, `FLAG_SYS_GAME_CLEAR` via `GameClear`)
- [x] Kanto gyms 9–16; Power Plant Machine Part → Copycat Lost Item → Magnet Train Pass
- [x] Mt. Silver access + Red battle (`FLAG_DEFEATED_RED`, SHARED for co-op)
- [x] Postgame Sinjoh storyline vars 1–13 (plates → Arceus) present in scripts
- [x] Alola island maps (Melemele / Akala / Ula’ula / Poni) with totem / Tapu scripts
- [x] All HnS trainer parties in `trainers_hns.party` have Level entries (no empty parties)
- [x] Map warp targets: every `dest_map` / connection resolves to an existing map id

---

## 3. Missing / incomplete — main-story playability first

### Fixed this run (was blocking / softlock-class)

| Priority | Issue | Fix |
| --- | --- | --- |
| P0 | `NewBarkTown_hns` warps to `MAP_TEST_MAP1_HNS` (×2) and Tin Tower roof (debug). Test map only self-warps → early-game softlock without Fly. | Removed debug warps; renumbered Elm Lab return warp `6` → `4`. |
| P0 | `EcruteakCity_hns` warp one tile north of Pokémon Center → `MAP_TEST_MAP1_HNS` (was Battle Frontier lobby before). | Removed warp. |
| P1 | Copycat cutscenes used `MSGBOX_NPC` mid-script (inner `release`) and Magnet Train branch ended without `release` after setting `VAR_FAN_CLUB_CLEFAIRY`. | Switched cutscene text to `MSGBOX_DEFAULT`; added `release` on quest branches. |
| P2 | Missing commas on `call_if_eq` / `call_if_ge` (Elm police starter-ball hide; HOF Arceus respawn). GAS often still splits on spaces, but commas match project style. | Commas added. |

### Remaining (main-story / near-path)

| Priority | Issue | Notes |
| --- | --- | --- |
| P3 | Gate officers with `script: NULL` (`Gate_FuchsiaCity_Route18_hns`, `Gate_CeladonCity_Route16_hns`, …) | Cosmetic; cycling-road triggers still work. |
| P3 | Story NPCs with `NULL` scripts used only as cutscene props (Silver / Lance / Clair placements) | Expected for scripted scenes; not interactable outside triggers. |
| P3 | `NewSinjoh_hns/scripts.pory` empty | `scripts.inc` is complete; pory stub only. |

No missing badge/HM/gym/rival/Rocket script holes that clearly prevent reaching Red were found beyond the debug warps above.

---

## 4. Postgame / optional incomplete

- Challenge menu source still comments “skeleton / placeholder” item tables; feature is implemented for documented modes (`FEATURES.md`), comments are stale.
- Battle Tower double/multi “under construction” strings exist (vanilla Emerald leftovers); singles path is the supported BF path.
- Safari Zone “under construction” NPC flavor near Cliff Edge / gate; Baoba Safari quest itself is scripted.
- Route 19 “road closed” / Cianwood engineer lines are intentional GSC-style blocks, not unfinished content.
- Alola / Sinjoh / Battle Frontier / Trainer Hill are postgame; Alola Samson Oak dialogue is large pory-generated script (labels use single `:`), not missing.
- Emerald/FRLG leftover maps and `FLAG_SYS_GAME_CLEAR` checks on Hoenn/FRLG maps remain in tree but are outside HnS overworld progression.
- `TestMap1_hns` / `TestMap2_hns` remain in the repo for engine stress tests; no longer linked from New Bark / Ecruteak.

---

## 5. Co-op notes

Fixes in this run do **not** add new story flags/vars. No updates to
`IsCoopSharedStoryFlag`, `sCoopSharedStoryVars`, or `docs/coop-flag-audit.md`
were required.

Reminder when touching progression later:

| Class | Examples |
| --- | --- |
| **SHARED** | Gym/badge flags, story hide/show, Rocket/quest flags, `FLAG_DEFEATED_RED`, `VAR_SINJOH_STORYLINE`, `VAR_ALOLA_STATE`, HMs / progress key items |
| **PER-PLAYER** | Legendaries/statics, gift ownership, normal item balls, phone gift pending flags |

Do not share personal inventory, party, or money.

---

## Audit method (this pass)

- Grep TODO/FIXME/stub/placeholder/NYI across `data/maps/*_hns`, `data/scripts`, `data/text`, `src`
- Near-empty `scripts.inc` / `.pory` scan
- Warp target existence + PC-adjacent debug warp scan
- Story flag/var set-vs-check heuristics; HM/badge grant sites; Red / League / Sinjoh state machine
- Trainer party emptiness check on `trainers_hns.party`
