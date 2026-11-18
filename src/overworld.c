#include "global.h"
#include "overworld.h"
#include "constants/heal_locations.h"
#include "battle_pyramid.h"
#include "battle_setup.h"
#include "battle_util.h"
#include "berry.h"
#include "bg.h"
#include "bug_contest.h"
#include "cable_club.h"
#include "credits_frlg.h"
#include "clock.h"
#include "dexnav.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "fake_rtc.h"
#include "field_camera.h"
#include "field_control_avatar.h"
#include "field_door.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "field_message_box.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_special_scene.h"
#include "field_specials.h"
#include "field_tasks.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "fldeff.h"
#include "follower_npc.h"
#include "gpu_regs.h"
#include "heal_location.h"
#include "io_reg.h"
#include "item.h"
#include "item_icon.h"
#include "link.h"
#include "link_rfu.h"
#include "load_save.h"
#include "main.h"
#include "malloc.h"
#include "m4a.h"
#include "map_name_popup.h"
#include "match_call.h"
#include "menu.h"
#include "metatile_behavior.h"
#include "mirage_tower.h"
#include "money.h"
#include "new_game.h"
#include "oras_dowse.h"
#include "palette.h"
#include "play_time.h"
#include "random.h"
#include "roamer.h"
#include "rotating_gate.h"
#include "rtc.h"
#include "safari_zone.h"
#include "save.h"
#include "save_location.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "secret_base.h"
#include "sound.h"
#include "start_menu.h"
#include "string_util.h"
#include "task.h"
#include "tileset_anims.h"
#include "time_events.h"
#include "trainer_hill.h"
#include "trainer_pokemon_sprites.h"
#include "tv.h"
#include "scanline_effect.h"
#include "wild_encounter.h"
#include "vs_seeker.h"
#include "frontier_util.h"
#include "constants/abilities.h"
#include "constants/event_object_movement.h"
#include "constants/flags.h"
#include "constants/event_objects.h"
#include "constants/layouts.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/trainer_hill.h"
#include "constants/weather.h"
#include "constants/vars_hns.h"
#include "nuzlocke.h"
#include "randomizer.h"
#include "pokemon_storage_system.h"

STATIC_ASSERT((B_FLAG_FOLLOWERS_DISABLED == 0 || OW_FOLLOWERS_ENABLED), FollowersFlagAssignedWithoutEnablingThem);

struct CableClubPlayer
{
    u8 playerId;
    u8 isLocalPlayer;
    u8 movementMode;
    u8 facing;
    struct MapPosition pos;
    u16 metatileBehavior;
};

#define PLAYER_LINK_STATE_IDLE 0x80
#define PLAYER_LINK_STATE_BUSY 0x81
#define PLAYER_LINK_STATE_READY 0x82
#define PLAYER_LINK_STATE_EXITING_ROOM 0x83

#define FACING_NONE 0
#define FACING_UP 1
#define FACING_DOWN 2
#define FACING_LEFT 3
#define FACING_RIGHT 4
#define FACING_FORCED_UP 7
#define FACING_FORCED_DOWN 8
#define FACING_FORCED_LEFT 9
#define FACING_FORCED_RIGHT 10

extern const struct MapLayout *const gMapLayouts[];
extern const struct MapHeader *const *const gMapGroups[];

static void Overworld_ResetStateAfterWhiteOut(void);
static void CB2_ReturnToFieldLocal(void);
static void CB2_ReturnToFieldLink(void);
static void CB2_LoadMapOnReturnToFieldCableClub(void);
static void CB2_LoadMap2(void);
static void VBlankCB_Field(void);
static void SpriteCB_LinkPlayer(struct Sprite *);
static void ChooseAmbientCrySpecies(void);
static void DoMapLoadLoop(u8 *);
static bool32 LoadMapInStepsLocal(u8 *, bool32);
static bool32 LoadMapInStepsLink(u8 *);
static bool32 ReturnToFieldLocal(u8 *);
static bool32 ReturnToFieldLink(u8 *);
static void InitObjectEventsLink(void);
static void InitObjectEventsLocal(void);
static void InitOverworldGraphicsRegisters(void);
static u8 GetSpriteForLinkedPlayer(u8);
static void DestroyLinkPlayerObject(u8);
static u16 KeyInterCB_SendNothing(u32);
static void ResetMirageTowerAndSaveBlockPtrs(void);
static void ResetScreenForMapLoad(void);
static void OffsetCameraFocusByLinkPlayerId(void);
static void SpawnLinkPlayers(void);
static void SetCameraToTrackGuestPlayer(void);
static void ResumeMap(bool32);
static void SetCameraToTrackPlayer(void);
static void InitObjectEventsReturnToField(void);
static void InitViewGraphics(void);
static void SetCameraToTrackGuestPlayer_2(void);
static void ClearAllPlayerKeys(void);
static void ResetAllPlayerLinkStates(void);
static void UpdateHeldKeyCode(u16);
static void UpdateAllLinkPlayers(u16 *, s32);
static enum Direction FlipVerticalAndClearForced(u8, u8);
static u8 LinkPlayerGetCollision(u8, enum Direction, s16, s16);
static void CreateLinkPlayerSprite(u8, enum GameVersion);
static void GetLinkPlayerCoords(u8, s16 *, s16 *);
static u8 GetLinkPlayerFacingDirection(u8);
static u8 GetLinkPlayerElevation(u8);
static u8 GetLinkPlayerIdAt(s16, s16);
static void SetPlayerFacingDirection(u8, u8);
static void ZeroObjectEvent(struct ObjectEvent *);
static void SpawnLinkPlayerObjectEvent(u8, s16, s16, u8);
static void InitLinkPlayerObjectEventPos(struct ObjectEvent *, s16, s16);
static u8 GetSpriteForLinkedPlayer(u8);
static void RunTerminateLinkScript(void);
static u32 GetLinkSendQueueLength(void);
static void ZeroLinkPlayerObjectEvent(struct LinkPlayerObjectEvent *);
static const u8 *TryInteractWithPlayer(struct CableClubPlayer *);
static u16 GetDirectionForEventScript(const u8 *);
static void InitLinkPlayerQueueScript(void);
static void InitLinkRoomStartMenuScript(void);
static void RunInteractLocalPlayerScript(const u8 *);
static void RunConfirmLeaveCableClubScript(void);
static void InitMenuBasedScript(const u8 *);
static void LoadCableClubPlayer(s32, s32, struct CableClubPlayer *);
static bool32 IsCableClubPlayerUnfrozen(struct CableClubPlayer *);
static bool32 CanCableClubPlayerPressStart(struct CableClubPlayer *);
static const u8 *TryGetTileEventScript(struct CableClubPlayer *);
static bool32 PlayerIsAtSouthExit(struct CableClubPlayer *);
static const u8 *TryInteractWithPlayer(struct CableClubPlayer *);
static u16 KeyInterCB_DeferToRecvQueue(u32);
static u16 KeyInterCB_DeferToSendQueue(u32);
static void ResetPlayerHeldKeys(u16 *);
static u16 KeyInterCB_SelfIdle(u32);
static u16 KeyInterCB_DeferToEventScript(u32);
static u16 GetDirectionForDpadKey(u16);
static void CB1_OverworldLink(void);
#if IS_HNS
#define HNS_COOP_MAILBOX_MAGIC 0x504F4F43
#define HNS_COOP_MAILBOX_VERSION 14
#define HNS_COOP_CONFIG_BLOB_SIZE 44
// localFollower / remoteFollower packing (u32):
//   bit 31     = visible (0 → treat as no follower / hide)
//   bits 0-15  = graphicsId (species + OBJ_EVENT_MON [+ SHINY] [+ FEMALE])
// Facing is not packed; remote render derives orientation from remoteFacing /
// the partner avatar. Publish 0 when there is no visible local follower.
#define HNS_COOP_FOLLOWER_VISIBLE (1u << 31)
#define HNS_COOP_FOLLOWER_GFX_MASK 0xFFFFu

// Note: Emerald reuses the direction and range variables during Link mode
// as special gender and direction values. The types and placement
// conflict with the usual Event Object struct, thus the definitions.
#define linkGender(obj) obj->singleMovementActive
// not even one can reference *byte* aligned bitfield members...
#define linkDirection(obj) ((u8 *)obj)[offsetof(typeof(*obj), range)] // -> rangeX
#define HNS_COOP_FLAG_QUEUE_SIZE 128
#define HNS_COOP_VAR_QUEUE_SIZE 128

struct HnsCoopMailbox
{
    volatile u32 magic;
    volatile u16 version;
    volatile u16 size;
    volatile u32 transportConnected;
    volatile u32 gameReady;
    volatile u32 outgoingFlagWrite;
    volatile u32 outgoingFlagRead;
    volatile u32 incomingFlagWrite;
    volatile u32 incomingFlagRead;
    volatile u32 outgoingFlags[HNS_COOP_FLAG_QUEUE_SIZE];
    volatile u32 incomingFlags[HNS_COOP_FLAG_QUEUE_SIZE];
    volatile u32 outgoingVarWrite;
    volatile u32 outgoingVarRead;
    volatile u32 incomingVarWrite;
    volatile u32 incomingVarRead;
    volatile u32 outgoingVars[HNS_COOP_VAR_QUEUE_SIZE];
    volatile u32 incomingVars[HNS_COOP_VAR_QUEUE_SIZE];
    volatile u32 localStorySyncReady;
    volatile u32 remoteStorySyncReady;
    volatile u32 localMapId;
    volatile u32 remoteMapId;
    volatile u32 remoteMapReady;
    volatile u32 localSioActive;
    volatile u32 remoteSioActive;
    volatile u32 hostRole;
    volatile u32 localConfigRandomizer;
    volatile u32 localConfigNuzlocke;
    volatile u32 localRandomizerSeed;
    volatile u32 localConfigReady;
    volatile u32 remoteConfigRandomizer;
    volatile u32 remoteConfigNuzlocke;
    volatile u32 remoteRandomizerSeed;
    volatile u32 remoteConfigReady;
    volatile u32 localX;
    volatile u32 localY;
    volatile u32 remoteX;
    volatile u32 remoteY;
    volatile u32 localGender;
    volatile u32 remoteGender;
    volatile u32 localFacing;
    volatile u32 remoteFacing;
    volatile u32 peerLinkReady;
    volatile u8 localConfigBlob[HNS_COOP_CONFIG_BLOB_SIZE];
    volatile u8 remoteConfigBlob[HNS_COOP_CONFIG_BLOB_SIZE];
    // Appended in mailbox v12 — do not reorder earlier fields (Android config
    // blob offsets stay valid). Offsets: localFollower @ 2284, remoteFollower
    // @ 2288.
    volatile u32 localFollower;   // packed graphics + visible; written by ROM
    volatile u32 remoteFollower;  // peer's localFollower; written by Android
    // v13: ROM increments on return-to-field so Android forces POSITION republish
    // and asks the peer for a fresh presence snapshot. Offset 2292; sizeof 2296.
    volatile u32 localPresenceResync;
    // v14: locomotion / door meta for HeldMovement partner replay.
    // Packing: action:8 | avatarState:8 | doorPhase:8 | claimSeq:8. Offsets 2296/2300.
    volatile u32 localAvatarState;
    volatile u32 remoteAvatarState;
};

STATIC_ASSERT((sizeof(struct ChallengeSettings) <= HNS_COOP_CONFIG_BLOB_SIZE - 4), CoopChallengeSettingsFitsBlob);
STATIC_ASSERT((sizeof(struct HnsCoopMailbox) == 2304), HnsCoopMailboxV14Size);

// EWRAM_DATA is placed in .sbss by the GBA linker, so initialize the
// mailbox header at runtime instead of using a nonzero static initializer.
EWRAM_DATA struct HnsCoopMailbox gHnsCoopMailbox = {0};
static EWRAM_DATA bool8 sCoopLinkStarted = FALSE;
static EWRAM_DATA bool8 sApplyingRemoteStoryFlag = FALSE;
static EWRAM_DATA bool8 sApplyingRemoteStoryVar = FALSE;
static EWRAM_DATA u16 sCoopInitialFlagCursor = 0;
static EWRAM_DATA u16 sCoopInitialVarCursor = 0;
static EWRAM_DATA bool8 sCoopInitialHidePokemonStarted = FALSE;
static EWRAM_DATA bool8 sCoopInitialNpcHideStarted = FALSE;
static EWRAM_DATA bool8 sCoopInitialGymFlagsStarted = FALSE;
static EWRAM_DATA bool8 sCoopInitialBadgeFlagsStarted = FALSE;
static EWRAM_DATA bool8 sCoopInitialTrainerFlagsStarted = FALSE;
static EWRAM_DATA bool8 sCoopInitialRegisteredFlagsStarted = FALSE;
static EWRAM_DATA bool8 sCoopInitialExtraFlagsStarted = FALSE;
static EWRAM_DATA u16 sCoopInitialExtraFlagCursor = 0;
static EWRAM_DATA bool8 sCoopInitialSyncStarted = FALSE;
static EWRAM_DATA bool8 sCoopStorySyncReady = FALSE;
static EWRAM_DATA bool8 sCoopPrevPeerLinkReady = FALSE;
static EWRAM_DATA u8 sCoopStorySyncQuietFrames = 0;
static EWRAM_DATA u32 sCoopLastIncomingFlagWrite = 0;
static EWRAM_DATA u32 sCoopLastIncomingVarWrite = 0;
static EWRAM_DATA u32 sCoopLastRemoteConfigReady = 0;

static const u16 sCoopSharedStoryVars[] =
{
    // Johto story, towns, gyms, and scripted encounters.
    // Legendary/static personal progress (Ho-Oh/Lugia/Suicune/Regis/Nobles)
    // stays local and is intentionally omitted.
    VAR_AZALEA_TOWN_STATE,
    VAR_BAOBA_QUEST_STATE,
    VAR_BILLS_GRANDPA,
    VAR_BLACKTHORN_CITY_STATE,
    VAR_BLACKTHORN_GYM_STATE,
    VAR_CHERRYGROVE_CITY_STATE,
    VAR_CIANWOOD_CITY_STATE,
    VAR_CLEFAIRY_DOLL_STATE,
    VAR_DRAGONS_DEN_QUIZ,
    VAR_ECRUTEAK_CITY_STATE,
    VAR_ECRUTEAK_CITY_THEATER,
    VAR_ELECTRODES_FAINTED,
    VAR_ELEVATOR,
    VAR_FAN_CLUB_CLEFAIRY,
    VAR_FAN_CLUB_GUY,
    VAR_GOLDENROD_CITY_STATE,
    VAR_ICE_PATH_STATE,
    VAR_ILEX_FOREST_FARFETCHD,
    VAR_KANTO_ROCKET_STORY_STATE,
    VAR_KENYA,
    VAR_LAKE_OF_RAGE_FISHERMAN,
    VAR_LEAGUE_STATE,
    VAR_MAHOGANY_TOWN_STATE,
    VAR_MT_SILVER_STATE,
    VAR_NEWBARKTOWN_LABSTATE,
    VAR_NEWBARK_TOWN_STATE,
    VAR_NUM_BADGES,
    VAR_OLIVINE_CITY_STATE,
    VAR_ROCKET_HIDEOUT_STATUE_1,
    VAR_ROCKET_HIDEOUT_STATUE_2,
    VAR_ROCKET_HIDEOUT_STATUE_3,
    VAR_ROCKET_HIDEOUT_STATUE_4,
    VAR_ROCKET_HIDEOUT_STATUE_5,
    VAR_ROCKET_PASSWORD,
    VAR_ROUTE27_BAOBA_CALL,
    VAR_ROUTE27_STATE,
    VAR_ROUTE39_BARN,
    VAR_RUINSOFALPH_STATE,
    VAR_SLOWPOKE_TAIL,
    VAR_SPROUT_TOWER,
    VAR_SSAQUA_STATE,
    VAR_TOHJO_FALLS_GIOVANNI_STATE,
    VAR_TRAIN,
    VAR_TRIGGER_ELM_ROCKET_CALL,
    VAR_VIOLET_CITY_KIMONO_GIRL,
    VAR_VIOLET_CITY_STATE,
    // Kanto postgame and connected Johto/Kanto progression.
    VAR_CELADON_CITY_STATE,
    VAR_CERULEAN_CITY_STATE,
    VAR_CINNABAR_ISLAND_STATE,
    VAR_FUCHSIA_CITY_STATE,
    VAR_GETFLY,
    VAR_KANTO_SAFARI_ZONE_PROGRESS,
    VAR_LAVENDER_TOWN_STATE,
    VAR_PALLET_TOWN_STATE,
    VAR_PALLETTOWN_LABSTATE,
    VAR_PEWTER_CITY_STATE,
    VAR_ROUTE22_GIOVANNI_STATE,
    VAR_ROUTE28_SCIENTIST,
    VAR_OAK_MT_SILVER_CALL,
    VAR_SAFFRON_CITY_STATE,
    VAR_SAFARI_ZONE_GATE_STATE,
    VAR_VERMILION_CITY_STATE,
    VAR_VERMILION_CITY_SAMSON,
    VAR_VERMILION_CITY_STEVEN,
    VAR_VIRIDIAN_CITY_STATE,
    VAR_VIRIDIAN_FOREST_PICHU,
    // Later postgame story arcs (non-legendary).
    VAR_ALOLA_STATE,
    VAR_ALOLA_TRAVEL,
    VAR_PLATECOUNTER,
    VAR_SINJOH_STORYLINE,
};

// Extra shared flags outside the contiguous adventure/badge/trainer scans
// (item-ball HM, Whirlpool receipt, Ice Path boulders, electrodes, safari,
// sys unlocks, Match Call registration, extended story / Alola gates).
static const u16 sCoopInitialExtraSharedFlags[] =
{
    FLAG_ITEM_ICEPATH1_HM_WATERFALL,
    FLAG_RECEIVED_HM_WHIRLPOOL,
    FLAG_HIDE_ICEPATH_B1F_BOULDER1,
    FLAG_HIDE_ICEPATH_B1F_BOULDER2,
    FLAG_HIDE_ICEPATH_B1F_BOULDER3,
    FLAG_HIDE_ICEPATH_B1F_BOULDER4,
    FLAG_HIDE_ICEPATH_B2F_BOULDER1,
    FLAG_HIDE_ICEPATH_B2F_BOULDER2,
    FLAG_HIDE_ICEPATH_B2F_BOULDER3,
    FLAG_HIDE_ICEPATH_B2F_BOULDER4,
    FLAG_HIDE_ROCKETHIDEOUT_ELECTRODE1,
    FLAG_HIDE_ROCKETHIDEOUT_ELECTRODE2,
    FLAG_HIDE_ROCKETHIDEOUT_ELECTRODE3,
    FLAG_SAFARI_ZONE_EAST_EXPANSION,
    FLAG_SAFARI_ZONE_WEST_EXPANSION,
    FLAG_SYS_GAME_CLEAR,
    FLAG_SYS_NATIONAL_DEX,
    FLAG_SYS_POKENAV_GET,
    FLAG_HIDE_SLOWPOKE_WELL_TAILLESS_SLOWPOKES,
    FLAG_HIDE_AZALEA_TOWN_TAILED_SLOWPOKES,
    FLAG_HIDE_ROUTE13_BOAT,
    FLAG_HIDE_FLYOVER,
    FLAG_HIDE_PHANT_OUTSIDE,
    FLAG_HIDE_PHANT_INSIDE,
    FLAG_VISITED_NEW_SINJOH,
    FLAG_VISITED_ALOLA,
    FLAG_HIDE_SINJOHRUINS_STEVEN,
    FLAG_HIDE_SINJOHRUINS_LAB_STEVEN,
    FLAG_HIDE_SINJOHRUINS_ARCEUSROOM_STEVEN,
    FLAG_HIDE_NEWSINJOH_STEVEN,
    FLAG_HIDE_VERMILION_CITY_SAMSON,
    FLAG_HIDE_ROUTE28_SCIENTIST,
    FLAG_HIDE_SAMSONOAK_INTRO,
    FLAG_ROUTE_49_KIMONOGIRL,
    FLAG_TRIGGERED_PONI_TOTEM,
    FLAG_TRIGGERED_ULAULA_TOTEM,
    FLAG_TRIGGERED_AKALA_TOTEM,
    FLAG_TRIGGERED_MELEMELE_TOTEM,
    FLAG_DEFEATED_ULAULA_TOTEM,
    FLAG_DEFEATED_PONI_TOTEM,
    FLAG_DEFEATED_AKALA_TOTEM,
    FLAG_DEFEATED_MELEMELE_TOTEM,
};

static bool8 IsCoopSharedStoryPokemonHide(u16 flagId)
{
    return flagId == FLAG_HIDE_AZALEA_TOWN_FARFETCHD
        || flagId == FLAG_HIDE_AZALEA_TOWN_SLOWPOKE
        || flagId == FLAG_HIDE_AZALEA_ARIADOS_LEFT
        || flagId == FLAG_HIDE_AZALEA_ARIADOS_MID
        || flagId == FLAG_HIDE_AZALEA_ARIADOS_RIGHT
        || flagId == FLAG_HIDE_ILEX_FOREST_FARFETCHD_1
        || flagId == FLAG_HIDE_ILEX_FOREST_FARFETCHD_2
        || flagId == FLAG_HIDE_ILEX_FOREST_FARFETCHD_3
        || flagId == FLAG_HIDE_ILEX_FOREST_FARFETCHD_4
        || flagId == FLAG_HIDE_LAKE_OF_RAGE_GYARADOS
        || flagId == FLAG_HIDE_WHIRL_ISLANDS_TENTACRUEL
        || flagId == FLAG_HIDE_SUDOWOODO
        || flagId == FLAG_HIDE_CERULEAN_GYM_POKEMON
        || flagId == FLAG_HIDE_VERMILION_SNORLAX
        || flagId == FLAG_HIDE_BATTLE_FRONTIER_SUDOWOODO
        || flagId == FLAG_HIDE_ROCKETHIDEOUT_ELECTRODE1
        || flagId == FLAG_HIDE_ROCKETHIDEOUT_ELECTRODE2
        || flagId == FLAG_HIDE_ROCKETHIDEOUT_ELECTRODE3;
}

static bool8 IsCoopSharedExtendedStoryHide(u16 flagId)
{
    return flagId == FLAG_HIDE_SLOWPOKE_WELL_TAILLESS_SLOWPOKES
        || flagId == FLAG_HIDE_AZALEA_TOWN_TAILED_SLOWPOKES
        || flagId == FLAG_HIDE_ROUTE13_BOAT
        || flagId == FLAG_HIDE_FLYOVER
        || flagId == FLAG_HIDE_PHANT_OUTSIDE
        || flagId == FLAG_HIDE_PHANT_INSIDE
        || flagId == FLAG_HIDE_SINJOHRUINS_STEVEN
        || flagId == FLAG_HIDE_SINJOHRUINS_LAB_STEVEN
        || flagId == FLAG_HIDE_SINJOHRUINS_ARCEUSROOM_STEVEN
        || flagId == FLAG_HIDE_NEWSINJOH_STEVEN
        || flagId == FLAG_HIDE_VERMILION_CITY_SAMSON
        || flagId == FLAG_HIDE_ROUTE28_SCIENTIST
        || flagId == FLAG_HIDE_SAMSONOAK_INTRO
        || flagId == FLAG_ROUTE_49_KIMONOGIRL;
}

static bool8 IsCoopIcePathBoulderFlag(u16 flagId)
{
    return flagId >= FLAG_HIDE_ICEPATH_B1F_BOULDER1
        && flagId <= FLAG_HIDE_ICEPATH_B2F_BOULDER4;
}

// Hide/show object flags that must mirror CLEARs on reconnect (not badges /
// trainers / RECEIVED progress, which stay SET-only on initial sync).
static bool8 IsCoopTowerLinkMultiFlag(u16 flagId)
{
    return flagId == FLAG_COOP_TOWER_LINK_MULTI_INVITE
        || flagId == FLAG_COOP_TOWER_LINK_MULTI_LVL_OPEN
        || flagId == FLAG_COOP_TOWER_LINK_MULTI_PARTNER_READY;
}

static bool8 IsCoopBidirectionalHideFlag(u16 flagId)
{
    if (flagId >= HNS_NPC_HIDE_START
        && flagId < HNS_NPC_HIDE_START + HNS_NPC_HIDE_COUNT
        && flagId != FLAG_HIDE_ILEX_FOREST_SECOND_CELEBI)
        return TRUE;
    if (IsCoopSharedStoryPokemonHide(flagId))
        return TRUE;
    if (IsCoopIcePathBoulderFlag(flagId))
        return TRUE;
    if (IsCoopSharedExtendedStoryHide(flagId))
        return TRUE;
    // Invite / ready / level-open must clear on both phones when a challenge
    // cancels or a peer drops mid-Tower Link Multi.
    if (IsCoopTowerLinkMultiFlag(flagId))
        return TRUE;
    return FALSE;
}

static bool8 IsCoopSharedStoryFlag(u16 flagId)
{
    if (flagId >= FLAG_ADVENTURE_STARTED && flagId <= FLAG_UNLOCKED_GOLDENROD_UNDERGROUND_DOOR
        && flagId != FLAG_ROUTE25_GOT_NUGGET)
        return TRUE;
    if (flagId >= FLAG_DEFEATED_VIOLET_GYM && flagId <= FLAG_DEFEATED_VIRIDIAN_GYM)
        return TRUE;
    // Legendary/static defeat and catch flags stay per-player. Only share
    // FLAG_DEFEATED_RED from the legend/boss block (story boss, not a static).
    // HNS_LEGEND_CAUGHT_* (FLAG_CAUGHT_HO_OH..FLAG_CAUGHT_MEW) are never shared.
    if (flagId >= FLAG_BADGE01_GET && flagId <= FLAG_BADGE16_GET)
        return TRUE;
    if (flagId >= TRAINER_FLAGS_START && flagId <= TRAINER_FLAGS_END)
        return TRUE;

    // Story NPC hide/show for a consistent world. Legendary Celebi NPC stay
    // per-player with the other static/legendary encounter hides.
    if (flagId >= HNS_NPC_HIDE_START
        && flagId < HNS_NPC_HIDE_START + HNS_NPC_HIDE_COUNT
        && flagId != FLAG_HIDE_ILEX_FOREST_SECOND_CELEBI)
        return TRUE;

    // Path/story world Pokémon hides (not static legendaries / starter balls /
    // beasts / Suicune route spawns / Union Cave Lapras). Rocket electrodes are
    // shared with VAR_ELECTRODES_FAINTED for hideout world consistency.
    if (IsCoopSharedStoryPokemonHide(flagId))
        return TRUE;

    // Ice Path boulder puzzle object hides.
    if (IsCoopIcePathBoulderFlag(flagId))
        return TRUE;

    // Progress-gating key items / HMs / unlocks. Consumable gifts, TMs, berries,
    // nuggets, and gift-Pokémon ownership flags stay local to each player.
    if (flagId == FLAG_RECEIVED_BIKE
        || flagId == FLAG_HAS_MATCH_CALL
        || flagId == FLAG_RECEIVED_HM_CUT
        || flagId == FLAG_RECEIVED_HM_FLASH
        || flagId == FLAG_RECEIVED_HM_ROCK_SMASH
        || flagId == FLAG_RECEIVED_HM_STRENGTH
        || flagId == FLAG_RECEIVED_HM_SURF
        || flagId == FLAG_RECEIVED_HM_WHIRLPOOL
        || flagId == FLAG_RECEIVED_POKENAV
        || flagId == FLAG_RECEIVED_RUNNING_SHOES
        || flagId == FLAG_RECEIVED_SQUIRTBOTTLE
        || flagId == FLAG_GOT_SUPER_ROD
#if FLAG_GET_HEADBUTT
        || flagId == FLAG_GET_HEADBUTT
#endif
        || flagId == FLAG_ITEM_ICEPATH1_HM_WATERFALL)
        return TRUE;

    // Match Call unlocks that gate story / Baoba safari progress.
    if (flagId == FLAG_ADDED_MATCH_CALL_TO_POKENAV
        || flagId == FLAG_ENABLE_BAOBA_MATCH_CALL
        || flagId == FLAG_REGISTER_RIVAL_POKENAV)
        return TRUE;

    // Phone book rematch contacts (registration only). Gift pending flags
    // FLAG_*_HAS_ITEM_HNS and VAR_*_ITEM_HNS stay per-player.
    if (flagId >= TRAINER_REGISTERED_FLAGS_START
        && flagId <= FLAG_REGISTERED_REENA_HNS)
        return TRUE;

    // Safari Zone world expansions (not in-zone session flags).
    if (flagId == FLAG_SAFARI_ZONE_EAST_EXPANSION
        || flagId == FLAG_SAFARI_ZONE_WEST_EXPANSION)
        return TRUE;

    // Postgame / PokeNav system unlocks needed for shared world access.
    if (flagId == FLAG_SYS_GAME_CLEAR
        || flagId == FLAG_SYS_NATIONAL_DEX
        || flagId == FLAG_SYS_POKENAV_GET)
        return TRUE;

    // Extended story NPC / visit / path flags (legendaries and items stay out).
    if (IsCoopSharedExtendedStoryHide(flagId)
        || flagId == FLAG_VISITED_NEW_SINJOH
        || flagId == FLAG_VISITED_ALOLA)
        return TRUE;

    // Alola totem story gates (not personal totem/static encounter hides).
    if (flagId == FLAG_TRIGGERED_PONI_TOTEM
        || flagId == FLAG_TRIGGERED_ULAULA_TOTEM
        || flagId == FLAG_TRIGGERED_AKALA_TOTEM
        || flagId == FLAG_TRIGGERED_MELEMELE_TOTEM
        || flagId == FLAG_DEFEATED_ULAULA_TOTEM
        || flagId == FLAG_DEFEATED_PONI_TOTEM
        || flagId == FLAG_DEFEATED_AKALA_TOTEM
        || flagId == FLAG_DEFEATED_MELEMELE_TOTEM)
        return TRUE;

    // Battle Tower co-op Link Multi facility invite / ready handshake.
    if (IsCoopTowerLinkMultiFlag(flagId))
        return TRUE;

    return flagId == FLAG_GOT_PASSWORD_FROM_ETO
        || flagId == FLAG_GOT_PASSWORD_FROM_GRUNTF
        || flagId == FLAG_DEFEATED_SUDOWOODO
        || flagId == FLAG_DEFEATED_RED;
}

// When a peer shares a progress key/HM flag, grant the bag item if missing.
// RUNNING_SHOES / MATCH_CALL / POKENAV / HEADBUTT are flag-only unlocks.
static void Coop_GrantSharedProgressItem(u16 flagId)
{
    enum Item itemId = ITEM_NONE;

    switch (flagId)
    {
    case FLAG_RECEIVED_BIKE:
        itemId = ITEM_BICYCLE;
        break;
    case FLAG_RECEIVED_SQUIRTBOTTLE:
        itemId = ITEM_SQUIRT_BOTTLE;
        break;
    case FLAG_RECEIVED_HM_CUT:
        itemId = ITEM_HM_CUT;
        break;
    case FLAG_RECEIVED_HM_FLASH:
        itemId = ITEM_HM_FLASH;
        break;
    case FLAG_RECEIVED_HM_ROCK_SMASH:
        itemId = ITEM_HM_ROCK_SMASH;
        break;
    case FLAG_RECEIVED_HM_STRENGTH:
        itemId = ITEM_HM_STRENGTH;
        break;
    case FLAG_RECEIVED_HM_SURF:
        itemId = ITEM_HM_SURF;
        break;
    case FLAG_RECEIVED_HM_WHIRLPOOL:
        itemId = ITEM_HM_WHIRLPOOL;
        break;
    case FLAG_ITEM_ICEPATH1_HM_WATERFALL:
        itemId = ITEM_HM_WATERFALL;
        break;
    case FLAG_GOT_SUPER_ROD:
        itemId = ITEM_SUPER_ROD;
        break;
    default:
        return;
    }

    if (!CheckBagHasItem(itemId, 1))
        AddBagItem(itemId, 1);
}

static bool8 IsCoopSharedStoryVar(u16 varId)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sCoopSharedStoryVars); i++)
    {
        if (sCoopSharedStoryVars[i] == varId)
            return TRUE;
    }

    return FALSE;
}

// Presence-only co-op never completes the GBA SIO handshake (gameReady), but
// the Android bridge still sets peerLinkReady once both phones are linked.
// Queue story sync whenever transport is up and either path is ready.
static bool32 Coop_CanQueueStorySync(void)
{
    return gHnsCoopMailbox.transportConnected
        && (gHnsCoopMailbox.gameReady || gHnsCoopMailbox.peerLinkReady);
}

void Coop_QueueSharedVarChange(u16 varId, u16 value)
{
    u32 write;
    u32 read;

    // Story-state variables only advance; never propagate a reset.
    if (!value || value <= VarGet(varId) || sApplyingRemoteStoryVar
        || !IsCoopSharedStoryVar(varId) || !Coop_CanQueueStorySync())
        return;

    write = gHnsCoopMailbox.outgoingVarWrite;
    read = gHnsCoopMailbox.outgoingVarRead;
    if (write - read >= HNS_COOP_VAR_QUEUE_SIZE)
        return;

    gHnsCoopMailbox.outgoingVars[write % HNS_COOP_VAR_QUEUE_SIZE] = varId | ((u32)value << 16);
    gHnsCoopMailbox.outgoingVarWrite = write + 1;
}

void Coop_QueueSharedFlagChange(u16 flagId, bool8 value)
{
    u32 write;
    u32 read;

    // Shared story flags propagate both SET and CLEAR so peer hide/show state
    // stays mirrored (badges/trainers still only clear locally via scripts).
    if (sApplyingRemoteStoryFlag || !IsCoopSharedStoryFlag(flagId)
        || !Coop_CanQueueStorySync())
        return;

    write = gHnsCoopMailbox.outgoingFlagWrite;
    read = gHnsCoopMailbox.outgoingFlagRead;
    if (write - read >= HNS_COOP_FLAG_QUEUE_SIZE)
        return;

    gHnsCoopMailbox.outgoingFlags[write % HNS_COOP_FLAG_QUEUE_SIZE] =
        flagId | ((value ? 1u : 0u) << 16);
    gHnsCoopMailbox.outgoingFlagWrite = write + 1;
}

static void Coop_QueueInitialSharedFlags(void)
{
    u8 scanned = 0;

    // Bounded per-frame scan: hide Pokémon → NPC hides → adventure/quest
    // through Battle Frontier Sudowoodo → badges → trainers → Match Call
    // registration → extra named shared flags (Ice Path HM/boulders,
    // Whirlpool, electrodes, safari, sys unlocks, extended/Alola story).
    // IsCoopSharedStoryFlag filters personal gifts/statics out of each range.
    // Bidirectional hide ranges also queue CLEAR when locally unset.
    while (scanned < 16)
    {
        u16 flagId;
        u32 write;
        u32 read;
        bool8 fromExtraList = FALSE;

        if (!sCoopInitialHidePokemonStarted)
        {
            if (sCoopInitialFlagCursor < HNS_HIDE_POKEMON_START + HNS_HIDE_POKEMON_COUNT)
                flagId = sCoopInitialFlagCursor++;
            else
            {
                sCoopInitialHidePokemonStarted = TRUE;
                sCoopInitialFlagCursor = HNS_NPC_HIDE_START;
                continue;
            }
        }
        else if (!sCoopInitialNpcHideStarted)
        {
            if (sCoopInitialFlagCursor < HNS_NPC_HIDE_START + HNS_NPC_HIDE_COUNT)
                flagId = sCoopInitialFlagCursor++;
            else
            {
                sCoopInitialNpcHideStarted = TRUE;
                sCoopInitialFlagCursor = FLAG_ADVENTURE_STARTED;
                continue;
            }
        }
        else if (!sCoopInitialGymFlagsStarted)
        {
            if (sCoopInitialFlagCursor <= FLAG_DEFEATED_SUDOWOODO)
                flagId = sCoopInitialFlagCursor++;
            else
            {
                sCoopInitialGymFlagsStarted = TRUE;
                // Re-scan RECEIVED key-item/HM flags through passwords / Red
                // so late joins still pick up progress unlocks quickly.
                sCoopInitialFlagCursor = HNS_RECEIVED_START;
                continue;
            }
        }
        else if (!sCoopInitialBadgeFlagsStarted)
        {
            if (sCoopInitialFlagCursor <= FLAG_DEFEATED_RED)
                flagId = sCoopInitialFlagCursor++;
            else
            {
                sCoopInitialBadgeFlagsStarted = TRUE;
                sCoopInitialFlagCursor = FLAG_BADGE01_GET;
                continue;
            }
        }
        else if (!sCoopInitialTrainerFlagsStarted)
        {
            if (sCoopInitialFlagCursor <= FLAG_BADGE16_GET)
                flagId = sCoopInitialFlagCursor++;
            else
            {
                sCoopInitialTrainerFlagsStarted = TRUE;
                sCoopInitialFlagCursor = TRAINER_FLAGS_START;
                continue;
            }
        }
        else if (!sCoopInitialRegisteredFlagsStarted)
        {
            if (sCoopInitialFlagCursor <= TRAINER_FLAGS_END)
                flagId = sCoopInitialFlagCursor++;
            else
            {
                sCoopInitialRegisteredFlagsStarted = TRUE;
                sCoopInitialFlagCursor = TRAINER_REGISTERED_FLAGS_START;
                continue;
            }
        }
        else if (!sCoopInitialExtraFlagsStarted)
        {
            if (sCoopInitialFlagCursor <= FLAG_REGISTERED_REENA_HNS)
                flagId = sCoopInitialFlagCursor++;
            else
            {
                sCoopInitialExtraFlagsStarted = TRUE;
                sCoopInitialExtraFlagCursor = 0;
                continue;
            }
        }
        else
        {
            if (sCoopInitialExtraFlagCursor >= ARRAY_COUNT(sCoopInitialExtraSharedFlags))
                return;
            flagId = sCoopInitialExtraSharedFlags[sCoopInitialExtraFlagCursor++];
            fromExtraList = TRUE;
        }

        scanned++;
        if (!IsCoopSharedStoryFlag(flagId))
            continue;

        // Progress flags (badges/trainers/quest/received/registration): SET-only.
        // Bidirectional hide ranges also queue CLEAR when locally unset so
        // reconnect mirrors object show state without wiping badges.
        if (FlagGet(flagId))
        {
            write = gHnsCoopMailbox.outgoingFlagWrite;
            read = gHnsCoopMailbox.outgoingFlagRead;
            if (write - read >= HNS_COOP_FLAG_QUEUE_SIZE)
            {
                if (fromExtraList)
                    sCoopInitialExtraFlagCursor--;
                else
                    sCoopInitialFlagCursor--;
                return;
            }
            Coop_QueueSharedFlagChange(flagId, TRUE);
        }
        else if (IsCoopBidirectionalHideFlag(flagId))
        {
            write = gHnsCoopMailbox.outgoingFlagWrite;
            read = gHnsCoopMailbox.outgoingFlagRead;
            if (write - read >= HNS_COOP_FLAG_QUEUE_SIZE)
            {
                if (fromExtraList)
                    sCoopInitialExtraFlagCursor--;
                else
                    sCoopInitialFlagCursor--;
                return;
            }
            Coop_QueueSharedFlagChange(flagId, FALSE);
        }
    }
}

static void Coop_QueueInitialSharedVars(void)
{
    u8 scanned = 0;

    while (sCoopInitialVarCursor < ARRAY_COUNT(sCoopSharedStoryVars) && scanned < 8)
    {
        u16 varId = sCoopSharedStoryVars[sCoopInitialVarCursor++];
        u16 value = VarGet(varId);
        u32 write;
        u32 read;
        scanned++;

        if (!value)
            continue;

        write = gHnsCoopMailbox.outgoingVarWrite;
        read = gHnsCoopMailbox.outgoingVarRead;
        if (write - read >= HNS_COOP_VAR_QUEUE_SIZE)
        {
            sCoopInitialVarCursor--;
            return;
        }

        // Initial sync must send the existing nonzero value even though it
        // already equals the local variable; runtime updates use the monotonic
        // increase check in Coop_QueueSharedVarChange.
        gHnsCoopMailbox.outgoingVars[write % HNS_COOP_VAR_QUEUE_SIZE] = varId | ((u32)value << 16);
        gHnsCoopMailbox.outgoingVarWrite = write + 1;
    }
}

static void Coop_ProcessIncomingFlags(void)
{
    u32 read = gHnsCoopMailbox.incomingFlagRead;
    u32 write = gHnsCoopMailbox.incomingFlagWrite;

    while (read != write)
    {
        u32 packed = gHnsCoopMailbox.incomingFlags[read % HNS_COOP_FLAG_QUEUE_SIZE];
        u16 flagId = packed & 0xFFFF;
        bool8 setValue = ((packed >> 16) & 1) != 0;

        // Ignore non-shared flags so older peers that still broadcast
        // legendary/static defeat or catch flags cannot apply them locally.
        if (IsCoopSharedStoryFlag(flagId))
        {
            sApplyingRemoteStoryFlag = TRUE;
            if (setValue)
            {
                bool8 wasSet = FlagGet(flagId);

                FlagSet(flagId);
                sApplyingRemoteStoryFlag = FALSE;

                // Newly shared progress key/HM: grant the bag item if missing.
                if (!wasSet)
                    Coop_GrantSharedProgressItem(flagId);
            }
            else
            {
                FlagClear(flagId);
                sApplyingRemoteStoryFlag = FALSE;
            }
        }

        read++;
    }

    gHnsCoopMailbox.incomingFlagRead = read;
}

static void Coop_ProcessIncomingVars(void)
{
    u32 read = gHnsCoopMailbox.incomingVarRead;
    u32 write = gHnsCoopMailbox.incomingVarWrite;

    while (read != write)
    {
        u32 packed = gHnsCoopMailbox.incomingVars[read % HNS_COOP_VAR_QUEUE_SIZE];
        u16 varId = packed & 0xFFFF;
        u16 value = packed >> 16;

        if (IsCoopSharedStoryVar(varId) && value > VarGet(varId))
        {
            sApplyingRemoteStoryVar = TRUE;
            VarSet(varId, value);
            sApplyingRemoteStoryVar = FALSE;
        }

        read++;
    }

    gHnsCoopMailbox.incomingVarRead = read;
}

static void Coop_ResetStorySyncQueues(void)
{
    gHnsCoopMailbox.outgoingFlagWrite = 0;
    gHnsCoopMailbox.outgoingFlagRead = 0;
    gHnsCoopMailbox.incomingFlagWrite = 0;
    gHnsCoopMailbox.incomingFlagRead = 0;
    gHnsCoopMailbox.outgoingVarWrite = 0;
    gHnsCoopMailbox.outgoingVarRead = 0;
    gHnsCoopMailbox.incomingVarWrite = 0;
    gHnsCoopMailbox.incomingVarRead = 0;
    gHnsCoopMailbox.localStorySyncReady = 0;
    gHnsCoopMailbox.remoteStorySyncReady = 0;
    gHnsCoopMailbox.localMapId = 0;
    gHnsCoopMailbox.remoteMapId = 0;
    gHnsCoopMailbox.remoteMapReady = 0;
    gHnsCoopMailbox.localSioActive = 0;
    gHnsCoopMailbox.remoteSioActive = 0;
    gHnsCoopMailbox.remoteX = 0;
    gHnsCoopMailbox.remoteY = 0;
    gHnsCoopMailbox.localFollower = 0;
    gHnsCoopMailbox.remoteFollower = 0;
    gHnsCoopMailbox.localPresenceResync = 0;
    sCoopInitialFlagCursor = HNS_HIDE_POKEMON_START;
    sCoopInitialVarCursor = 0;
    sCoopInitialHidePokemonStarted = FALSE;
    sCoopInitialNpcHideStarted = FALSE;
    sCoopInitialGymFlagsStarted = FALSE;
    sCoopInitialBadgeFlagsStarted = FALSE;
    sCoopInitialTrainerFlagsStarted = FALSE;
    sCoopInitialRegisteredFlagsStarted = FALSE;
    sCoopInitialExtraFlagsStarted = FALSE;
    sCoopInitialExtraFlagCursor = 0;
    sCoopInitialSyncStarted = FALSE;
    sCoopStorySyncReady = FALSE;
    sCoopPrevPeerLinkReady = FALSE;
    sCoopStorySyncQuietFrames = 0;
    sCoopLastIncomingFlagWrite = 0;
    sCoopLastIncomingVarWrite = 0;
}

static void Coop_BeginInitialSharedFlagSync(void)
{
    sCoopInitialFlagCursor = HNS_HIDE_POKEMON_START;
    sCoopInitialVarCursor = 0;
    sCoopInitialHidePokemonStarted = FALSE;
    sCoopInitialNpcHideStarted = FALSE;
    sCoopInitialGymFlagsStarted = FALSE;
    sCoopInitialBadgeFlagsStarted = FALSE;
    sCoopInitialTrainerFlagsStarted = FALSE;
    sCoopInitialRegisteredFlagsStarted = FALSE;
    sCoopInitialExtraFlagsStarted = FALSE;
    sCoopInitialExtraFlagCursor = 0;
    sCoopInitialSyncStarted = TRUE;
    sCoopStorySyncReady = FALSE;
    gHnsCoopMailbox.localStorySyncReady = 0;
    gHnsCoopMailbox.remoteStorySyncReady = 0;
    sCoopStorySyncQuietFrames = 0;
    sCoopLastIncomingFlagWrite = gHnsCoopMailbox.incomingFlagWrite;
    sCoopLastIncomingVarWrite = gHnsCoopMailbox.incomingVarWrite;
}

static void Coop_UpdateStorySyncReadiness(void)
{
    bool32 initialScanComplete =
        sCoopInitialExtraFlagsStarted
        && sCoopInitialExtraFlagCursor >= ARRAY_COUNT(sCoopInitialExtraSharedFlags)
        && sCoopInitialVarCursor >= ARRAY_COUNT(sCoopSharedStoryVars);
    u32 incomingFlagWrite = gHnsCoopMailbox.incomingFlagWrite;
    u32 incomingVarWrite = gHnsCoopMailbox.incomingVarWrite;
    bool32 queuesEmpty =
        gHnsCoopMailbox.outgoingFlagRead == gHnsCoopMailbox.outgoingFlagWrite
        && gHnsCoopMailbox.incomingFlagRead == incomingFlagWrite
        && gHnsCoopMailbox.outgoingVarRead == gHnsCoopMailbox.outgoingVarWrite
        && gHnsCoopMailbox.incomingVarRead == incomingVarWrite;

    if (sCoopStorySyncReady)
        return;

    if (incomingFlagWrite != sCoopLastIncomingFlagWrite
        || incomingVarWrite != sCoopLastIncomingVarWrite)
    {
        sCoopLastIncomingFlagWrite = incomingFlagWrite;
        sCoopLastIncomingVarWrite = incomingVarWrite;
        sCoopStorySyncQuietFrames = 0;
        return;
    }

    if (!initialScanComplete || !queuesEmpty)
    {
        sCoopStorySyncQuietFrames = 0;
        return;
    }

    // Require a full two seconds with no new peer updates after both local
    // queues drain, so the slower console can finish its initial state transfer.
    if (++sCoopStorySyncQuietFrames >= 120)
    {
        sCoopStorySyncReady = TRUE;
        gHnsCoopMailbox.localStorySyncReady = 1;
    }
}

// Run the same initial story queue/process path used by the full-SIO link
// callback, but also from presence-only overworld when peerLinkReady is set.
static void Coop_ProcessStorySyncFrame(void)
{
    bool8 peerReady = gHnsCoopMailbox.peerLinkReady != 0;

    // Presence expiry / soft return: peerLinkReady can drop and rise again
    // without a full transport disconnect. Re-queue initial shared flags/vars
    // on the rising edge, and force WORLD_READY to re-handshake on the fall.
    if (peerReady && !sCoopPrevPeerLinkReady)
    {
        if (gHnsCoopMailbox.transportConnected)
            Coop_BeginInitialSharedFlagSync();
    }
    else if (!peerReady && sCoopPrevPeerLinkReady)
    {
        gHnsCoopMailbox.remoteStorySyncReady = 0;
        gHnsCoopMailbox.localStorySyncReady = 0;
        sCoopStorySyncReady = FALSE;
        sCoopStorySyncQuietFrames = 0;
    }
    else if (peerReady && !sCoopInitialSyncStarted)
    {
        Coop_BeginInitialSharedFlagSync();
    }

    sCoopPrevPeerLinkReady = peerReady;

    if (!Coop_CanQueueStorySync())
        return;

    Coop_ProcessIncomingFlags();
    Coop_ProcessIncomingVars();
    Coop_QueueInitialSharedFlags();
    Coop_QueueInitialSharedVars();
    Coop_UpdateStorySyncReadiness();
}

// The host publishes the ENTIRE ChallengeSettings struct (minus the local
// player's per-save Nuzlocke encounter table) so every challenge, mode, and
// randomizer option matches on both consoles. Partial bit-packing drifted
// out of sync with new options; a versioned whole-struct snapshot cannot.
static void Coop_ApplySharedSettings(void)
{
    struct ChallengeSettings *cs = &gSaveBlock3Ptr->challengeSettings;
    u32 sequence = gHnsCoopMailbox.remoteConfigReady;
    u8 savedEncounters[16];
    u8 nuzlockeFlags;
    u32 i;

    if (!sequence || sequence == sCoopLastRemoteConfigReady)
        return;

    for (i = 0; i < ARRAY_COUNT(savedEncounters); i++)
        savedEncounters[i] = cs->nuzlockeEncounterFlags[i];
    for (i = 0; i < sizeof(struct ChallengeSettings); i++)
        ((u8 *)cs)[i] = gHnsCoopMailbox.remoteConfigBlob[i];
    for (i = 0; i < ARRAY_COUNT(savedEncounters); i++)
        cs->nuzlockeEncounterFlags[i] = savedEncounters[i];

    nuzlockeFlags = gHnsCoopMailbox.remoteConfigBlob[HNS_COOP_CONFIG_BLOB_SIZE - 4];
    if (nuzlockeFlags & 1)
        FlagSet(FLAG_START_NUZLOCKE);
    else
        FlagClear(FLAG_START_NUZLOCKE);
    if (nuzlockeFlags & 2)
        FlagSet(FLAG_END_NUZLOCKE);
    else
        FlagClear(FLAG_END_NUZLOCKE);

#if RANDOMIZER_AVAILABLE
    Randomizer_SetCoopSeed(gHnsCoopMailbox.remoteRandomizerSeed, TRUE);
#endif
    sCoopLastRemoteConfigReady = sequence;
}

static void Coop_UpdateSharedSettings(void)
{
    if (gHnsCoopMailbox.hostRole)
    {
        // Publish the whole ruleset as one blob so any challenge, mode, or
        // randomizer option added in the future syncs automatically.
        u8 packed[HNS_COOP_CONFIG_BLOB_SIZE];
        u32 seed;
        u32 i;
#if RANDOMIZER_AVAILABLE
        // Force both consoles, including CHAOS mode, through the same
        // deterministic seed while this host owns the co-op session.
        Randomizer_SetCoopSeed(0, FALSE);
        seed = GetRandomizerSeed();
        Randomizer_SetCoopSeed(seed, TRUE);
#else
        seed = 0;
#endif
        for (i = 0; i < HNS_COOP_CONFIG_BLOB_SIZE; i++)
            packed[i] = 0;
        for (i = 0; i < sizeof(struct ChallengeSettings); i++)
            packed[i] = ((const u8 *)&gSaveBlock3Ptr->challengeSettings)[i];
        if (FlagGet(FLAG_START_NUZLOCKE))
            packed[HNS_COOP_CONFIG_BLOB_SIZE - 4] |= 1;
        if (FlagGet(FLAG_END_NUZLOCKE))
            packed[HNS_COOP_CONFIG_BLOB_SIZE - 4] |= 2;

        bool8 changed = !gHnsCoopMailbox.localConfigReady
            || gHnsCoopMailbox.localRandomizerSeed != seed;

        if (!changed)
        {
            for (i = 0; i < HNS_COOP_CONFIG_BLOB_SIZE; i++)
            {
                if (gHnsCoopMailbox.localConfigBlob[i] != packed[i])
                {
                    changed = TRUE;
                    break;
                }
            }
        }
        if (changed)
        {
            for (i = 0; i < HNS_COOP_CONFIG_BLOB_SIZE; i++)
                gHnsCoopMailbox.localConfigBlob[i] = packed[i];
            gHnsCoopMailbox.localRandomizerSeed = seed;
            gHnsCoopMailbox.localConfigReady++;
            if (!gHnsCoopMailbox.localConfigReady)
                gHnsCoopMailbox.localConfigReady = 1;
        }
    }
    else
    {
        Coop_ApplySharedSettings();
    }
}

static bool32 UpdateCoopLink(void)
{
    if (gHnsCoopMailbox.magic != HNS_COOP_MAILBOX_MAGIC
        || gHnsCoopMailbox.version != HNS_COOP_MAILBOX_VERSION
        || gHnsCoopMailbox.size != sizeof(struct HnsCoopMailbox))
    {
        // Invalidate an old mailbox before changing its layout. This matters
        // when an emulator keeps EWRAM alive across a ROM reset or hot reload.
        gHnsCoopMailbox.magic = 0;
        gHnsCoopMailbox.version = HNS_COOP_MAILBOX_VERSION;
        gHnsCoopMailbox.size = sizeof(struct HnsCoopMailbox);
        gHnsCoopMailbox.transportConnected = 0;
        gHnsCoopMailbox.gameReady = 0;
        gHnsCoopMailbox.localConfigReady = 0;
        gHnsCoopMailbox.remoteConfigReady = 0;
        sCoopLastRemoteConfigReady = 0;
        sCoopLinkStarted = FALSE;
        Coop_ResetStorySyncQueues();
        // Publish the magic last so the Android bridge never sees a partial header.
        gHnsCoopMailbox.magic = HNS_COOP_MAILBOX_MAGIC;
    }

    if (gHnsCoopMailbox.transportConnected)
    {
        if (!sCoopLinkStarted)
        {
            Coop_ResetStorySyncQueues();
            gHnsCoopMailbox.localSioActive = 1;
            gLinkType = LINKTYPE_COOP;
            OpenLink();
            sCoopLinkStarted = TRUE;
        }

        if (sCoopLinkStarted && gReceivedRemoteLinkPlayers && !IsOverworldLinkActive())
        {
            gFieldLinkPlayerCount = GetLinkPlayerCount_2();
            gLocalLinkPlayerId = GetMultiplayerId();
            SaveLinkPlayers(gFieldLinkPlayerCount);
            gHnsCoopMailbox.gameReady = TRUE;
            Coop_BeginInitialSharedFlagSync();
            CB2_ReturnToFieldFromMultiplayer();
            return TRUE;
        }
    }
    else if (sCoopLinkStarted)
    {
        sCoopLastRemoteConfigReady = 0;
#if RANDOMIZER_AVAILABLE
        Randomizer_SetCoopSeed(0, FALSE);
#endif
        CloseLink();
        ResetAllMultiplayerState();
        Coop_ResetStorySyncQueues();
        sCoopLinkStarted = FALSE;
        gHnsCoopMailbox.gameReady = FALSE;

        if (IsOverworldLinkActive())
        {
            SetMainCallback1(CB1_Overworld);
            CB2_ReturnToField();
            return TRUE;
        }
    }

    return FALSE;
}

#else

// Heart & Soul co-op mailbox support is unavailable in the base games.
void Coop_QueueSharedVarChange(u16 varId, u16 value) { (void)varId; (void)value; }
void Coop_QueueSharedFlagChange(u16 flagId, bool8 value) { (void)flagId; (void)value; }
static bool32 UpdateCoopLink(void) { return FALSE; }

#endif // IS_HNS

static void SetKeyInterceptCallback(u16 (*func)(u32));
static void SetFieldVBlankCallback(void);
static void FieldClearVBlankHBlankCallbacks(void);
static void TransitionMapMusic(void);
static u8 GetAdjustedInitialTransitionFlags(struct InitialPlayerAvatarState *playerStruct, u16 metatileBehavior, enum MapType mapType);
static enum Direction GetAdjustedInitialDirection(struct InitialPlayerAvatarState *playerStruct, u8 transitionFlags, u16 metatileBehavior, enum MapType mapType);
static u16 GetCenterScreenMetatileBehavior(void);
static bool32 SetUpScrollSceneForCredits(u8 *state, u8 unused);
static bool8 MapLdr_Credits(void);
static void CameraCB_CreditsPan(struct CameraObject *camera);
static void Task_OvwldCredits_FadeOut(u8 taskId);
static void Task_OvwldCredits_WaitFade(u8 taskId);

static void *sUnusedOverworldCallback;
static u8 sPlayerLinkStates[MAX_LINK_PLAYERS];
// This callback is called with a player's key code. It then returns an
// adjusted key code, effectively intercepting the input before anything
// can process it.
static u16 (*sPlayerKeyInterceptCallback)(u32);
static bool8 sReceivingFromLink;
static u8 sRfuKeepAliveTimer;

COMMON_DATA u16 *gOverworldTilemapBuffer_Bg2 = NULL;
COMMON_DATA u16 *gOverworldTilemapBuffer_Bg1 = NULL;
COMMON_DATA u16 *gOverworldTilemapBuffer_Bg3 = NULL;
COMMON_DATA u16 gHeldKeyCodeToSend = 0;
COMMON_DATA void (*gFieldCallback)(void) = NULL;
COMMON_DATA bool8 (*gFieldCallback2)(void) = NULL;
COMMON_DATA u8 gLocalLinkPlayerId = 0; // This is our player id in a multiplayer mode.
COMMON_DATA u8 gFieldLinkPlayerCount = 0;

u8 gTimeOfDay;
struct TimeBlendSettings gTimeBlend;
s16 gTimeUpdateCounter; // playTimeVBlanks will eventually overflow, so this is used to update TOD

// EWRAM vars
EWRAM_DATA static u8 sObjectEventLoadFlag = 0;
EWRAM_DATA struct WarpData gLastUsedWarp = {0};
EWRAM_DATA static struct WarpData sWarpDestination = {0};  // new warp position
EWRAM_DATA static struct WarpData sFixedDiveWarp = {0};
EWRAM_DATA static struct WarpData sFixedHoleWarp = {0};
EWRAM_DATA static mapsec_u16_t sLastMapSectionId = 0;
EWRAM_DATA static struct InitialPlayerAvatarState sInitialPlayerAvatarState = {0};
EWRAM_DATA static u16 sAmbientCrySpecies = 0;
EWRAM_DATA static bool8 sIsAmbientCryWaterMon = FALSE;
EWRAM_DATA static u8 sHoursOverride = 0; // used to override apparent time of day hours
EWRAM_DATA struct LinkPlayerObjectEvent gLinkPlayerObjectEvents[4] = {0};
EWRAM_DATA bool8 gExitStairsMovementDisabled = FALSE;
EWRAM_DATA bool8 gDisableMapMusicChangeOnMapLoad = MUSIC_DISABLE_OFF;
static EWRAM_DATA const struct CreditsOverworldCmd *sCreditsOverworld_Script = NULL;
static EWRAM_DATA s16 sCreditsOverworld_CmdLength = 0;
static EWRAM_DATA s16 sCreditsOverworld_CmdIndex = 0;

static const struct WarpData sDummyWarpData =
{
    .mapGroup = MAP_GROUP(MAP_UNDEFINED),
    .mapNum = MAP_NUM(MAP_UNDEFINED),
    .warpId = WARP_ID_NONE,
    .x = -1,
    .y = -1,
};

static const u32 sUnusedData[] =
{
    1200, 3600, 1200, 2400, 50, 80, -44, 44
};

const struct UCoords32 gDirectionToVectors[] =
{
    [DIR_NONE] =
    {
        .x =  0,
        .y =  0,
    },
    [DIR_SOUTH] =
    {
        .x =  0,
        .y =  1,
    },
    [DIR_NORTH] =
    {
        .x =  0,
        .y = -1,
    },
    [DIR_WEST] =
    {
        .x = -1,
        .y =  0,
    },
    [DIR_EAST] =
    {
        .x =  1,
        .y =  0,
    },
    [DIR_SOUTHWEST] =
    {
        .x = -1,
        .y =  1,
    },
    [DIR_SOUTHEAST] =
    {
        .x =  1,
        .y =  1,
    },
    [DIR_NORTHWEST] =
    {
        .x = -1,
        .y = -1,
    },
    [DIR_NORTHEAST] =
    {
        .x =  1,
        .y = -1,
    },
};

static const struct BgTemplate sOverworldBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static const struct ScanlineEffectParams sFlashEffectParams =
{
    &REG_WIN0H,
    ((DMA_ENABLE | DMA_START_HBLANK | DMA_REPEAT | DMA_DEST_RELOAD) << 16) | 1,
    1,
    0,
};

static u8 MovementEventModeCB_Normal(struct LinkPlayerObjectEvent *, struct ObjectEvent *, enum Direction);
static u8 MovementEventModeCB_Ignored(struct LinkPlayerObjectEvent *, struct ObjectEvent *, enum Direction);
static u8 MovementEventModeCB_Scripted(struct LinkPlayerObjectEvent *, struct ObjectEvent *, enum Direction);

static u8 (*const sLinkPlayerMovementModes[])(struct LinkPlayerObjectEvent *, struct ObjectEvent *, enum Direction) =
{
    [MOVEMENT_MODE_FREE]     = MovementEventModeCB_Normal,
    [MOVEMENT_MODE_FROZEN]   = MovementEventModeCB_Ignored,
    [MOVEMENT_MODE_SCRIPTED] = MovementEventModeCB_Scripted,
};

static u8 FacingHandler_DoNothing(struct LinkPlayerObjectEvent *, struct ObjectEvent *, enum Direction);
static u8 FacingHandler_DpadMovement(struct LinkPlayerObjectEvent *, struct ObjectEvent *, enum Direction);
static u8 FacingHandler_ForcedFacingChange(struct LinkPlayerObjectEvent *, struct ObjectEvent *, enum Direction);

// These handlers return TRUE if the movement was scripted and successful, and FALSE otherwise.
static bool8 (*const sLinkPlayerFacingHandlers[])(struct LinkPlayerObjectEvent *, struct ObjectEvent *, enum Direction) =
{
    FacingHandler_DoNothing,
    FacingHandler_DpadMovement,
    FacingHandler_DpadMovement,
    FacingHandler_DpadMovement,
    FacingHandler_DpadMovement,
    FacingHandler_DoNothing,
    FacingHandler_DoNothing,
    FacingHandler_ForcedFacingChange,
    FacingHandler_ForcedFacingChange,
    FacingHandler_ForcedFacingChange,
    FacingHandler_ForcedFacingChange,
};

static void MovementStatusHandler_EnterFreeMode(struct LinkPlayerObjectEvent *, struct ObjectEvent *);
static void MovementStatusHandler_TryAdvanceScript(struct LinkPlayerObjectEvent *, struct ObjectEvent *);

// These handlers are run after an attempted movement.
static void (*const sMovementStatusHandler[])(struct LinkPlayerObjectEvent *, struct ObjectEvent *) =
{
    // FALSE:
    MovementStatusHandler_EnterFreeMode,
    // TRUE:
    MovementStatusHandler_TryAdvanceScript,
};

// code
void DoWhiteOut(void)
{
    if (IsNuzlockeActive() || IsNuzlockeEasyActive())
    {
        if (GetFirstAliveBoxPokemon() == IN_BOX_COUNT * TOTAL_BOXES_COUNT)
        {
            ZeroPlayerPartyMons();
            CreateMon(&gPlayerParty[0], SPECIES_RATTATA, 1, 0, OTID_STRUCT_PLAYER_ID);
            CalculateMonStats(&gPlayerParty[0]);
            gPlayerPartyCount = 1;
        }
        else
        {
            MoveFirstBoxPokemonToParty();
        }
    }
    RunScriptImmediately(EventScript_WhiteOut);
    HealPlayerParty();
    Overworld_ResetStateAfterWhiteOut();
    SetWarpDestinationToLastHealLocation();
    WarpIntoMap();
}

void Overworld_ResetStateAfterFly(void)
{
    ResetInitialPlayerAvatarState();
    FlagClear(FLAG_SYS_CYCLING_ROAD);
    FlagClear(FLAG_SYS_CRUISE_MODE);
    FlagClear(FLAG_SYS_SAFARI_MODE);
#if IS_FRLG
    VarSet(VAR_MAP_SCENE_FUCHSIA_CITY_SAFARI_ZONE_ENTRANCE, 0);
#endif
    FlagClear(FLAG_SYS_USE_STRENGTH);
    FlagClear(FLAG_SYS_USE_FLASH);
}

void Overworld_ResetStateAfterTeleport(void)
{
    ResetInitialPlayerAvatarState();
    FlagClear(FLAG_SYS_CYCLING_ROAD);
    FlagClear(FLAG_SYS_CRUISE_MODE);
    FlagClear(FLAG_SYS_SAFARI_MODE);
#if IS_FRLG
    VarSet(VAR_MAP_SCENE_FUCHSIA_CITY_SAFARI_ZONE_ENTRANCE, 0);
#endif
    FlagClear(FLAG_SYS_USE_STRENGTH);
    FlagClear(FLAG_SYS_USE_FLASH);
    RunScriptImmediately(EventScript_ResetMrBriney);
}

void Overworld_ResetStateAfterDigEscRope(void)
{
    ResetInitialPlayerAvatarState();
    FlagClear(FLAG_SYS_CYCLING_ROAD);
    FlagClear(FLAG_SYS_CRUISE_MODE);
    FlagClear(FLAG_SYS_SAFARI_MODE);
#if IS_FRLG
    VarSet(VAR_MAP_SCENE_FUCHSIA_CITY_SAFARI_ZONE_ENTRANCE, 0);
#endif
    FlagClear(FLAG_SYS_USE_STRENGTH);
    FlagClear(FLAG_SYS_USE_FLASH);
}

#if B_RESET_FLAGS_VARS_AFTER_WHITEOUT == TRUE
void Overworld_ResetBattleFlagsAndVars(void)
{
    ResetStartingStatuses();

    #if B_VAR_WILD_AI_FLAGS != 0
        VarSet(B_VAR_WILD_AI_FLAGS,0);
    #endif

    #if B_VAR_NO_BAG_USE != 0
        VarSet(B_VAR_NO_BAG_USE, 0);
    #endif

    FlagClear(B_FLAG_INVERSE_BATTLE);
    FlagClear(B_FLAG_FORCE_DOUBLE_WILD);
    FlagClear(B_SMART_WILD_AI_FLAG);
    FlagClear(B_FLAG_NO_CATCHING);
    FlagClear(B_FLAG_NO_RUNNING);
    FlagClear(B_FLAG_DYNAMAX_BATTLE);
    FlagClear(B_FLAG_SKY_BATTLE);
    FlagClear(B_FLAG_NO_WHITEOUT);
}
#endif

static void Overworld_ResetStateAfterWhiteOut(void)
{
    ResetInitialPlayerAvatarState();
    FlagClear(FLAG_SYS_CYCLING_ROAD);
    FlagClear(FLAG_SYS_CRUISE_MODE);
    FlagClear(FLAG_SYS_SAFARI_MODE);
#if IS_FRLG
    VarSet(VAR_MAP_SCENE_FUCHSIA_CITY_SAFARI_ZONE_ENTRANCE, 0);
#endif
    FlagClear(FLAG_SYS_USE_STRENGTH);
    FlagClear(FLAG_SYS_USE_FLASH);
    if (B_RESET_FLAGS_VARS_AFTER_WHITEOUT == TRUE)
        Overworld_ResetBattleFlagsAndVars();
    // If you were defeated by Kyogre/Groudon and the step counter has
    // maxed out, end the abnormal weather.
    if (VarGet(VAR_SHOULD_END_ABNORMAL_WEATHER) == 1)
    {
        VarSet(VAR_SHOULD_END_ABNORMAL_WEATHER, 0);
        VarSet(VAR_ABNORMAL_WEATHER_LOCATION, ABNORMAL_WEATHER_NONE);
    }
    FollowerNPC_TryRemoveFollowerOnWhiteOut();
}

static void UpdateMiscOverworldStates(void)
{
    FlagClear(FLAG_SYS_SAFARI_MODE);
#if IS_FRLG
    VarSet(VAR_MAP_SCENE_FUCHSIA_CITY_SAFARI_ZONE_ENTRANCE, 0);
#endif
    ChooseAmbientCrySpecies();
    ResetCyclingRoadChallengeData();
    UpdateLocationHistoryForRoamer();
    MoveAllRoamersToOtherLocationSets();
}

void ResetGameStats(void)
{
    s32 i;

    for (i = 0; i < NUM_GAME_STATS; i++)
        SetGameStat(i, 0);
}

void IncrementGameStat(u8 index)
{
    if (index < NUM_USED_GAME_STATS)
    {
        u32 statVal = GetGameStat(index);
        if (statVal < 0xFFFFFF)
            statVal++;
        else
            statVal = 0xFFFFFF;

        SetGameStat(index, statVal);
    }
}

u32 GetGameStat(u8 index)
{
    if (index >= NUM_USED_GAME_STATS)
        return 0;

    return gSaveBlock1Ptr->gameStats[index] ^ gSaveBlock2Ptr->encryptionKey;
}

void SetGameStat(u8 index, u32 value)
{
    if (index < NUM_USED_GAME_STATS)
        gSaveBlock1Ptr->gameStats[index] = value ^ gSaveBlock2Ptr->encryptionKey;
}

void ApplyNewEncryptionKeyToGameStats(u32 newKey)
{
    u8 i;

    for (i = 0; i < NUM_GAME_STATS; i++)
        ApplyNewEncryptionKeyToWord(&gSaveBlock1Ptr->gameStats[i], newKey);
}

void LoadObjEventTemplatesFromHeader(void)
{
    // Clear map object templates
    CpuFill32(0, gSaveBlock1Ptr->objectEventTemplates, sizeof(gSaveBlock1Ptr->objectEventTemplates));

    for (u32 i = 0; i < gMapHeader.events->objectEventCount; i++)
    {
        if (gMapHeader.events->objectEvents[i].kind == OBJ_KIND_CLONE)
        {
            // load target object from the connecting map
            u8 localId = gMapHeader.events->objectEvents[i].targetLocalId;
            u8 mapNum = gMapHeader.events->objectEvents[i].targetMapNum;
            u8 mapGroup = gMapHeader.events->objectEvents[i].targetMapGroup;
            const struct MapHeader *connectionMap = Overworld_GetMapHeaderByGroupAndId(mapGroup, mapNum);

            gSaveBlock1Ptr->objectEventTemplates[i] = connectionMap->events->objectEvents[localId - 1];
            gSaveBlock1Ptr->objectEventTemplates[i].localId = gMapHeader.events->objectEvents[i].localId;
            gSaveBlock1Ptr->objectEventTemplates[i].x = gMapHeader.events->objectEvents[i].x;
            gSaveBlock1Ptr->objectEventTemplates[i].y = gMapHeader.events->objectEvents[i].y;
            gSaveBlock1Ptr->objectEventTemplates[i].targetLocalId = localId;
            gSaveBlock1Ptr->objectEventTemplates[i].targetMapNum = mapNum;
            gSaveBlock1Ptr->objectEventTemplates[i].targetMapGroup = mapGroup;
            gSaveBlock1Ptr->objectEventTemplates[i].kind = OBJ_KIND_CLONE;
        }
        else
        {
            gSaveBlock1Ptr->objectEventTemplates[i] = gMapHeader.events->objectEvents[i];
        }
    }
}

void LoadSaveblockObjEventScripts(void)
{
    const struct ObjectEventTemplate *mapHeaderObjTemplates = gMapHeader.events->objectEvents;
    struct ObjectEventTemplate *savObjTemplates = gSaveBlock1Ptr->objectEventTemplates;
    s32 i;

    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
        savObjTemplates[i].script = mapHeaderObjTemplates[i].script;
}

static struct ObjectEventTemplate *GetObjectEventTemplate(u8 localId)
{
    for (u32 i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
    {
        if (gSaveBlock1Ptr->objectEventTemplates[i].localId == localId)
            return &gSaveBlock1Ptr->objectEventTemplates[i];
    }

    errorf("no object event template for localId %d", localId);
    return NULL;
}

void SetObjEventTemplateCoords(u8 localId, s16 x, s16 y)
{
    struct ObjectEventTemplate *objectEventTemplate = GetObjectEventTemplate(localId);
    if (objectEventTemplate)
    {
        objectEventTemplate->x = x;
        objectEventTemplate->y = y;
    }
}

void SetObjEventTemplateMovementType(u8 localId, u8 movementType)
{
    struct ObjectEventTemplate *objectEventTemplate = GetObjectEventTemplate(localId);
    if (objectEventTemplate)
        objectEventTemplate->movementType = movementType;
}

static void InitMapView(void)
{
    ResetFieldCamera();
    CopyMapTilesetsToVram(gMapHeader.mapLayout);
    LoadMapTilesetPalettes(gMapHeader.mapLayout);
    DrawWholeMapView();
    InitTilesetAnimations();
}

const struct MapLayout *GetMapLayout(u16 mapLayoutId)
{
    return gMapLayouts[mapLayoutId - 1];
}

void ApplyCurrentWarp(void)
{
    gLastUsedWarp = gSaveBlock1Ptr->location;
    gSaveBlock1Ptr->location = sWarpDestination;
    sFixedDiveWarp = sDummyWarpData;
    sFixedHoleWarp = sDummyWarpData;
}

static void ClearDiveAndHoleWarps(void)
{
    sFixedDiveWarp = sDummyWarpData;
    sFixedHoleWarp = sDummyWarpData;
}

static void SetWarpData(struct WarpData *warp, s8 mapGroup, s8 mapNum, s8 warpId, s8 x, s8 y)
{
    warp->mapGroup = mapGroup;
    warp->mapNum = mapNum;
    warp->warpId = warpId;
    warp->x = x;
    warp->y = y;
}

static bool32 IsDummyWarp(struct WarpData *warp)
{
    if (warp->mapGroup != (s8)MAP_GROUP(MAP_UNDEFINED))
        return FALSE;
    else if (warp->mapNum != (s8)MAP_NUM(MAP_UNDEFINED))
        return FALSE;
    else if (warp->warpId != WARP_ID_NONE)
        return FALSE;
    else if (warp->x != -1)
        return FALSE;
    else if (warp->y != -1)
        return FALSE;
    else
        return TRUE;
}

struct MapHeader const *const Overworld_GetMapHeaderByGroupAndId(u16 mapGroup, u16 mapNum)
{
    return gMapGroups[mapGroup][mapNum];
}

struct MapHeader const *const GetDestinationWarpMapHeader(void)
{
    return Overworld_GetMapHeaderByGroupAndId(sWarpDestination.mapGroup, sWarpDestination.mapNum);
}

static void LoadCurrentMapData(void)
{
    sLastMapSectionId = gMapHeader.regionMapSectionId;
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
    gSaveBlock1Ptr->mapLayoutId = gMapHeader.mapLayoutId;
    gMapHeader.mapLayout = GetMapLayout(gMapHeader.mapLayoutId);
}

static void LoadSaveblockMapHeader(void)
{
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
    gMapHeader.mapLayout = GetMapLayout(gSaveBlock1Ptr->mapLayoutId);
}

static void SetPlayerCoordsFromWarp(void)
{
    if (gSaveBlock1Ptr->location.warpId >= 0 && gSaveBlock1Ptr->location.warpId < gMapHeader.events->warpCount)
    {
        // warpId is a valid warp for this map, use the coords of that warp.
        gSaveBlock1Ptr->pos.x = gMapHeader.events->warps[gSaveBlock1Ptr->location.warpId].x;
        gSaveBlock1Ptr->pos.y = gMapHeader.events->warps[gSaveBlock1Ptr->location.warpId].y;
    }
    else if (gSaveBlock1Ptr->location.x >= 0 && gSaveBlock1Ptr->location.y >= 0)
    {
        // Invalid warpId given. The given coords are valid, use those instead.
        // WARP_ID_NONE is used to reach this intentionally.
        gSaveBlock1Ptr->pos.x = gSaveBlock1Ptr->location.x;
        gSaveBlock1Ptr->pos.y = gSaveBlock1Ptr->location.y;
    }
    else
    {
        // Invalid warpId and coords given. Put player in center of map.
        gSaveBlock1Ptr->pos.x = gMapHeader.mapLayout->width / 2;
        gSaveBlock1Ptr->pos.y = gMapHeader.mapLayout->height / 2;
    }
}

#if IS_HNS
static void Coop_MarkMapTransition(void)
{
    // Stop advertising the old map while map data and object events are being
    // replaced. The peer will treat this as "not currently in the overworld"
    // until CB1_OverworldLink publishes the new map identity after loading.
    if (gHnsCoopMailbox.magic == HNS_COOP_MAILBOX_MAGIC)
    {
        gHnsCoopMailbox.localSioActive = 0;
        gHnsCoopMailbox.localMapId = 0;
    }
}
#endif

void WarpIntoMap(void)
{
#if IS_HNS
    Coop_MarkMapTransition();
#endif
    ApplyCurrentWarp();
    LoadCurrentMapData();
    SetPlayerCoordsFromWarp();
}

void SetWarpDestination(s8 mapGroup, s8 mapNum, s8 warpId, s8 x, s8 y)
{
    SetWarpData(&sWarpDestination, mapGroup, mapNum, warpId, x, y);
}

void SetWarpDestinationToMapWarp(s8 mapGroup, s8 mapNum, s8 warpId)
{
    SetWarpDestination(mapGroup, mapNum, warpId, -1, -1);
}

void SetDynamicWarp(s32 unused, s8 mapGroup, s8 mapNum, s8 warpId)
{
    SetWarpData(&gSaveBlock1Ptr->dynamicWarp, mapGroup, mapNum, warpId, gSaveBlock1Ptr->pos.x, gSaveBlock1Ptr->pos.y);
}

void SetDynamicWarpWithCoords(s32 unused, s8 mapGroup, s8 mapNum, s8 warpId, s8 x, s8 y)
{
    SetWarpData(&gSaveBlock1Ptr->dynamicWarp, mapGroup, mapNum, warpId, x, y);
}

void SetWarpDestinationToDynamicWarp(u8 unusedWarpId)
{
    sWarpDestination = gSaveBlock1Ptr->dynamicWarp;
}

void SetWarpDestinationToHealLocation(u8 healLocationId)
{
    const struct HealLocation *healLocation = GetHealLocation(healLocationId);
    if (healLocation)
        SetWarpDestination(healLocation->mapGroup, healLocation->mapNum, WARP_ID_NONE, healLocation->x, healLocation->y);
}

static bool32 IsWhiteoutCutscene(void)
{
    if (OW_WHITEOUT_CUTSCENE < GEN_4)
        return FALSE;
    return GetHealNpcLocalId(GetHealLocationIndexByWarpData(&gSaveBlock1Ptr->lastHealLocation)) != LOCALID_NONE;
}

void SetWarpDestinationToLastHealLocation(void)
{
    if (IsWhiteoutCutscene())
        SetWhiteoutRespawnWarpAndHealerNPC(&sWarpDestination);
    else
        sWarpDestination = gSaveBlock1Ptr->lastHealLocation;

#if IS_HNS
    if (sWarpDestination.mapGroup == 0 && sWarpDestination.mapNum == 0)
    {
        const struct HealLocation *fallback = GetHealLocation(HEAL_LOCATION_NEW_BARK_TOWN_HNS);
        if (fallback)
            SetWarpDestination(fallback->mapGroup, fallback->mapNum, WARP_ID_NONE, fallback->x, fallback->y);
    }
#endif
}

void SetWarpDestinationForTeleport(void)
{
    sWarpDestination = gSaveBlock1Ptr->lastHealLocation;
}

void SetLastHealLocationWarp(u8 healLocationId)
{
    const struct HealLocation *healLocation = GetHealLocation(healLocationId);
    if (healLocation)
        SetWarpData(&gSaveBlock1Ptr->lastHealLocation, healLocation->mapGroup, healLocation->mapNum, WARP_ID_NONE, healLocation->x, healLocation->y);
}

void UpdateEscapeWarp(s16 x, s16 y)
{
    enum MapType currMapType = GetCurrentMapType();
    enum MapType destMapType = GetMapTypeByGroupAndId(sWarpDestination.mapGroup, sWarpDestination.mapNum);
    if (IsMapTypeOutdoors(currMapType) && IsMapTypeOutdoors(destMapType) != TRUE)
        SetEscapeWarp(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE, x - MAP_OFFSET, y - MAP_OFFSET + 1);
}

void SetEscapeWarp(s8 mapGroup, s8 mapNum, s8 warpId, s8 x, s8 y)
{
    SetWarpData(&gSaveBlock1Ptr->escapeWarp, mapGroup, mapNum, warpId, x, y);
}

void SetWarpDestinationToEscapeWarp(void)
{
    sWarpDestination = gSaveBlock1Ptr->escapeWarp;
}

void SetFixedDiveWarp(s8 mapGroup, s8 mapNum, s8 warpId, s8 x, s8 y)
{
    SetWarpData(&sFixedDiveWarp, mapGroup, mapNum, warpId, x, y);
}

static void SetWarpDestinationToDiveWarp(void)
{
    sWarpDestination = sFixedDiveWarp;
}

void SetFixedHoleWarp(s8 mapGroup, s8 mapNum, s8 warpId, s8 x, s8 y)
{
    SetWarpData(&sFixedHoleWarp, mapGroup, mapNum, warpId, x, y);
}

void SetWarpDestinationToFixedHoleWarp(s16 x, s16 y)
{
    if (IsDummyWarp(&sFixedHoleWarp) == TRUE)
        sWarpDestination = gLastUsedWarp;
    else
        SetWarpDestination(sFixedHoleWarp.mapGroup, sFixedHoleWarp.mapNum, WARP_ID_NONE, x, y);
}

static void SetWarpDestinationToContinueGameWarp(void)
{
    sWarpDestination = gSaveBlock1Ptr->continueGameWarp;
}

void SetContinueGameWarp(s8 mapGroup, s8 mapNum, s8 warpId, s8 x, s8 y)
{
    SetWarpData(&gSaveBlock1Ptr->continueGameWarp, mapGroup, mapNum, warpId, x, y);
}

void SetContinueGameWarpToHealLocation(u8 healLocationId)
{
    const struct HealLocation *healLocation = GetHealLocation(healLocationId);
    if (healLocation)
        SetWarpData(&gSaveBlock1Ptr->continueGameWarp, healLocation->mapGroup, healLocation->mapNum, WARP_ID_NONE, healLocation->x, healLocation->y);
}

void SetContinueGameWarpToDynamicWarp(int unused)
{
    gSaveBlock1Ptr->continueGameWarp = gSaveBlock1Ptr->dynamicWarp;
}

const struct MapConnection *GetMapConnection(u8 dir)
{
    s32 i;
    s32 count = gMapHeader.connections->count;
    const struct MapConnection *connection = gMapHeader.connections->connections;

    if (connection == NULL)
        return NULL;

    for (i = 0; i < count; i++, connection++)
        if (connection->direction == dir)
            return connection;

    return NULL;
}

static bool8 SetDiveWarp(enum Connection dir, u16 x, u16 y)
{
    const struct MapConnection *connection = GetMapConnection(dir);

    if (connection != NULL)
    {
        SetWarpDestination(connection->mapGroup, connection->mapNum, WARP_ID_NONE, x, y);
    }
    else
    {
        RunOnDiveWarpMapScript();
        if (IsDummyWarp(&sFixedDiveWarp))
            return FALSE;
        SetWarpDestinationToDiveWarp();
    }
    return TRUE;
}

bool8 SetDiveWarpEmerge(u16 x, u16 y)
{
    return SetDiveWarp(CONNECTION_EMERGE, x, y);
}

bool8 SetDiveWarpDive(u16 x, u16 y)
{
    return SetDiveWarp(CONNECTION_DIVE, x, y);
}

void LoadMapFromCameraTransition(u8 mapGroup, u8 mapNum)
{
#if IS_HNS
    Coop_MarkMapTransition();
#endif
    SetWarpDestination(mapGroup, mapNum, WARP_ID_NONE, -1, -1);

    // Dont transition map music between BF Outside West/East
    if (gMapHeader.regionMapSectionId != MAPSEC_BATTLE_FRONTIER)
        TransitionMapMusic();

    ApplyCurrentWarp();
    LoadCurrentMapData();
    LoadObjEventTemplatesFromHeader();
    TrySetMapSaveWarpStatus();
    ClearTempFieldEventData();
    ResetDexNavSearch();
    ResetCyclingRoadChallengeData();
    RestartWildEncounterImmunitySteps();
#if FREE_MATCH_CALL == FALSE
    TryUpdateRandomTrainerRematches(mapGroup, mapNum);
#endif //FREE_MATCH_CALL

    if (I_VS_SEEKER_CHARGING != 0)
        MapResetTrainerRematches(mapGroup, mapNum);

    DoTimeBasedEvents();
    SetSavedWeatherFromCurrMapHeader();
    ChooseAmbientCrySpecies();
    SetDefaultFlashLevel();
    Overworld_ClearSavedMusic();
    RunOnTransitionMapScript();
    InitMap();
    CopySecondaryTilesetToVramUsingHeap(gMapHeader.mapLayout);
    LoadSecondaryTilesetPalette(gMapHeader.mapLayout, TRUE); // skip copying to Faded, gamma shift will take care of it

    ApplyWeatherColorMapToPals(GetNumPalsInPrimary(gMapHeader.mapLayout), NUM_PALS_TOTAL - GetNumPalsInPrimary(gMapHeader.mapLayout)); // palettes [6,12]

    InitSecondaryTilesetAnimation();
    UpdateLocationHistoryForRoamer();
    MoveAllRoamers();
    TryShowRoamerFlash();
    DoCurrentWeather();
    ResetFieldTasksArgs();
    RunOnResumeMapScript();

    if (OW_HIDE_REPEAT_MAP_POPUP)
    {
        if (gMapHeader.regionMapSectionId != sLastMapSectionId)
            ShowMapNamePopup();
    }
    else
    {
        if (gMapHeader.regionMapSectionId != MAPSEC_BATTLE_FRONTIER
         || gMapHeader.regionMapSectionId != sLastMapSectionId)
            ShowMapNamePopup();
    }
}

static void LoadMapFromWarp(bool32 a1)
{
    bool8 isOutdoors;
    bool8 isIndoors;

    LoadCurrentMapData();
    if (!(sObjectEventLoadFlag & SKIP_OBJECT_EVENT_LOAD))
    {
        if ((gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR || gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR_HNS))
            LoadBattlePyramidObjectEventTemplates();
        else if (InTrainerHill())
            LoadTrainerHillObjectEventTemplates();
        else
            LoadObjEventTemplatesFromHeader();
    }

    isOutdoors = IsMapTypeOutdoors(gMapHeader.mapType);
    isIndoors = IsMapTypeIndoors(gMapHeader.mapType);

    CheckLeftFriendsSecretBase();
    TrySetMapSaveWarpStatus();
    ClearTempFieldEventData();
    ResetDexNavSearch();
    // reset hours override on every warp
    sHoursOverride = 0;
    ResetCyclingRoadChallengeData();
    RestartWildEncounterImmunitySteps();
#if FREE_MATCH_CALL == FALSE
    TryUpdateRandomTrainerRematches(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
#endif //FREE_MATCH_CALL

    if (I_VS_SEEKER_CHARGING != 0)
         MapResetTrainerRematches(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);

    if (a1 != TRUE)
        DoTimeBasedEvents();
    SetSavedWeatherFromCurrMapHeader();
    ChooseAmbientCrySpecies();
    if (isOutdoors)
        FlagClear(FLAG_SYS_USE_FLASH);
    SetDefaultFlashLevel();
    Overworld_ClearSavedMusic();
    RunOnTransitionMapScript();
    UpdateLocationHistoryForRoamer();
    MoveAllRoamersToOtherLocationSets();
    gChainFishingDexNavStreak = 0;
    if ((gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR || gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR_HNS))
        InitBattlePyramidMap(FALSE);
    else if (InTrainerHill())
        InitTrainerHillMap();
    else
        InitMap();

    if (a1 != TRUE && isIndoors)
    {
        UpdateTVScreensOnMap(gBackupMapLayout.width, gBackupMapLayout.height);
        InitSecretBaseAppearance(TRUE);
    }
}

void ResetInitialPlayerAvatarState(void)
{
    sInitialPlayerAvatarState.direction = DIR_SOUTH;
    sInitialPlayerAvatarState.transitionFlags = PLAYER_AVATAR_FLAG_ON_FOOT;
}

void StoreInitialPlayerAvatarState(void)
{
    sInitialPlayerAvatarState.direction = GetPlayerFacingDirection();

    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE))
        sInitialPlayerAvatarState.transitionFlags = PLAYER_AVATAR_FLAG_MACH_BIKE;
    else if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_ACRO_BIKE))
        sInitialPlayerAvatarState.transitionFlags = PLAYER_AVATAR_FLAG_ACRO_BIKE;
    else if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
        sInitialPlayerAvatarState.transitionFlags = PLAYER_AVATAR_FLAG_SURFING;
    else if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_UNDERWATER))
        sInitialPlayerAvatarState.transitionFlags = PLAYER_AVATAR_FLAG_UNDERWATER;
    else
        sInitialPlayerAvatarState.transitionFlags = PLAYER_AVATAR_FLAG_ON_FOOT;
}

static struct InitialPlayerAvatarState *GetInitialPlayerAvatarState(void)
{
    struct InitialPlayerAvatarState playerStruct;
    enum MapType mapType = GetCurrentMapType();
    u16 metatileBehavior = GetCenterScreenMetatileBehavior();
    u8 transitionFlags = GetAdjustedInitialTransitionFlags(&sInitialPlayerAvatarState, metatileBehavior, mapType);
    playerStruct.transitionFlags = transitionFlags;
    playerStruct.direction = GetAdjustedInitialDirection(&sInitialPlayerAvatarState, transitionFlags, metatileBehavior, mapType);
    sInitialPlayerAvatarState = playerStruct;
    return &sInitialPlayerAvatarState;
}

static u8 GetAdjustedInitialTransitionFlags(struct InitialPlayerAvatarState *playerStruct, u16 metatileBehavior, enum MapType mapType)
{
    if (mapType != MAP_TYPE_INDOOR && FlagGet(FLAG_SYS_CRUISE_MODE))
        return PLAYER_AVATAR_FLAG_ON_FOOT;
    else if (mapType == MAP_TYPE_UNDERWATER)
        return PLAYER_AVATAR_FLAG_UNDERWATER;
    else if (MetatileBehavior_IsSurfableInSeafoamIslands(metatileBehavior) == TRUE)
        return PLAYER_AVATAR_FLAG_ON_FOOT;
    else if (MetatileBehavior_IsSurfableWaterOrUnderwater(metatileBehavior) == TRUE)
        return PLAYER_AVATAR_FLAG_SURFING;
    else if (Overworld_IsBikingAllowed() != TRUE)
        return PLAYER_AVATAR_FLAG_ON_FOOT;
    else if (playerStruct->transitionFlags == PLAYER_AVATAR_FLAG_MACH_BIKE)
        return PLAYER_AVATAR_FLAG_MACH_BIKE;
    else if (playerStruct->transitionFlags != PLAYER_AVATAR_FLAG_ACRO_BIKE)
        return PLAYER_AVATAR_FLAG_ON_FOOT;
    else
        return PLAYER_AVATAR_FLAG_ACRO_BIKE;
}

bool8 MetatileBehavior_IsSurfableInSeafoamIslands(u16 metatileBehavior)
{
    if (MetatileBehavior_IsSurfableWaterOrUnderwater(metatileBehavior) != TRUE)
        return FALSE;
    if ((gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_SEAFOAM_ISLANDS_B3F)
          && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_SEAFOAM_ISLANDS_B3F))
     || (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_SEAFOAM_ISLANDS_B4F)
          && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_SEAFOAM_ISLANDS_B4F)))
    {
        return TRUE;
    }

    return FALSE;
}

static enum Direction GetAdjustedInitialDirection(struct InitialPlayerAvatarState *playerStruct, u8 transitionFlags, u16 metatileBehavior, enum MapType mapType)
{
    if (FlagGet(FLAG_SYS_CRUISE_MODE) && mapType == MAP_TYPE_OCEAN_ROUTE)
        return DIR_EAST;
    else if (MetatileBehavior_IsDeepSouthWarp(metatileBehavior) == TRUE)
        return DIR_NORTH;
    else if (MetatileBehavior_IsNonAnimDoor(metatileBehavior) == TRUE || MetatileBehavior_IsDoor(metatileBehavior) == TRUE)
        return DIR_SOUTH;
    else if (MetatileBehavior_IsSouthArrowWarp(metatileBehavior) == TRUE)
        return DIR_NORTH;
    else if (MetatileBehavior_IsNorthArrowWarp(metatileBehavior) == TRUE)
        return DIR_SOUTH;
    else if (MetatileBehavior_IsWestArrowWarp(metatileBehavior) == TRUE)
        return DIR_EAST;
    else if (MetatileBehavior_IsEastArrowWarp(metatileBehavior) == TRUE)
        return DIR_WEST;
    else if (MetatileBehavior_IsDirectionalUpRightStairWarp(metatileBehavior) == TRUE || MetatileBehavior_IsDirectionalDownRightStairWarp(metatileBehavior) == TRUE)
        return DIR_WEST;
    else if (MetatileBehavior_IsDirectionalUpLeftStairWarp(metatileBehavior) == TRUE || MetatileBehavior_IsDirectionalDownLeftStairWarp(metatileBehavior) == TRUE)
        return DIR_EAST;
    else if ((playerStruct->transitionFlags == PLAYER_AVATAR_FLAG_UNDERWATER  && transitionFlags == PLAYER_AVATAR_FLAG_SURFING)
          || (playerStruct->transitionFlags == PLAYER_AVATAR_FLAG_SURFING && transitionFlags == PLAYER_AVATAR_FLAG_UNDERWATER))
        return playerStruct->direction;
    else if (MetatileBehavior_IsLadder(metatileBehavior) == TRUE)
        return playerStruct->direction;
    else
        return DIR_SOUTH;
}

static u16 GetCenterScreenMetatileBehavior(void)
{
    return MapGridGetMetatileBehaviorAt(gSaveBlock1Ptr->pos.x + MAP_OFFSET, gSaveBlock1Ptr->pos.y + MAP_OFFSET);
}

bool32 Overworld_IsBikingAllowed(void)
{
    if (!gMapHeader.allowCycling)
        return FALSE;
    else
        return TRUE;
}

// Flash level of 0 is fully bright
// Flash level of 1 is the largest flash radius
// Flash level of 7 is the smallest flash radius
// Flash level of 8 is fully black
void SetDefaultFlashLevel(void)
{
    if (!gMapHeader.cave)
        gSaveBlock1Ptr->flashLevel = 0;
    else if (FlagGet(FLAG_SYS_USE_FLASH))
        gSaveBlock1Ptr->flashLevel = 1;
    else
        gSaveBlock1Ptr->flashLevel = gMaxFlashLevel - 1;
}

void SetFlashLevel(s32 flashLevel)
{
    if (flashLevel < 0 || flashLevel > gMaxFlashLevel)
        flashLevel = 0;
    gSaveBlock1Ptr->flashLevel = flashLevel;
}

u8 GetFlashLevel(void)
{
    return gSaveBlock1Ptr->flashLevel;
}

void SetCurrentMapLayout(u16 mapLayoutId)
{
    gSaveBlock1Ptr->mapLayoutId = mapLayoutId;
    gMapHeader.mapLayout = GetMapLayout(mapLayoutId);
}

void SetObjectEventLoadFlag(u8 flag)
{
    sObjectEventLoadFlag = flag;
}

// sObjectEventLoadFlag is read directly
static u8 UNUSED GetObjectEventLoadFlag(void)
{
    return sObjectEventLoadFlag;
}

static bool16 ShouldLegendaryMusicPlayAtLocation(struct WarpData *warp)
{
    if (!FlagGet(FLAG_SYS_WEATHER_CTRL))
        return FALSE;
    if (warp->mapGroup == 0)
    {
        switch (warp->mapNum)
        {
        case MAP_NUM(MAP_LILYCOVE_CITY):
        case MAP_NUM(MAP_MOSSDEEP_CITY):
        case MAP_NUM(MAP_SOOTOPOLIS_CITY):
        case MAP_NUM(MAP_EVER_GRANDE_CITY):
        case MAP_NUM(MAP_ROUTE124):
        case MAP_NUM(MAP_ROUTE125):
        case MAP_NUM(MAP_ROUTE126):
        case MAP_NUM(MAP_ROUTE127):
        case MAP_NUM(MAP_ROUTE128):
            return TRUE;
        default:
            if (VarGet(VAR_SOOTOPOLIS_CITY_STATE) < 4)
                return FALSE;
            switch (warp->mapNum)
            {
            case MAP_NUM(MAP_ROUTE129):
            case MAP_NUM(MAP_ROUTE130):
            case MAP_NUM(MAP_ROUTE131):
                return TRUE;
            }
        }
    }
    return FALSE;
}

static bool16 NoMusicInSootopolisWithLegendaries(struct WarpData *warp)
{
    if (VarGet(VAR_SKY_PILLAR_STATE) != 1)
        return FALSE;
    else if (warp->mapGroup != MAP_GROUP(MAP_SOOTOPOLIS_CITY))
        return FALSE;
    else if (warp->mapNum == MAP_NUM(MAP_SOOTOPOLIS_CITY))
        return TRUE;
    else
        return FALSE;
}

static bool16 IsInfiltratedWeatherInstitute(struct WarpData *warp)
{
    if (VarGet(VAR_WEATHER_INSTITUTE_STATE))
        return FALSE;
    else if (warp->mapGroup != MAP_GROUP(MAP_ROUTE119_WEATHER_INSTITUTE_1F))
        return FALSE;
    else if (warp->mapNum == MAP_NUM(MAP_ROUTE119_WEATHER_INSTITUTE_1F)
     || warp->mapNum == MAP_NUM(MAP_ROUTE119_WEATHER_INSTITUTE_2F))
        return TRUE;
    else
        return FALSE;
}

static bool16 IsInfiltratedSpaceCenter(struct WarpData *warp)
{
    if (VarGet(VAR_MOSSDEEP_CITY_STATE) == 0)
        return FALSE;
    else if (VarGet(VAR_MOSSDEEP_CITY_STATE) > 2)
        return FALSE;
    else if (warp->mapGroup != MAP_GROUP(MAP_MOSSDEEP_CITY_SPACE_CENTER_1F))
        return FALSE;
    else if (warp->mapNum == MAP_NUM(MAP_MOSSDEEP_CITY_SPACE_CENTER_1F)
     || warp->mapNum == MAP_NUM(MAP_MOSSDEEP_CITY_SPACE_CENTER_2F))
        return TRUE;
    return FALSE;
}

#define IS_MAP(mapGroupId, mapNumId) \
    (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(mapGroupId) && gSaveBlock1Ptr->location.mapNum == MAP_NUM(mapNumId))

static bool16 IsRocketTakeover(struct WarpData *warp)
{
    // Inject Rocket Takeover override logic
    if (VarGet(VAR_MAHOGANY_TOWN_STATE) == 16)
    {
        if (IS_MAP(MAP_GOLDENROD_CITY_HNS, MAP_GOLDENROD_CITY_HNS) ||
            IS_MAP(MAP_GOLDENROD_CITY_RADIO_TOWER_1F_HNS, MAP_GOLDENROD_CITY_RADIO_TOWER_1F_HNS) ||
            IS_MAP(MAP_GOLDENROD_CITY_RADIO_TOWER_2F_HNS, MAP_GOLDENROD_CITY_RADIO_TOWER_2F_HNS) ||
            IS_MAP(MAP_GOLDENROD_CITY_RADIO_TOWER_3F_HNS, MAP_GOLDENROD_CITY_RADIO_TOWER_3F_HNS) ||
            IS_MAP(MAP_GOLDENROD_CITY_RADIO_TOWER_4F_HNS, MAP_GOLDENROD_CITY_RADIO_TOWER_4F_HNS) ||
            IS_MAP(MAP_GOLDENROD_CITY_RADIO_TOWER_5F_HNS, MAP_GOLDENROD_CITY_RADIO_TOWER_5F_HNS) ||
            IS_MAP(MAP_GOLDENROD_CITY_UNDERGROUND_ENTRANCE_HNS, MAP_GOLDENROD_CITY_UNDERGROUND_ENTRANCE_HNS) ||
            IS_MAP(MAP_GOLDENROD_CITY_UNDERGROUND_TUNNEL_HNS, MAP_GOLDENROD_CITY_UNDERGROUND_TUNNEL_HNS))
            return TRUE;
    }
    return FALSE;
}

u16 GetLocationMusic(struct WarpData *warp)
{
    if (IsRocketTakeover(warp) == TRUE)
        return MUS_HG_ROCKET_TAKEOVER;
    else if (NoMusicInSootopolisWithLegendaries(warp) == TRUE)
        return MUS_NONE;
    else if (ShouldLegendaryMusicPlayAtLocation(warp) == TRUE)
        return MUS_ABNORMAL_WEATHER;
    else if (IsInfiltratedSpaceCenter(warp) == TRUE)
        return MUS_ENCOUNTER_MAGMA;
    else if (IsInfiltratedWeatherInstitute(warp) == TRUE)
        return MUS_MT_CHIMNEY;
    else
        return Overworld_GetMapHeaderByGroupAndId(warp->mapGroup, warp->mapNum)->music;
}

u16 GetCurrLocationDefaultMusic(void)
{
    u16 music;

    // Play the desert music only when the sandstorm is active on Route 111.
    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_ROUTE111)
     && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_ROUTE111)
     && GetSavedWeather() == WEATHER_SANDSTORM)
        return MUS_DESERT;

    music = GetLocationMusic(&gSaveBlock1Ptr->location);
    if (music != MUS_ROUTE118)
    {
        return music;
    }
    else
    {
        if (gSaveBlock1Ptr->pos.x < 24)
            return MUS_ROUTE110;
        else
            return MUS_ROUTE119;
    }
}

u16 GetWarpDestinationMusic(void)
{
    u16 music = GetLocationMusic(&sWarpDestination);
    if (music != MUS_ROUTE118)
    {
        return music;
    }
    else
    {
        if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_MAUVILLE_CITY)
         && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_MAUVILLE_CITY))
            return MUS_ROUTE110;
        else
            return MUS_ROUTE119;
    }
}

void Overworld_ResetMapMusic(void)
{
    ResetMapMusic();
}

void Overworld_PlaySpecialMapMusic(void)
{
    u16 music = GetCurrLocationDefaultMusic();

    if (gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_STOP)
    {
        StopMapMusic();
        return;
    }
    if (gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_KEEP)
        return;

    if (IS_HNS && VarGet(VAR_TEMP_RADIO_MUSIC) != 0)
    {
        music = VarGet(VAR_TEMP_RADIO_MUSIC);
    }
    else if (music != MUS_ABNORMAL_WEATHER && music != MUS_NONE)
    {
        if (gSaveBlock1Ptr->savedMusic)
            music = gSaveBlock1Ptr->savedMusic;
        else if (GetCurrentMapType() == MAP_TYPE_UNDERWATER)
            music = MUS_UNDERWATER;
        else if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING) && !gSaveblock3.challengeSettings.surfMusic)
            music = (IS_HNS ? MUS_HG_SURF : IS_FRLG ? MUS_RG_SURF : MUS_SURF);
    }

    if (music != GetCurrentMapMusic())
        PlayNewMapMusic(music);
}

void Overworld_SetSavedMusic(u16 songNum)
{
    gSaveBlock1Ptr->savedMusic = songNum;
}

void Overworld_ClearSavedMusic(void)
{
    gSaveBlock1Ptr->savedMusic = MUS_DUMMY;
}

static void TransitionMapMusic(void)
{
    if (gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_STOP)
    {
        StopMapMusic();
        return;
    }
    if (gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_KEEP)
        return;

    if (FlagGet(FLAG_DONT_TRANSITION_MUSIC) != TRUE)
    {
        u16 newMusic = GetWarpDestinationMusic();
        u16 currentMusic = GetCurrentMapMusic();
        if (newMusic != MUS_ABNORMAL_WEATHER && newMusic != MUS_NONE)
        {
            if (currentMusic == MUS_UNDERWATER
             || (!gSaveblock3.challengeSettings.surfMusic && currentMusic == (IS_HNS ? MUS_HG_SURF : IS_FRLG ? MUS_RG_SURF : MUS_SURF)))
                return;
            if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING) && !gSaveblock3.challengeSettings.surfMusic)
                newMusic = (IS_HNS ? MUS_HG_SURF : IS_FRLG ? MUS_RG_SURF : MUS_SURF);
        }
        if (newMusic != currentMusic)
        {
            if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
                FadeOutAndFadeInNewMapMusic(newMusic, 4, 4);
            else
                FadeOutAndPlayNewMapMusic(newMusic, 8);
        }
    }
}

void Overworld_ChangeMusicToDefault(void)
{
    u16 currentMusic = GetCurrentMapMusic();
    if (currentMusic != GetCurrLocationDefaultMusic())
        FadeOutAndPlayNewMapMusic(GetCurrLocationDefaultMusic(), 8);
}

void Overworld_ChangeMusicTo(u16 newMusic)
{
    u16 currentMusic = GetCurrentMapMusic();
    if (currentMusic != newMusic && currentMusic != MUS_ABNORMAL_WEATHER)
        FadeOutAndPlayNewMapMusic(newMusic, 8);
}

u8 GetMapMusicFadeoutSpeed(void)
{
    const struct MapHeader *mapHeader = GetDestinationWarpMapHeader();
    if (IsMapTypeIndoors(mapHeader->mapType) == TRUE)
        return 2;
    else
        return 4;
}

void TryFadeOutOldMapMusic(void)
{
    u16 currentMusic = GetCurrentMapMusic();
    u16 warpMusic = GetWarpDestinationMusic();
    if (FlagGet(FLAG_DONT_TRANSITION_MUSIC) != TRUE && warpMusic != GetCurrentMapMusic())
    {
        if ((currentMusic == MUS_SURF
            && VarGet(VAR_SKY_PILLAR_STATE) == 2
            && gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_SOOTOPOLIS_CITY)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_SOOTOPOLIS_CITY)
            && sWarpDestination.mapGroup == MAP_GROUP(MAP_SOOTOPOLIS_CITY)
            && sWarpDestination.mapNum == MAP_NUM(MAP_SOOTOPOLIS_CITY)
            && sWarpDestination.x == 29
            && sWarpDestination.y == 53)
            || (currentMusic == MUS_HG_ROCKET_TAKEOVER
            && sWarpDestination.mapGroup == MAP_GROUP(MAP_GOLDENROD_CITY_UNDERGROUND_TUNNEL_HNS)
            && sWarpDestination.mapNum == MAP_NUM(MAP_GOLDENROD_CITY_UNDERGROUND_TUNNEL_HNS)
            && sWarpDestination.warpId == 2))
            return;
        FadeOutMapMusic(GetMapMusicFadeoutSpeed());
    }
}

bool8 BGMusicStopped(void)
{
    return IsNotWaitingForBGMStop();
}

void Overworld_FadeOutMapMusic(void)
{
    FadeOutMapMusic(4);
}

static void PlayAmbientCry(void)
{
    s16 x, y;
    s8 pan;
    s8 volume;

    PlayerGetDestCoords(&x, &y);
    if (sIsAmbientCryWaterMon == TRUE
     && !MetatileBehavior_IsSurfableWaterOrUnderwater(MapGridGetMetatileBehaviorAt(x, y)))
        return;
    pan = (Random() % 88) + 212;
    volume = (Random() % 30) + 50;

    if (gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_STOP)
    {
        StopMapMusic();
        return;
    }
    if (gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_KEEP)
        return;

    PlayCry_NormalNoDucking(sAmbientCrySpecies, pan, volume, CRY_PRIORITY_AMBIENT);
}

// States for UpdateAmbientCry
enum {
    AMB_CRY_INIT,
    AMB_CRY_FIRST,
    AMB_CRY_RESET,
    AMB_CRY_WAIT,
    AMB_CRY_IDLE,
};

void UpdateAmbientCry(s16 *state, u16 *delayCounter)
{
    u8 i, monsCount, divBy;

    switch (*state)
    {
    case AMB_CRY_INIT:
        // This state will be revisited whenever ResetFieldTasksArgs is called (which happens on map transition)
        if (sAmbientCrySpecies == SPECIES_NONE)
            *state = AMB_CRY_IDLE;
        else
            *state = AMB_CRY_FIRST;
        break;
    case AMB_CRY_FIRST:
        // It takes between 1200-3599 frames (~20-60 seconds) to play the first ambient cry after entering a map
        *delayCounter = (Random() % 2400) + 1200;
        *state = AMB_CRY_WAIT;
        break;
    case AMB_CRY_RESET:
        divBy = 1;
        monsCount = CalculatePlayerPartyCount();
        for (i = 0; i < monsCount; i++)
        {
            if (!GetMonData(&gPlayerParty[i], MON_DATA_SANITY_IS_EGG)
                && GetMonAbility(&gPlayerParty[0]) == ABILITY_SWARM)
            {
                divBy = 2;
                break;
            }
        }
        // Ambient cries after the first one take between 1200-2399 frames (~20-40 seconds)
        // If the player has a Pokémon with the ability Swarm in their party, the time is halved to 600-1199 frames (~10-20 seconds)
        *delayCounter = ((Random() % 1200) + 1200) / divBy;
        *state = AMB_CRY_WAIT;
        break;
    case AMB_CRY_WAIT:
        if (--(*delayCounter) == 0)
        {
            PlayAmbientCry();
            *state = AMB_CRY_RESET;
        }
        break;
    case AMB_CRY_IDLE:
        // No land/water Pokémon on this map
        break;
    }
}

static void ChooseAmbientCrySpecies(void)
{
    if ((gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_ROUTE130)
     && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_ROUTE130))
     && !IsMirageIslandPresent())
    {
        // Only play water Pokémon cries on this route
        // when Mirage Island is not present
        sIsAmbientCryWaterMon = TRUE;
        sAmbientCrySpecies = GetLocalWaterMon();
    }
    else
    {
        sAmbientCrySpecies = GetLocalWildMon(&sIsAmbientCryWaterMon);
    }
}

enum MapType GetMapTypeByGroupAndId(s8 mapGroup, s8 mapNum)
{
    return Overworld_GetMapHeaderByGroupAndId(mapGroup, mapNum)->mapType;
}

enum MapType GetMapTypeByWarpData(struct WarpData *warp)
{
    return GetMapTypeByGroupAndId(warp->mapGroup, warp->mapNum);
}

enum MapType GetCurrentMapType(void)
{
    return GetMapTypeByWarpData(&gSaveBlock1Ptr->location);
}

enum MapType GetLastUsedWarpMapType(void)
{
    return GetMapTypeByWarpData(&gLastUsedWarp);
}

bool8 IsMapTypeOutdoors(enum MapType mapType)
{
    if (mapType == MAP_TYPE_ROUTE
     || mapType == MAP_TYPE_TOWN
     || mapType == MAP_TYPE_UNDERWATER
     || mapType == MAP_TYPE_CITY
     || mapType == MAP_TYPE_OCEAN_ROUTE)
        return TRUE;
    else
        return FALSE;
}

bool8 Overworld_MapTypeAllowsTeleportAndFly(enum MapType mapType)
{
    if (mapType == MAP_TYPE_ROUTE
     || mapType == MAP_TYPE_TOWN
     || mapType == MAP_TYPE_OCEAN_ROUTE
     || mapType == MAP_TYPE_CITY)
        return TRUE;
    else
        return FALSE;
}

bool8 IsMapTypeIndoors(enum MapType mapType)
{
    if (mapType == MAP_TYPE_INDOOR
     || mapType == MAP_TYPE_SECRET_BASE)
        return TRUE;
    else
        return FALSE;
}

mapsec_u8_t GetSavedWarpRegionMapSectionId(void)
{
    return Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->dynamicWarp.mapGroup, gSaveBlock1Ptr->dynamicWarp.mapNum)->regionMapSectionId;
}

mapsec_u8_t GetCurrentRegionMapSectionId(void)
{
    return Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum)->regionMapSectionId;
}

enum MapBattleScene GetCurrentMapBattleScene(void)
{
    return Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum)->battleType;
}

static void InitOverworldBgs(void)
{
    InitBgsFromTemplates(0, sOverworldBgTemplates, ARRAY_COUNT(sOverworldBgTemplates));
    SetBgAttribute(1, BG_ATTR_MOSAIC, 1);
    SetBgAttribute(2, BG_ATTR_MOSAIC, 1);
    SetBgAttribute(3, BG_ATTR_MOSAIC, 1);
    gOverworldTilemapBuffer_Bg1 = AllocZeroed(BG_SCREEN_SIZE);
    gOverworldTilemapBuffer_Bg2 = AllocZeroed(BG_SCREEN_SIZE);
    gOverworldTilemapBuffer_Bg3 = AllocZeroed(BG_SCREEN_SIZE);
    SetBgTilemapBuffer(1, gOverworldTilemapBuffer_Bg1);
    SetBgTilemapBuffer(2, gOverworldTilemapBuffer_Bg2);
    SetBgTilemapBuffer(3, gOverworldTilemapBuffer_Bg3);
    InitStandardTextBoxWindows();
}

static void InitOverworldBgs_NoResetHeap(void)
{
    ResetBgsAndClearDma3BusyFlags(FALSE);
    InitBgsFromTemplates(0, sOverworldBgTemplates, NELEMS(sOverworldBgTemplates));
    SetBgAttribute(1, BG_ATTR_MOSAIC, TRUE);
    SetBgAttribute(2, BG_ATTR_MOSAIC, TRUE);
    SetBgAttribute(3, BG_ATTR_MOSAIC, TRUE);
    gOverworldTilemapBuffer_Bg1 = AllocZeroed(BG_SCREEN_SIZE);
    gOverworldTilemapBuffer_Bg2 = AllocZeroed(BG_SCREEN_SIZE);
    gOverworldTilemapBuffer_Bg3 = AllocZeroed(BG_SCREEN_SIZE);
    SetBgTilemapBuffer(1, gOverworldTilemapBuffer_Bg1);
    SetBgTilemapBuffer(2, gOverworldTilemapBuffer_Bg2);
    SetBgTilemapBuffer(3, gOverworldTilemapBuffer_Bg3);
    InitStandardTextBoxWindows();
    InitTextBoxGfxAndPrinters();
    InitFieldMessageBox();
}

void CleanupOverworldWindowsAndTilemaps(void)
{
    ClearMirageTowerPulseBlendEffect();
    FreeAllOverworldWindowBuffers();
    TRY_FREE_AND_SET_NULL(gOverworldTilemapBuffer_Bg3);
    TRY_FREE_AND_SET_NULL(gOverworldTilemapBuffer_Bg2);
    TRY_FREE_AND_SET_NULL(gOverworldTilemapBuffer_Bg1);
}

static void ResetSafariZoneFlag_(void)
{
    ResetSafariZoneFlag();
}

#if IS_HNS
bool8 IsCoopLinkActive(void)
{
    return gHnsCoopMailbox.magic == HNS_COOP_MAILBOX_MAGIC
        && gHnsCoopMailbox.gameReady
        && gReceivedRemoteLinkPlayers;
}

bool8 Coop_IsTransportConnected(void)
{
    return gHnsCoopMailbox.magic == HNS_COOP_MAILBOX_MAGIC
        && gHnsCoopMailbox.transportConnected != 0;
}

bool8 Coop_IsStorySyncReady(void)
{
    return IsCoopLinkActive() && sCoopStorySyncReady
        && gHnsCoopMailbox.remoteStorySyncReady;
}
#else
bool8 IsCoopLinkActive(void) { return FALSE; }
bool8 Coop_IsTransportConnected(void) { return FALSE; }
bool8 Coop_IsStorySyncReady(void) { return FALSE; }
#endif

#if IS_HNS
// Partners must be within this Manhattan tile distance to join a trainer
// double. Farther (or unpublished remote coords) keeps the encounter solo.
#define COOP_JOIN_NEARBY_TILES 12

static bool8 Coop_ArePartnersNearby(void)
{
    struct ObjectEvent *obj;
    s16 localX, localY;
    s16 remoteX = (s16)gHnsCoopMailbox.remoteX;
    s16 remoteY = (s16)gHnsCoopMailbox.remoteY;
    s16 dx, dy;

    // (0,0) means the peer has not published a position yet — do not guess.
    if (remoteX == 0 && remoteY == 0)
        return FALSE;

    if (IsOverworldLinkActive())
    {
        struct LinkPlayerObjectEvent *linkPlayer = &gLinkPlayerObjectEvents[gLocalLinkPlayerId];
        if (!linkPlayer->active)
            return FALSE;
        obj = &gObjectEvents[linkPlayer->objEventId];
        if (!obj->active)
            return FALSE;
    }
    else
    {
        obj = &gObjectEvents[gPlayerAvatar.objectEventId];
        if (!obj->active)
            return FALSE;
    }

    localX = obj->currentCoords.x;
    localY = obj->currentCoords.y;
    dx = localX - remoteX;
    dy = localY - remoteY;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    return (dx + dy) <= COOP_JOIN_NEARBY_TILES;
}

bool8 Coop_ArePlayersOnSameMap(void)
{
    // Full SIO link + challenge handshake + same map + partner not in a solo
    // activity (remoteSioActive) + nearby. Callers that also need story sync
    // (trainer battles) check Coop_IsStorySyncReady themselves.
    return IsCoopLinkActive()
        && gHnsCoopMailbox.remoteConfigReady != 0
        && (!gHnsCoopMailbox.hostRole || gHnsCoopMailbox.remoteConfigReady == gHnsCoopMailbox.localConfigReady)
        && gHnsCoopMailbox.remoteMapReady
        && gHnsCoopMailbox.remoteSioActive
        && gHnsCoopMailbox.localMapId == gHnsCoopMailbox.remoteMapId
        && Coop_ArePartnersNearby();
}

void Coop_SetSioActive(bool8 active)
{
    gHnsCoopMailbox.localSioActive = active;
}

static u32 Coop_PackLocalFollower(void)
{
    struct ObjectEvent *follower;

#if !OW_FOLLOWERS_ENABLED
    return 0;
#else
    // Mirror local hide rules: ball / surf / bike / forced-movement tiles and
    // a missing follower object all publish 0 so the peer destroys theirs.
    if (!IsFollowerVisible())
        return 0;
    follower = GetFollowerObject();
    if (follower == NULL || follower->invisible)
        return 0;
    if (!(follower->graphicsId & OBJ_EVENT_MON))
        return 0;
    return HNS_COOP_FOLLOWER_VISIBLE | ((u32)follower->graphicsId & HNS_COOP_FOLLOWER_GFX_MASK);
#endif
}

static u8 Coop_LocalAvatarStateClass(void)
{
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE))
        return PLAYER_AVATAR_STATE_MACH_BIKE;
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_ACRO_BIKE))
        return PLAYER_AVATAR_STATE_ACRO_BIKE;
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
        return PLAYER_AVATAR_STATE_SURFING;
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_UNDERWATER))
        return PLAYER_AVATAR_STATE_UNDERWATER;
    return PLAYER_AVATAR_STATE_NORMAL;
}

static u8 Coop_LocalDoorPhase(struct ObjectEvent *obj, u8 facing)
{
    u8 behavior;

    if (obj == NULL || !obj->active)
        return 0;
    behavior = MapGridGetMetatileBehaviorAt(obj->currentCoords.x, obj->currentCoords.y);
    if (MetatileBehavior_IsDoor(behavior) || MetatileBehavior_IsNonAnimDoor(behavior))
    {
        if (facing == DIR_NORTH)
            return 1; // entering
        if (facing == DIR_SOUTH)
            return 2; // exiting / standing on door
    }
    return 0;
}

static void Coop_PublishLocalPosition(void)
{
    // Publish from the local player avatar itself, not from the link-player
    // slot: the slot only exists after a full SIO handshake, while the
    // avatar object is always present in the overworld. Reading the slot
    // published stale zeros whenever the link was down, parking every remote
    // avatar at the map corner.
    //
    // But in link mode there is no player avatar object: the link loader
    // never creates one, and ResetObjectEvents zeroes gPlayerAvatar, so
    // objectEventId is 0 (an NPC slot, or inactive). Publishing from it
    // sends an NPC's coords or nothing, and the peer snaps your avatar to
    // the map corner, so you are invisible. While the link is up, publish
    // from the local link player object instead.
    struct ObjectEvent *obj;
    u8 facing;
    u8 action;
    u8 stateClass;
    u8 doorPhase;
    u8 claimSeq;

    if (IsOverworldLinkActive())
    {
        struct LinkPlayerObjectEvent *linkPlayer = &gLinkPlayerObjectEvents[gLocalLinkPlayerId];
        if (!linkPlayer->active)
            return;
        obj = &gObjectEvents[linkPlayer->objEventId];
        if (!obj->active)
            return;
        facing = linkDirection(obj);
    }
    else
    {
        obj = &gObjectEvents[gPlayerAvatar.objectEventId];
        if (!obj->active)
            return;
        facing = GetPlayerFacingDirection();
    }
    gHnsCoopMailbox.localX = (u32)(u16)obj->currentCoords.x;
    gHnsCoopMailbox.localY = (u32)(u16)obj->currentCoords.y;
    gHnsCoopMailbox.localGender = (u32)gSaveBlock2Ptr->playerGender;
    gHnsCoopMailbox.localFacing = (u32)facing;
    gHnsCoopMailbox.localFollower = Coop_PackLocalFollower();

    action = obj->movementActionId;
    stateClass = Coop_LocalAvatarStateClass();
    doorPhase = Coop_LocalDoorPhase(obj, facing);
    claimSeq = (u8)((gHnsCoopMailbox.localAvatarState >> 24) + 1);
    gHnsCoopMailbox.localAvatarState = (u32)action
        | ((u32)stateClass << 8)
        | ((u32)doorPhase << 16)
        | ((u32)claimSeq << 24);
}

// Presence that does not depend on the GBA link handshake: as long as the
// relay session is up and both games report the same map, each phone shows
// the other's avatar at the peer's published coordinates. This is what
// makes both players visible even when the SIO link itself never completes.
bool8 Coop_IsPeerVisible(void)
{
    // Do NOT reject remoteMapId == 0: New Bark Town packs as group 0 / num 0.
    // remoteMapReady is the unpublished/unset gate; map equality still required.
    return gHnsCoopMailbox.magic == HNS_COOP_MAILBOX_MAGIC
        && gHnsCoopMailbox.peerLinkReady
        && gHnsCoopMailbox.remoteMapReady
        && gHnsCoopMailbox.localMapId == gHnsCoopMailbox.remoteMapId;
}

#define COOP_PRESENCE_OBJ_EVENT_ID          (OBJECT_EVENTS_COUNT - 1)
#define COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID   (OBJECT_EVENTS_COUNT - 2)

u8 Coop_GetReservedPresenceObjectEventId(void)
{
    return COOP_PRESENCE_OBJ_EVENT_ID;
}

u8 Coop_GetReservedRemoteFollowerObjectEventId(void)
{
    return COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID;
}

bool8 Coop_IsReservedPresenceObjectEventId(u8 objectEventId)
{
    return objectEventId == COOP_PRESENCE_OBJ_EVENT_ID
        || objectEventId == COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID;
}

// Kept in overworld.c (not event_object_movement.o .data) so --gc-sections
// cannot discard the prefer slot while GetFirstInactive still references it.
// EWRAM starts zeroed; Take validates !active so a stale 0 is harmless.
EWRAM_DATA static u8 sCoopPreferObjectEventId;

void Coop_PreferObjectEventSlot(u8 objectEventId)
{
    sCoopPreferObjectEventId = objectEventId;
}

u8 Coop_TakePreferredObjectEventId(void)
{
    u8 id = sCoopPreferObjectEventId;

    sCoopPreferObjectEventId = OBJECT_EVENTS_COUNT;
    if (id < OBJECT_EVENTS_COUNT && !gObjectEvents[id].active)
        return id;
    return OBJECT_EVENTS_COUNT;
}

static void Coop_ForceClearObjectEventSlot(u8 objectEventId)
{
    struct ObjectEvent *obj;

    if (objectEventId >= OBJECT_EVENTS_COUNT)
        return;
    // Never touch the local player avatar slot. Presence/follower reserved
    // ids are COUNT-1/COUNT-2; a bug or stale prefer-id must not wipe slot 0.
    if (objectEventId == gPlayerAvatar.objectEventId)
        return;
    obj = &gObjectEvents[objectEventId];
    // ClearObjectEvent / memset leave spriteId == 0 on unused slots.
    // CreateSprite scans from 0, so the local player is almost always
    // gSprites[0]. Destroying that sprite makes the player invisible and
    // leaves the camera tracking a dead/reused id (cannot "move").
    // Only destroy a sprite that this active object event actually owns.
    if (obj->active
     && obj->spriteId < MAX_SPRITES
     && gSprites[obj->spriteId].inUse)
        DestroySprite(&gSprites[obj->spriteId]);
    memset(obj, 0, sizeof(*obj));
}

#define COOP_PRESENCE_LINK_ID 1
#define COOP_STEP_QUEUE_SIZE 24
#define COOP_PRESENCE_SNAP_TILES 8
// After field entry, wait for map NPCs to claim slots before partner spawn.
#define COOP_SPAWN_DELAY_FRAMES 90
// Back off after a failed create instead of retrying every CB1 tick.
#define COOP_SPAWN_RETRY_FRAMES 60
// When the step queue runs deep, play faster walk/run actions to catch up.
#define COOP_HURRY_QUEUE_DEPTH 4
// Door enter hold: refuse respawn in front of the door after partner enters.
#define COOP_DOOR_HOLD_FRAMES 180
#define COOP_DOOR_WAIT_FRAMES 60
#define COOP_DOOR_GIVEUP_FRAMES 150

#define COOP_DOOR_NONE       0
#define COOP_DOOR_HOLD       1
#define COOP_DOOR_EXIT_OPEN  2
#define COOP_DOOR_EXIT_STEP  3
#define COOP_DOOR_ENTER_OPEN 4

static u8 sCoopStepQueue[COOP_STEP_QUEUE_SIZE];
static u8 sCoopStepQueueRead;
static u8 sCoopStepQueueWrite;
static u8 sCoopStepQueueCount;
static s16 sCoopQueuedTipX;
static s16 sCoopQueuedTipY;
static u32 sCoopQueuedForMapId;
static bool8 sCoopQueueTipValid;
static u16 sCoopFramesInOverworld;
static u16 sCoopSpawnCooldown;
static u8 sCoopPresenceGfxState;
static u8 sCoopDoorState;
static u16 sCoopDoorTimer;
static s16 sCoopDoorX;
static s16 sCoopDoorY;
static bool8 sCoopWasOnField;

static void Coop_ClearPresenceStepQueue(void)
{
    sCoopStepQueueRead = 0;
    sCoopStepQueueWrite = 0;
    sCoopStepQueueCount = 0;
    sCoopQueueTipValid = FALSE;
}

static void Coop_EnqueuePresenceFacing(u8 facing)
{
    if (sCoopStepQueueCount >= COOP_STEP_QUEUE_SIZE)
        return;
    sCoopStepQueue[sCoopStepQueueWrite] = facing;
    sCoopStepQueueWrite = (sCoopStepQueueWrite + 1) % COOP_STEP_QUEUE_SIZE;
    sCoopStepQueueCount++;
}

static u8 Coop_DequeuePresenceFacing(void)
{
    u8 facing;

    if (sCoopStepQueueCount == 0)
        return 0;
    facing = sCoopStepQueue[sCoopStepQueueRead];
    sCoopStepQueueRead = (sCoopStepQueueRead + 1) % COOP_STEP_QUEUE_SIZE;
    sCoopStepQueueCount--;
    return facing;
}

// Build an axis-aligned path of FACING_* steps from (fromX,fromY) to (toX,toY).
// Prefer matching the peer's last published facing on the final axis.
static void Coop_EnqueuePathTo(s16 fromX, s16 fromY, s16 toX, s16 toY, u8 preferFacing)
{
    s16 x = fromX;
    s16 y = fromY;
    u8 steps = 0;

    while ((x != toX || y != toY) && sCoopStepQueueCount < COOP_STEP_QUEUE_SIZE && steps < COOP_STEP_QUEUE_SIZE)
    {
        u8 facing;
        bool8 moveX = (x != toX);
        bool8 moveY = (y != toY);

        // Prefer the published facing's axis when both remain, so L-paths
        // match how the peer actually walked instead of always X-then-Y.
        if (moveX && moveY)
        {
            if (preferFacing == FACING_UP || preferFacing == FACING_DOWN)
                moveX = FALSE;
            else if (preferFacing == FACING_LEFT || preferFacing == FACING_RIGHT)
                moveY = FALSE;
            else
                moveY = FALSE; // default horizontal-first
        }

        if (sCoopStepQueueCount >= COOP_STEP_QUEUE_SIZE)
            break;
        if (moveX)
        {
            facing = (toX > x) ? FACING_RIGHT : FACING_LEFT;
            x += (toX > x) ? 1 : -1;
        }
        else
        {
            facing = (toY > y) ? FACING_DOWN : FACING_UP;
            y += (toY > y) ? 1 : -1;
        }
        Coop_EnqueuePresenceFacing(facing);
        // Tip tracks the last enqueued tile so a full queue does not pretend
        // we already reached the target (which would stop further enqueues).
        sCoopQueuedTipX = x;
        sCoopQueuedTipY = y;
        sCoopQueueTipValid = TRUE;
        steps++;
    }
}

// Presence avatars must not get stuck on NPCs/collision — force the one-tile
// glide the same way FacingHandler_DpadMovement does after a clear check.
static void Coop_ForcePresenceStep(struct ObjectEvent *obj, u8 facing)
{
    s16 x, y;

    linkDirection(obj) = FlipVerticalAndClearForced(facing, linkDirection(obj));
    ObjectEventMoveDestCoords(obj, linkDirection(obj), &x, &y);
    obj->directionSequenceIndex = 16;
    ShiftObjectEventCoords(obj, x, y);
    ObjectEventUpdateElevation(obj, NULL);
}

static u8 Coop_FacingToDirection(u8 facing)
{
    switch (facing)
    {
    case FACING_UP: return DIR_NORTH;
    case FACING_DOWN: return DIR_SOUTH;
    case FACING_LEFT: return DIR_WEST;
    case FACING_RIGHT: return DIR_EAST;
    default: return facing;
    }
}

// Pick a real engine movement action from host-confirmed avatar state + facing.
// Walk/run/bike/surf use the same action families the local player produces so
// the engine owns sub-pixel interpolation instead of a manual coord chase.
static u8 Coop_MovementActionForStep(u8 facing, u8 avatarState, u8 publishedAction, bool8 hurry)
{
    u8 dir = Coop_FacingToDirection(facing);
    u8 action = publishedAction;

    if (action != 0
     && action != MOVEMENT_ACTION_FACE_DOWN
     && action != MOVEMENT_ACTION_FACE_UP
     && action != MOVEMENT_ACTION_FACE_LEFT
     && action != MOVEMENT_ACTION_FACE_RIGHT
     && (action <= 0x0F
      || (action >= 0x15 && action <= 0x38)
      || (action >= 0x3A && action <= 0x3D)
      || (action >= 0x42 && action <= 0x4D)
      || (action >= 0x64 && action <= 0x8B)))
    {
        if (hurry
         && ((action >= MOVEMENT_ACTION_WALK_NORMAL_DOWN && action <= MOVEMENT_ACTION_WALK_NORMAL_RIGHT)
          || (action >= MOVEMENT_ACTION_WALK_FAST_DOWN && action <= MOVEMENT_ACTION_WALK_FAST_RIGHT)
          || (action >= MOVEMENT_ACTION_WALK_FASTER_DOWN && action <= MOVEMENT_ACTION_WALK_FASTER_RIGHT)
          || (action >= MOVEMENT_ACTION_PLAYER_RUN_DOWN && action <= MOVEMENT_ACTION_PLAYER_RUN_RIGHT)))
            return GetWalkFasterMovementAction(dir);
        return action;
    }

    switch (avatarState)
    {
    case PLAYER_AVATAR_STATE_MACH_BIKE:
    case PLAYER_AVATAR_STATE_ACRO_BIKE:
        return GetWalkFastMovementAction(dir);
    case PLAYER_AVATAR_STATE_SURFING:
    case PLAYER_AVATAR_STATE_UNDERWATER:
        return GetRideWaterCurrentMovementAction(dir);
    default:
        if (hurry)
            return GetPlayerRunMovementAction(dir);
        return GetWalkNormalMovementAction(dir);
    }
}

static bool8 Coop_PresenceReadyForMovement(struct ObjectEvent *obj)
{
    if (!ObjectEventIsHeldMovementActive(obj))
        return TRUE;
    if (ObjectEventCheckHeldMovementStatus(obj))
    {
        ObjectEventClearHeldMovement(obj);
        return TRUE;
    }
    return FALSE;
}

static void Coop_EnsurePresenceObjectSlot(void)
{
    // Always own the reserved high slot so map NPCs / cutscenes cannot
    // permanently steal the partner's object-event id.
    Coop_ForceClearObjectEventSlot(COOP_PRESENCE_OBJ_EVENT_ID);
}

static void Coop_ScrubSaveStaging(void)
{
    u8 i;

    // Save serializes gObjectEvents into SaveBlock1. Partner / remote-follower
    // entries must never persist or a continue shows ghosts / missing sprites.
    // Scrub the staging copy only — live screen objects stay untouched.
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *saved = &gSaveBlock1Ptr->objectEvents[i];

        if (i == COOP_PRESENCE_OBJ_EVENT_ID
         || i == COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID
         || saved->localId == OBJ_EVENT_ID_REMOTE_FOLLOWER
         || (saved->localId >= OBJ_EVENT_ID_DYNAMIC_BASE
          && saved->localId < OBJ_EVENT_ID_DYNAMIC_BASE + MAX_LINK_PLAYERS))
        {
            memset(saved, 0, sizeof(*saved));
            saved->localId = 0xFF;
            saved->mapNum = MAP_NUM(MAP_UNDEFINED);
            saved->mapGroup = MAP_GROUP(MAP_UNDEFINED);
            saved->movementActionId = MOVEMENT_ACTION_NONE;
            saved->spriteId = MAX_SPRITES;
        }
    }
}

static void Coop_TickDoorMachine(struct ObjectEvent *obj, bool8 peerVisible)
{
    u8 remoteDoor = (u8)((gHnsCoopMailbox.remoteAvatarState >> 16) & 0xFF);
    u8 behavior;

    if (sCoopDoorTimer != 0)
        sCoopDoorTimer--;

    switch (sCoopDoorState)
    {
    case COOP_DOOR_NONE:
        if (!peerVisible && remoteDoor == 1)
        {
            // Partner walked into a door on their screen: hold respawn so they
            // do not flicker back in front of the threshold.
            sCoopDoorState = COOP_DOOR_HOLD;
            sCoopDoorTimer = COOP_DOOR_HOLD_FRAMES;
            if (obj != NULL && obj->active)
            {
                sCoopDoorX = obj->currentCoords.x;
                sCoopDoorY = obj->currentCoords.y;
            }
            else
            {
                sCoopDoorX = (s16)gHnsCoopMailbox.remoteX;
                sCoopDoorY = (s16)gHnsCoopMailbox.remoteY;
            }
        }
        else if (peerVisible && remoteDoor == 2 && obj != NULL && obj->active)
        {
            behavior = MapGridGetMetatileBehaviorAt(obj->currentCoords.x, obj->currentCoords.y);
            if (MetatileBehavior_IsDoor(behavior) || MetatileBehavior_IsNonAnimDoor(behavior))
            {
                sCoopDoorX = obj->currentCoords.x;
                sCoopDoorY = obj->currentCoords.y;
                if (!FieldIsDoorAnimationRunning())
                {
                    FieldAnimateDoorOpen(sCoopDoorX, sCoopDoorY);
                    PlaySE(GetDoorSoundEffect(sCoopDoorX, sCoopDoorY));
                }
                sCoopDoorState = COOP_DOOR_EXIT_OPEN;
                sCoopDoorTimer = COOP_DOOR_WAIT_FRAMES;
            }
        }
        break;
    case COOP_DOOR_HOLD:
        if (sCoopDoorTimer == 0 || peerVisible)
            sCoopDoorState = COOP_DOOR_NONE;
        break;
    case COOP_DOOR_EXIT_OPEN:
        if (!FieldIsDoorAnimationRunning() && sCoopDoorTimer == 0)
        {
            sCoopDoorState = COOP_DOOR_EXIT_STEP;
            sCoopDoorTimer = COOP_DOOR_WAIT_FRAMES;
        }
        else if (sCoopDoorTimer == 0 && FieldIsDoorAnimationRunning())
        {
            // Give up waiting for the anim so presence cannot freeze forever.
            sCoopDoorState = COOP_DOOR_NONE;
        }
        break;
    case COOP_DOOR_EXIT_STEP:
        if (!peerVisible || sCoopDoorTimer == 0)
        {
            if (!FieldIsDoorAnimationRunning())
                FieldAnimateDoorClose(sCoopDoorX, sCoopDoorY);
            sCoopDoorState = COOP_DOOR_NONE;
        }
        break;
    case COOP_DOOR_ENTER_OPEN:
        if (sCoopDoorTimer == 0 || !FieldIsDoorAnimationRunning())
        {
            if (!FieldIsDoorAnimationRunning())
                FieldAnimateDoorClose(sCoopDoorX, sCoopDoorY);
            sCoopDoorState = COOP_DOOR_HOLD;
            sCoopDoorTimer = COOP_DOOR_HOLD_FRAMES;
        }
        break;
    default:
        sCoopDoorState = COOP_DOOR_NONE;
        break;
    }

    (void)COOP_DOOR_GIVEUP_FRAMES;
    (void)COOP_DOOR_ENTER_OPEN;
}

static bool8 Coop_TrySpawnPresenceAvatar(void)
{
    s16 spawnX = (s16)gHnsCoopMailbox.remoteX;
    s16 spawnY = (s16)gHnsCoopMailbox.remoteY;
    u8 gfxState = (u8)((gHnsCoopMailbox.remoteAvatarState >> 8) & 0xFF);

    if (sCoopDoorState == COOP_DOOR_HOLD)
        return FALSE;
    if (sCoopFramesInOverworld < COOP_SPAWN_DELAY_FRAMES)
        return FALSE;
    if (sCoopSpawnCooldown != 0)
    {
        sCoopSpawnCooldown--;
        return FALSE;
    }

    // (0,0) = unpublished coords (MAP_OFFSET makes real tiles ≥7). Fall
    // back to the local avatar only until the first POSITION arrives.
    if (spawnX == 0 && spawnY == 0)
    {
        struct ObjectEvent *playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
        spawnX = playerObj->currentCoords.x;
        spawnY = playerObj->currentCoords.y;
    }

    if (gfxState > PLAYER_AVATAR_STATE_WATERING)
        gfxState = PLAYER_AVATAR_STATE_NORMAL;
    sCoopPresenceGfxState = gfxState;

    Coop_EnsurePresenceObjectSlot();
    SpawnLinkPlayerObjectEvent(COOP_PRESENCE_LINK_ID,
        spawnX, spawnY, (u8)gHnsCoopMailbox.remoteGender);
    if (!gLinkPlayerObjectEvents[COOP_PRESENCE_LINK_ID].active)
    {
        sCoopSpawnCooldown = COOP_SPAWN_RETRY_FRAMES;
        return FALSE;
    }

    CreateLinkPlayerSprite(COOP_PRESENCE_LINK_ID, 0);
    if (!gLinkPlayerObjectEvents[COOP_PRESENCE_LINK_ID].active
     || gObjectEvents[COOP_PRESENCE_OBJ_EVENT_ID].spriteId >= MAX_SPRITES)
    {
        DestroyLinkPlayerObject(COOP_PRESENCE_LINK_ID);
        sCoopSpawnCooldown = COOP_SPAWN_RETRY_FRAMES;
        return FALSE;
    }

    {
        struct ObjectEvent *spawned = &gObjectEvents[COOP_PRESENCE_OBJ_EVENT_ID];

        spawned->invisible = FALSE;
        if (spawned->spriteId < MAX_SPRITES)
            gSprites[spawned->spriteId].invisible = FALSE;
        // Spawning while the camera scrolls can leave the sprite one tile off
        // grid. Re-place with the engine map-coord mover (pixel compensation
        // is the wrong fix).
        MoveObjectEventToMapCoords(spawned, spawnX, spawnY);
    }

    Coop_ClearPresenceStepQueue();
    sCoopQueuedTipX = spawnX;
    sCoopQueuedTipY = spawnY;
    sCoopQueueTipValid = TRUE;
    sCoopQueuedForMapId = gHnsCoopMailbox.localMapId;
    return TRUE;
}

static void Coop_SyncPresenceGraphics(struct ObjectEvent *obj)
{
    u8 want = (u8)((gHnsCoopMailbox.remoteAvatarState >> 8) & 0xFF);
    u16 graphicsId;

    if (want > PLAYER_AVATAR_STATE_WATERING)
        want = PLAYER_AVATAR_STATE_NORMAL;
    if (want == sCoopPresenceGfxState)
        return;

    // Bike/surf sprites change VRAM size; SetGraphicsId alone can leave a dead
    // anim table. Destroy and recreate in the same tick (skip spawn delay).
    graphicsId = GetPlayerAvatarGraphicsIdByStateIdAndGender(want, (u8)gHnsCoopMailbox.remoteGender);
    DestroyLinkPlayerObject(COOP_PRESENCE_LINK_ID);
    Coop_ClearPresenceStepQueue();
    sCoopPresenceGfxState = want;
    sCoopFramesInOverworld = COOP_SPAWN_DELAY_FRAMES;
    sCoopSpawnCooldown = 0;
    (void)graphicsId;
    Coop_TrySpawnPresenceAvatar();
    (void)obj;
}

static void Coop_UpdateRemotePresence(void)
{
    struct LinkPlayerObjectEvent *remote = &gLinkPlayerObjectEvents[COOP_PRESENCE_LINK_ID];
    u8 i;
    bool8 peerVisible = Coop_IsPeerVisible();

    // The vanilla link flow owns these objects once the SIO link is up.
    if (IsOverworldLinkActive())
        return;

    // Clear stale objects from dead link sessions; the presence avatar
    // below is the only link object allowed while the SIO link is down.
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (i == COOP_PRESENCE_LINK_ID)
            continue;
        if (gLinkPlayerObjectEvents[i].active)
            DestroyLinkPlayerObject(i);
    }

    // Every-frame watchdog: same-map peer must always have a live, visible
    // avatar. Cutscenes, fades, bag/menu returns, and object reloads can leave
    // an active link entry with a dead/hidden sprite — treat that as gone and
    // re-spawn immediately in this same CB1 tick.
    if (remote->active)
    {
        struct ObjectEvent *obj = &gObjectEvents[remote->objEventId];
        bool8 slotWrong = (remote->objEventId != COOP_PRESENCE_OBJ_EVENT_ID);
        bool8 dead = !obj->active
            || obj->spriteId >= MAX_SPRITES
            || !gSprites[obj->spriteId].inUse;
        bool8 hidden = !dead && gSprites[obj->spriteId].invisible;

        if (slotWrong || dead)
        {
            DestroyLinkPlayerObject(COOP_PRESENCE_LINK_ID);
            Coop_ClearPresenceStepQueue();
        }
        else if ((hidden || obj->invisible) && peerVisible)
        {
            obj->invisible = FALSE;
            gSprites[obj->spriteId].invisible = FALSE;
        }
        else if (peerVisible)
        {
            Coop_SyncPresenceGraphics(obj);
        }
    }

    Coop_TickDoorMachine(remote->active ? &gObjectEvents[remote->objEventId] : NULL, peerVisible);

    if (peerVisible)
    {
        if (!remote->active)
            Coop_TrySpawnPresenceAvatar();
    }
    else if (remote->active && sCoopDoorState != COOP_DOOR_HOLD
          && sCoopDoorState != COOP_DOOR_ENTER_OPEN)
    {
        // Entering a door: open once, then hold without respawning in front.
        if (((gHnsCoopMailbox.remoteAvatarState >> 16) & 0xFF) == 1)
        {
            struct ObjectEvent *obj = &gObjectEvents[remote->objEventId];

            sCoopDoorX = obj->currentCoords.x;
            sCoopDoorY = obj->currentCoords.y;
            if (!FieldIsDoorAnimationRunning())
            {
                FieldAnimateDoorOpen(sCoopDoorX, sCoopDoorY);
                PlaySE(GetDoorSoundEffect(sCoopDoorX, sCoopDoorY));
            }
            DestroyLinkPlayerObject(COOP_PRESENCE_LINK_ID);
            Coop_ClearPresenceStepQueue();
            sCoopDoorState = COOP_DOOR_ENTER_OPEN;
            sCoopDoorTimer = COOP_DOOR_WAIT_FRAMES;
        }
        else
        {
            DestroyLinkPlayerObject(COOP_PRESENCE_LINK_ID);
            Coop_ClearPresenceStepQueue();
        }
    }
}

static void Coop_SnapRemoteLinkPlayers(void)
{
    u8 i;
    s16 x, y;
    bool8 linkActive;
    // Without the SIO link there is no link identity; the local player is
    // always slot 0. Using the stale gLocalLinkPlayerId from a dead link
    // session would skip the presence avatar forever.
    u8 selfId;
    // Whether the peer is actually on this map right now.
    bool8 sameMap;

    linkActive = IsOverworldLinkActive();
    selfId = linkActive ? gLocalLinkPlayerId : 0;

    // Each phone is authoritative for its own player's map coordinates.
    // Key input still drives facing and step animation, but the remote
    // avatar's position snaps to the peer's published coordinates so both
    // screens agree on where everyone is standing.
    // Visibility is map-only: do not reuse Coop_ArePlayersOnSameMap, which
    // also requires the ≤12-tile battle-nearby gate (and SIO/config checks).
    // Hide the peer avatar only when maps differ; keep walk/snap polish.
    sameMap = gHnsCoopMailbox.remoteMapReady
        && gHnsCoopMailbox.localMapId == gHnsCoopMailbox.remoteMapId;

    x = (s16)gHnsCoopMailbox.remoteX;
    y = (s16)gHnsCoopMailbox.remoteY;
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        struct LinkPlayerObjectEvent *linkPlayer;
        struct ObjectEvent *obj;

        if (i == selfId)
            continue;
        linkPlayer = &gLinkPlayerObjectEvents[i];
        if (!linkPlayer->active)
            continue;
        obj = &gObjectEvents[linkPlayer->objEventId];
        if (!obj->active)
            continue;
        if (!sameMap)
        {
            // Different map: hide the peer's avatar instead of leaving it
            // standing where it spawned on this map. The object keeps
            // existing (so the link protocol is untouched) and comes back
            // the moment the maps match again.
            obj->invisible = TRUE;
            if (obj->spriteId != MAX_SPRITES)
                gSprites[obj->spriteId].invisible = TRUE;
            Coop_ClearPresenceStepQueue();
            continue;
        }
        obj->invisible = FALSE;
        if (obj->spriteId != MAX_SPRITES)
            gSprites[obj->spriteId].invisible = FALSE;

        // Presence and same-map link: drive the avatar from an authoritative
        // tile step queue (no dead-reckoning).
        // New remote targets append cardinal steps; only snap on large desync
        // (warp/resync). Never dead-reckon or invent tiles.
        if (sCoopQueuedForMapId != gHnsCoopMailbox.localMapId)
        {
            Coop_ClearPresenceStepQueue();
            sCoopQueuedForMapId = gHnsCoopMailbox.localMapId;
        }

        // Ignore unpublished (0,0) targets — keep last known / queued path.
        if (!(x == 0 && y == 0))
        {
            s16 tipX = sCoopQueueTipValid ? sCoopQueuedTipX : obj->currentCoords.x;
            s16 tipY = sCoopQueueTipValid ? sCoopQueuedTipY : obj->currentCoords.y;
            s16 gapX = x - tipX;
            s16 gapY = y - tipY;
            s16 gap;

            if (gapX < 0) gapX = -gapX;
            if (gapY < 0) gapY = -gapY;
            gap = gapX + gapY;
            if (gap >= COOP_PRESENCE_SNAP_TILES)
            {
                Coop_ClearPresenceStepQueue();
                obj->previousCoords.x = obj->currentCoords.x = x;
                obj->previousCoords.y = obj->currentCoords.y = y;
                SetSpritePosToMapCoords(x, y, &obj->initialCoords.x, &obj->initialCoords.y);
                obj->initialCoords.x += 8;
                linkDirection(obj) = (u8)gHnsCoopMailbox.remoteFacing;
                obj->directionSequenceIndex = 0;
                sCoopQueuedTipX = x;
                sCoopQueuedTipY = y;
                sCoopQueueTipValid = TRUE;
            }
            else if (gap > 0)
            {
                Coop_EnqueuePathTo(tipX, tipY, x, y, (u8)gHnsCoopMailbox.remoteFacing);
            }
        }

        // Apply at most one host-confirmed step once the prior engine movement
        // finishes. Prefer ObjectEventSetHeldMovement so walk/run/bike/surf/ledge
        // animate natively with sub-pixel interpolation; fall back to the link
        // force-step if the engine rejects the action.
        if (Coop_PresenceReadyForMovement(obj) && obj->directionSequenceIndex == 0)
        {
            if (sCoopStepQueueCount != 0)
            {
                u8 facing = Coop_DequeuePresenceFacing();
                u8 avatarState = (u8)((gHnsCoopMailbox.remoteAvatarState >> 8) & 0xFF);
                u8 publishedAction = (u8)(gHnsCoopMailbox.remoteAvatarState & 0xFF);
                bool8 hurry = (sCoopStepQueueCount >= COOP_HURRY_QUEUE_DEPTH);
                u8 action = Coop_MovementActionForStep(facing, avatarState, publishedAction, hurry);
                u16 beforeX = obj->currentCoords.x;
                u16 beforeY = obj->currentCoords.y;

                linkDirection(obj) = FlipVerticalAndClearForced(facing, linkDirection(obj));
                // SetHeldMovement returns TRUE when already overridden (rejected).
                if (ObjectEventSetHeldMovement(obj, action))
                {
                    SetPlayerFacingDirection(i, facing);
                    if (obj->currentCoords.x == beforeX && obj->currentCoords.y == beforeY
                     && obj->directionSequenceIndex == 0)
                        Coop_ForcePresenceStep(obj, facing);
                }
                else
                {
                    // Held movement owns the glide; tip tracks this step's dest.
                    sCoopQueuedTipX = beforeX;
                    sCoopQueuedTipY = beforeY;
                    switch (Coop_FacingToDirection(facing))
                    {
                    case DIR_NORTH: sCoopQueuedTipY--; break;
                    case DIR_SOUTH: sCoopQueuedTipY++; break;
                    case DIR_WEST:  sCoopQueuedTipX--; break;
                    case DIR_EAST:  sCoopQueuedTipX++; break;
                    }
                    sCoopQueueTipValid = TRUE;
                }
            }
            else if (!(x == 0 && y == 0))
            {
                u8 faceAction = GetFaceDirectionMovementAction(Coop_FacingToDirection((u8)gHnsCoopMailbox.remoteFacing));

                linkDirection(obj) = (u8)gHnsCoopMailbox.remoteFacing;
                ObjectEventSetHeldMovement(obj, faceAction);
            }
        }
        ObjectEventUpdateElevation(obj, NULL);
    }
}

static struct ObjectEvent *Coop_GetRemoteFollowerObject(void)
{
    u32 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].localId == OBJ_EVENT_ID_REMOTE_FOLLOWER)
            return &gObjectEvents[i];
    }
    return NULL;
}

static void Coop_DestroyRemoteFollower(void)
{
    struct ObjectEvent *follower = Coop_GetRemoteFollowerObject();

    if (follower != NULL)
        RemoveObjectEvent(follower);
    // RemoveObjectEvent no-ops on the reserved id; ForceClear actually frees
    // it. Skip when already empty so CB1 does not memset every frame.
    if (gObjectEvents[COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID].active)
        Coop_ForceClearObjectEventSlot(COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID);
}

static struct ObjectEvent *Coop_GetRemotePartnerObject(void)
{
    u8 i;
    u8 selfId;

    if (!IsOverworldLinkActive())
    {
        struct LinkPlayerObjectEvent *remote = &gLinkPlayerObjectEvents[COOP_PRESENCE_LINK_ID];

        if (remote->active)
            return &gObjectEvents[remote->objEventId];
        return NULL;
    }

    selfId = gLocalLinkPlayerId;
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (i == selfId)
            continue;
        if (gLinkPlayerObjectEvents[i].active)
            return &gObjectEvents[gLinkPlayerObjectEvents[i].objEventId];
    }
    return NULL;
}

// Spawn / walk / hide the peer's follower one tile behind their avatar.
// Walk toward the partner; snap only on large desync (≥ COOP_PRESENCE_SNAP_TILES).
// Same-map only via Coop_IsPeerVisible.
static void Coop_UpdateRemoteFollower(void)
{
    struct ObjectEvent *follower;
    struct ObjectEvent *partner;
    u32 packed = gHnsCoopMailbox.remoteFollower;
    u16 graphicsId;
    s16 targetX, targetY;
    s16 dx, dy;
    enum Direction facing;
    enum Direction stepDir;
    u8 elevation;
    u8 objId;

    if (!Coop_IsPeerVisible()
     || packed == 0
     || (packed & HNS_COOP_FOLLOWER_VISIBLE) == 0)
    {
        Coop_DestroyRemoteFollower();
        return;
    }

    graphicsId = (u16)(packed & HNS_COOP_FOLLOWER_GFX_MASK);
    if (!(graphicsId & OBJ_EVENT_MON)
     || SpeciesToGraphicsInfo(graphicsId & OBJ_EVENT_MON_SPECIES_MASK,
                              graphicsId & OBJ_EVENT_MON_SHINY,
                              graphicsId & OBJ_EVENT_MON_FEMALE) == NULL)
    {
        Coop_DestroyRemoteFollower();
        return;
    }

    partner = Coop_GetRemotePartnerObject();
    if (partner != NULL && partner->active)
    {
        targetX = partner->currentCoords.x;
        targetY = partner->currentCoords.y;
        facing = linkDirection(partner);
        elevation = partner->currentElevation;
    }
    else
    {
        targetX = (s16)gHnsCoopMailbox.remoteX;
        targetY = (s16)gHnsCoopMailbox.remoteY;
        facing = (enum Direction)gHnsCoopMailbox.remoteFacing;
        elevation = 3;
        if (targetX == 0 && targetY == 0)
        {
            Coop_DestroyRemoteFollower();
            return;
        }
    }

    if (facing == DIR_NONE)
        facing = DIR_SOUTH;
    // One tile behind the partner, opposite of their facing (same idea as
    // local followers tracking the avatar's previous tile).
    MoveCoords(GetOppositeDirection(facing), &targetX, &targetY);

    follower = Coop_GetRemoteFollowerObject();
    if (follower != NULL)
    {
        bool8 dead = follower->spriteId >= MAX_SPRITES
            || !gSprites[follower->spriteId].inUse;
        bool8 wrongSlot = ((u8)(follower - gObjectEvents) != COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID);

        if (dead || wrongSlot || follower->graphicsId != graphicsId)
        {
            // Dead/hidden after menu/battle/cutscene, wrong slot, or gfx change:
            // destroy and re-spawn into the reserved slot this frame.
            Coop_DestroyRemoteFollower();
            follower = NULL;
        }
    }

    if (follower == NULL)
    {
        struct ObjectEventTemplate template =
        {
            .localId = OBJ_EVENT_ID_REMOTE_FOLLOWER,
            .graphicsId = graphicsId,
            .flagId = 0,
            .x = targetX - MAP_OFFSET,
            .y = targetY - MAP_OFFSET,
            .elevation = elevation,
            // Idle type; co-op code drives steps via held movements below.
            .movementType = MOVEMENT_TYPE_FACE_DOWN,
        };

        Coop_ForceClearObjectEventSlot(COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID);
        Coop_PreferObjectEventSlot(COOP_REMOTE_FOLLOWER_OBJ_EVENT_ID);
        objId = SpawnSpecialObjectEvent(&template);
        if (objId >= OBJECT_EVENTS_COUNT)
            return;
        follower = &gObjectEvents[objId];
        ObjectEventTurn(follower, facing);
        if (follower->spriteId < MAX_SPRITES)
            gSprites[follower->spriteId].invisible = FALSE;
        return;
    }

    if (follower->spriteId < MAX_SPRITES)
        gSprites[follower->spriteId].invisible = FALSE;
    ObjectEventClearHeldMovementIfFinished(follower);
    dx = targetX - follower->currentCoords.x;
    dy = targetY - follower->currentCoords.y;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;

    if (dx + dy >= COOP_PRESENCE_SNAP_TILES)
    {
        // Large desync only (warp / resync): teleport. Smaller gaps walk.
        ObjectEventClearHeldMovementIfActive(follower);
        MoveObjectEventToMapCoords(follower, targetX, targetY);
        ObjectEventTurn(follower, facing);
        ObjectEventUpdateElevation(follower, NULL);
        return;
    }

    if (dx == 0 && dy == 0)
    {
        if (!ObjectEventIsMovementOverridden(follower))
            ObjectEventTurn(follower, facing);
        ObjectEventUpdateElevation(follower, NULL);
        return;
    }

    // Exactly one tile behind the target: take a vanilla walk step.
    if (ObjectEventIsMovementOverridden(follower))
        return;

    if (dx >= dy)
        stepDir = (targetX > follower->currentCoords.x) ? DIR_EAST : DIR_WEST;
    else
        stepDir = (targetY > follower->currentCoords.y) ? DIR_SOUTH : DIR_NORTH;
    ObjectEventSetHeldMovement(follower, GetWalkNormalMovementAction(stepDir));
    ObjectEventUpdateElevation(follower, NULL);
}
#else
bool8 Coop_ArePlayersOnSameMap(void) { return FALSE; }
bool8 Coop_IsPeerVisible(void) { return FALSE; }
void Coop_SetSioActive(bool8 active) { (void)active; }
#endif

u16 Script_IsCoopLinkActive(void)
{
    return IsCoopLinkActive();
}

u16 Script_CoopArePlayersOnSameMap(void)
{
    return Coop_ArePlayersOnSameMap();
}

bool32 IsOverworldLinkActive(void)
{
    if (gMain.callback1 == CB1_OverworldLink)
        return TRUE;
    else
        return FALSE;
}

static void DoCB1_Overworld(u16 newKeys, u16 heldKeys)
{
    struct FieldInput inputStruct;

    UpdatePlayerAvatarTransitionState();
    FieldClearPlayerInput(&inputStruct);
    FieldGetPlayerInput(&inputStruct, newKeys, heldKeys);
    CancelSignPostMessageBox(&inputStruct);
    if (!ArePlayerFieldControlsLocked())
    {
        if (ProcessPlayerFieldInput(&inputStruct) == 1)
        {
            LockPlayerFieldControls();
            HideMapNamePopUpWindow();
        }
        else
        {
            PlayerStep(inputStruct.dpadDirection, newKeys, heldKeys);
        }
    }
    // If stop running but keep holding B -> fix follower frame.
    if (PlayerHasFollowerNPC() && (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ON_FOOT) && IsPlayerStandingStill())
        ObjectEventSetHeldMovement(&gObjectEvents[GetFollowerNPCObjectId()], GetFaceDirectionAnimNum(gObjectEvents[GetFollowerNPCObjectId()].facingDirection));
}

void CB1_Overworld(void)
{
    if (gMain.callback2 == CB2_Overworld)
    {
        if (UpdateCoopLink())
            return;
        DoCB1_Overworld(gMain.newKeys, gMain.heldKeys);
#if IS_HNS
        // Publish presence and show the peer avatar even when the GBA SIO
        // link never completes. The vanilla link flow above owns this once
        // CB1 switches to CB1_OverworldLink. Story sync also runs here so
        // presence-only sessions still exchange shared adventure flags/vars.
        if (!sCoopWasOnField)
        {
            // Wake from battle/menu/special screen: restart spawn delay and
            // clear stale movement so the partner re-enters cleanly.
            sCoopFramesInOverworld = 0;
            sCoopSpawnCooldown = 0;
            sCoopDoorState = COOP_DOOR_NONE;
            Coop_ClearPresenceStepQueue();
            sCoopWasOnField = TRUE;
        }
        if (sCoopFramesInOverworld < 0xFFFF)
            sCoopFramesInOverworld++;
        gHnsCoopMailbox.localMapId = (u32)gSaveBlock1Ptr->location.mapGroup
                                  | ((u32)gSaveBlock1Ptr->location.mapNum << 16);
        Coop_ProcessStorySyncFrame();
        Coop_PublishLocalPosition();
        Coop_UpdateRemotePresence();
        Coop_SnapRemoteLinkPlayers();
        Coop_UpdateRemoteFollower();
        Coop_ScrubSaveStaging();
#endif
    }
#if IS_HNS
    else if (sCoopWasOnField)
    {
        // Sleep presence locomotion while battles/menus/special screens run.
        Coop_ClearPresenceStepQueue();
        sCoopDoorState = COOP_DOOR_NONE;
        sCoopWasOnField = FALSE;
        Coop_ScrubSaveStaging();
    }
#endif
}

#define TINT_NIGHT Q_8_8(0.456) | Q_8_8(0.456) << 8 | Q_8_8(0.615) << 16

const struct BlendSettings gTimeOfDayBlend[] =
{
    [TIME_MORNING] = {.coeff = 4,  .blendColor = 0xA8B0E0,   .isTint = TRUE},
    [TIME_DAY]     = {.coeff = 0,  .blendColor = 0,          .isTint = FALSE},
    [TIME_EVENING] = {.coeff = 4,  .blendColor = 0xA8B0E0,   .isTint = TRUE},
    [TIME_NIGHT]   = {.coeff = 10, .blendColor = TINT_NIGHT, .isTint = TRUE},
};

#define DEFAULT_WEIGHT 256
#define TIME_BLEND_WEIGHT(begin, end) (DEFAULT_WEIGHT - SAFE_DIV((DEFAULT_WEIGHT * ((hours - begin) * MINUTES_PER_HOUR + minutes)), ((end - begin) * MINUTES_PER_HOUR)))

#define MORNING_HOUR_MIDDLE (MORNING_HOUR_BEGIN + ((MORNING_HOUR_END - MORNING_HOUR_BEGIN) / 2))

void UpdateTimeOfDay(void)
{
    s32 hours, minutes;
    RtcCalcLocalTime();
    hours = sHoursOverride ? sHoursOverride : gLocalTime.hours;
    minutes = sHoursOverride ? 0 : gLocalTime.minutes;

    if (IsBetweenHours(hours, MORNING_HOUR_BEGIN, MORNING_HOUR_MIDDLE)) // night->morning
    {
        gTimeBlend.startBlend = gTimeOfDayBlend[TIME_NIGHT];
        gTimeBlend.endBlend = gTimeOfDayBlend[TIME_MORNING];
        gTimeBlend.weight = TIME_BLEND_WEIGHT(MORNING_HOUR_BEGIN, MORNING_HOUR_MIDDLE);
        gTimeBlend.altWeight = (DEFAULT_WEIGHT - gTimeBlend.weight) / 2;
        gTimeOfDay = TIME_MORNING;
    }
    else if (IsBetweenHours(hours, MORNING_HOUR_MIDDLE, MORNING_HOUR_END)) // morning->day
    {
        gTimeBlend.startBlend = gTimeOfDayBlend[TIME_MORNING];
        gTimeBlend.endBlend = gTimeOfDayBlend[TIME_DAY];
        gTimeBlend.weight = TIME_BLEND_WEIGHT(MORNING_HOUR_MIDDLE, MORNING_HOUR_END);
        gTimeBlend.altWeight = (DEFAULT_WEIGHT - gTimeBlend.weight) / 2 + (DEFAULT_WEIGHT / 2);
        gTimeOfDay = TIME_MORNING;
    }
    else if (IsBetweenHours(hours, EVENING_HOUR_BEGIN, EVENING_HOUR_END)) // evening
    {
        gTimeBlend.startBlend = gTimeOfDayBlend[TIME_DAY];
        gTimeBlend.endBlend = gTimeOfDayBlend[TIME_EVENING];
        gTimeBlend.weight = TIME_BLEND_WEIGHT(EVENING_HOUR_BEGIN, EVENING_HOUR_END);
        gTimeBlend.altWeight = gTimeBlend.weight / 2 + (DEFAULT_WEIGHT / 2);
        gTimeOfDay = TIME_EVENING;
    }
    else if (IsBetweenHours(hours, NIGHT_HOUR_BEGIN, NIGHT_HOUR_BEGIN + 1)) // evening->night
    {
        gTimeBlend.startBlend = gTimeOfDayBlend[TIME_EVENING];
        gTimeBlend.endBlend = gTimeOfDayBlend[TIME_NIGHT];
        gTimeBlend.weight = TIME_BLEND_WEIGHT(NIGHT_HOUR_BEGIN, NIGHT_HOUR_BEGIN + 1);
        gTimeBlend.altWeight = gTimeBlend.weight / 2;
        gTimeOfDay = TIME_NIGHT;
    }
    else if (IsBetweenHours(hours, NIGHT_HOUR_BEGIN, NIGHT_HOUR_END)) // night
    {
        gTimeBlend.weight = DEFAULT_WEIGHT;
        gTimeBlend.altWeight = 0;
        gTimeBlend.startBlend = gTimeBlend.endBlend = gTimeOfDayBlend[TIME_NIGHT];
        gTimeOfDay = TIME_NIGHT;
    }
    else // day
    {
        gTimeBlend.weight = gTimeBlend.altWeight = DEFAULT_WEIGHT;
        gTimeBlend.startBlend = gTimeBlend.endBlend = gTimeOfDayBlend[TIME_DAY];
        gTimeOfDay = TIME_DAY;
    }

#if IS_HNS
    // Keep HnS day/night encounter flags aligned with the active encounter table.
    if (gTimeOfDay == TIME_NIGHT)
    {
        FlagClear(FLAG_NIGHT_POKEMON);
        FlagSet(FLAG_DAY_POKEMON);
    }
    else
    {
        FlagSet(FLAG_NIGHT_POKEMON);
        FlagClear(FLAG_DAY_POKEMON);
    }
#endif
}

#undef MORNING_HOUR_MIDDLE
#undef TIME_BLEND_WEIGHT
#undef DEFAULT_WEIGHT

// The apparent hour of day, honouring the debug time-of-day override.
u32 GetTimeOfDayHours(void)
{
    RtcCalcLocalTime();
    return sHoursOverride ? sHoursOverride : gLocalTime.hours;
}

// Whether a map type is naturally lit/outside
bool32 MapHasNaturalLight(enum MapType mapType)
{
    return (OW_ENABLE_DNS
         && (mapType == MAP_TYPE_TOWN
          || mapType == MAP_TYPE_CITY
          || mapType == MAP_TYPE_ROUTE
          || mapType == MAP_TYPE_OCEAN_ROUTE));
}

bool32 CurrentMapHasShadows(void)
{
    // Add all conditionals here for maps that shouldn't have shadows
    // By default only cave maps are excluded from having shadows under object events
    return (gMapHeader.mapType != MAP_TYPE_UNDERGROUND);
}

// Update & mix day / night bg palettes (into unfaded)
void UpdateAltBgPalettes(u16 palettes)
{
    const struct Tileset *primary = gMapHeader.mapLayout->primaryTileset;
    const struct Tileset *secondary = gMapHeader.mapLayout->secondaryTileset;
    u32 i = 1;
    if (!MapHasNaturalLight(gMapHeader.mapType))
        return;
    palettes &= ~((1 << GetNumPalsInPrimary(gMapHeader.mapLayout)) - 1) | primary->swapPalettes;
    palettes &= ((1 << GetNumPalsInPrimary(gMapHeader.mapLayout)) - 1) | (secondary->swapPalettes << GetNumPalsInPrimary(gMapHeader.mapLayout));
    palettes &= PALETTES_MAP ^ (1 << 0); // don't blend palette 0, [13,15]
    palettes >>= 1; // start at palette 1
    if (!palettes)
        return;
    while (palettes)
    {
        if (palettes & 1)
        {
            if (i < GetNumPalsInPrimary(gMapHeader.mapLayout))
                AvgPaletteWeighted(&((u16 *)primary->palettes)[i * 16], &((u16 *)primary->palettes)[((i + 9) % 16) * 16], gPlttBufferUnfaded + i * 16, gTimeBlend.altWeight);
            else
                AvgPaletteWeighted(&((u16 *)secondary->palettes)[i * 16], &((u16 *)secondary->palettes)[((i + 9) % 16) * 16], gPlttBufferUnfaded + i * 16, gTimeBlend.altWeight);
        }
        i++;
        palettes >>= 1;
    }
}

void UpdatePalettesWithTime(u32 palettes)
{
    if (!MapHasNaturalLight(gMapHeader.mapType))
        return;
    u32 i;
    u32 mask = 1 << 16;
    if (palettes >= (1 << 16))
        for (i = 0; i < 16; i++, mask <<= 1)
        {
            if (IS_BLEND_IMMUNE_TAG(GetSpritePaletteTagByPaletteNum(i)))
                palettes &= ~(mask);
        }

    palettes &= PALETTES_MAP | PALETTES_OBJECTS; // Don't blend UI pals
    if (!palettes)
        return;
    TimeMixPalettes(palettes, gPlttBufferUnfaded, gPlttBufferFaded, &gTimeBlend.startBlend, &gTimeBlend.endBlend, gTimeBlend.weight);
}

u8 UpdateSpritePaletteWithTime(u8 paletteNum)
{
    if (MapHasNaturalLight(gMapHeader.mapType)
     && !IS_BLEND_IMMUNE_TAG(GetSpritePaletteTagByPaletteNum(paletteNum)))
        TimeMixPalettes(1, &gPlttBufferUnfaded[OBJ_PLTT_ID(paletteNum)], &gPlttBufferFaded[OBJ_PLTT_ID(paletteNum)], &gTimeBlend.startBlend, &gTimeBlend.endBlend, gTimeBlend.weight);
    return paletteNum;
}

static void OverworldBasic(void)
{
    ScriptContext_RunScript();
    RunTasks();
    AnimateSprites();
    CameraUpdate();
    UpdateCameraPanning();
    BuildOamBuffer();
    UpdatePaletteFade();
    UpdateTilesetAnimations();
    DoScheduledBgTilemapCopiesToVram();
    // Every minute if no palette fade is active, update TOD blending as needed
    if (!gPaletteFade.active && --gTimeUpdateCounter <= 0)
    {
        struct TimeBlendSettings cachedBlend = gTimeBlend;
        u32 *bld0 = (u32*)&cachedBlend;
        u32 *bld1 = (u32*)&gTimeBlend;
        gTimeUpdateCounter = (SECONDS_PER_MINUTE * 60 / FakeRtc_GetSecondsRatio());
        UpdateTimeOfDay();
        FormChangeTimeUpdate();
        if (MapHasNaturalLight(gMapHeader.mapType) &&
           (bld0[0] != bld1[0]
         || bld0[1] != bld1[1]
         || bld0[2] != bld1[2]))
        {
            ApplyWeatherColorMapIfIdle(gWeatherPtr->colorMapIndex);
        }
    }
}

// This CB2 is used when starting
void CB2_OverworldBasic(void)
{
    OverworldBasic();
}

void CB2_Overworld(void)
{
    bool32 fading = (gPaletteFade.active != 0);
    if (fading)
        SetVBlankCallback(NULL);
    OverworldBasic();
    if (fading)
    {
        SetFieldVBlankCallback();
        return;
    }
}

void SetMainCallback1(MainCallback cb)
{
    gMain.callback1 = cb;
}

// This function is never called.
void SetUnusedCallback(void *func)
{
    sUnusedOverworldCallback = func;
}

static bool8 RunFieldCallback(void)
{
    if (gFieldCallback2)
    {
        if (!gFieldCallback2())
        {
            return FALSE;
        }
        else
        {
            gFieldCallback2 = NULL;
            gFieldCallback = NULL;
        }
    }
    else
    {
        if (gFieldCallback)
            gFieldCallback();
        else
            FieldCB_DefaultWarpExit();

        gFieldCallback = NULL;
    }

    return TRUE;
}

void CB2_NewGame(void)
{
    FieldClearVBlankHBlankCallbacks();
    StopMapMusic();
    ResetSafariZoneFlag_();
    NewGameInitData();
    ResetInitialPlayerAvatarState();
    PlayTimeCounter_Start();
    ScriptContext_Init();
    UnlockPlayerFieldControls();
    if (IS_FRLG || IS_HNS)
        gFieldCallback = FieldCB_WarpExitFadeFromBlack;
    else
        gFieldCallback = ExecuteTruckSequence;
    gFieldCallback2 = NULL;
    DoMapLoadLoop(&gMain.state);
    SetFieldVBlankCallback();
    SetMainCallback1(CB1_Overworld);
    SetMainCallback2(CB2_Overworld);
    if (UseFakeRtc())
        RtcCalcLocalTimeOffset(0, 10, 0, 0);
}

void CB2_WhiteOut(void)
{
    u8 state;

    if (++gMain.state >= 120)
    {
        FieldClearVBlankHBlankCallbacks();
        StopMapMusic();
        if (gSaveBlock3Ptr->challengeSettings.tx_Challenges_NuzlockeHardcore && !FlagGet(FLAG_END_NUZLOCKE))
        {
            ClearSaveData();
            DoSoftReset();
        }
        ResetSafariZoneFlag_();
        DoWhiteOut();
        ResetInitialPlayerAvatarState();
        ScriptContext_Init();
        UnlockPlayerFieldControls();
        if (IsWhiteoutCutscene())
            gFieldCallback = FieldCB_RushInjuredPokemonToCenter;
        else
            gFieldCallback = FieldCB_WarpExitFadeFromBlack;
        state = 0;
        SetFollowerNPCData(FNPC_DATA_SURF_BLOB, FNPC_SURF_BLOB_NONE);
        DoMapLoadLoop(&state);
        SetFieldVBlankCallback();
        SetMainCallback1(CB1_Overworld);
        SetMainCallback2(CB2_Overworld);
    }
}

void CB2_BugContestWhiteOut(void)
{
    u8 state;

    if (++gMain.state >= 120)
    {
        FieldClearVBlankHBlankCallbacks();
        StopMapMusic();
        ResetSafariZoneFlag_();
        ResetInitialPlayerAvatarState();
        ScriptContext_Init();
        UnlockPlayerFieldControls();
        gFieldCallback = FieldCB_WarpExitFadeFromBlack;
        state = 0;
        DoMapLoadLoop(&state);
        SetFieldVBlankCallback();
        SetMainCallback1(CB1_Overworld);
        SetMainCallback2(CB2_Overworld);
        ScriptContext_SetupScript(BugContest_EventScript_WhiteOut);
    }
}

void CB2_LoadMap(void)
{
    FieldClearVBlankHBlankCallbacks();
    ScriptContext_Init();
    UnlockPlayerFieldControls();
    SetMainCallback1(NULL);
    SetMainCallback2(CB2_DoChangeMap);
    gMain.savedCallback = CB2_LoadMap2;
}

static void CB2_LoadMap2(void)
{
#if IS_HNS
    // Keep link-player objects alive across ordinary warps only after the
    // initial GBA handshake has created both players. Before gameReady, use
    // the local loader so it cannot follow an unspawned link-player sprite.
    if (sCoopLinkStarted && gHnsCoopMailbox.gameReady)
    {
        if (LoadMapInStepsLink(&gMain.state))
        {
            SetFieldVBlankCallback();
            SetMainCallback1(CB1_OverworldLink);
            SetMainCallback2(CB2_Overworld);
        }
        return;
    }
#endif
    DoMapLoadLoop(&gMain.state);
    SetFieldVBlankCallback();
    SetMainCallback1(CB1_Overworld);
    SetMainCallback2(CB2_Overworld);
}

void CB2_ReturnToFieldContestHall(void)
{
    if (!gMain.state)
    {
        FieldClearVBlankHBlankCallbacks();
        ScriptContext_Init();
        UnlockPlayerFieldControls();
        SetMainCallback1(NULL);
    }
    if (LoadMapInStepsLocal(&gMain.state, TRUE))
    {
        SetFieldVBlankCallback();
        SetMainCallback1(CB1_Overworld);
        SetMainCallback2(CB2_Overworld);
    }
}

void CB2_ReturnToFieldCableClub(void)
{
    FieldClearVBlankHBlankCallbacks();
    gFieldCallback = FieldCB_ReturnToFieldWirelessLink;
    SetMainCallback2(CB2_LoadMapOnReturnToFieldCableClub);
}

static void CB2_LoadMapOnReturnToFieldCableClub(void)
{
    if (LoadMapInStepsLink(&gMain.state))
    {
        SetFieldVBlankCallback();
        SetMainCallback1(CB1_OverworldLink);
        ResetAllMultiplayerState();
        SetMainCallback2(CB2_Overworld);
    }
}

void CB2_ReturnToField(void)
{
    if (IsOverworldLinkActive() == TRUE)
    {
        SetMainCallback2(CB2_ReturnToFieldLink);
    }
    else
    {
        FieldClearVBlankHBlankCallbacks();
        SetMainCallback2(CB2_ReturnToFieldLocal);
    }
}

static void CB2_ReturnToFieldLocal(void)
{
    if (ReturnToFieldLocal(&gMain.state))
    {
        SetFieldVBlankCallback();
        SetMainCallback2(CB2_Overworld);
    }
}

static void CB2_ReturnToFieldLink(void)
{
    if (!Overworld_IsRecvQueueAtMax() && ReturnToFieldLink(&gMain.state))
        SetMainCallback2(CB2_Overworld);
}

void CB2_ReturnToFieldFromMultiplayer(void)
{
    FieldClearVBlankHBlankCallbacks();
    StopMapMusic();
    SetMainCallback1(CB1_OverworldLink);
    ResetAllMultiplayerState();

    if (gWirelessCommType != 0)
        gFieldCallback = FieldCB_ReturnToFieldWirelessLink;
    else
        gFieldCallback = FieldCB_ReturnToFieldCableLink;

    ScriptContext_Init();
    UnlockPlayerFieldControls();
    CB2_ReturnToField();
}

void CB2_ReturnToFieldWithOpenMenu(void)
{
    FieldClearVBlankHBlankCallbacks();
    gFieldCallback2 = FieldCB_ReturnToFieldOpenStartMenu;
    CB2_ReturnToField();
}

void CB2_ReturnToFieldContinueScript(void)
{
    FieldClearVBlankHBlankCallbacks();
    gFieldCallback = FieldCB_ContinueScript;
    CB2_ReturnToField();
}

void CB2_ReturnToFieldContinueScriptPlayMapMusic(void)
{
    FieldClearVBlankHBlankCallbacks();
    gFieldCallback = FieldCB_ContinueScriptHandleMusic;
    CB2_ReturnToField();
}

void CB2_ReturnToFieldFadeFromBlack(void)
{
    FieldClearVBlankHBlankCallbacks();
    gFieldCallback = FieldCB_WarpExitFadeFromBlack;
    CB2_ReturnToField();
}

static void FieldCB_FadeTryShowMapPopup(void)
{
    if (gMapHeader.showMapName == TRUE && SecretBaseMapPopupEnabled() == TRUE)
        ShowMapNamePopup();
    FieldCB_WarpExitFadeFromBlack();
}

void CB2_ContinueSavedGame(void)
{
    u8 trainerHillMapId;

    FieldClearVBlankHBlankCallbacks();
    StopMapMusic();
    ResetSafariZoneFlag_();
    if (gSaveFileStatus == SAVE_STATUS_ERROR)
        ResetWinStreaks();

    LoadSaveblockMapHeader();
    ClearDiveAndHoleWarps();
    trainerHillMapId = GetCurrentTrainerHillMapId();
    if ((gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR || gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR_HNS))
        LoadBattlePyramidFloorObjectEventScripts();
    else if (trainerHillMapId != 0 && trainerHillMapId != TRAINER_HILL_ENTRANCE)
        LoadTrainerHillFloorObjectEventScripts();
    else
        LoadSaveblockObjEventScripts();

    UnfreezeObjectEvents();
    DoTimeBasedEvents();
    UpdateMiscOverworldStates();
    if ((gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR || gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR_HNS))
        InitBattlePyramidMap(TRUE);
    else if (trainerHillMapId != 0)
        InitTrainerHillMap();
    else
        InitMapFromSavedGame();

    PlayTimeCounter_Start();
    ScriptContext_Init();
    UnlockPlayerFieldControls();
    gExitStairsMovementDisabled = TRUE;
    InitMatchCallCounters();
    if (UseContinueGameWarp() == TRUE)
    {
        ClearContinueGameWarpStatus();
        SetWarpDestinationToContinueGameWarp();
        WarpIntoMap();
        TryPutTodaysRivalTrainerOnAir();
        SetMainCallback2(CB2_LoadMap);
    }
    else
    {
        TryPutTodaysRivalTrainerOnAir();
        gFieldCallback = FieldCB_FadeTryShowMapPopup;
        SetMainCallback1(CB1_Overworld);
        CB2_ReturnToField();
    }
}

static void FieldClearVBlankHBlankCallbacks(void)
{
    if (UsedPokemonCenterWarp() == TRUE)
        CloseLink();

    if (gWirelessCommType != 0)
    {
        EnableInterrupts(INTR_FLAG_VBLANK | INTR_FLAG_VCOUNT | INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
        DisableInterrupts(INTR_FLAG_HBLANK);
    }
    else
    {
        u16 savedIme = REG_IME;
        REG_IME = 0;
        REG_IE &= ~INTR_FLAG_HBLANK;
        REG_IE |= INTR_FLAG_VBLANK;
        REG_IME = savedIme;
    }

    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
}

static void SetFieldVBlankCallback(void)
{
    SetVBlankCallback(VBlankCB_Field);
}

static void VBlankCB_Field(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    ScanlineEffect_InitHBlankDmaTransfer();
    FieldUpdateBgTilemapScroll();
    TransferPlttBuffer();
    TransferTilesetAnimsBuffer();
}

static void InitCurrentFlashLevelScanlineEffect(void)
{
    u8 flashLevel;

    if (InBattlePyramid_())
    {
        WriteBattlePyramidViewScanlineEffectBuffer();
        ScanlineEffect_SetParams(sFlashEffectParams);
    }
    else if ((flashLevel = GetFlashLevel()))
    {
        WriteFlashScanlineEffectBuffer(flashLevel);
        ScanlineEffect_SetParams(sFlashEffectParams);
    }
}

static bool32 LoadMapInStepsLink(u8 *state)
{
    switch (*state)
    {
    case 0:
        InitOverworldBgs();
        ScriptContext_Init();
        UnlockPlayerFieldControls();
        ResetMirageTowerAndSaveBlockPtrs();
        ResetScreenForMapLoad();
        (*state)++;
        break;
    case 1:
        gExitStairsMovementDisabled = FALSE;
        LoadMapFromWarp(TRUE);
        (*state)++;
        break;
    case 2:
        ResumeMap(TRUE);
        (*state)++;
        break;
    case 3:
        OffsetCameraFocusByLinkPlayerId();
        InitObjectEventsLink();
        SpawnLinkPlayers();
        SetCameraToTrackGuestPlayer();
        (*state)++;
        break;
    case 4:
        InitCurrentFlashLevelScanlineEffect();
        InitOverworldGraphicsRegisters();
        InitTextBoxGfxAndPrinters();
        (*state)++;
        break;
    case 5:
        ResetFieldCamera();
        (*state)++;
        break;
    case 6:
        CopyPrimaryTilesetToVram(gMapHeader.mapLayout);
        (*state)++;
        break;
    case 7:
        CopySecondaryTilesetToVram(gMapHeader.mapLayout);
        (*state)++;
        break;
    case 8:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LoadMapTilesetPalettes(gMapHeader.mapLayout);
            (*state)++;
        }
        break;
    case 9:
        DrawWholeMapView();
        (*state)++;
        break;
    case 10:
        InitTilesetAnimations();
        (*state)++;
        break;
    case 11:
        if (gWirelessCommType != 0)
        {
            LoadWirelessStatusIndicatorSpriteGfx();
            CreateWirelessStatusIndicatorSprite(0, 0);
        }
        (*state)++;
        break;
    case 12:
        if (RunFieldCallback())
            (*state)++;
        break;
    case 13:
        return TRUE;
    }

    return FALSE;
}

static bool32 LoadMapInStepsLocal(u8 *state, bool32 a2)
{
    switch (*state)
    {
    case 0:
        FieldClearVBlankHBlankCallbacks();
        LoadMapFromWarp(a2);
        (*state)++;
        break;
    case 1:
        ResetMirageTowerAndSaveBlockPtrs();
        ResetScreenForMapLoad();
        (*state)++;
        break;
    case 2:
        ResumeMap(a2);
        (*state)++;
        break;
    case 3:
        InitObjectEventsLocal();
        SetCameraToTrackPlayer();
        (*state)++;
        break;
    case 4:
        InitCurrentFlashLevelScanlineEffect();
        InitOverworldGraphicsRegisters();
        InitTextBoxGfxAndPrinters();
        (*state)++;
        break;
    case 5:
        ResetFieldCamera();
        (*state)++;
        break;
    case 6:
        CopyPrimaryTilesetToVram(gMapHeader.mapLayout);
        (*state)++;
        break;
    case 7:
        CopySecondaryTilesetToVram(gMapHeader.mapLayout);
        (*state)++;
        break;
    case 8:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LoadMapTilesetPalettes(gMapHeader.mapLayout);
            (*state)++;
        }
        break;
    case 9:
        DrawWholeMapView();
        (*state)++;
        break;
    case 10:
        InitTilesetAnimations();
        (*state)++;
        break;
    case 11:
        if (gMapHeader.showMapName == TRUE && SecretBaseMapPopupEnabled() == TRUE)
            ShowMapNamePopup();
        (*state)++;
        break;
    case 12:
        if (RunFieldCallback())
            (*state)++;
        break;
    case 13:
        return TRUE;
    }

    return FALSE;
}

static bool32 ReturnToFieldLocal(u8 *state)
{
    switch (*state)
    {
    case 0:
        ResetMirageTowerAndSaveBlockPtrs();
        ResetScreenForMapLoad();
        ResumeMap(FALSE);
        InitObjectEventsReturnToField();
        if (gFieldCallback == FieldCallback_UseFly)
            RemoveFollowingPokemon();
        else
            UpdateFollowingPokemon();
        SetCameraToTrackPlayer();
        (*state)++;
        break;
    case 1:
        InitViewGraphics();
        TryLoadTrainerHillEReaderPalette();
        FollowerNPC_BindToSurfBlobOnReloadScreen();
        ResumeORASDowseFieldEffect();
        (*state)++;
        break;
    case 2:
        if (RunFieldCallback())
            (*state)++;
        break;
    case 3:
        return TRUE;
    }

    return FALSE;
}

static bool32 ReturnToFieldLink(u8 *state)
{
    switch (*state)
    {
    case 0:
        FieldClearVBlankHBlankCallbacks();
        ResetMirageTowerAndSaveBlockPtrs();
        ResetScreenForMapLoad();
        (*state)++;
        break;
    case 1:
        ResumeMap(TRUE);
        (*state)++;
        break;
    case 2:
        // Restore map objects first; this resets the object-event table.
        // Spawn linked players afterward so their object events survive.
        InitObjectEventsReturnToField();
        SpawnLinkPlayers();
        SetCameraToTrackGuestPlayer_2();
        (*state)++;
        break;
    case 3:
        InitCurrentFlashLevelScanlineEffect();
        InitOverworldGraphicsRegisters();
        InitTextBoxGfxAndPrinters();
        (*state)++;
        break;
    case 4:
        ResetFieldCamera();
        (*state)++;
        break;
    case 5:
        CopyPrimaryTilesetToVram(gMapHeader.mapLayout);
        (*state)++;
        break;
    case 6:
        CopySecondaryTilesetToVram(gMapHeader.mapLayout);
        (*state)++;
        break;
    case 7:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LoadMapTilesetPalettes(gMapHeader.mapLayout);
            (*state)++;
        }
        break;
    case 8:
        DrawWholeMapView();
        (*state)++;
        break;
    case 9:
        InitTilesetAnimations();
        (*state)++;
        break;
    case 11:
        if (gWirelessCommType != 0)
        {
            LoadWirelessStatusIndicatorSpriteGfx();
            CreateWirelessStatusIndicatorSprite(0, 0);
        }
        (*state)++;
        break;
    case 12:
        if (RunFieldCallback())
            (*state)++;
        break;
    case 10:
        (*state)++;
        break;
    case 13:
        SetFieldVBlankCallback();
        (*state)++;
        return TRUE;
    }

    return FALSE;
}

static void DoMapLoadLoop(u8 *state)
{
    while (!LoadMapInStepsLocal(state, FALSE));
}

static void ResetMirageTowerAndSaveBlockPtrs(void)
{
    ClearMirageTowerPulseBlend();
    MoveSaveBlocks_ResetHeap();
}

static void ResetScreenForMapLoad(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    ScanlineEffect_Stop();

    DmaClear16(3, PLTT + 2, PLTT_SIZE - 2);
    DmaFillLarge16(3, 0, (void *)VRAM, VRAM_SIZE, 0x1000);
    ResetOamRange(0, 128);
    LoadOam();
}

static void InitViewGraphics(void)
{
    InitCurrentFlashLevelScanlineEffect();
    InitOverworldGraphicsRegisters();
    InitTextBoxGfxAndPrinters();
    InitMapView();
}

static void InitOverworldGraphicsRegisters(void)
{
    ClearScheduledBgCopiesToVram();
    ResetTempTileDataBuffers();
    SetGpuReg(REG_OFFSET_MOSAIC, 0);
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WINOBJ_BG0);
    SetGpuReg(REG_OFFSET_WIN0H, 0xFF);
    SetGpuReg(REG_OFFSET_WIN0V, 0xFF);
    SetGpuReg(REG_OFFSET_WIN1H, 0xFFFF);
    SetGpuReg(REG_OFFSET_WIN1V, 0xFFFF);
    SetGpuReg(REG_OFFSET_BLDCNT, gOverworldBackgroundLayerFlags[1] | gOverworldBackgroundLayerFlags[2] | gOverworldBackgroundLayerFlags[3]
                               | BLDCNT_TGT2_OBJ | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(13, 7));
    InitOverworldBgs();
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    ScheduleBgCopyTilemapToVram(3);
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON
                                | DISPCNT_OBJ_1D_MAP | DISPCNT_HBLANK_INTERVAL);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
    InitFieldMessageBox();
}

static void InitOverworldGraphicsRegistersCreditsFrlg(void)
{
    ClearScheduledBgCopiesToVram();
    ResetTempTileDataBuffers();
    SetGpuReg(REG_OFFSET_MOSAIC, 0);
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WINOBJ_BG0);
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, 255));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(0, 255));
    SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(255, 255));
    SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(255, 255));
    SetGpuReg(REG_OFFSET_BLDCNT, gOverworldBackgroundLayerFlags[1] | gOverworldBackgroundLayerFlags[2] | gOverworldBackgroundLayerFlags[3]
                                 | BLDCNT_TGT2_OBJ | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(13, 7));
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    ScheduleBgCopyTilemapToVram(3);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | 0x20 | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
    ChangeBgX(0, 0, 0);
    ChangeBgY(0, 0, 0);
    ChangeBgX(1, 0, 0);
    ChangeBgY(1, 0, 0);
    ChangeBgX(2, 0, 0);
    ChangeBgY(2, 0, 0);
    ChangeBgX(3, 0, 0);
    ChangeBgY(3, 0, 0);
}

static void ResumeMap(bool32 a1)
{
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    ScanlineEffect_Clear();
    ResetAllPicSprites();
    ResetCameraUpdateInfo();
    InstallCameraPanAheadCallback();
    FreeAllSpritePalettes();

    FieldEffectActiveListClear();
    StartWeather();
    ResumePausedWeather();
    if (!a1)
        SetUpFieldTasks();
    RunOnResumeMapScript();
    TryStartMirageTowerPulseBlendEffect();
}

static void InitObjectEventsLink(void)
{
    gTotalCameraPixelOffsetX = 0;
    gTotalCameraPixelOffsetY = 0;
    ResetObjectEvents();
    TrySpawnObjectEvents(0, 0);
    TryRunOnWarpIntoMapScript();
}

static void InitObjectEventsLocal(void)
{
    u16 x, y;
    struct InitialPlayerAvatarState *player;

    gTotalCameraPixelOffsetX = 0;
    gTotalCameraPixelOffsetY = 0;
    ResetObjectEvents();
    GetCameraFocusCoords(&x, &y);
    player = GetInitialPlayerAvatarState();
    InitPlayerAvatar(x, y, player->direction, gSaveBlock2Ptr->playerGender);
    SetPlayerAvatarTransitionFlags(player->transitionFlags);
    ResetInitialPlayerAvatarState();
    TrySpawnObjectEvents(0, 0);
    FollowerNPC_HandleSprite();
    UpdateFollowingPokemon();
    TryRunOnWarpIntoMapScript();
#if IS_HNS
    // Warps/cutscenes wipe link players via ResetObjectEvents; still ask Android
    // for a fresh mutual POSITION exchange as soon as the field is up.
    Coop_ClearPresenceStepQueue();
    gHnsCoopMailbox.localPresenceResync++;
#endif
}

#if IS_HNS
static void Coop_InvalidatePresenceAfterFieldReload(void)
{
    u8 i;

    // Full SIO link owns link-player objects via SpawnLinkPlayers.
    if (IsOverworldLinkActive())
        return;

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (gLinkPlayerObjectEvents[i].active)
            DestroyLinkPlayerObject(i);
    }
    Coop_DestroyRemoteFollower();
    Coop_ClearPresenceStepQueue();
    sCoopFramesInOverworld = 0;
    sCoopSpawnCooldown = 0;
    sCoopDoorState = COOP_DOOR_NONE;
    sCoopPresenceGfxState = PLAYER_AVATAR_STATE_NORMAL;
    sCoopWasOnField = FALSE;
    // Ask Android to force-publish our POSITION and request the peer's.
    gHnsCoopMailbox.localPresenceResync++;
}
#endif

static void InitObjectEventsReturnToField(void)
{
    SpawnObjectEventsOnReturnToField(0, 0);
#if IS_HNS
    // SpawnObjectEventsOnReturnToField skips link-player sprites, so the co-op
    // presence avatar would stay invisible after battles/menus. Drop it here;
    // CB1 Coop_UpdateRemotePresence respawns immediately with a fresh sprite.
    Coop_InvalidatePresenceAfterFieldReload();
#endif
    RotatingGate_InitPuzzleAndGraphics();
    RunOnReturnToFieldMapScript();
}

static void SetCameraToTrackPlayer(void)
{
    gObjectEvents[gPlayerAvatar.objectEventId].trackedByCamera = TRUE;
    InitCameraUpdateCallback(gPlayerAvatar.spriteId);
}

static void SetCameraToTrackGuestPlayer(void)
{
    InitCameraUpdateCallback(GetSpriteForLinkedPlayer(gLocalLinkPlayerId));
}

// Duplicate function.
static void SetCameraToTrackGuestPlayer_2(void)
{
    InitCameraUpdateCallback(GetSpriteForLinkedPlayer(gLocalLinkPlayerId));
}

static void OffsetCameraFocusByLinkPlayerId(void)
{
    u16 x, y;
    GetCameraFocusCoords(&x, &y);

    // This is a hack of some kind; it's undone in SpawnLinkPlayers, which is called
    // soon after this function.
    SetCameraFocusCoords(x + gLocalLinkPlayerId, y);
}

static void SpawnLinkPlayers(void)
{
    u16 i;
    u16 x, y;

    GetCameraFocusCoords(&x, &y);
    x -= gLocalLinkPlayerId;

    for (i = 0; i < gFieldLinkPlayerCount; i++)
    {
        SpawnLinkPlayerObjectEvent(i, i + x, y, gLinkPlayers[i].gender);
        CreateLinkPlayerSprite(i, gLinkPlayers[i].version);
    }

    ClearAllPlayerKeys();
}

static void CB1_OverworldLink(void)
{
#if IS_HNS
    Coop_UpdateSharedSettings();
#endif
    if (UpdateCoopLink())
        return;

#if IS_HNS
    gHnsCoopMailbox.localSioActive = 1;
    gHnsCoopMailbox.localMapId = (u32)gSaveBlock1Ptr->location.mapGroup
                               | ((u32)gSaveBlock1Ptr->location.mapNum << 16);
    Coop_ProcessStorySyncFrame();
    Coop_PublishLocalPosition();
#endif

    if (gWirelessCommType == 0 || !IsRfuRecvQueueEmpty() || !IsSendingKeysToLink())
    {
        u8 selfId = gLocalLinkPlayerId;
        UpdateAllLinkPlayers(gLinkPartnersHeldKeys, selfId);
#if IS_HNS
        Coop_SnapRemoteLinkPlayers();
        Coop_UpdateRemoteFollower();
#endif

        // Note: Because guestId is between 0 and 4, while the smallest key code is
        // LINK_KEY_CODE_EMPTY, this is functionally equivalent to `sPlayerKeyInterceptCallback(0)`.
        // It is expecting the callback to be KeyInterCB_SelfIdle, and that will
        // completely ignore any input parameters.
        //
        // UpdateHeldKeyCode performs a sanity check on its input; if
        // sPlayerKeyInterceptCallback echoes back the argument, which is selfId, then
        // it'll use LINK_KEY_CODE_EMPTY instead.
        //
        // Note 2: There are some key intercept callbacks that treat the key as a player
        // ID. It's so hacky.
        UpdateHeldKeyCode(sPlayerKeyInterceptCallback(selfId));
        ClearAllPlayerKeys();
    }
}

void ResetAllMultiplayerState(void)
{
    ResetAllPlayerLinkStates();
    SetKeyInterceptCallback(KeyInterCB_SelfIdle);
}

static void ClearAllPlayerKeys(void)
{
    ResetPlayerHeldKeys(gLinkPartnersHeldKeys);
}

static void SetKeyInterceptCallback(u16 (*func)(u32))
{
    sRfuKeepAliveTimer = 0;
    sPlayerKeyInterceptCallback = func;
}

// Once every ~60 frames, if the link state hasn't changed (timer reset by calls
// to SetKeyInterceptCallback), it does a bunch of sanity checks on the connection.
// I'm not sure if sRfuKeepAliveTimer is reset in the process, though; rfu stuff is
// still undocumented.
static void CheckRfuKeepAliveTimer(void)
{
    if (gWirelessCommType != 0 && ++sRfuKeepAliveTimer > 60)
        LinkRfu_FatalError();
}

static void ResetAllPlayerLinkStates(void)
{
    s32 i;
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
        sPlayerLinkStates[i] = PLAYER_LINK_STATE_IDLE;
}

// Returns true if all connected players are in state.
static bool32 AreAllPlayersInLinkState(u16 state)
{
    s32 i;
    s32 count = gFieldLinkPlayerCount;

    for (i = 0; i < count; i++)
        if (sPlayerLinkStates[i] != state)
            return FALSE;
    return TRUE;
}

static bool32 IsAnyPlayerInLinkState(u16 state)
{
    s32 i;
    s32 count = gFieldLinkPlayerCount;

    for (i = 0; i < count; i++)
        if (sPlayerLinkStates[i] == state)
            return TRUE;
    return FALSE;
}

static void HandleLinkPlayerKeyInput(u32 playerId, u16 key, struct CableClubPlayer *trainer, u16 *forceFacing)
{
    const u8 *script;

    if (sPlayerLinkStates[playerId] == PLAYER_LINK_STATE_IDLE)
    {
        script = TryGetTileEventScript(trainer);
        if (script)
        {
            *forceFacing = GetDirectionForEventScript(script);
            sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_BUSY;
            if (trainer->isLocalPlayer)
            {
                SetKeyInterceptCallback(KeyInterCB_DeferToEventScript);
                RunInteractLocalPlayerScript(script);
            }
            return;
        }
        if (IsAnyPlayerInLinkState(PLAYER_LINK_STATE_EXITING_ROOM) == TRUE)
        {
            sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_BUSY;
            if (trainer->isLocalPlayer)
            {
                SetKeyInterceptCallback(KeyInterCB_DeferToEventScript);
                RunTerminateLinkScript();
            }
            return;
        }

        switch (key)
        {
        case LINK_KEY_CODE_START_BUTTON:
            if (CanCableClubPlayerPressStart(trainer))
            {
                sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_BUSY;
                if (trainer->isLocalPlayer)
                {
                    SetKeyInterceptCallback(KeyInterCB_DeferToEventScript);
                    InitLinkRoomStartMenuScript();
                }
            }
            break;
        case LINK_KEY_CODE_DPAD_DOWN:
            if (PlayerIsAtSouthExit(trainer) == TRUE)
            {
                sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_BUSY;
                if (trainer->isLocalPlayer)
                {
                    SetKeyInterceptCallback(KeyInterCB_DeferToEventScript);
                    RunConfirmLeaveCableClubScript();
                }
            }
            break;
        case LINK_KEY_CODE_A_BUTTON:
            script = TryInteractWithPlayer(trainer);
            if (script)
            {
                sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_BUSY;
                if (trainer->isLocalPlayer)
                {
                    SetKeyInterceptCallback(KeyInterCB_DeferToEventScript);
                    InitMenuBasedScript(script);
                }
            }
            break;
        case LINK_KEY_CODE_HANDLE_RECV_QUEUE:
            if (IsCableClubPlayerUnfrozen(trainer))
            {
                sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_BUSY;
                if (trainer->isLocalPlayer)
                {
                    SetKeyInterceptCallback(KeyInterCB_DeferToRecvQueue);
                    InitLinkPlayerQueueScript();
                }
            }
            break;
        case LINK_KEY_CODE_HANDLE_SEND_QUEUE:
            if (IsCableClubPlayerUnfrozen(trainer))
            {
                sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_BUSY;
                if (trainer->isLocalPlayer)
                {
                    SetKeyInterceptCallback(KeyInterCB_DeferToSendQueue);
                    InitLinkPlayerQueueScript();
                }
            }
            break;
        }
    }

    switch (key)
    {
    case LINK_KEY_CODE_EXIT_ROOM:
        sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_EXITING_ROOM;
        break;
    case LINK_KEY_CODE_READY:
        sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_READY;
        break;
    case LINK_KEY_CODE_IDLE:
        sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_IDLE;
        if (trainer->isLocalPlayer)
            SetKeyInterceptCallback(KeyInterCB_SelfIdle);
        break;
    case LINK_KEY_CODE_EXIT_SEAT:
        if (sPlayerLinkStates[playerId] == PLAYER_LINK_STATE_READY)
            sPlayerLinkStates[playerId] = PLAYER_LINK_STATE_BUSY;
        break;
    }
}

static void UpdateAllLinkPlayers(u16 *keys, s32 selfId)
{
    struct CableClubPlayer trainer;
    s32 i;

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        u8 key = keys[i];
        u16 setFacing = FACING_NONE;
        LoadCableClubPlayer(i, selfId, &trainer);
        HandleLinkPlayerKeyInput(i, key, &trainer, &setFacing);
        if (sPlayerLinkStates[i] == PLAYER_LINK_STATE_IDLE)
            setFacing = GetDirectionForDpadKey(key);
        SetPlayerFacingDirection(i, setFacing);
    }
}

static void UpdateHeldKeyCode(u16 key)
{
    if (key >= LINK_KEY_CODE_EMPTY && key < LINK_KEY_CODE_UNK_8)
        gHeldKeyCodeToSend = key;
    else
        gHeldKeyCodeToSend = LINK_KEY_CODE_EMPTY;

    if (gWirelessCommType != 0
        && GetLinkSendQueueLength() > 1
        && IsOverworldLinkActive() == TRUE
        && IsSendingKeysToLink() == TRUE)
    {
        switch (key)
        {
        case LINK_KEY_CODE_EMPTY:
        case LINK_KEY_CODE_DPAD_DOWN:
        case LINK_KEY_CODE_DPAD_UP:
        case LINK_KEY_CODE_DPAD_LEFT:
        case LINK_KEY_CODE_DPAD_RIGHT:
        case LINK_KEY_CODE_START_BUTTON:
        case LINK_KEY_CODE_A_BUTTON:
            gHeldKeyCodeToSend = LINK_KEY_CODE_NULL;
            break;
        }
    }
}

static u16 KeyInterCB_ReadButtons(u32 key)
{
    if (JOY_HELD(DPAD_UP))
        return LINK_KEY_CODE_DPAD_UP;
    if (JOY_HELD(DPAD_DOWN))
        return LINK_KEY_CODE_DPAD_DOWN;
    if (JOY_HELD(DPAD_LEFT))
        return LINK_KEY_CODE_DPAD_LEFT;
    if (JOY_HELD(DPAD_RIGHT))
        return LINK_KEY_CODE_DPAD_RIGHT;
    if (JOY_NEW(START_BUTTON))
        return LINK_KEY_CODE_START_BUTTON;
    if (JOY_NEW(A_BUTTON))
        return LINK_KEY_CODE_A_BUTTON;
    return LINK_KEY_CODE_EMPTY;
}

static u16 GetDirectionForDpadKey(u16 key)
{
    switch (key)
    {
    case LINK_KEY_CODE_DPAD_RIGHT:
        return FACING_RIGHT;
    case LINK_KEY_CODE_DPAD_LEFT:
        return FACING_LEFT;
    case LINK_KEY_CODE_DPAD_UP:
        return FACING_UP;
    case LINK_KEY_CODE_DPAD_DOWN:
        return FACING_DOWN;
    default:
        return FACING_NONE;
    }
}

// Overwrites the keys with 0x11
static void ResetPlayerHeldKeys(u16 *keys)
{
    s32 i;
    for (i = 0; i < 4; i++)
        keys[i] = LINK_KEY_CODE_EMPTY;
}


static u16 KeyInterCB_SelfIdle(u32 key)
{
    if (ArePlayerFieldControlsLocked() == TRUE)
        return LINK_KEY_CODE_EMPTY;
    if (GetLinkRecvQueueLength() > 4)
        return LINK_KEY_CODE_HANDLE_RECV_QUEUE;
    if (GetLinkSendQueueLength() <= 4)
        return KeyInterCB_ReadButtons(key);
    return LINK_KEY_CODE_HANDLE_SEND_QUEUE;
}

static u16 KeyInterCB_Idle(u32 key)
{
    CheckRfuKeepAliveTimer();
    return LINK_KEY_CODE_EMPTY;
}

// Ignore the player's inputs as long as there is an event script being executed.
static u16 KeyInterCB_DeferToEventScript(u32 key)
{
    u16 retVal;
    if (ArePlayerFieldControlsLocked() == TRUE)
    {
        retVal = LINK_KEY_CODE_EMPTY;
    }
    else
    {
        retVal = LINK_KEY_CODE_IDLE;
        SetKeyInterceptCallback(KeyInterCB_Idle);
    }
    return retVal;
}

// Ignore the player's inputs as long as there are events being recived.
static u16 KeyInterCB_DeferToRecvQueue(u32 key)
{
    u16 retVal;
    if (GetLinkRecvQueueLength() >= OVERWORLD_RECV_QUEUE_MAX)
    {
        retVal = LINK_KEY_CODE_EMPTY;
    }
    else
    {
        retVal = LINK_KEY_CODE_IDLE;
        UnlockPlayerFieldControls();
        SetKeyInterceptCallback(KeyInterCB_Idle);
    }
    return retVal;
}

// Ignore the player's inputs as long as there are events being sent.
static u16 KeyInterCB_DeferToSendQueue(u32 key)
{
    u16 retVal;
    if (GetLinkSendQueueLength() > 2)
    {
        retVal = LINK_KEY_CODE_EMPTY;
    }
    else
    {
        retVal = LINK_KEY_CODE_IDLE;
        UnlockPlayerFieldControls();
        SetKeyInterceptCallback(KeyInterCB_Idle);
    }
    return retVal;
}

static u16 KeyInterCB_ExitingSeat(u32 key)
{
    CheckRfuKeepAliveTimer();
    return LINK_KEY_CODE_EMPTY;
}

static u16 KeyInterCB_Ready(u32 keyOrPlayerId)
{
    if (sPlayerLinkStates[keyOrPlayerId] == PLAYER_LINK_STATE_READY)
    {
        if (JOY_NEW(B_BUTTON))
        {
            SetKeyInterceptCallback(KeyInterCB_ExitingSeat);
            return LINK_KEY_CODE_EXIT_SEAT;
        }
        else
        {
            return LINK_KEY_CODE_EMPTY;
        }
    }
    else
    {
        CheckRfuKeepAliveTimer();
        return LINK_KEY_CODE_EMPTY;
    }
}

static u16 KeyInterCB_SetReady(u32 key)
{
    SetKeyInterceptCallback(KeyInterCB_Ready);
    return LINK_KEY_CODE_READY;
}

static u16 KeyInterCB_SendNothing(u32 key)
{
    return LINK_KEY_CODE_EMPTY;
}

static u16 KeyInterCB_WaitForPlayersToExit(u32 keyOrPlayerId)
{
    // keyOrPlayerId could be any keycode. This callback does no sanity checking
    // on the size of the key. It's assuming that it is being called from
    // CB1_OverworldLink.
    if (sPlayerLinkStates[keyOrPlayerId] != PLAYER_LINK_STATE_EXITING_ROOM)
        CheckRfuKeepAliveTimer();
    if (AreAllPlayersInLinkState(PLAYER_LINK_STATE_EXITING_ROOM) == TRUE)
    {
        ScriptContext_SetupScript(EventScript_DoLinkRoomExit);
        SetKeyInterceptCallback(KeyInterCB_SendNothing);
    }
    return LINK_KEY_CODE_EMPTY;
}

static u16 KeyInterCB_SendExitRoomKey(u32 key)
{
    SetKeyInterceptCallback(KeyInterCB_WaitForPlayersToExit);
    return LINK_KEY_CODE_EXIT_ROOM;
}

// Identical to KeyInterCB_SendNothing
static u16 KeyInterCB_InLinkActivity(u32 key)
{
    return LINK_KEY_CODE_EMPTY;
}

u32 GetCableClubPartnersReady(void)
{
    if (IsAnyPlayerInLinkState(PLAYER_LINK_STATE_EXITING_ROOM) == TRUE)
        return CABLE_SEAT_FAILED;
    if (sPlayerKeyInterceptCallback == KeyInterCB_Ready && sPlayerLinkStates[gLocalLinkPlayerId] != PLAYER_LINK_STATE_READY)
        return CABLE_SEAT_WAITING;
    if (sPlayerKeyInterceptCallback == KeyInterCB_ExitingSeat && sPlayerLinkStates[gLocalLinkPlayerId] == PLAYER_LINK_STATE_BUSY)
        return CABLE_SEAT_FAILED;
    if (AreAllPlayersInLinkState(PLAYER_LINK_STATE_READY))
        return CABLE_SEAT_SUCCESS;
    return CABLE_SEAT_WAITING;
}

static bool32 UNUSED IsAnyPlayerExitingCableClub(void)
{
    return IsAnyPlayerInLinkState(PLAYER_LINK_STATE_EXITING_ROOM);
}

u16 SetInCableClubSeat(void)
{
    SetKeyInterceptCallback(KeyInterCB_SetReady);
    return 0;
}

u16 SetLinkWaitingForScript(void)
{
    SetKeyInterceptCallback(KeyInterCB_DeferToEventScript);
    return 0;
}

// The exit room key will be sent at the next opportunity.
// The return value is meaningless.
u16 QueueExitLinkRoomKey(void)
{
    SetKeyInterceptCallback(KeyInterCB_SendExitRoomKey);
    return 0;
}

u16 SetStartedCableClubActivity(void)
{
    SetKeyInterceptCallback(KeyInterCB_InLinkActivity);
    return 0;
}

static void LoadCableClubPlayer(s32 linkPlayerId, s32 myPlayerId, struct CableClubPlayer *trainer)
{
    s16 x, y;

    trainer->playerId = linkPlayerId;
    trainer->isLocalPlayer = (linkPlayerId == myPlayerId) ? 1 : 0;
    trainer->movementMode = gLinkPlayerObjectEvents[linkPlayerId].movementMode;
    trainer->facing = GetLinkPlayerFacingDirection(linkPlayerId);
    GetLinkPlayerCoords(linkPlayerId, &x, &y);
    trainer->pos.x = x;
    trainer->pos.y = y;
    trainer->pos.elevation = GetLinkPlayerElevation(linkPlayerId);
    trainer->metatileBehavior = MapGridGetMetatileBehaviorAt(x, y);
}

static bool32 IsCableClubPlayerUnfrozen(struct CableClubPlayer *player)
{
    u8 mode = player->movementMode;
    if (mode == MOVEMENT_MODE_SCRIPTED || mode == MOVEMENT_MODE_FREE)
        return TRUE;
    else
        return FALSE;
}

// Identical to IsCableClubPlayerUnfrozen
static bool32 CanCableClubPlayerPressStart(struct CableClubPlayer *player)
{
    u8 mode = player->movementMode;
    if (mode == MOVEMENT_MODE_SCRIPTED || mode == MOVEMENT_MODE_FREE)
        return TRUE;
    else
        return FALSE;
}

static const u8 *TryGetTileEventScript(struct CableClubPlayer *player)
{
    if (player->movementMode != MOVEMENT_MODE_SCRIPTED)
        return FACING_NONE;
    return GetCoordEventScriptAtMapPosition(&player->pos);
}

static bool32 PlayerIsAtSouthExit(struct CableClubPlayer *player)
{
    if (player->movementMode != MOVEMENT_MODE_SCRIPTED && player->movementMode != MOVEMENT_MODE_FREE)
        return FALSE;
    else if (!MetatileBehavior_IsSouthArrowWarp(player->metatileBehavior))
        return FALSE;
    else if (player->facing != DIR_SOUTH)
        return FALSE;
    else
        return TRUE;
}

static const u8 *TryInteractWithPlayer(struct CableClubPlayer *player)
{
    struct MapPosition otherPlayerPos;
    u8 linkPlayerId;

    if (player->movementMode != MOVEMENT_MODE_FREE && player->movementMode != MOVEMENT_MODE_SCRIPTED)
        return FACING_NONE;

    otherPlayerPos = player->pos;
    otherPlayerPos.x += gDirectionToVectors[player->facing].x;
    otherPlayerPos.y += gDirectionToVectors[player->facing].y;
    otherPlayerPos.elevation = ELEVATION_TRANSITION;
    linkPlayerId = GetLinkPlayerIdAt(otherPlayerPos.x, otherPlayerPos.y);

    if (linkPlayerId != MAX_LINK_PLAYERS)
    {
        if (!player->isLocalPlayer)
            return CableClub_EventScript_TooBusyToNotice;
        else if (sPlayerLinkStates[linkPlayerId] != PLAYER_LINK_STATE_IDLE)
            return CableClub_EventScript_TooBusyToNotice;
        else if (!GetLinkTrainerCardColor(linkPlayerId))
            return CableClub_EventScript_ReadTrainerCard;
        else
            return CableClub_EventScript_ReadTrainerCardColored;
    }

    return GetInteractedLinkPlayerScript(&otherPlayerPos, player->metatileBehavior, player->facing);
}

// This returns which direction to force the player to look when one of
// these event scripts runs.
static u16 GetDirectionForEventScript(const u8 *script)
{
    if (script == EventScript_BattleColosseum_4P_PlayerSpot0)
        return FACING_FORCED_RIGHT;
    else if (script == EventScript_BattleColosseum_4P_PlayerSpot1)
        return FACING_FORCED_LEFT;
    else if (script == EventScript_BattleColosseum_4P_PlayerSpot2)
        return FACING_FORCED_RIGHT;
    else if (script == EventScript_BattleColosseum_4P_PlayerSpot3)
        return FACING_FORCED_LEFT;
    else if (script == EventScript_RecordCenter_Spot0)
        return FACING_FORCED_RIGHT;
    else if (script == EventScript_RecordCenter_Spot1)
        return FACING_FORCED_LEFT;
    else if (script == EventScript_RecordCenter_Spot2)
        return FACING_FORCED_RIGHT;
    else if (script == EventScript_RecordCenter_Spot3)
        return FACING_FORCED_LEFT;
    else if (script == EventScript_BattleColosseum_2P_PlayerSpot0)
        return FACING_FORCED_RIGHT;
    else if (script == EventScript_BattleColosseum_2P_PlayerSpot1)
        return FACING_FORCED_LEFT;
    else if (script == EventScript_TradeCenter_Chair0)
        return FACING_FORCED_RIGHT;
    else if (script == EventScript_TradeCenter_Chair1)
        return FACING_FORCED_LEFT;
    else
        return FACING_NONE;
}

static void InitLinkPlayerQueueScript(void)
{
    LockPlayerFieldControls();
}

static void InitLinkRoomStartMenuScript(void)
{
    PlaySE(SE_WIN_OPEN);
    ShowStartMenu();
    LockPlayerFieldControls();
}

static void RunInteractLocalPlayerScript(const u8 *script)
{
    PlaySE(SE_SELECT);
    ScriptContext_SetupScript(script);
    LockPlayerFieldControls();
}

static void RunConfirmLeaveCableClubScript(void)
{
    PlaySE(SE_WIN_OPEN);
    ScriptContext_SetupScript(EventScript_ConfirmLeaveCableClubRoom);
    LockPlayerFieldControls();
}

static void InitMenuBasedScript(const u8 *script)
{
    PlaySE(SE_SELECT);
    ScriptContext_SetupScript(script);
    LockPlayerFieldControls();
}

static void RunTerminateLinkScript(void)
{
    ScriptContext_SetupScript(EventScript_TerminateLink);
    LockPlayerFieldControls();
}

bool32 Overworld_IsRecvQueueAtMax(void)
{
    if (!IsOverworldLinkActive())
        return FALSE;
    if (GetLinkRecvQueueLength() >= OVERWORLD_RECV_QUEUE_MAX)
        sReceivingFromLink = TRUE;
    else
        sReceivingFromLink = FALSE;
    return sReceivingFromLink;
}

bool32 Overworld_RecvKeysFromLinkIsRunning(void)
{
    u8 temp;

    if (GetLinkRecvQueueLength() < OVERWORLD_RECV_QUEUE_MAX - 1)
        return FALSE;
    else if (IsOverworldLinkActive() != TRUE)
        return FALSE;
    else if (IsSendingKeysToLink() != TRUE)
        return FALSE;
    else if (sPlayerKeyInterceptCallback == KeyInterCB_DeferToRecvQueue)
        return TRUE;
    else if (sPlayerKeyInterceptCallback != KeyInterCB_DeferToEventScript)
        return FALSE;

    temp = sReceivingFromLink;
    sReceivingFromLink = FALSE;

    if (temp == TRUE)
        return TRUE;
    else if (gPaletteFade.active && gPaletteFade.softwareFadeFinishing)
        return TRUE;
    else
        return FALSE;
}

bool32 Overworld_SendKeysToLinkIsRunning(void)
{
    if (GetLinkSendQueueLength() < 2)
        return FALSE;
    else if (IsOverworldLinkActive() != TRUE)
        return FALSE;
    else if (IsSendingKeysToLink() != TRUE)
        return FALSE;
    else if (sPlayerKeyInterceptCallback == KeyInterCB_DeferToSendQueue)
        return TRUE;
    else
        return FALSE;
}

bool32 IsSendingKeysOverCable(void)
{
    if (gWirelessCommType != 0)
        return FALSE;
    else if (!IsSendingKeysToLink())
        return FALSE;
    else
        return TRUE;
}

static u32 GetLinkSendQueueLength(void)
{
    if (gWirelessCommType != 0)
        return gRfu.sendQueue.count;
    else
        return gLink.sendQueue.count;
}

static void ZeroLinkPlayerObjectEvent(struct LinkPlayerObjectEvent *linkPlayerObjEvent)
{
    memset(linkPlayerObjEvent, 0, sizeof(struct LinkPlayerObjectEvent));
}

void ClearLinkPlayerObjectEvents(void)
{
    memset(gLinkPlayerObjectEvents, 0, sizeof(gLinkPlayerObjectEvents));
}

static void ZeroObjectEvent(struct ObjectEvent *objEvent)
{
    memset(objEvent, 0, sizeof(struct ObjectEvent));
}

static void SpawnLinkPlayerObjectEvent(u8 linkPlayerId, s16 x, s16 y, u8 gender)
{
    u8 objEventId;
    struct LinkPlayerObjectEvent *linkPlayerObjEvent;
    struct ObjectEvent *objEvent;

#if IS_HNS
    // Presence partner always claims the reserved high slot so outdoor NPC
    // density and cutscene reloads cannot leave us without a free id.
    if (linkPlayerId == COOP_PRESENCE_LINK_ID && !IsOverworldLinkActive())
    {
        objEventId = COOP_PRESENCE_OBJ_EVENT_ID;
        Coop_ForceClearObjectEventSlot(objEventId);
    }
    else
#endif
    {
        objEventId = GetFirstInactiveObjectEventId();
        // GetFirstInactiveObjectEventId returns OBJECT_EVENTS_COUNT when every
        // slot is taken. Spawning then would write past gObjectEvents and
        // corrupt the table. Bail out instead; the presence update retries next
        // frame once a slot frees up.
        if (objEventId >= OBJECT_EVENTS_COUNT)
            return;
    }
    linkPlayerObjEvent = &gLinkPlayerObjectEvents[linkPlayerId];
    objEvent = &gObjectEvents[objEventId];

    ZeroLinkPlayerObjectEvent(linkPlayerObjEvent);
    ZeroObjectEvent(objEvent);

    linkPlayerObjEvent->active = TRUE;
    linkPlayerObjEvent->linkPlayerId = linkPlayerId;
    linkPlayerObjEvent->objEventId = objEventId;
    linkPlayerObjEvent->movementMode = MOVEMENT_MODE_FREE;

    objEvent->active = TRUE;
    linkGender(objEvent) = gender;
    linkDirection(objEvent) = DIR_NORTH;
    objEvent->spriteId = MAX_SPRITES;

    InitLinkPlayerObjectEventPos(objEvent, x, y);
}

static void InitLinkPlayerObjectEventPos(struct ObjectEvent *objEvent, s16 x, s16 y)
{
    objEvent->currentCoords.x = x;
    objEvent->currentCoords.y = y;
    objEvent->previousCoords.x = x;
    objEvent->previousCoords.y = y;
    SetSpritePosToMapCoords(x, y, &objEvent->initialCoords.x, &objEvent->initialCoords.y);
    objEvent->initialCoords.x += 8;
    ObjectEventUpdateElevation(objEvent, NULL);
}

static void UNUSED SetLinkPlayerObjectRange(u8 linkPlayerId, enum Direction dir)
{
    if (gLinkPlayerObjectEvents[linkPlayerId].active)
    {
        u8 objEventId = gLinkPlayerObjectEvents[linkPlayerId].objEventId;
        struct ObjectEvent *objEvent = &gObjectEvents[objEventId];
        linkDirection(objEvent) = dir;
    }
}

static void UNUSED DestroyLinkPlayerObject(u8 linkPlayerId)
{
    struct LinkPlayerObjectEvent *linkPlayerObjEvent = &gLinkPlayerObjectEvents[linkPlayerId];
    u8 objEventId = linkPlayerObjEvent->objEventId;
    struct ObjectEvent *objEvent = &gObjectEvents[objEventId];
    if (objEvent->spriteId != MAX_SPRITES)
        DestroySprite(&gSprites[objEvent->spriteId]);
    linkPlayerObjEvent->active = 0;
    objEvent->active = 0;
}

// Returns the spriteId corresponding to this player.
static u8 GetSpriteForLinkedPlayer(u8 linkPlayerId)
{
    u8 objEventId = gLinkPlayerObjectEvents[linkPlayerId].objEventId;
    struct ObjectEvent *objEvent = &gObjectEvents[objEventId];
    return objEvent->spriteId;
}

static void GetLinkPlayerCoords(u8 linkPlayerId, s16 *x, s16 *y)
{
    u8 objEventId = gLinkPlayerObjectEvents[linkPlayerId].objEventId;
    struct ObjectEvent *objEvent = &gObjectEvents[objEventId];
    *x = objEvent->currentCoords.x;
    *y = objEvent->currentCoords.y;
}

static u8 GetLinkPlayerFacingDirection(u8 linkPlayerId)
{
    u8 objEventId = gLinkPlayerObjectEvents[linkPlayerId].objEventId;
    struct ObjectEvent *objEvent = &gObjectEvents[objEventId];
    return linkDirection(objEvent);
}

static u8 GetLinkPlayerElevation(u8 linkPlayerId)
{
    u8 objEventId = gLinkPlayerObjectEvents[linkPlayerId].objEventId;
    struct ObjectEvent *objEvent = &gObjectEvents[objEventId];
    return objEvent->currentElevation;
}

static s32 UNUSED GetLinkPlayerObjectStepTimer(u8 linkPlayerId)
{
    u8 objEventId = gLinkPlayerObjectEvents[linkPlayerId].objEventId;
    struct ObjectEvent *objEvent = &gObjectEvents[objEventId];
    return 16 - (s8)objEvent->directionSequenceIndex;
}

static u8 GetLinkPlayerIdAt(s16 x, s16 y)
{
    u8 i;
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (gLinkPlayerObjectEvents[i].active
         && (gLinkPlayerObjectEvents[i].movementMode == 0 || gLinkPlayerObjectEvents[i].movementMode == 2))
        {
            struct ObjectEvent *objEvent = &gObjectEvents[gLinkPlayerObjectEvents[i].objEventId];
            if (objEvent->currentCoords.x == x && objEvent->currentCoords.y == y)
                return i;
        }
    }
    return 4;
}

static void SetPlayerFacingDirection(u8 linkPlayerId, u8 facing)
{
    struct LinkPlayerObjectEvent *linkPlayerObjEvent = &gLinkPlayerObjectEvents[linkPlayerId];
    u8 objEventId = linkPlayerObjEvent->objEventId;
    struct ObjectEvent *objEvent = &gObjectEvents[objEventId];

    if (linkPlayerObjEvent->active)
    {
        if (facing > FACING_FORCED_RIGHT)
        {
            objEvent->triggerGroundEffectsOnMove = TRUE;
        }
        else
        {
            // This is a hack to split this code onto two separate lines, without declaring a local variable.
            // C++ style inline variables would be nice here.
            #define TEMP sLinkPlayerMovementModes[linkPlayerObjEvent->movementMode](linkPlayerObjEvent, objEvent, facing)

            sMovementStatusHandler[TEMP](linkPlayerObjEvent, objEvent);

            // Clean up the hack.
            #undef TEMP
        }
    }
}


static u8 MovementEventModeCB_Normal(struct LinkPlayerObjectEvent *linkPlayerObjEvent, struct ObjectEvent *objEvent, enum Direction dir)
{
    return sLinkPlayerFacingHandlers[dir](linkPlayerObjEvent, objEvent, dir);
}

static u8 MovementEventModeCB_Ignored(struct LinkPlayerObjectEvent *linkPlayerObjEvent, struct ObjectEvent *objEvent, enum Direction dir)
{
    return FACING_UP;
}

// Identical to MovementEventModeCB_Normal
static u8 MovementEventModeCB_Scripted(struct LinkPlayerObjectEvent *linkPlayerObjEvent, struct ObjectEvent *objEvent, enum Direction dir)
{
    return sLinkPlayerFacingHandlers[dir](linkPlayerObjEvent, objEvent, dir);
}

static bool8 FacingHandler_DoNothing(struct LinkPlayerObjectEvent *linkPlayerObjEvent, struct ObjectEvent *objEvent, enum Direction dir)
{
    return FALSE;
}

static bool8 FacingHandler_DpadMovement(struct LinkPlayerObjectEvent *linkPlayerObjEvent, struct ObjectEvent *objEvent, enum Direction dir)
{
    s16 x, y;

    linkDirection(objEvent) = FlipVerticalAndClearForced(dir, linkDirection(objEvent));
    ObjectEventMoveDestCoords(objEvent, linkDirection(objEvent), &x, &y);

    if (LinkPlayerGetCollision(linkPlayerObjEvent->objEventId, linkDirection(objEvent), x, y))
    {
        return FALSE;
    }
    else
    {
        objEvent->directionSequenceIndex = 16;
        ShiftObjectEventCoords(objEvent, x, y);
        ObjectEventUpdateElevation(objEvent, NULL);
        return TRUE;
    }
}

static bool8 FacingHandler_ForcedFacingChange(struct LinkPlayerObjectEvent *linkPlayerObjEvent, struct ObjectEvent *objEvent, enum Direction dir)
{
    linkDirection(objEvent) = FlipVerticalAndClearForced(dir, linkDirection(objEvent));
    return FALSE;
}

// This is called every time a free movement happens. Most of the time it's a No-Op.
static void MovementStatusHandler_EnterFreeMode(struct LinkPlayerObjectEvent *linkPlayerObjEvent, struct ObjectEvent *objEvent)
{
    linkPlayerObjEvent->movementMode = MOVEMENT_MODE_FREE;
}

static void MovementStatusHandler_TryAdvanceScript(struct LinkPlayerObjectEvent *linkPlayerObjEvent, struct ObjectEvent *objEvent)
{
    objEvent->directionSequenceIndex--;
    linkPlayerObjEvent->movementMode = MOVEMENT_MODE_FROZEN;
    MoveCoords(linkDirection(objEvent), &objEvent->initialCoords.x, &objEvent->initialCoords.y);
    if (!objEvent->directionSequenceIndex)
    {
        ShiftStillObjectEventCoords(objEvent);
        linkPlayerObjEvent->movementMode = MOVEMENT_MODE_SCRIPTED;
    }
}

// Flip Up/Down facing codes. If newFacing doesn't specify a direction, default
// to oldFacing. Note that this clears also the "FORCED" part of the facing code,
// even for Left/Right codes.
static enum Direction FlipVerticalAndClearForced(u8 newFacing, u8 oldFacing)
{
    switch (newFacing)
    {
    case FACING_UP:
    case FACING_FORCED_UP:
        return DIR_NORTH;
    case FACING_DOWN:
    case FACING_FORCED_DOWN:
        return DIR_SOUTH;
    case FACING_LEFT:
    case FACING_FORCED_LEFT:
        return DIR_WEST;
    case FACING_RIGHT:
    case FACING_FORCED_RIGHT:
        return DIR_EAST;
    }
    return oldFacing;
}

static u8 LinkPlayerGetCollision(u8 selfObjEventId, enum Direction direction, s16 x, s16 y)
{
    u8 i;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (i != selfObjEventId)
        {
            if ((gObjectEvents[i].currentCoords.x == x && gObjectEvents[i].currentCoords.y == y)
             || (gObjectEvents[i].previousCoords.x == x && gObjectEvents[i].previousCoords.y == y))
            {
                return 1;
            }
        }
    }
    return MapGridGetCollisionAt(x, y);
}

#if IS_HNS
// Stop the UpdateObjectEventCurrentMovement while-loop immediately when the
// partner is idle; held-movement actions still run through the engine path.
static bool8 Coop_PresenceMovementIdle(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    (void)objectEvent;
    (void)sprite;
    return TRUE;
}

// Drive presence partners through the real OE movement executor so walk/run/
// bike/surf/ledge get native sub-pixel interpolation instead of link-glides.
static void SpriteCB_CoopPresence(struct Sprite *sprite)
{
    struct LinkPlayerObjectEvent *linkPlayerObjEvent = &gLinkPlayerObjectEvents[sprite->data[0]];
    struct ObjectEvent *objEvent = &gObjectEvents[linkPlayerObjEvent->objEventId];

    UpdateObjectEventCurrentMovement(objEvent, sprite, Coop_PresenceMovementIdle);
}
#endif

static void CreateLinkPlayerSprite(u8 linkPlayerId, enum GameVersion gameVersion)
{
    struct LinkPlayerObjectEvent *linkPlayerObjEvent = &gLinkPlayerObjectEvents[linkPlayerId];
    u8 objEventId = linkPlayerObjEvent->objEventId;
    struct ObjectEvent *objEvent = &gObjectEvents[objEventId];
    struct Sprite *sprite;

    if (linkPlayerObjEvent->active)
    {
#if IS_HNS
        (void)gameVersion;
        // Heart & Soul's remote player must use the same boy/girl avatar set
        // selected on the remote save, independent of the link version byte.
        // Presence partners may be on bike/surf — use the host-confirmed state.
        {
            u8 gfxState = sCoopPresenceGfxState;
            void (*callback)(struct Sprite *) = SpriteCB_LinkPlayer;

            if (gfxState > PLAYER_AVATAR_STATE_WATERING)
                gfxState = PLAYER_AVATAR_STATE_NORMAL;
            if (linkPlayerId == COOP_PRESENCE_LINK_ID)
                callback = SpriteCB_CoopPresence;
            objEvent->spriteId = CreateObjectGraphicsSprite(
                GetPlayerAvatarGraphicsIdByStateIdAndGender(gfxState, linkGender(objEvent)),
                callback, 0, 0, 0);
        }
#else
        switch (gameVersion)
        {
        case VERSION_FIRE_RED:
        case VERSION_LEAF_GREEN:
            objEvent->spriteId = CreateObjectGraphicsSprite(GetFRLGAvatarGraphicsIdByGender(linkGender(objEvent)), SpriteCB_LinkPlayer, 0, 0, 0);
            break;
        case VERSION_RUBY:
        case VERSION_SAPPHIRE:
            objEvent->spriteId = CreateObjectGraphicsSprite(GetRSAvatarGraphicsIdByGender(linkGender(objEvent)), SpriteCB_LinkPlayer, 0, 0, 0);
            break;
        case VERSION_EMERALD:
            objEvent->spriteId = CreateObjectGraphicsSprite(GetRivalAvatarGraphicsIdByStateIdAndGender(PLAYER_AVATAR_STATE_NORMAL, linkGender(objEvent)), SpriteCB_LinkPlayer, 0, 0, 0);
            break;
        default:
            break;
        }
#endif

        sprite = &gSprites[objEvent->spriteId];
        sprite->coordOffsetEnabled = TRUE;
        sprite->data[0] = linkPlayerId;
        objEvent->triggerGroundEffectsOnMove = FALSE;
        objEvent->localId = OBJ_EVENT_ID_DYNAMIC_BASE + linkPlayerId;
        SetUpShadow(objEvent);
    }
}

static void SpriteCB_LinkPlayer(struct Sprite *sprite)
{
    struct LinkPlayerObjectEvent *linkPlayerObjEvent = &gLinkPlayerObjectEvents[sprite->data[0]];
    struct ObjectEvent *objEvent = &gObjectEvents[linkPlayerObjEvent->objEventId];
    sprite->x = objEvent->initialCoords.x;
    sprite->y = objEvent->initialCoords.y;
    SetObjectSubpriorityByElevation(objEvent->previousElevation, sprite, 1);
    sprite->oam.priority = ElevationToPriority(objEvent->previousElevation);

    if (linkPlayerObjEvent->movementMode == MOVEMENT_MODE_FREE)
        StartSpriteAnim(sprite, GetFaceDirectionAnimNum(linkDirection(objEvent)));
    else
        StartSpriteAnimIfDifferent(sprite, GetMoveDirectionAnimNum(linkDirection(objEvent)));

    UpdateObjectEventSpriteInvisibility(sprite, FALSE);
    if (objEvent->triggerGroundEffectsOnMove)
    {
        sprite->invisible = ((sprite->data[7] & 4) >> 2);
        sprite->data[7]++;
    }
}

// ----------------
// Item Header Descriptions
// Item Description Header

#define ITEM_ICON_X     26
#define ITEM_ICON_Y     24
#define ITEM_TAG        0x2722 //same as money label

bool8 GetSetItemObtained(enum Item item, enum ItemObtainFlags caseId)
{
#if OW_SHOW_ITEM_DESCRIPTIONS == OW_ITEM_DESCRIPTIONS_FIRST_TIME
    u8 index = item / 8;
    u8 bit = item % 8;
    u8 mask = 1 << bit;
    switch (caseId)
    {
    case FLAG_GET_ITEM_OBTAINED:
        return gSaveBlock3Ptr->itemFlags[index] & mask;
    case FLAG_SET_ITEM_OBTAINED:
        gSaveBlock3Ptr->itemFlags[index] |= mask;
        return TRUE;
    }
#endif
    return FALSE;
}

EWRAM_DATA static u8 sHeaderBoxWindowId = 0;
EWRAM_DATA u8 sItemIconSpriteId = 0;
EWRAM_DATA u8 sItemIconSpriteId2 = 0;

static void ShowItemIconSprite(enum Item item, bool8 firstTime, bool8 flash);
static void DestroyItemIconSprite(void);

static u8 ReformatItemDescription(enum Item item, u8 *dest)
{
    u8 count = 0;
    u8 numLines = 1;
    u8 maxChars = 32;
    u8 *desc = (u8 *)GetItemDescription(item);

    while (*desc != EOS)
    {
        if (count >= maxChars)
        {
            while (*desc != CHAR_SPACE && *desc != CHAR_NEWLINE)
            {
                *dest = *desc;  //finish word
                dest++;
                desc++;
            }

            *dest = CHAR_NEWLINE;
            count = 0;
            numLines++;
            dest++;
            desc++;
            continue;
        }

        *dest = *desc;
        if (*desc == CHAR_NEWLINE)
        {
            *dest = CHAR_SPACE;
        }

        dest++;
        desc++;
        count++;
    }

    // finish string
    *dest = EOS;
    return numLines;
}

void ScriptShowItemDescription(struct ScriptContext *ctx)
{
    if (OW_SHOW_ITEM_DESCRIPTIONS == OW_ITEM_DESCRIPTIONS_OFF)
    {
        (void) ScriptReadByte(ctx);
        return;
    }

    u8 headerType = ScriptReadByte(ctx);

    Script_RequestEffects(SCREFF_V1 | SCREFF_HARDWARE);

    struct WindowTemplate template;
    enum Item item = gSpecialVar_0x8006;
    u8 textY;
    u8 *dst;
    bool8 handleFlash = FALSE;

    if (GetFlashLevel() > 0 || InBattlePyramid_())
        handleFlash = TRUE;

    if (headerType == 1) // berry
        dst = gStringVar3;
    else
        dst = gStringVar1;

    if (GetSetItemObtained(item, FLAG_GET_ITEM_OBTAINED))
    {
        ShowItemIconSprite(item, FALSE, handleFlash);
        return; //no box if item obtained previously
    }

    SetWindowTemplateFields(&template, 0, 1, 1, 28, 3, 15, 8);
    sHeaderBoxWindowId = AddWindow(&template);
    FillWindowPixelBuffer(sHeaderBoxWindowId, PIXEL_FILL(0));
    PutWindowTilemap(sHeaderBoxWindowId);
    CopyWindowToVram(sHeaderBoxWindowId, 3);
    SetStandardWindowBorderStyle(sHeaderBoxWindowId, FALSE);
    DrawStdFrameWithCustomTileAndPalette(sHeaderBoxWindowId, FALSE, 0x214, 14);

    if (ReformatItemDescription(item, dst) == 1)
        textY = 4;
    else
        textY = 0;

    ShowItemIconSprite(item, TRUE, handleFlash);
    AddTextPrinterParameterized(sHeaderBoxWindowId, 0, dst, ITEM_ICON_X + 2, textY, 0, NULL);
}

void ScriptHideItemDescription(struct ScriptContext *ctx)
{
    if (OW_SHOW_ITEM_DESCRIPTIONS == OW_ITEM_DESCRIPTIONS_OFF)
        return;

    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE | SCREFF_HARDWARE);

    DestroyItemIconSprite();

    if (!GetSetItemObtained(gSpecialVar_0x8006, FLAG_GET_ITEM_OBTAINED))
    {
        //header box only exists if haven't seen item before
        GetSetItemObtained(gSpecialVar_0x8006, FLAG_SET_ITEM_OBTAINED);
        ClearStdWindowAndFrameToTransparent(sHeaderBoxWindowId, FALSE);
        CopyWindowToVram(sHeaderBoxWindowId, 3);
        RemoveWindow(sHeaderBoxWindowId);
    }
}

static void ShowItemIconSprite(enum Item item, bool8 firstTime, bool8 flash)
{
    s16 x = 0, y = 0;
    u8 iconSpriteId;
    u8 spriteId2 = MAX_SPRITES;

    if (flash)
    {
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
        SetGpuRegBits(REG_OFFSET_WINOUT, WINOUT_WINOBJ_OBJ);
    }

    iconSpriteId = AddItemIconSprite(ITEM_TAG, ITEM_TAG, item);
    if (flash)
        spriteId2 = AddItemIconSprite(ITEM_TAG, ITEM_TAG, item);
    if (iconSpriteId != MAX_SPRITES)
    {
        if (!firstTime)
        {
            //show in message box
            x = 213;
            y = 140;
        }
        else
        {
            // show in header box
            x = ITEM_ICON_X;
            y = ITEM_ICON_Y;
        }

        gSprites[iconSpriteId].x2 = x;
        gSprites[iconSpriteId].y2 = y;
        gSprites[iconSpriteId].oam.priority = 0;
    }

    if (spriteId2 != MAX_SPRITES)
    {
        gSprites[spriteId2].x2 = x;
        gSprites[spriteId2].y2 = y;
        gSprites[spriteId2].oam.priority = 0;
        gSprites[spriteId2].oam.objMode = ST_OAM_OBJ_WINDOW;
        sItemIconSpriteId2 = spriteId2;
    }

    sItemIconSpriteId = iconSpriteId;
}

static void DestroyItemIconSprite(void)
{
    FreeSpriteTilesByTag(ITEM_TAG);
    FreeSpritePaletteByTag(ITEM_TAG);
    FreeSpriteOamMatrix(&gSprites[sItemIconSpriteId]);
    DestroySprite(&gSprites[sItemIconSpriteId]);

    if ((GetFlashLevel() > 0 || InBattlePyramid_()) && sItemIconSpriteId2 != MAX_SPRITES)
    {
        FreeSpriteOamMatrix(&gSprites[sItemIconSpriteId2]);
        DestroySprite(&gSprites[sItemIconSpriteId2]);
    }
}

// returns old sHoursOverride
u16 SetTimeOfDay(u16 hours)
{
    u16 oldHours = sHoursOverride;
    sHoursOverride = hours;
    gTimeUpdateCounter = 0;
    return oldHours;
}

bool8 ScrFunc_settimeofday(struct ScriptContext *ctx)
{
    SetTimeOfDay(ScriptReadByte(ctx));
    return FALSE;
}

// Credits

void Overworld_CreditsMainCB(void)
{
    bool8 fading = !!gPaletteFade.active;
    if (fading)
        SetVBlankCallback(NULL);
    RunTasks();
    AnimateSprites();
    CameraUpdateNoObjectRefresh();
    UpdateCameraPanning();
    BuildOamBuffer();
    UpdatePaletteFade();
    UpdateTilesetAnimations();
    DoScheduledBgTilemapCopiesToVram();
    if (fading)
        SetFieldVBlankCallback();
}

static bool8 FieldCB2_Credits_WaitFade(void)
{
    if (gPaletteFade.active)
        return TRUE;
    else
        return FALSE;
}

bool32 Overworld_DoScrollSceneForCredits(u8 *state_p, const struct CreditsOverworldCmd * script)
{
    sCreditsOverworld_Script = script;
    return SetUpScrollSceneForCredits(state_p, 0);
}

static bool32 SetUpScrollSceneForCredits(u8 *state, u8 unused)
{
    struct WarpData warp;
    switch (*state)
    {
    case 0:
        sCreditsOverworld_CmdIndex = 0;
        sCreditsOverworld_CmdLength = 0;
        (*state)++;
        return FALSE;
    case 1:
        warp.mapGroup = sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_2;
        warp.mapNum = sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_4;
        warp.warpId = -1;
        sCreditsOverworld_CmdIndex++;
        warp.x = sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_0;
        warp.y = sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_2;
        sWarpDestination = warp;
        sCreditsOverworld_CmdLength = sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_4;
        WarpIntoMap();
        gPaletteFade.bufferTransferDisabled = TRUE;
        ScriptContext_Init();
        UnlockPlayerFieldControls();
        SetMainCallback1(NULL);
        gFieldCallback2 = FieldCB2_Credits_WaitFade;
        gMain.state = 0;
        (*state)++;
        return FALSE;
    case 2:
        if (MapLdr_Credits())
        {
            (*state)++;
            return FALSE;
        }
        break;
    case 3:
        gFieldCamera.callback = CameraCB_CreditsPan;
        SetFieldVBlankCallback();
        *state = 0;
        return TRUE;
    }
    return FALSE;
}

static bool8 MapLdr_Credits(void)
{
    u8 *state = &gMain.state;
    switch (*state)
    {
    case 0:
        InitOverworldBgs_NoResetHeap();
        LoadMapFromWarp(FALSE);
        (*state)++;
        break;
    case 1:
        ScanlineEffect_Clear();
        ResetAllPicSprites();
        ResetCameraUpdateInfo();
        InstallCameraPanAheadCallback();
        FieldEffectActiveListClear();
        StartWeather();
        ResumePausedWeather();
        SetUpFieldTasks();
        RunOnResumeMapScript();
        (*state)++;
        break;
    case 2:
        InitCurrentFlashLevelScanlineEffect();
        InitOverworldGraphicsRegistersCreditsFrlg();
        (*state)++;
        break;
    case 3:
        ResetFieldCamera();
        (*state)++;
        break;
    case 4:
        CopyPrimaryTilesetToVram(gMapHeader.mapLayout);
        (*state)++;
        break;
    case 5:
        CopySecondaryTilesetToVram(gMapHeader.mapLayout);
        (*state)++;
        break;
    case 6:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LoadMapTilesetPalettes(gMapHeader.mapLayout);
            (*state)++;
        }
        break;
    case 7:
        DrawWholeMapView();
        (*state)++;
        break;
    case 8:
        InitTilesetAnimations();
        gPaletteFade.bufferTransferDisabled = FALSE;
        FadeSelectedPals(FADE_FROM_BLACK, 0, 0x3FFFFFFF);
        (*state)++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static void CameraCB_CreditsPan(struct CameraObject * camera)
{
    if (sCreditsOverworld_CmdLength == 0)
    {
        sCreditsOverworld_CmdIndex++;
        switch (sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_0)
        {
        case CREDITSOVWLDCMD_FC:
        case CREDITSOVWLDCMD_LOADMAP:
            return;
        case CREDITSOVWLDCMD_FF:
            camera->movementSpeedX = 0;
            camera->movementSpeedY = 0;
            camera->callback = NULL;
            CreateTask(Task_OvwldCredits_FadeOut, 0);
            return;
        case CREDITSOVWLDCMD_FB:
            camera->movementSpeedX = 0;
            camera->movementSpeedY = 0;
            camera->callback = NULL;
            break;
        case CREDITSOVWLDCMD_END:
            camera->movementSpeedX = 0;
            camera->movementSpeedY = 0;
            camera->callback = NULL;
            return;
        default:
            sCreditsOverworld_CmdLength = sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_4;
            camera->movementSpeedX = sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_0;
            camera->movementSpeedY = sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_2;
            break;
        }
    }
    if (sCreditsOverworld_Script[sCreditsOverworld_CmdIndex].unk_0 == 0xFF)
    {
        camera->movementSpeedX = 0;
        camera->movementSpeedY = 0;
    }
    else
        sCreditsOverworld_CmdLength--;
}

static void Task_OvwldCredits_FadeOut(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_OvwldCredits_WaitFade;
}

static void Task_OvwldCredits_WaitFade(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(taskId);
    }
}
