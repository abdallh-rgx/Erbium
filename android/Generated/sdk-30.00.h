// GENERATED from mining data — v32.00 (FN 30.00, UE 5.4, libUnreal.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 30.00
#define ERBIUM_BAKED_ENGINE 5.4
#define ERBIUM_BAKED_LIB "libUnreal.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0xa774890;
    inline constexpr uint64_t kFNameConstructor = 0xa240ab0;
    inline constexpr uint64_t kGEngine = 0x155b5da0;
    inline constexpr uint64_t kGObjects = 0;
    inline constexpr uint64_t kProcessEvent = 0xa23eb10;
    inline constexpr uint64_t kRealloc = 0xa22b218;
    inline constexpr uint64_t kReleaseString = 0x2183970;
    inline constexpr uint64_t kSpawnActor = 0xa92b220;
    inline constexpr uint64_t kStaticFindObject = 0;
    inline constexpr uint64_t kStaticLoadObject = 0;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kAppendString = "FD 7B BB A9 FC 67 01 A9 FD 03 00 91 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FF 03 20 D1 D7 66 96 52";
        inline constexpr const char* kFNameConstructor = "FD 7B BA A9 FC 6F 01 A9 FD 03 00 91 FA 67 02 A9 F8 5F 03 A9 F6 57 04 A9 F4 4F 05 A9 FF C3 11 D1 FC 03 00 AA E1 29 00 B4 29 00 40 79 A9 29 00 34";
        inline constexpr const char* kProcessEvent = "FF 83 05 D1 FD 7B 11 A9 FD 43 04 91 FC 67 12 A9 F8 5F 13 A9 F6 57 14 A9 F4 4F 15 A9 28 10 40 F9";
        inline constexpr const char* kRealloc = "FF C3 00 D1 FD 7B 01 A9 FD 43 00 91 F3 13 00 F9 ?? ?? ?? ?? E3 03 02 2A E2 03 01 AA E1 03 00 AA 29 65 42 F9 28 01 40 F9 08 01 00 B4 09 01 40 F9";
        inline constexpr const char* kSpawnActor = "EF 3B B6 6D ED 33 01 6D EB 2B 02 6D E9 23 03 6D FD 7B 04 A9 FD 03 01 91 FC 6F 05 A9 FA 67 06 A9 F8 5F 07 A9 F6 57 08 A9 F4 4F 09 A9 FF C3 0A D1 ?? ?? ?? ?? ?? ?? ?? ?? F5 03 01 AA 29 71 3B 91";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindActivatePhase = 0xf1f6210;
        inline constexpr uint64_t kFindCallPreReplication = 0x5dc1f24;
        inline constexpr uint64_t kFindCantBuild = 0x4f74824;
        inline constexpr uint64_t kFindClearAbility = 0x73ebb48;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0xf8a95b0;
        inline constexpr uint64_t kFindCreateChannel = 0xa296ae4;
        inline constexpr uint64_t kFindCreateNetDriverWorldContext = 0xee5c1c8;
        inline constexpr uint64_t kFindEnterAircraft = 0xbb3b050;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0x8e4ad50;
        inline constexpr uint64_t kFindFinishedTargetSpline = 0xc1f90f4;
        inline constexpr uint64_t kFindGIsClient = 0x13c47d7a;
        inline constexpr uint64_t kFindGIsServer = 0x13c47d7b;
        inline constexpr uint64_t kFindGameSessionPatch = 0x9845310;
        inline constexpr uint64_t kFindGetMaxTickRate = 0x6483040;
        inline constexpr uint64_t kFindGetNamePool = 0x13c56800;
        inline constexpr uint64_t kFindGetNetMode = 0x9439034;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0x62d3f8c;
        inline constexpr uint64_t kFindGetWorldContext = 0x6025aec;
        inline constexpr uint64_t kFindGiveAbility = 0x92b77c8;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x73e9b0c;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0x8e42090;
        inline constexpr uint64_t kFindInitializeBuildingActor = 0x91f465c;
        inline constexpr uint64_t kFindInitializeBuildingActor_next = 0x91f51a0;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0xbf7992c;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x73fc31c;
        inline constexpr uint64_t kFindKickPlayer = 0xc41c794;
        inline constexpr uint64_t kFindListenCall = 0x6485a04;
        inline constexpr uint64_t kFindLoadPlayset = 0xc8a8e6c;
        inline constexpr uint64_t kFindOnRep_ZiplineState = 0x9e29fc4;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0x8daff30;
        inline constexpr uint64_t kFindPickTeam = 0x8e4179c;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0x91f5384;
        inline constexpr uint64_t kFindPreSendUpdate = 0x534764c;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0xc7ec57c;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0xc7ec6dc;
        inline constexpr uint64_t kFindReplaceBuildingActor = 0x92012e8;
        inline constexpr uint64_t kFindReplicateActor = 0x5fb5e2c;
        inline constexpr uint64_t kFindReset = 0x53f6bcc;
        inline constexpr uint64_t kFindSendDestructionInfo = 0x61bd0f4;
        inline constexpr uint64_t kFindSendRequestNow = 0x751b394;
        inline constexpr uint64_t kFindSetChannelActor = 0x9160170;
        inline constexpr uint64_t kFindSetGamePhase = 0xcd2fc14;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0xc7ed498;
        inline constexpr uint64_t kFindSetPickupItems = 0xd41c968;
        inline constexpr uint64_t kFindSetPickupTarget = 0xc1fb1bc;
        inline constexpr uint64_t kFindSetState = 0xc2bf018;
        inline constexpr uint64_t kFindSetWorld = 0x9360c78;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0xccb70b0;
        inline constexpr uint64_t kFindSpawnLoot = 0x8d82cb8;
        inline constexpr uint64_t kFindStartAircraftPhase = 0xa111f00;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0xbf4e558;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0x919cbe0;
        inline constexpr uint64_t kFindTickFlush = 0x61b7764;
        inline constexpr uint64_t kFindUnEquipVehicleWeapon = 0xcc32530;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0xc89218c;
        inline constexpr uint64_t kInline_EquipVehicleWeaponFunc = 0x9360c78;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0xcce2824;
        inline constexpr uint64_t kInline_OnAuthorityRandomUpgradeAppliedCaller = 0x9194b80;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0x9194b80;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0x9382550;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0x976398c;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0xa323678;
        inline constexpr uint64_t kInline_UnequipVehicleWeaponFunc = 0xcc32530;
    }
}
