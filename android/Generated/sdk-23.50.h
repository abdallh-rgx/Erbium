// GENERATED from mining data — v23.50 (FN 23.50, UE 5.3, libUnreal.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 23.50
#define ERBIUM_BAKED_ENGINE 5.3
#define ERBIUM_BAKED_LIB "libUnreal.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0x63d8b7c;
    inline constexpr uint64_t kFNameConstructor = 0x63dc420;
    inline constexpr uint64_t kGEngine = 0xe18ea98;
    inline constexpr uint64_t kGObjects = 0xddac6f0;
    inline constexpr uint64_t kProcessEvent = 0x6841184;
    inline constexpr uint64_t kRealloc = 0x60ed688;
    inline constexpr uint64_t kReleaseString = 0x158f124;
    inline constexpr uint64_t kSpawnActor = 0x65c8cd4;
    inline constexpr uint64_t kStaticFindObject = 0x626ee28;
    inline constexpr uint64_t kStaticLoadObject = 0x6e923e0;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kAppendString = "FD 7B BB A9 FC 0B 00 F9 FD 03 00 91 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FF 03 20 D1 ?? ?? ?? ??";
        inline constexpr const char* kFNameConstructor = "FD 7B BC A9 FC 0B 00 F9 FD 03 00 91 F6 57 02 A9 F4 4F 03 A9 FF 03 10 D1 F3 03 00 AA 01 07 00 B4";
        inline constexpr const char* kProcessEvent = "FF 43 05 D1 FD 7B 0F A9 FD C3 03 91 FC 6F 10 A9 FA 67 11 A9 F8 5F 12 A9 F6 57 13 A9 F4 4F 14 A9 FA 03 01 AA FF 1F 00 F9 ?? ?? ?? ?? F3 03 02 AA";
        inline constexpr const char* kRealloc = "FD 7B BD A9 F6 57 01 A9 FD 03 00 91 F4 4F 02 A9 ?? ?? ?? ?? E3 03 02 2A E2 03 01 AA E1 03 00 AA";
        inline constexpr const char* kSpawnActor = "EF 3B B6 6D ED 33 01 6D EB 2B 02 6D E9 23 03 6D FD 7B 04 A9 FD 03 01 91 FC 6F 05 A9 FA 67 06 A9 F8 5F 07 A9 F6 57 08 A9 F4 4F 09 A9 FF C3 0A D1 ?? ?? ?? ?? ?? ?? ?? ?? 29 09 16 91 F3 03 00 AA";
        inline constexpr const char* kStaticFindObject = "FF 43 03 D1 FD 7B 07 A9 FD C3 01 91 FC 6F 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9 F4 4F 0C A9 F9 03 05 AA FA 03 04 2A F5 03 01 AA EE 03 02 2A";
        inline constexpr const char* kStaticLoadObject = "FF 43 03 D1 FD 7B 07 A9 FD C3 01 91 FC 6F 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9 F4 4F 0C A9 ?? ?? ?? ?? E6 1F 00 B9 E3 97 00 A9 F8 03 00 AA";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindActivatePhase = 0x48e31fc;
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0x89e0904;
        inline constexpr uint64_t kFindCallPreReplication = 0x9b120f8;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0x89877b4;
        inline constexpr uint64_t kFindCantBuild = 0xa33c950;
        inline constexpr uint64_t kFindClearAbility = 0x35bd0c0;
        inline constexpr uint64_t kFindCloseActorChannel = 0x9c4d2f4;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0x36df2b8;
        inline constexpr uint64_t kFindCreateChannel = 0x653225c;
        inline constexpr uint64_t kFindEnterAircraft = 0x589acf4;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0x54fb068;
        inline constexpr uint64_t kFindGIsClient = 0xdfd33e2;
        inline constexpr uint64_t kFindGIsServer = 0xdfd33e3;
        inline constexpr uint64_t kFindGameSessionPatch = 0x669c00c;
        inline constexpr uint64_t kFindGetMaxTickRate = 0x9f6018c;
        inline constexpr uint64_t kFindGetNamePool = 0xdfe2000;
        inline constexpr uint64_t kFindGetNetMode = 0x63adfd4;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0x9c37dc8;
        inline constexpr uint64_t kFindGiveAbility = 0x35bd0c0;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x35bc1d8;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0x54eecdc;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0x54e3968;
        inline constexpr uint64_t kFindInitListen = 0x39b4100;
        inline constexpr uint64_t kFindInitializeBuildingActor = 0x57d4af8;
        inline constexpr uint64_t kFindInitializeBuildingActor_next = 0x57d5438;
        inline constexpr uint64_t kFindInitializeFlightPath = 0x4027dd0;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0x5b7a908;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x35bc400;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0x9b1b294;
        inline constexpr uint64_t kFindKickPlayer = 0x5e9c300;
        inline constexpr uint64_t kFindListenCall = 0x4634178;
        inline constexpr uint64_t kFindLoadPlayset = 0x89ed8d8;
        inline constexpr uint64_t kFindOnRep_ZiplineState = 0x6fd8ed8;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0x8987960;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0x5474cf8;
        inline constexpr uint64_t kFindPickTeam = 0x54ee344;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0x57d56e0;
        inline constexpr uint64_t kFindQueueStatEvent = 0x8a36880;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0x8955ecc;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0x8955c24;
        inline constexpr uint64_t kFindReplaceBuildingActor = 0x57dc9e8;
        inline constexpr uint64_t kFindReplicateActor = 0x9c4dd28;
        inline constexpr uint64_t kFindReset = 0x71fe5f8;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0x578f328;
        inline constexpr uint64_t kFindSendDestructionInfo = 0x9daa560;
        inline constexpr uint64_t kFindSendRequestNow = 0x3895d1c;
        inline constexpr uint64_t kFindServerReplicateActors = 0x580f0fc;
        inline constexpr uint64_t kFindSetChannelActor = 0x58e4058;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0x53667e8;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0x8955cec;
        inline constexpr uint64_t kFindSetPickupItems = 0x5dd9984;
        inline constexpr uint64_t kFindSetPickupTarget = 0x5d2fce8;
        inline constexpr uint64_t kFindSetState = 0x5dae1a0;
        inline constexpr uint64_t kFindSetWorld = 0x39bcbe4;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0x8cadb34;
        inline constexpr uint64_t kFindSpawnDecoVft = 0x8cb7fa4;
        inline constexpr uint64_t kFindSpawnInitialSafeZone = 0x4028668;
        inline constexpr uint64_t kFindSpawnLoot = 0x544b9f4;
        inline constexpr uint64_t kFindStartAircraftPhase = 0x61c39a4;
        inline constexpr uint64_t kFindStartBecomingDormant = 0x9c50614;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0x5b5ba94;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0x578cc38;
        inline constexpr uint64_t kFindTickFlush = 0x690f454;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0x8c57eb8;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0x75c9cdc;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0x64a31d4;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0x62c2e38;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0x84dd7e8;
        inline constexpr uint64_t kInline_OnAuthorityRandomUpgradeAppliedCaller = 0x7e4e624;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0x828c8f8;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0x615d760;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0x6d132b4;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0x55c0ec4;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0x8c57eb8;
    }
}
