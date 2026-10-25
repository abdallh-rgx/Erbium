// GENERATED from mining data — v22.40 (FN 22.40, UE 5.1, libUnreal.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 22.40
#define ERBIUM_BAKED_ENGINE 5.1
#define ERBIUM_BAKED_LIB "libUnreal.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0x7e2be28;
    inline constexpr uint64_t kFNameConstructor = 0x7e2a408;
    inline constexpr uint64_t kGEngine = 0xe4937a8;
    inline constexpr uint64_t kGObjects = 0xe33d500;
    inline constexpr uint64_t kProcessEvent = 0x7fa1ca0;
    inline constexpr uint64_t kRealloc = 0x7cfc074;
    inline constexpr uint64_t kReleaseString = 0x1390d24;
    inline constexpr uint64_t kSpawnActor = 0x9bf411c;
    inline constexpr uint64_t kStaticFindObject = 0x7fc4378;
    inline constexpr uint64_t kStaticLoadObject = 0x7fc711c;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kAppendString = "FC 0F 1B F8 F8 5F 01 A9 F6 57 02 A9 F4 4F 03 A9 FD 7B 04 A9 FD 03 01 91 FF 03 20 D1 ?? ?? ?? ?? E8 02 4B 39 15 00 40 B9 ?? ?? ?? ?? F3 03 01 AA";
        inline constexpr const char* kFNameConstructor = "F3 0F 1E F8 FD 7B 01 A9 FD 43 00 91 E8 03 01 AA F3 03 00 AA 01 03 00 B4 0B 01 40 79 6B 02 00 34";
        inline constexpr const char* kProcessEvent = "FF 83 06 D1 FC A3 00 F9 FA 67 15 A9 F8 5F 16 A9 F6 57 17 A9 F4 4F 18 A9 FD 7B 19 A9 FD 43 06 91 F7 03 01 AA FF 0E 03 F8 BF 83 1B F8 E9 02 5F F8";
        inline constexpr const char* kRealloc = "F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ?? ?? ?? ?? 29 D9 46 F9 E3 03 02 2A E2 03 01 AA";
        inline constexpr const char* kSpawnActor = "E8 0F 19 FC FC 6F 01 A9 FA 67 02 A9 F8 5F 03 A9 F6 57 04 A9 F4 4F 05 A9 FD 7B 06 A9 FD 83 01 91 FF 43 0A D1 ?? ?? ?? ?? 5A 87 46 F9 ?? ?? ?? ??";
        inline constexpr const char* kStaticFindObject = "FF 83 03 D1 FC 6F 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9 F4 4F 0C A9 FD 7B 0D A9 FD 43 03 91 28 08 80 B9 F6 03 01 AA F7 03 00 AA E2 37 00 B9";
        inline constexpr const char* kStaticLoadObject = "FC 6F BA A9 FA 67 01 A9 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FD 7B 05 A9 FD 43 01 91 FF 43 0A D1 E6 1F 00 B9 E3 97 00 A9 ?? ?? ?? ?? 08 85 46 F9";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindActivatePhase = 0x4d2bdb4;
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0x7708b34;
        inline constexpr uint64_t kFindCallPreReplication = 0x96e5454;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0x766fed4;
        inline constexpr uint64_t kFindCantBuild = 0x4e970a4;
        inline constexpr uint64_t kFindClearAbility = 0x34e7340;
        inline constexpr uint64_t kFindCloseActorChannel = 0x99f6be0;
        inline constexpr uint64_t kFindCreateChannel = 0x9c9d3cc;
        inline constexpr uint64_t kFindCreateNetDriverWorldContext = 0x9435530;
        inline constexpr uint64_t kFindEnterAircraft = 0x675a8d4;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0x61196f8;
        inline constexpr uint64_t kFindFinishedTargetSpline = 0x801bb74;
        inline constexpr uint64_t kFindGIsClient = 0xe2ccfe2;
        inline constexpr uint64_t kFindGIsServer = 0xe2ccfe3;
        inline constexpr uint64_t kFindGameSessionPatch = 0x6165934;
        inline constexpr uint64_t kFindGetMaxTickRate = 0xa06e300;
        inline constexpr uint64_t kFindGetNamePool = 0xe2da300;
        inline constexpr uint64_t kFindGetNetMode = 0xa0edf1c;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0x9e2773c;
        inline constexpr uint64_t kFindGetWorldContext = 0x96eb280;
        inline constexpr uint64_t kFindGiveAbility = 0x34e7a4c;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x34fdbdc;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0x610c28c;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0x60fc388;
        inline constexpr uint64_t kFindInitListen = 0x3b7cad8;
        inline constexpr uint64_t kFindInitializeBuildingActor = 0x65fe1f4;
        inline constexpr uint64_t kFindInitializeBuildingActor_next = 0x65fec10;
        inline constexpr uint64_t kFindInitializeFlightPath = 0x47690b0;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0x6c954fc;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x34fddb4;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0x9734c4c;
        inline constexpr uint64_t kFindKickPlayer = 0x71f1e60;
        inline constexpr uint64_t kFindListenCall = 0xa082148;
        inline constexpr uint64_t kFindLoadPlayset = 0x771ea94;
        inline constexpr uint64_t kFindNotifyGameMemberAdded = 0x6140e48;
        inline constexpr uint64_t kFindOnRep_ZiplineState = 0x754f6e8;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0x767006c;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0x603e4f8;
        inline constexpr uint64_t kFindPickTeam = 0x610b7fc;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0x65feec8;
        inline constexpr uint64_t kFindQueueStatEvent = 0x77997ac;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0x6109f38;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0x761f41c;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0x761f154;
        inline constexpr uint64_t kFindReplaceBuildingActor = 0x6612c34;
        inline constexpr uint64_t kFindReplicateActor = 0x99fbbe0;
        inline constexpr uint64_t kFindReset = 0x3fc940c;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0x65855ac;
        inline constexpr uint64_t kFindSendDestructionInfo = 0x9cb8a30;
        inline constexpr uint64_t kFindSendRequestNow = 0x39d6bc8;
        inline constexpr uint64_t kFindServerReplicateActors = 0x6672b24;
        inline constexpr uint64_t kFindSetChannelActor = 0x6fb7200;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0x5e65944;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0x761f228;
        inline constexpr uint64_t kFindSetPickupItems = 0x42801a8;
        inline constexpr uint64_t kFindSetPickupTarget = 0x6fbf744;
        inline constexpr uint64_t kFindSetState = 0x70634a8;
        inline constexpr uint64_t kFindSetWorld = 0x9cbaccc;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0x7b78844;
        inline constexpr uint64_t kFindSpawnDecoVft = 0x7b87d94;
        inline constexpr uint64_t kFindSpawnInitialSafeZone = 0x476cb64;
        inline constexpr uint64_t kFindSpawnLoot = 0x6001298;
        inline constexpr uint64_t kFindStartAircraftPhase = 0x7de299c;
        inline constexpr uint64_t kFindStartBecomingDormant = 0x99fe6d8;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0x6c3f0e0;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0x6582514;
        inline constexpr uint64_t kFindTickFlush = 0x9cb0734;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0x7af1f48;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0x76a73d0;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0x7f01a50;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0x7f9a464;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0x7b61c54;
        inline constexpr uint64_t kInline_OnAuthorityRandomUpgradeAppliedCaller = 0x656e824;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0x64ecb30;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0x7fd1928;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0x6ccabb0;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0x62808fc;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0x7af1f48;
    }
}
