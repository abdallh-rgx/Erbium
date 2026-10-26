#pragma once
#include "../../pch.h"
#include "Utils.h"

// these dont really fit into a class
class Misc
{
public:
    static inline bool bHookedAll = false;

    // ── listen-server state (Android listen mode) ────────────────────────
    // bHostingActive: flipped by the world watcher the moment WorldTick is
    // ticking a battle-royale terrain map — GetNetMode() then reports
    // NM_ListenServer (2) instead of the lobby's NM_Standalone (0).
    static inline bool bHostingActive = false;
    // bNetDriverListening: set once the GameNetDriver accepted connections
    // (InitListen succeeded) — guards ReadyToStartMatch_'s inline bring-up
    // from double-creating the driver when the watcher already listened.
    static inline bool bNetDriverListening = false;
    // HostedWorld: the terrain world currently being hosted (listen mode).
    // GetNetMode() reports NM_ListenServer only while this world is the live
    // one — leaving to the frontend reports NM_Standalone again (vanilla).
    static inline void* HostedWorld = nullptr;

    static int GetNetMode();
    DefHookOg(void*, SendRequestNow, void*, void*, int);
    DefHookOg(float, GetMaxTickRate, UEngine*, float, bool);
    static uint32 CheckCheckpointHeartBeat();
    DefHookOg(void, ApplyHomebaseEffectsOnPlayerSetup, __int64*, __int64, __int64, __int64, UObject*, char, uint8_t);
    static void InitClient();

    // Full listen bring-up, called by the Android world watcher when the
    // world switches to a terrain map: NM_ListenServer + GIsServer + NetDriver
    // InitListen(Port) + WarmupRequiredPlayerCount=1. The local player is
    // kept alive — this is a listen server, not a dedicated host.
    static bool ArmListenHosting();

    InitHooks;
};

// UWorld-level listen bring-up (NetDriver + InitListen + LevelCollections).
bool Listen();