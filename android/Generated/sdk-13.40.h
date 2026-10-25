// GENERATED from mining data — v13.40 (FN 13.40, UE 4.26, libUE4.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 13.40
#define ERBIUM_BAKED_ENGINE 4.26
#define ERBIUM_BAKED_LIB "libUE4.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0xbbca6bc;
    inline constexpr uint64_t kFNameConstructor = 0xbbc96b0;
    inline constexpr uint64_t kGEngine = 0x12b4b270;
    inline constexpr uint64_t kGObjects = 0x12b8ecb0;
    inline constexpr uint64_t kProcessEvent = 0xbd02264;
    inline constexpr uint64_t kRealloc = 0xba96110;
    inline constexpr uint64_t kReleaseString = 0;
    inline constexpr uint64_t kSpawnActor = 0xc07e6b8;
    inline constexpr uint64_t kStaticFindObject = 0xbcf2d1c;
    inline constexpr uint64_t kStaticLoadObject = 0xbd075c4;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kAppendString = "F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ?? ?? ?? ?? C8 22 62 39 ?? ?? ?? ?? F3 03 01 AA F4 03 00 AA B5 02 23 91 C8 00 00 37 ?? ?? ?? ?? 00 00 23 91 ?? ?? ?? ?? 28 00 80 52 C8 22 22 39 88 02 40";
        inline constexpr const char* kFNameConstructor = "F3 0F 1E F8 FD 7B 01 A9 FD 43 00 91 E8 03 01 AA F3 03 00 AA A1 01 00 B4 0B 01 40 79 CB 01 00 34";
        inline constexpr const char* kProcessEvent = "FF 03 06 D1 FC 6F 12 A9 FA 67 13 A9 F8 5F 14 A9 F6 57 15 A9 F4 4F 16 A9 FD 7B 17 A9 FD C3 05 91 5A D0 3B D5 48 17 40 F9 FB 03 01 AA ?? ?? ?? ??";
        inline constexpr const char* kRealloc = "F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ?? ?? ?? ?? C8 9E 46 F9 F3 03 02 2A F4 03 01 AA F5 03 00 AA A8 00 00 B4 09 01 40 F9 E0 03 08 AA";
        inline constexpr const char* kSpawnActor = "42 B0 15 91 84 28 17 91 63 00 80 52 E0 03 1F AA E1 03 1F 2A ?? ?? ?? ??";
        inline constexpr const char* kStaticFindObject = "FF 43 02 D1 FC 6F 03 A9 FA 67 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9 FD 7B 08 A9 FD 03 02 91 59 D0 3B D5 28 17 40 F9 ?? ?? ?? ?? F3 03 03 2A";
        inline constexpr const char* kStaticLoadObject = "FF 43 03 D1 FC 6F 07 A9 FA 67 08 A9 F8 5F 09 A9 F6 57 0A A9 F4 4F 0B A9 FD 7B 0C A9 FD 03 03 91 E3 97 00 A9 54 D0 3B D5 88 16 40 F9 ?? ?? ?? ??";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0xb099f94;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0xb00348c;
        inline constexpr uint64_t kFindCantBuild = 0xdec4040;
        inline constexpr uint64_t kFindClearAbility = 0x67d3cd4;
        inline constexpr uint64_t kFindCloseActorChannel = 0xbf30aa8;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0x67fd050;
        inline constexpr uint64_t kFindCreateChannel = 0xc0eab64;
        inline constexpr uint64_t kFindEnterAircraft = 0xa93e874;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0xa5bcf9c;
        inline constexpr uint64_t kFindGIsClient = 0x12aefe30;
        inline constexpr uint64_t kFindGIsServer = 0x12811dc4;
        inline constexpr uint64_t kFindGameSessionPatch = 0xa5f7098;
        inline constexpr uint64_t kFindGetMaxTickRate = 0xc3aac3c;
        inline constexpr uint64_t kFindGetNamePool = 0x12b008c0;
        inline constexpr uint64_t kFindGetNetMode = 0xc4133f4;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0xc1f5798;
        inline constexpr uint64_t kFindGiveAbility = 0x67d3cd4;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x67d2bec;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0xa5baa5c;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0xa5d8650;
        inline constexpr uint64_t kFindInitListen = 0x9e32514;
        inline constexpr uint64_t kFindInitializeBuildingActor = 0xa86f108;
        inline constexpr uint64_t kFindInitializeBuildingActor_next = 0xa86fab4;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0xab6c90c;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x67d2e20;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0xbd592e0;
        inline constexpr uint64_t kFindKickPlayer = 0x6a9c194;
        inline constexpr uint64_t kFindListenCall = 0xc3b30a8;
        inline constexpr uint64_t kFindLoadPlayset = 0xb0a0b30;
        inline constexpr uint64_t kFindNotifyGameMemberAdded = 0xa5fff58;
        inline constexpr uint64_t kFindOnRep_ZiplineState = 0xafc5b34;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0xa2fe5c8;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0xa55020c;
        inline constexpr uint64_t kFindPickTeam = 0xa5bdedc;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0xa859950;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0xa5cc63c;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0xb875408;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0xb875608;
        inline constexpr uint64_t kFindReplaceBuildingActor = 0xa860560;
        inline constexpr uint64_t kFindReplicateActor = 0xbf35ecc;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0xa834ba8;
        inline constexpr uint64_t kFindSendDestructionInfo = 0xc1175a4;
        inline constexpr uint64_t kFindSendRequestNow = 0x723c1b4;
        inline constexpr uint64_t kFindServerReplicateActors = 0xd046b7c;
        inline constexpr uint64_t kFindSetChannelActor = 0xa7448bc;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0xbf3299c;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0xb8761f8;
        inline constexpr uint64_t kFindSetPickupTarget = 0xacece94;
        inline constexpr uint64_t kFindSetState = 0xad3ca90;
        inline constexpr uint64_t kFindSetWorld = 0xc11b238;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0xb381b64;
        inline constexpr uint64_t kFindSpawnDecoVft = 0xb39a758;
        inline constexpr uint64_t kFindSpawnInitialSafeZone = 0xa5d79f8;
        inline constexpr uint64_t kFindSpawnLoot = 0xa546450;
        inline constexpr uint64_t kFindStartAircraftPhase = 0xa5c3fa8;
        inline constexpr uint64_t kFindStartBecomingDormant = 0xbf37d40;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0xab3fac0;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0xa831be8;
        inline constexpr uint64_t kFindTickFlush = 0xc108ee0;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0xa6da880;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0xd259434;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0xbca2074;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0xbd1866c;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0xd298448;
        inline constexpr uint64_t kInline_OnAuthorityRandomUpgradeAppliedCaller = 0xd167da4;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0xd2485ac;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0xbd1866c;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0xab9052c;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0xa6ac6fc;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0xa6da880;
    }
}
