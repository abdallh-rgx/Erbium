// GENERATED from mining data — v17.50 (FN 17.50, UE 4.27, libUE4.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 17.50
#define ERBIUM_BAKED_ENGINE 4.27
#define ERBIUM_BAKED_LIB "libUE4.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0xc58587c;
    inline constexpr uint64_t kFNameConstructor = 0xc584ed0;
    inline constexpr uint64_t kGEngine = 0x13753830;
    inline constexpr uint64_t kGObjects = 0x137a7dc0;
    inline constexpr uint64_t kProcessEvent = 0xc5b6f14;
    inline constexpr uint64_t kRealloc = 0xc560434;
    inline constexpr uint64_t kReleaseString = 0x433465a;
    inline constexpr uint64_t kSpawnActor = 0xc29f33c;
    inline constexpr uint64_t kStaticFindObject = 0xc5b054c;
    inline constexpr uint64_t kStaticLoadObject = 0xc1a273c;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kAppendString = "FC 5F BC A9 F6 57 01 A9 F4 4F 02 A9 FD 7B 03 A9 FD C3 00 91 FF 43 20 D1 56 D0 3B D5 C8 16 40 F9 ?? ?? ?? ?? ?? ?? ?? ?? F3 03 01 AA A8 83 1C F8 E8 62 51 39 F4 03 00 AA B5 02 12 91 48 08 00 36";
        inline constexpr const char* kFNameConstructor = "F3 0F 1E F8 FD 7B 01 A9 FD 43 00 91 E8 03 01 AA F3 03 00 AA 01 03 00 B4 0B 01 40 79 6B 02 00 34";
        inline constexpr const char* kProcessEvent = "FF 03 07 D1 FC 6F 16 A9 FA 67 17 A9 F8 5F 18 A9 F6 57 19 A9 F4 4F 1A A9 FD 7B 1B A9 FD C3 06 91 E2 07 00 F9 58 D0 3B D5 08 17 40 F9 F9 03 01 AA";
        inline constexpr const char* kRealloc = "F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ?? ?? ?? ?? 28 BD 40 F9 E3 03 02 2A E2 03 01 AA";
        inline constexpr const char* kSpawnActor = "FF 43 07 D1 E9 23 16 6D FC 6F 17 A9 FA 67 18 A9 F8 5F 19 A9 F6 57 1A A9 F4 4F 1B A9 FD 7B 1C A9 FD 03 07 91 5A D0 3B D5 48 17 40 F9 ?? ?? ?? ??";
        inline constexpr const char* kStaticFindObject = "FF 43 02 D1 FC 6F 03 A9 FA 67 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9 FD 7B 08 A9 FD 03 02 91 59 D0 3B D5 28 17 40 F9 ?? ?? ?? ?? F3 03 03 2A";
        inline constexpr const char* kStaticLoadObject = "FF 83 03 D1 FC 6F 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9 F4 4F 0C A9 FD 7B 0D A9 FD 43 03 91 E6 27 00 B9 54 D0 3B D5 88 16 40 F9 ?? ?? ?? ??";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindActivatePhase = 0x86fd3f8;
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0xbf1fe24;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0xd59a084;
        inline constexpr uint64_t kFindClearAbility = 0x76978d0;
        inline constexpr uint64_t kFindCloseActorChannel = 0xdf96540;
        inline constexpr uint64_t kFindCreateChannel = 0xc2c0e5c;
        inline constexpr uint64_t kFindEnterAircraft = 0xd00f1fc;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0xcd214e4;
        inline constexpr uint64_t kFindGIsClient = 0x136a0330;
        inline constexpr uint64_t kFindGIsServer = 0x13722e68;
        inline constexpr uint64_t kFindGameSessionPatch = 0xbc6bab0;
        inline constexpr uint64_t kFindGetMaxTickRate = 0xe245d68;
        inline constexpr uint64_t kFindGetNamePool = 0x136e0380;
        inline constexpr uint64_t kFindGetNetMode = 0xc6f7fa4;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0xc318048;
        inline constexpr uint64_t kFindGiveAbility = 0x76978d0;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x76968f4;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0xe9e4390;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0xcd39bec;
        inline constexpr uint64_t kFindInitListen = 0xc716c04;
        inline constexpr uint64_t kFindInitializeBuildingActor = 0xcf4f47c;
        inline constexpr uint64_t kFindInitializeBuildingActor_next = 0xcf4feb0;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0xd1d954c;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x769525c;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0xdea1ccc;
        inline constexpr uint64_t kFindKickPlayer = 0x7ac7ca8;
        inline constexpr uint64_t kFindListenCall = 0xc392780;
        inline constexpr uint64_t kFindLoadPlayset = 0xd60da50;
        inline constexpr uint64_t kFindNotifyGameMemberAdded = 0xbc6cfe4;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0xd59b380;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0xcce042c;
        inline constexpr uint64_t kFindPickTeam = 0xcd22304;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0xcf4feb0;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0xcd2d3d4;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0xdbd5a38;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0xdbd5c4c;
        inline constexpr uint64_t kFindReplaceBuildingActor = 0xbf34d40;
        inline constexpr uint64_t kFindReplicateActor = 0xdf96bac;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0xcf3942c;
        inline constexpr uint64_t kFindSendDestructionInfo = 0xe0c92c0;
        inline constexpr uint64_t kFindSendRequestNow = 0x791ea50;
        inline constexpr uint64_t kFindServerReplicateActors = 0xaa72120;
        inline constexpr uint64_t kFindSetChannelActor = 0xd28e444;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0xdf969ec;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0xdbd6b1c;
        inline constexpr uint64_t kFindSetPickupItems = 0xba34900;
        inline constexpr uint64_t kFindSetPickupTarget = 0xd2f4ce8;
        inline constexpr uint64_t kFindSetState = 0xd326a30;
        inline constexpr uint64_t kFindSetWorld = 0xe0cad4c;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0xd8751a8;
        inline constexpr uint64_t kFindSpawnDecoVft = 0xd887244;
        inline constexpr uint64_t kFindSpawnInitialSafeZone = 0xcd38f28;
        inline constexpr uint64_t kFindSpawnLoot = 0xccc4c64;
        inline constexpr uint64_t kFindStartAircraftPhase = 0xcd25f88;
        inline constexpr uint64_t kFindStartBecomingDormant = 0xdf9864c;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0xd1b1f60;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0xbcc98c8;
        inline constexpr uint64_t kFindTickFlush = 0xc673190;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0xce03858;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0xef73d7c;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0xc18295c;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0xc1ac498;
        inline constexpr uint64_t kInline_GetSparseClassData = 0xc558e9c;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0xefca694;
        inline constexpr uint64_t kInline_OnAuthorityRandomUpgradeAppliedCaller = 0xee28e88;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0xef5afdc;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0xc1ac498;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0xbda7710;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0xcde30ec;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0xce03858;
    }
}
