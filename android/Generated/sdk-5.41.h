// GENERATED from mining data — v5.2.0 (FN 5.41, UE 4.19, libUE4.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 5.41
#define ERBIUM_BAKED_ENGINE 4.19
#define ERBIUM_BAKED_LIB "libUE4.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0;
    inline constexpr uint64_t kFNameConstructor = 0x40d4a30;
    inline constexpr uint64_t kGEngine = 0x9544870;
    inline constexpr uint64_t kGObjects = 0x945ff38;
    inline constexpr uint64_t kProcessEvent = 0x3c31708;
    inline constexpr uint64_t kRealloc = 0;
    inline constexpr uint64_t kReleaseString = 0;
    inline constexpr uint64_t kSpawnActor = 0x43660c8;
    inline constexpr uint64_t kStaticFindObject = 0x420ea6c;
    inline constexpr uint64_t kStaticLoadObject = 0x4578a6c;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kFNameConstructor = "E8 03 02 2A C1 00 00 B4 E4 03 00 32 05 00 80 12 E2 03 1F 2A E3 03 08 2A ?? ?? ?? ?? 1F 00 00 F9 C0 03 5F D6 00 A8 41 F9 40 00 00 B4 ?? ?? ?? ??";
        inline constexpr const char* kProcessEvent = "FF 83 06 D1 FC A3 00 F9 FA 67 15 A9 F8 5F 16 A9 F6 57 17 A9 F4 4F 18 A9 FD 7B 19 A9 FD 43 06 91 F4 03 01 AA F8 03 14 AA 1F 0F 03 F8 BF 83 1B F8";
        inline constexpr const char* kSpawnActor = "EF 3B B6 6D ED 33 01 6D EB 2B 02 6D E9 23 03 6D FC 6F 04 A9 FA 67 05 A9 F8 5F 06 A9 F6 57 07 A9 F4 4F 08 A9 FD 7B 09 A9 FD 43 02 91 FF 03 08 D1 5A D0 3B D5 48 17 40 F9 F4 03 03 AA F9 03 02 AA";
        inline constexpr const char* kStaticFindObject = "FF 83 01 D1 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FD 7B 05 A9 FD 43 01 91 ?? ?? ?? ?? 08 C1 40 39";
        inline constexpr const char* kStaticLoadObject = "FF C3 06 D1 FC 6F 15 A9 FA 67 16 A9 F8 5F 17 A9 F6 57 18 A9 F4 4F 19 A9 FD 7B 1A A9 FD 83 06 91 5A D0 3B D5 48 17 40 F9 ?? ?? ?? ?? F8 03 06 2A";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0x5548048;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0x5bde450;
        inline constexpr uint64_t kFindCantBuild = 0x4e64af8;
        inline constexpr uint64_t kFindClearAbility = 0x2341f0c;
        inline constexpr uint64_t kFindCloseActorChannel = 0x45cc1ec;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0x237de00;
        inline constexpr uint64_t kFindEnterAircraft = 0x5b36830;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0x5b7ffb8;
        inline constexpr uint64_t kFindGIsClient = 0x97f6ef0;
        inline constexpr uint64_t kFindGIsServer = 0x98228b8;
        inline constexpr uint64_t kFindGameSessionPatch = 0x3c3598c;
        inline constexpr uint64_t kFindGetMaxTickRate = 0x3e969fc;
        inline constexpr uint64_t kFindGetNetMode = 0x4a39f68;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0x5d7a158;
        inline constexpr uint64_t kFindGiveAbility = 0x2340d1c;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x2340d1c;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0x5814b98;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0x5430084;
        inline constexpr uint64_t kFindInitListen = 0x3f18d6c;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0x59e76ac;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x2340f00;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0x47e1354;
        inline constexpr uint64_t kFindKickPlayer = 0x63aece4;
        inline constexpr uint64_t kFindListenCall = 0x422adfc;
        inline constexpr uint64_t kFindLoadPlayset = 0x49d9c94;
        inline constexpr uint64_t kFindOnRep_ZiplineState = 0x4334394;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0x5c79e8c;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0x5e61fc4;
        inline constexpr uint64_t kFindPickTeam = 0x5dc6e24;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0x5ad799c;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0x5edd508;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0x633901c;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0x6352188;
        inline constexpr uint64_t kFindReset = 0x354d0e8;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0x5bed650;
        inline constexpr uint64_t kFindSendDestructionInfo = 0x23d3064;
        inline constexpr uint64_t kFindSendRequestNow = 0x27b36fc;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0x3c5955c;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0x5f37f4c;
        inline constexpr uint64_t kFindSetPickupItems = 0x2c4e8b4;
        inline constexpr uint64_t kFindSetPickupTarget = 0x5be9ab4;
        inline constexpr uint64_t kFindSetState = 0x26a1a70;
        inline constexpr uint64_t kFindSetWorld = 0x4ddeda4;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0x5c58520;
        inline constexpr uint64_t kFindSpawnDecoVft = 0x5b5a5c4;
        inline constexpr uint64_t kFindStartAircraftPhase = 0x5a0a660;
        inline constexpr uint64_t kFindStartBecomingDormant = 0x3d43624;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0x567f4d8;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0x560a580;
        inline constexpr uint64_t kFindTickFlush = 0x4a68070;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0x20311cc;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0x45a6f44;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0x1f35300;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0x2041638;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0x58bd138;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0x5bc9420;
    }
}
