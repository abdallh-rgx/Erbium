// GENERATED from mining data — v19.10 (FN 19.10, UE 5.0, libUE4.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 19.10
#define ERBIUM_BAKED_ENGINE 5.0
#define ERBIUM_BAKED_LIB "libUE4.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0xa72cb80;
    inline constexpr uint64_t kFNameConstructor = 0xa72b2ec;
    inline constexpr uint64_t kGEngine = 0x104f7280;
    inline constexpr uint64_t kGObjects = 0x103b60e0;
    inline constexpr uint64_t kProcessEvent = 0xa898c74;
    inline constexpr uint64_t kRealloc = 0xa601b2c;
    inline constexpr uint64_t kReleaseString = 0x4bb95cc;
    inline constexpr uint64_t kSpawnActor = 0xc3b929c;
    inline constexpr uint64_t kStaticFindObject = 0xa8b38ac;
    inline constexpr uint64_t kStaticLoadObject = 0xa8b5364;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kAppendString = "FC 5F BC A9 F6 57 01 A9 F4 4F 02 A9 FD 7B 03 A9 FD C3 00 91 FF 43 20 D1 56 D0 3B D5 C8 16 40 F9 ?? ?? ?? ?? ?? ?? ?? ?? F3 03 01 AA A8 83 1C F8 E8 02 68 39 F4 03 00 AA B5 02 29 91 08 07 00 36";
        inline constexpr const char* kFNameConstructor = "F3 0F 1E F8 FD 7B 01 A9 FD 43 00 91 E8 03 01 AA F3 03 00 AA 01 03 00 B4 0B 01 40 79 6B 02 00 34";
        inline constexpr const char* kProcessEvent = "FF C3 06 D1 FC 6F 15 A9 FA 67 16 A9 F8 5F 17 A9 F6 57 18 A9 F4 4F 19 A9 FD 7B 1A A9 FD 83 06 91 58 D0 3B D5 08 17 40 F9 F9 03 01 AA ?? ?? ?? ??";
        inline constexpr const char* kRealloc = "F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ?? ?? ?? ?? 29 11 47 F9 E3 03 02 2A E2 03 01 AA";
        inline constexpr const char* kSpawnActor = "FF 03 07 D1 E9 23 15 6D FC 6F 16 A9 FA 67 17 A9 F8 5F 18 A9 F6 57 19 A9 F4 4F 1A A9 FD 7B 1B A9 FD C3 06 91 5A D0 3B D5 48 17 40 F9 ?? ?? ?? ??";
        inline constexpr const char* kStaticFindObject = "FF C3 03 D1 FC 6F 09 A9 FA 67 0A A9 F8 5F 0B A9 F6 57 0C A9 F4 4F 0D A9 FD 7B 0E A9 FD 83 03 91 E4 2F 00 B9 5C D0 3B D5 88 17 40 F9 FB 03 05 AA";
        inline constexpr const char* kStaticLoadObject = "FF 83 03 D1 FC 6F 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9 F4 4F 0C A9 FD 7B 0D A9 FD 43 03 91 E6 27 00 B9 E3 97 00 A9 56 D0 3B D5 C8 16 40 F9";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindActivatePhase = 0x7aced40;
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0x99aff04;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0x8b2be98;
        inline constexpr uint64_t kFindCantBuild = 0x9cde0dc;
        inline constexpr uint64_t kFindClearAbility = 0x67430fc;
        inline constexpr uint64_t kFindCloseActorChannel = 0xc243d20;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0xcfa1844;
        inline constexpr uint64_t kFindCreateChannel = 0xc448e38;
        inline constexpr uint64_t kFindCreateNetDriverWorldContext = 0x8c758b8;
        inline constexpr uint64_t kFindEnterAircraft = 0x8e6e39c;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0x895c7ac;
        inline constexpr uint64_t kFindGIsClient = 0x103421ec;
        inline constexpr uint64_t kFindGIsServer = 0x103421fa;
        inline constexpr uint64_t kFindGameSessionPatch = 0x8993b24;
        inline constexpr uint64_t kFindGetMaxTickRate = 0xc7adc90;
        inline constexpr uint64_t kFindGetNamePool = 0x10357a40;
        inline constexpr uint64_t kFindGetNetMode = 0xc81c478;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0xc5a2f80;
        inline constexpr uint64_t kFindGiveAbility = 0x67430fc;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x674210c;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0x894f15c;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0x8943df4;
        inline constexpr uint64_t kFindInitListen = 0x6ed9974;
        inline constexpr uint64_t kFindInitializeBuildingActor = 0x8ce97b0;
        inline constexpr uint64_t kFindInitializeBuildingActor_next = 0x8ce96dc;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0x91f8f64;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x6742310;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0xbfc9c90;
        inline constexpr uint64_t kFindKickPlayer = 0x9583574;
        inline constexpr uint64_t kFindListenCall = 0xc7be314;
        inline constexpr uint64_t kFindLoadPlayset = 0x99c0c80;
        inline constexpr uint64_t kFindNotifyGameMemberAdded = 0x8971fd0;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0x9929ec4;
        inline constexpr uint64_t kFindPickTeam = 0x894e72c;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0x8d007a0;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0x894c79c;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0xa2f7ea8;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0xa2f7af4;
        inline constexpr uint64_t kFindReplaceBuildingActor = 0x8d12324;
        inline constexpr uint64_t kFindReplicateActor = 0xc2491dc;
        inline constexpr uint64_t kFindReset = 0x9a6fedc;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0x8ca36ec;
        inline constexpr uint64_t kFindSendDestructionInfo = 0xc462d94;
        inline constexpr uint64_t kFindSendRequestNow = 0x6d23164;
        inline constexpr uint64_t kFindServerReplicateActors = 0x6ec99bc;
        inline constexpr uint64_t kFindSetChannelActor = 0xc5a567c;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0xc245ca0;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0xa2f7c2c;
        inline constexpr uint64_t kFindSetPickupItems = 0x7e0004c;
        inline constexpr uint64_t kFindSetPickupTarget = 0x94373d4;
        inline constexpr uint64_t kFindSetState = 0x94a9554;
        inline constexpr uint64_t kFindSetWorld = 0xc4649d4;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0x9cf048c;
        inline constexpr uint64_t kFindSpawnDecoVft = 0x9cfbe54;
        inline constexpr uint64_t kFindSpawnInitialSafeZone = 0x89420c4;
        inline constexpr uint64_t kFindSpawnLoot = 0x8867ac0;
        inline constexpr uint64_t kFindStartAircraftPhase = 0x876b5e4;
        inline constexpr uint64_t kFindStartBecomingDormant = 0xc24b798;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0x91bfe84;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0x8ca046c;
        inline constexpr uint64_t kFindTickFlush = 0xc459c74;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0x9c9f8a4;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0xa3454a4;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0xa800db4;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0xa0f7e50;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0xa48a3a8;
        inline constexpr uint64_t kInline_OnAuthorityRandomUpgradeAppliedCaller = 0x9dc4fec;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0xa2c2a18;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0xa8bfc38;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0x9225be4;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0x8a81478;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0x9c9f8a4;
    }
}
