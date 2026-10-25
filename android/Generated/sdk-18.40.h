// GENERATED from mining data — v18.40 (FN 18.40, UE 5.0, libUE4.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 18.40
#define ERBIUM_BAKED_ENGINE 5.0
#define ERBIUM_BAKED_LIB "libUE4.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0xad46a34;
    inline constexpr uint64_t kFNameConstructor = 0xad46078;
    inline constexpr uint64_t kGEngine = 0x11584490;
    inline constexpr uint64_t kGObjects = 0x115d8ed0;
    inline constexpr uint64_t kProcessEvent = 0xad776e0;
    inline constexpr uint64_t kRealloc = 0xad23778;
    inline constexpr uint64_t kReleaseString = 0x47842ba;
    inline constexpr uint64_t kSpawnActor = 0xaa632bc;
    inline constexpr uint64_t kStaticFindObject = 0xad7100c;
    inline constexpr uint64_t kStaticLoadObject = 0xa95f4d4;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kAppendString = "FC 5F BC A9 F6 57 01 A9 F4 4F 02 A9 FD 7B 03 A9 FD C3 00 91 FF 43 20 D1 56 D0 3B D5 C8 16 40 F9 ?? ?? ?? ?? ?? ?? ?? ?? F3 03 01 AA A8 83 1C F8 E8 E2 64 39 F4 03 00 AA B5 02 25 91 48 08 00 36";
        inline constexpr const char* kFNameConstructor = "F3 0F 1E F8 FD 7B 01 A9 FD 43 00 91 E8 03 01 AA F3 03 00 AA 01 03 00 B4 0B 01 40 79 6B 02 00 34";
        inline constexpr const char* kProcessEvent = "FF C3 06 D1 FC 6F 15 A9 FA 67 16 A9 F8 5F 17 A9 F6 57 18 A9 F4 4F 19 A9 FD 7B 1A A9 FD 83 06 91 58 D0 3B D5 08 17 40 F9 F9 03 01 AA ?? ?? ?? ??";
        inline constexpr const char* kRealloc = "F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ?? ?? ?? ?? 28 E9 42 F9 E3 03 02 2A E2 03 01 AA";
        inline constexpr const char* kSpawnActor = "FF 43 07 D1 E9 23 16 6D FC 6F 17 A9 FA 67 18 A9 F8 5F 19 A9 F6 57 1A A9 F4 4F 1B A9 FD 7B 1C A9 FD 03 07 91 5A D0 3B D5 48 17 40 F9 ?? ?? ?? ??";
        inline constexpr const char* kStaticFindObject = "FF 43 02 D1 FC 6F 03 A9 FA 67 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9 FD 7B 08 A9 FD 03 02 91 59 D0 3B D5 28 17 40 F9 ?? ?? ?? ?? F3 03 03 2A";
        inline constexpr const char* kStaticLoadObject = "FF 83 03 D1 FC 6F 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9 F4 4F 0C A9 FD 7B 0D A9 FD 43 03 91 E6 27 00 B9 E3 97 00 A9 54 D0 3B D5 88 16 40 F9";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindActivatePhase = 0x78164fc;
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0xbe77420;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0xbe072e8;
        inline constexpr uint64_t kFindCantBuild = 0xe2e843c;
        inline constexpr uint64_t kFindClearAbility = 0x6be0554;
        inline constexpr uint64_t kFindCloseActorChannel = 0xc83f704;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0x90049a0;
        inline constexpr uint64_t kFindCreateChannel = 0xaa830c0;
        inline constexpr uint64_t kFindCreateNetDriverWorldContext = 0xb737690;
        inline constexpr uint64_t kFindEnterAircraft = 0xb81ae10;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0xb529928;
        inline constexpr uint64_t kFindGIsClient = 0x114d4868;
        inline constexpr uint64_t kFindGIsServer = 0x1154f4a8;
        inline constexpr uint64_t kFindGameSessionPatch = 0xa4170c8;
        inline constexpr uint64_t kFindGetMaxTickRate = 0xcaecf78;
        inline constexpr uint64_t kFindGetNamePool = 0x1150c940;
        inline constexpr uint64_t kFindGetNetMode = 0xaeb23b8;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0xad094a4;
        inline constexpr uint64_t kFindGiveAbility = 0x6be0554;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x6bdf4f4;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0xb5273b0;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0xb542874;
        inline constexpr uint64_t kFindInitListen = 0xaed00fc;
        inline constexpr uint64_t kFindInitializeBuildingActor = 0xb75abe0;
        inline constexpr uint64_t kFindInitializeBuildingActor_next = 0xb75b614;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0xba0ccbc;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x6bdf738;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0xc74da0c;
        inline constexpr uint64_t kFindKickPlayer = 0xbc21188;
        inline constexpr uint64_t kFindListenCall = 0xab5952c;
        inline constexpr uint64_t kFindLoadPlayset = 0xbe7cd78;
        inline constexpr uint64_t kFindNotifyGameMemberAdded = 0xa4191e4;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0xbe08748;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0xb4fb72c;
        inline constexpr uint64_t kFindPickTeam = 0xb52a740;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0xb75b614;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0xb535338;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0xc479178;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0xc47938c;
        inline constexpr uint64_t kFindReplaceBuildingActor = 0xb75dac8;
        inline constexpr uint64_t kFindReplicateActor = 0xc83fd80;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0xb744da4;
        inline constexpr uint64_t kFindSendDestructionInfo = 0xc971d88;
        inline constexpr uint64_t kFindSendRequestNow = 0x68f7eac;
        inline constexpr uint64_t kFindServerReplicateActors = 0xaceec90;
        inline constexpr uint64_t kFindSetChannelActor = 0x9772d94;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0xc83fbc0;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0xc47a25c;
        inline constexpr uint64_t kFindSetPickupItems = 0x862009c;
        inline constexpr uint64_t kFindSetPickupTarget = 0xbb3dfa0;
        inline constexpr uint64_t kFindSetState = 0xbb72554;
        inline constexpr uint64_t kFindSetWorld = 0xc973890;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0xc0f4d9c;
        inline constexpr uint64_t kFindSpawnDecoVft = 0xc9c8274;
        inline constexpr uint64_t kFindSpawnInitialSafeZone = 0xb541bb4;
        inline constexpr uint64_t kFindSpawnLoot = 0xb4d4c7c;
        inline constexpr uint64_t kFindStartAircraftPhase = 0xb52e3f8;
        inline constexpr uint64_t kFindStartBecomingDormant = 0xc84180c;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0xb9e5448;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0xa471468;
        inline constexpr uint64_t kFindTickFlush = 0xae33598;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0xc0e7018;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0xd82a1e4;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0xa93c518;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0xa966290;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0xd8755b8;
        inline constexpr uint64_t kInline_OnAuthorityRandomUpgradeAppliedCaller = 0xd703ae4;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0xd816504;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0xa966290;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0xa55a224;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0xb5ef6f8;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0xc0e7018;
    }
}
