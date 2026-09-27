#!/usr/bin/env python3
"""gen_functions_bake.py — assemble the final function bake from the find82
results + hand-verified discoveries, emitting:
  - android/Generated/functions-21.30.json  (full provenance data)
  - android/Generated/offsets-21.30.h       (kFn_* section appended)

Every entry: {addr, method, anchor?, verified?}
  method: string-anchor | reflected-native | vtable-slot | semantic-disasm
          | core (prior bake) | runtime-reflection (TODO marker)
"""
import json, os, sys, re

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))

# ── hand-verified core + networking (vtable / semantic) ────────────────────
CORE = [
    # (name, addr, method, note)
    ("InitBase",        "0x3851c5c", "vtable-slot",   "UIpNetDriver::InitBase — vtable+0x2d8; verified: InitListen calls it first"),
    ("InitConnect",     "0x38525a4", "vtable-slot",   "UIpNetDriver::InitConnect — vtable+0x2e0; calls InitBase virtually"),
    ("InitListen",      "0x3852a0c", "vtable-slot",   "UIpNetDriver::InitListen — vtable+0x2e8; owns '%s IpNetDriver listening on port %i' log; calls InitBase → SetWorld-flow → socket GetPort into URL.Port"),
    ("GetNetMode",      "0x992f0e0", "semantic-disasm", "UWorld::GetNetMode — returns NM_Client(3) when NetDriver==null; delegates to NetDriver path"),
    ("UWorld_Listen",   "0x992bc7c", "semantic-disasm", "UWorld::Listen — WorldContext loop → FindNamedNetDriver flow"),
    ("LoadMap",         "0x98c8bc0", "string-anchor", "UGameEngine::LoadMap — owns 'LoadMap: failed to Listen(%s)'"),
    ("ListenCall",      "0x98c4e34", "semantic-disasm", "BL site calling UWorld::Listen inside LoadMap (FindListenCall equivalent — patch target)"),
    ("FindNamedNetDriver", "0x98bb050", "semantic-disasm", "UEngine::FindNamedNetDriver — iterates [Engine+0xc78] comparing [x+0x2b8] (NetDriverName FName)"),
    ("OnRep_ZiplineState", "0x65d22d0", "reflected-native", "via registration thunk 0x70acd24; verified: reads [x0+0x1a90] bIsZiplining, [x0+0x2480] ZiplineActor"),
    ("HandlePostSafeZonePhaseChanged", "0x5504ac4", "reflected-native", "big impl (sub sp,#0x160) — MegaStorm safe-zone phases; string-anchor gave the wrapper 0x55045c4"),
]

# ── string-anchored (bake82 results — tight-window owner resolution) ──────
BAKE82 = [
    ("ActorChannelClose",             "0x92f8d64", "UActorChannel::Close — 'ChIndex: %d, Actor: %s'"),
    ("IsNetRelevantFor",              "0x90784d8", "AActor::IsNetRelevantFor — 'no root component' log"),
    ("CreateChannel",                 "0x9519af0", "UNetConnection::CreateChannelByName — STAT_NetConnection_CreateChannelByName"),
    ("ReplicateActor",                "0x92fbd9c", "UActorChannel::ReplicateActor — STAT_NetReplicateActorTime"),
    ("SetChannelActorForDestroy",     "0x930cffc", "'SetChannelActorForDestroy: Channel %d.'"),
    ("SendClientAdjustment",          "0x921e620", "UCharacterMovementComponent::SendClientAdjustment"),
    ("HandleMatchHasStarted",         "0x552ab84", "AFortGameModeAthena::HandleMatchHasStarted"),
    ("SpawnInitialSafeZone",          "0x5505948", "'bShouldSpawnSafeZoneIndicator == false' log"),
    ("StartAircraftPhase",            "0x5509de8", "STAT_StartAircraftPhase"),
    ("PickTeam",                      "0x550d590", "'PickTeam for [%s] used beacon value'"),
    ("KickPlayer",                    "0x62b0c38", "'Validation Failure: %s. kicking %s'"),
    ("NotifyGameMemberAdded",         "0x556bb34", "'Adding Player state with UniqueId'"),
    ("EnterAircraft",                 "0x5a8227c", "'attempting to enter aircraft after having already exited'"),
    ("FinishWorldInitialization",     "0x551ba34", "'Can't find a FortAthenaMapInfo placed in map.'"),
    ("GiveAbility",                   "0x3176468", "'GiveAbilityAndActivateOnce called on ability %s on the client' — SHARED anchor with GAEO, disambiguate by disasm before hooking"),
    ("GiveAbilityAndActivateOnce",    "0x3176468", "same anchor as GiveAbility — AMBIGUOUS, verify before use"),
    ("InternalTryActivateAbility",    "0x317a94c", "'InternalTryActivateAbility called with invalid Handle!'"),
    ("InitializePlayerGameplayAbilities", "0x5e81e10", "'InitializePlayerGameplayAbilities with invalid PlayerStateOrProxy!'"),
    ("InitializeBuildingActor",       "0x591c5e4", "STAT_Fort_BuildingSMActorInitializeBuildingActor"),
    ("ReplaceBuildingActor",          "0x5932630", "STAT_Fort_BuildingSMActorReplaceBuildingActor"),
    ("PayBuildableClassPlacementCost", "0x665ad20", "'Failed to remove item %s during pay building costs'"),
    ("CanAffordToPlaceBuildableClass", "0x665ab4c", "'Resource not found! Resource Type is %i'"),
    ("SelectAndSetupMyBuildingLevel", "0x5899b90", "'SelectAndSetupMyBuildingLevel - Cannot get WorldManager!!' — owner chain"),
    ("StreamInMyBuilding",            "0x587ae00", "'%s.%s trying to load invalid level %s' — AMBIGUOUS with SelectAndSetup region"),
    ("GetPlayerViewPoint",            "0x55cb164", "'%s failed to spawn a pawn'"),
    ("SendRequestNow",                "0x36aad64", "'MCP-Profile: Dispatching request to %s'"),
    ("GameSessionPatch",              "0x5553e04", "'Gamephase Step: %s' — containing function; the actual patch byte is the season-21 gate inside"),
    ("QueueStatEvent",                "0x674a5bc", "UFortQuestManager::QueueStatEvent"),
    ("ApplyCharacterCustomization",   "0x66e6388", "AFortPlayerState::ApplyCharacterCustomization"),
    ("ActivatePhase",                 "0x4863308", "ASpecialEventScript::ActivatePhase"),
    ("UnEquipVehicleWeapon",          "0x6a1e5dc", "UFortVehicleSeatWeaponComponent::UnEquipVehicleWeapon"),
    ("StartStreamingAdditionalPlaylistLevel", "0x5e48430", "'PLAYLIST: Failed to locate valid level'"),
    ("SetState",                      "0x618c658", "'Time from Setup to InProgress: %6.2fms'"),
    ("GetMaxTickRate",                "0x98bb160", "'Hitching by request!' — UGameEngine::GetMaxTickRate"),
    ("SpawnDeco",                     "0x6a91214", "AFortTrapTool::SpawnDeco — 'World is tearing down'"),
    ("ShouldAllowServerSpawnDeco",    "0x6a8463c", "'Tried to place deco item %s %s'"),
]

# ── runtime-reflection TODOs (resolve via GObjects once SDK-Init lands) ────
RUNTIME = [
    "GObjects", "GNames", "GetWorldContext", "CreateNetDriver", "CreateNetDriverWorldContext",
    "SetWorld", "TickFlush", "ServerReplicateActors", "GetNamePool", "EncryptionPatch",
    "SetChannelActor", "ClientHasInitializedLevelFor", "CallPreReplication",
    "SendDestructionInfo", "StartBecomingDormant", "FlushDormancy", "IsNetReady",
    "IsNetRelevantForVft", "OnItemInstanceAddedVft", "SpawnDecoVft", "ShouldAllowServerSpawnDecoVft",
    "RemoveInventoryItem", "RemoveInventoryStateValue", "SetInventoryStateValue",
    "RemoveFromAlivePlayers", "UpdateSafeZonesPhase", "PickSupplyDropLocation", "SpawnLoot",
    "SetPickupTarget", "SetPickupItems", "CantBuild", "CanPlaceBuildableClassInStructuralGrid",
    "ConstructAbilitySpec", "ClearAbility", "FinishedTargetSpline", "MinigameSettingsBuilding__BeginPlay",
    "PostInitializeSpawnedBuildingActor", "Reset", "SetGamePhase", "InitializeFlightPath",
    "UpdateIrisReplicationViews", "PreSendUpdate", "LoadPlayset", "KickPlayer? resolved",
    "GameSessionPatchByte", "ListenRedirect",
]
RUNTIME = sorted(set(x for x in RUNTIME if not x.endswith("? resolved")))

def main():
    functions = {}
    provenance = []
    for name, addr, method, note in CORE:
        functions[name] = int(addr, 16)
        provenance.append({"name": name, "addr": addr, "method": method, "note": note, "status": "resolved"})
    for name, addr, note in BAKE82:
        functions[name] = int(addr, 16)
        provenance.append({"name": name, "addr": addr, "method": "string-anchor", "note": note,
                           "status": "ambiguous" if "AMBIGUOUS" in note else "resolved"})
    for name in RUNTIME:
        provenance.append({"name": name, "method": "runtime-reflection", "status": "todo"})

    out_json = {
        "lib": "libUnreal.so",
        "version": "21.30.0-CL-21088273",
        "generated_by": "scripts/find82 (see README)",
        "summary": {
            "resolved": sum(1 for p in provenance if p["status"] in ("resolved", "ambiguous")),
            "ambiguous": sum(1 for p in provenance if p["status"] == "ambiguous"),
            "todo_runtime": sum(1 for p in provenance if p["status"] == "todo"),
        },
        "functions": provenance,
    }
    jp = os.path.join(REPO, "android", "Generated", "functions-21.30.json")
    json.dump(out_json, open(jp, "w"), indent=1)

    # insert kFn_ section INSIDE the namespace, before its closing brace
    hp = os.path.join(REPO, "android", "Generated", "offsets-21.30.h")
    hdr = open(hp).read()
    marker = "// ── function bake (find82)"
    if marker in hdr:
        hdr = hdr[:hdr.index(marker)]
    if hdr.rstrip().endswith("} // namespace Erbium::Baked"):
        close = "} // namespace Erbium::Baked"
        body = hdr[:hdr.rstrip().rindex(close)]
    else:
        body, close = hdr, None
    lines = [marker, "// resolved by scripts/find82 — see functions-21.30.json for provenance", ""]
    for name, addr in sorted(functions.items()):
        lines.append(f"inline constexpr uint64_t kFn_{name} = {addr}; // {int(addr):#x}")
    lines.append("")
    tail = ("\n" + close + "\n") if close else ""
    open(hp, "w").write(body + "\n".join(lines) + tail)

    print(f"resolved: {out_json['summary']['resolved']}  ambiguous: {out_json['summary']['ambiguous']}  runtime-todo: {out_json['summary']['todo_runtime']}")
    print(f"wrote {jp}")

if __name__ == "__main__":
    main()
