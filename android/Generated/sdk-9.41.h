// GENERATED from mining data — v10.40 (FN 9.41, UE 4.23, libUE4.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 9.41
#define ERBIUM_BAKED_ENGINE 4.23
#define ERBIUM_BAKED_LIB "libUE4.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0;
    inline constexpr uint64_t kFNameConstructor = 0x5c2ac28;
    inline constexpr uint64_t kGEngine = 0xc9ec658;
    inline constexpr uint64_t kGObjects = 0xca61bc0;
    inline constexpr uint64_t kProcessEvent = 0x5d8bfd8;
    inline constexpr uint64_t kRealloc = 0x5ae2798;
    inline constexpr uint64_t kReleaseString = 0;
    inline constexpr uint64_t kSpawnActor = 0x720641c;
    inline constexpr uint64_t kStaticFindObject = 0x5da14fc;
    inline constexpr uint64_t kStaticLoadObject = 0x5da2e4c;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kFNameConstructor = "E8 03 02 2A C1 00 00 B4 E4 03 00 32 05 00 80 12 E2 03 1F 2A E3 03 08 2A ?? ?? ?? ?? 1F 00 00 F9 C0 03 5F D6 FC 6F BA A9 FA 67 01 A9 F8 5F 02 A9";
        inline constexpr const char* kProcessEvent = "FF C3 06 D1 FC 6F 15 A9 FA 67 16 A9 F8 5F 17 A9 F6 57 18 A9 F4 4F 19 A9 FD 7B 1A A9 FD 83 06 91 48 D0 3B D5 08 15 40 F9 F4 03 01 AA F9 03 14 AA";
        inline constexpr const char* kRealloc = "F7 0F 1C F8 F6 57 01 A9 F4 4F 02 A9 FD 7B 03 A9 FD C3 00 91 ?? ?? ?? ?? D6 D6 45 F9 F3 03 02 2A";
        inline constexpr const char* kSpawnActor = "EF 3B B6 6D ED 33 01 6D EB 2B 02 6D E9 23 03 6D FC 6F 04 A9 FA 67 05 A9 F8 5F 06 A9 F6 57 07 A9 F4 4F 08 A9 FD 7B 09 A9 FD 43 02 91 FF 03 09 D1 48 D0 3B D5 08 15 40 F9 ?? ?? ?? ?? 18 D3 42 F9";
        inline constexpr const char* kStaticFindObject = "FF 83 01 D1 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FD 7B 05 A9 FD 43 01 91 ?? ?? ?? ?? 08 E1 6F 39";
        inline constexpr const char* kStaticLoadObject = "FF 83 04 D1 FC 6F 0C A9 FA 67 0D A9 F8 5F 0E A9 F6 57 0F A9 F4 4F 10 A9 FD 7B 11 A9 FD 43 04 91 E6 27 00 B9 ?? ?? ?? ?? 08 D1 42 F9 F3 03 07 AA";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0x6f8c2e0;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0x7c3b830;
        inline constexpr uint64_t kFindClearAbility = 0x31df4cc;
        inline constexpr uint64_t kFindCloseActorChannel = 0x5853fa8;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0x321a8a8;
        inline constexpr uint64_t kFindEnterAircraft = 0x7293e98;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0x7382d08;
        inline constexpr uint64_t kFindGIsClient = 0xcec4ad4;
        inline constexpr uint64_t kFindGIsServer = 0xcfa86f0;
        inline constexpr uint64_t kFindGameSessionPatch = 0x3519890;
        inline constexpr uint64_t kFindGetMaxTickRate = 0x5ca2848;
        inline constexpr uint64_t kFindGetNetMode = 0x54e336c;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0x6cf8dc4;
        inline constexpr uint64_t kFindGiveAbility = 0x31de360;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x31de360;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0x7c2c758;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0x7689130;
        inline constexpr uint64_t kFindInitListen = 0x61ca36c;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0x7bbc498;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x31de544;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0x56e4d20;
        inline constexpr uint64_t kFindKickPlayer = 0x4e9a4d8;
        inline constexpr uint64_t kFindListenCall = 0x5a16734;
        inline constexpr uint64_t kFindLoadPlayset = 0x7a734d8;
        inline constexpr uint64_t kFindOnRep_ZiplineState = 0x726dee0;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0x74e749c;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0x773c2a4;
        inline constexpr uint64_t kFindPickTeam = 0x7ae2b64;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0x764d198;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0x7a26d3c;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0x7f878f8;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0x7e59828;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0x73ee464;
        inline constexpr uint64_t kFindSendDestructionInfo = 0x4f13f54;
        inline constexpr uint64_t kFindSendRequestNow = 0x350757c;
        inline constexpr uint64_t kFindSetChannelActor = 0x72db810;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0x5b04b64;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0x7f73d88;
        inline constexpr uint64_t kFindSetPickupItems = 0x2fe1a64;
        inline constexpr uint64_t kFindSetPickupTarget = 0x76b2d88;
        inline constexpr uint64_t kFindSetState = 0x7098ef8;
        inline constexpr uint64_t kFindSetWorld = 0x6672c90;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0x7b7ff40;
        inline constexpr uint64_t kFindSpawnDecoVft = 0x75a1048;
        inline constexpr uint64_t kFindStartAircraftPhase = 0x749f52c;
        inline constexpr uint64_t kFindStartBecomingDormant = 0x57f3804;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0x6e4fe14;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0x74a62c0;
        inline constexpr uint64_t kFindTickFlush = 0x562f658;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0x73f10dc;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0x2af6d58;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0x60619bc;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0x8259738;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0x2b03d70;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0x2c54de4;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0x58a3000;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0x712a708;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0x4e9a4d8;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0x73f10dc;
    }
}
