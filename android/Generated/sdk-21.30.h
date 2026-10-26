// GENERATED from mining data — v21.30 (FN 21.30, UE 5.0, libUnreal.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 21.30
#define ERBIUM_BAKED_ENGINE 5.0
#define ERBIUM_BAKED_LIB "libUnreal.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0x7493f48;
    inline constexpr uint64_t kFNameConstructor = 0x7492440;
    inline constexpr uint64_t kGEngine = 0xde34970;
    inline constexpr uint64_t kGObjects = 0xdcf8310;
    inline constexpr uint64_t kProcessEvent = 0x76172c0;
    inline constexpr uint64_t kRealloc = 0x735e520;
    inline constexpr uint64_t kReleaseString = 0x1690506; // ASCII copy (0x12c7b62 is UTF-16LE — the probe trap)
    inline constexpr uint64_t kSpawnActor = 0x947f4fc;
    inline constexpr uint64_t kStaticFindObject = 0x762a8d4;
    inline constexpr uint64_t kStaticLoadObject = 0x762ca34;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kAppendString = "FC 67 BB A9 F8 5F 01 A9 F6 57 02 A9 F4 4F 03 A9 FD 7B 04 A9 FD 03 01 91 FF 43 20 D1 56 D0 3B D5";
        inline constexpr const char* kFNameConstructor = "F3 0F 1E F8 FD 7B 01 A9 FD 43 00 91 E8 03 01 AA F3 03 00 AA 01 03 00 B4 0B 01 40 79 6B 02 00 34";
        inline constexpr const char* kProcessEvent = "FF C3 06 D1 FC 6F 15 A9 FA 67 16 A9 F8 5F 17 A9 F6 57 18 A9 F4 4F 19 A9 FD 7B 1A A9 FD 83 06 91 58 D0 3B D5 08 17 40 F9 F9 03 01 AA ?? ?? ?? ??";
        inline constexpr const char* kRealloc = "F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ?? ?? ?? ?? 29 DD 47 F9 E3 03 02 2A E2 03 01 AA";
        inline constexpr const char* kSpawnActor = "E8 0F 19 FC FC 6F 01 A9 FA 67 02 A9 F8 5F 03 A9 F6 57 04 A9 F4 4F 05 A9 FD 7B 06 A9 FD 83 01 91 FF 43 0A D1 5A D0 3B D5 48 17 40 F9 ?? ?? ?? ??";
        inline constexpr const char* kStaticFindObject = "FF 83 03 D1 FC 6F 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9 F4 4F 0C A9 FD 7B 0D A9 FD 43 03 91 E2 2F 00 B9 5B D0 3B D5 68 17 40 F9 F6 03 01 AA";
        inline constexpr const char* kStaticLoadObject = "FC 6F BA A9 FA 67 01 A9 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FD 7B 05 A9 FD 43 01 91 FF 43 0A D1 E6 17 00 B9 54 D0 3B D5 88 16 40 F9 ?? ?? ?? ??";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindActivatePhase = 0x4862878;
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0x670dbdc;
        inline constexpr uint64_t kFindCallPreReplication = 0x90572b0;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0x665ab4c;
        inline constexpr uint64_t kFindCantBuild = 0x75363f8;
        inline constexpr uint64_t kFindClearAbility = 0x3177508;
        inline constexpr uint64_t kFindClientHasInitializedLevelFor = 0x3850e10;
        inline constexpr uint64_t kFindCloseActorChannel = 0x92f8d64;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0xa3ec1a4;
        inline constexpr uint64_t kFindCreateChannel = 0x951a8a0;
        inline constexpr uint64_t kFindCreateNetDriverWorldContext = 0x6ec3db8;
        inline constexpr uint64_t kFindEnterAircraft = 0x5a8227c;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0x551ba34;
        inline constexpr uint64_t kFindFinishedTargetSpline = 0x768a940;
        inline constexpr uint64_t kFindFlushDormancy = 0x951d5e0;
        inline constexpr uint64_t kFindGIsClient = 0xdc80824;
        inline constexpr uint64_t kFindGIsServer = 0xdc80832;
        inline constexpr uint64_t kFindGameSessionPatch = 0x5554088;
        inline constexpr uint64_t kFindGetMaxTickRate = 0x98bc168;
        inline constexpr uint64_t kFindGetNamePool = 0xdc960c0;
        inline constexpr uint64_t kFindGetNetMode = 0x992f0e0;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0x99c4af8;
        inline constexpr uint64_t kFindGetWorldContext = 0x905d300;
        inline constexpr uint64_t kFindGiveAbility = 0x73d89c4;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x3176514;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0x550dfcc;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0x552ba6c;
        inline constexpr uint64_t kFindInitListen = 0x3852a0c;
        inline constexpr uint64_t kFindInitializeBuildingActor = 0x591c5e4;
        inline constexpr uint64_t kFindInitializeBuildingActor_next = 0x591d030;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0x5e81e10;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x3176718;
        inline constexpr uint64_t kFindIsNetReady = 0x9519a8c;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0x90784d8;
        inline constexpr uint64_t kFindKickPlayer = 0x62b187c;
        inline constexpr uint64_t kFindListenCall = 0x98d09d4;
        inline constexpr uint64_t kFindLoadPlayset = 0x66f61d0;
        inline constexpr uint64_t kFindNotifyGameMemberAdded = 0x5530a50;
        inline constexpr uint64_t kFindOnRep_ZiplineState = 0x65bf5c0;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0x665ad20;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0x5ab8664;
        inline constexpr uint64_t kFindPickTeam = 0x550d590;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0x591d3d0;
        inline constexpr uint64_t kFindQueueStatEvent = 0x674a0c4;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0x552d404;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0x7057148;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0x7056e08;
        inline constexpr uint64_t kFindReplaceBuildingActor = 0x5932630;
        inline constexpr uint64_t kFindReplicateActor = 0x92fce10;
        inline constexpr uint64_t kFindReset = 0x4b52538;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0x587ae00;
        inline constexpr uint64_t kFindSendClientAdjustment = 0x9688238;
        inline constexpr uint64_t kFindSendRequestNow = 0x36a0ef4;
        inline constexpr uint64_t kFindServerReplicateActors = 0x3516ccc;
        inline constexpr uint64_t kFindSetChannelActor = 0x5e616d0;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0x92fa114;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0x7056f04;
        inline constexpr uint64_t kFindSetPickupItems = 0x5c4263c;
        inline constexpr uint64_t kFindSetPickupTarget = 0x60f4920;
        inline constexpr uint64_t kFindSetState = 0x618a734;
        inline constexpr uint64_t kFindSetWorld = 0x9537820;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0x6a8463c;
        inline constexpr uint64_t kFindSpawnDecoVft = 0x6a91214;
        inline constexpr uint64_t kFindSpawnInitialSafeZone = 0x55064ec;
        inline constexpr uint64_t kFindSpawnLoot = 0x5414df4;
        inline constexpr uint64_t kFindStartAircraftPhase = 0x7449ec0;
        inline constexpr uint64_t kFindStartBecomingDormant = 0x92ff3c0;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0x5e48430;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0x5877748;
        inline constexpr uint64_t kFindTickFlush = 0x952ca44;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0x6a1c864;
        inline constexpr uint64_t kFindUpdateSafeZonesPhase = 0x55079f8;
        inline constexpr uint64_t kInline_AttemptDeriveFromURL = 0x9ab1ae0;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0x70a0290;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0x756fe88;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0x6e46400;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0x71dc27c;
        inline constexpr uint64_t kInline_OnAuthorityRandomUpgradeAppliedCaller = 0x6b50db0;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0x701e3ec;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0x7636050;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0x5eb2d70;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0x56407cc;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0x6a1c864;
    }
}
