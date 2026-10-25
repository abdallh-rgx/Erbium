// GENERATED from mining data — v7.40 (FN 7.40, UE 4.20, libUE4.so)
// Do not edit; regenerate via android/erbium/bake.py
#pragma once
#define ERBIUM_BAKED_VERSION 7.40
#define ERBIUM_BAKED_ENGINE 4.20
#define ERBIUM_BAKED_LIB "libUE4.so"
namespace ErbiumBaked {
    inline constexpr uint64_t kAppendString = 0;
    inline constexpr uint64_t kFNameConstructor = 0x48eafb8;
    inline constexpr uint64_t kGEngine = 0xa739d88;
    inline constexpr uint64_t kGObjects = 0xa781f30;
    inline constexpr uint64_t kProcessEvent = 0x4eadcac;
    inline constexpr uint64_t kRealloc = 0;
    inline constexpr uint64_t kReleaseString = 0;
    inline constexpr uint64_t kSpawnActor = 0x47edb08;
    inline constexpr uint64_t kStaticFindObject = 0x4db7ccc;
    inline constexpr uint64_t kStaticLoadObject = 0;
    // verified-unique ARM64 patterns (fast path before runtime chains)
    namespace Pats {
        inline constexpr const char* kFNameConstructor = "E8 03 02 2A C1 00 00 B4 E4 03 00 32 05 00 80 12 E2 03 1F 2A E3 03 08 2A ?? ?? ?? ?? 1F 00 00 F9 C0 03 5F D6 FF 03 04 D1 F5 6B 00 F9 F4 4F 0E A9";
        inline constexpr const char* kProcessEvent = "FF 83 06 D1 FC A3 00 F9 FA 67 15 A9 F8 5F 16 A9 F6 57 17 A9 F4 4F 18 A9 FD 7B 19 A9 FD 43 06 91 F4 03 01 AA F8 03 14 AA 1F 0F 03 F8 BF 83 1B F8";
        inline constexpr const char* kSpawnActor = "EF 3B B6 6D ED 33 01 6D EB 2B 02 6D E9 23 03 6D FC 6F 04 A9 FA 67 05 A9 F8 5F 06 A9 F6 57 07 A9 F4 4F 08 A9 FD 7B 09 A9 FD 43 02 91 FF 43 08 D1 5B D0 3B D5 68 17 40 F9 F4 03 03 AA F9 03 02 AA";
        inline constexpr const char* kStaticFindObject = "FF 83 01 D1 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FD 7B 05 A9 FD 43 01 91 ?? ?? ?? ?? 08 01 4E 39";
    }
    // v6-verified game function addresses (fast path for Finders)
    namespace Game {
        inline constexpr uint64_t kFindApplyCharacterCustomization = 0x63f6b80;
        inline constexpr uint64_t kFindCanAffordToPlaceBuildableClass = 0x676c810;
        inline constexpr uint64_t kFindCantBuild = 0x6587ac4;
        inline constexpr uint64_t kFindClearAbility = 0x2863dd4;
        inline constexpr uint64_t kFindCloseActorChannel = 0x4b51c04;
        inline constexpr uint64_t kFindConstructAbilitySpec = 0x28a0c00;
        inline constexpr uint64_t kFindEnterAircraft = 0x67777d4;
        inline constexpr uint64_t kFindFinishWorldInitialization = 0x65b7bd0;
        inline constexpr uint64_t kFindGIsClient = 0xa83a898;
        inline constexpr uint64_t kFindGIsServer = 0xa845d80;
        inline constexpr uint64_t kFindGetMaxTickRate = 0x4bac190;
        inline constexpr uint64_t kFindGetNetMode = 0x453c040;
        inline constexpr uint64_t kFindGetPlayerViewPoint = 0x5bdba84;
        inline constexpr uint64_t kFindGiveAbility = 0x2862b94;
        inline constexpr uint64_t kFindGiveAbilityAndActivateOnce = 0x2862b94;
        inline constexpr uint64_t kFindHandleMatchHasStarted = 0x62e284c;
        inline constexpr uint64_t kFindHandlePostSafeZonePhaseChanged = 0x61f7710;
        inline constexpr uint64_t kFindInitListen = 0x494861c;
        inline constexpr uint64_t kFindInitializePlayerGameplayAbilities = 0x691c224;
        inline constexpr uint64_t kFindInternalTryActivateAbility = 0x2862d78;
        inline constexpr uint64_t kFindIsNetRelevantForVft = 0x4c85970;
        inline constexpr uint64_t kFindKickPlayer = 0x431c16c;
        inline constexpr uint64_t kFindListenCall = 0x46603a4;
        inline constexpr uint64_t kFindLoadPlayset = 0x68dc400;
        inline constexpr uint64_t kFindOnRep_ZiplineState = 0x64bee74;
        inline constexpr uint64_t kFindPayBuildableClassPlacementCost = 0x65602b0;
        inline constexpr uint64_t kFindPickSupplyDropLocation = 0x5d300ac;
        inline constexpr uint64_t kFindPickTeam = 0x5febaa0;
        inline constexpr uint64_t kFindPostInitializeSpawnedBuildingActor = 0x5fca080;
        inline constexpr uint64_t kFindRemoveFromAlivePlayers = 0x5d857d0;
        inline constexpr uint64_t kFindRemoveInventoryItem = 0x6afaa20;
        inline constexpr uint64_t kFindRemoveInventoryStateValue = 0x6da10bc;
        inline constexpr uint64_t kFindReset = 0x3b5e570;
        inline constexpr uint64_t kFindSelectAndSetupMyBuildingLevel = 0x629a0c0;
        inline constexpr uint64_t kFindSendDestructionInfo = 0x4b92d60;
        inline constexpr uint64_t kFindSendRequestNow = 0x2e38e74;
        inline constexpr uint64_t kFindSetChannelActor = 0x61aa538;
        inline constexpr uint64_t kFindSetChannelActorForDestroy = 0x4b37f00;
        inline constexpr uint64_t kFindSetInventoryStateValue = 0x6bcb920;
        inline constexpr uint64_t kFindSetPickupItems = 0x32e2274;
        inline constexpr uint64_t kFindSetPickupTarget = 0x6016b8c;
        inline constexpr uint64_t kFindSetWorld = 0x4d88938;
        inline constexpr uint64_t kFindShouldAllowServerSpawnDecoVft = 0x5bf81f0;
        inline constexpr uint64_t kFindSpawnDecoVft = 0x6755aac;
        inline constexpr uint64_t kFindStartAircraftPhase = 0x6765748;
        inline constexpr uint64_t kFindStartBecomingDormant = 0x48a9544;
        inline constexpr uint64_t kFindStartStreamingAdditionalPlaylistLevel = 0x5c2f4e8;
        inline constexpr uint64_t kFindStreamInMyBuilding = 0x5e02a10;
        inline constexpr uint64_t kFindTickFlush = 0x4702d74;
        inline constexpr uint64_t kInline_ClientOnPawnDiedCaller = 0x23d1670;
        inline constexpr uint64_t kInline_CollectGarbagePatch = 0x4dd81bc;
        inline constexpr uint64_t kInline_K2_OnUnEquipCaller = 0x24ac1c0;
        inline constexpr uint64_t kInline_OnDamageServerCaller = 0x2484d1c;
        inline constexpr uint64_t kInline_OnPawnEnterSeatFunc = 0x5bdd8c0;
        inline constexpr uint64_t kInline_ServerOnAttemptInteract = 0x64ec248;
        inline constexpr uint64_t kInline_ToDeathCauseNative = 0x4d88938;
    }
}
