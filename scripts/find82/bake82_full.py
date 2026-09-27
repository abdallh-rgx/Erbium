#!/usr/bin/env python3
"""bake82 full — resolve all string-anchored Erbium functions in libUnreal.so 21.30.

Output: results82.json with per-function {addr, owners, method, verified}
"""
import sys, json, os
sys.path.insert(0, '/home/z/Erbium/scripts/find82')
import bake82, h

# (name, anchor string, pick, notes)
#   pick: 'single' — expect exactly 1 owner (the target function)
#         'first'  — first owner by address
#         'owner'  — owner whose range is closest below the xref
#         'firstbl'— owner function's first BL call target (GetNetMode-style)
ANCHORS = [
    # ── networking core ──────────────────────────────────────────────
    ("InitListen",        "%s IpNetDriver listening on port %i", "owner"),
    ("GetNetMode",        "PREPHYSBONES", "firstbl"),
    ("ActorChannelClose", "UActorChannel::Close: ChIndex: %d, Actor: %s", "owner"),
    ("IsNetRelevantFor",  "Actor %s / %s has no root component in AActor::IsNetRelevantFor", "owner"),
    ("CreateChannel",     "STAT_NetConnection_CreateChannelByName", "owner"),
    ("ReplicateActor",    "STAT_NetReplicateActorTime", "owner"),
    ("SendDestructionInfo", "STAT_NetSendDestructionInfo", "owner"),
    ("SetChannelActorForDestroy", "SetChannelActorForDestroy: Channel %d.", "owner"),
    ("StartBecomingDormant", "StartBecomingDormant: %s", "owner"),
    ("FlushDormancy",     "FlushDormancy: %s. Connection: %s", "owner"),
    ("IsNetReady",        "%s failed to spawn a pawn", "firstbl"),
    ("SendClientAdjustment", "STAT_CharacterMovement", "owner"),
    ("ListenCall",        "LoadMap: failed to Listen(%s)", "firstbl"),
    # ── game mode / athena ──────────────────────────────────────────
    ("HandleMatchHasStarted", "AFortGameModeAthena::HandleMatchHasStarted: NumPlayers: %i", "owner"),
    ("HandlePostSafeZonePhaseChanged", "FortGameModeAthena: No MegaStorm on SafeZone[%d].", "owner"),
    ("SpawnInitialSafeZone", "FortGameModeAthena::SpawnInitialSafeZone bShouldSpawnSafeZoneIndicator == false", "owner"),
    ("RemoveFromAlivePlayers", "FortGameModeAthena: Player [%s] removed from alive players list", "owner"),
    ("StartAircraftPhase", "STAT_StartAircraftPhase", "owner"),
    ("UpdateSafeZonesPhase", "UpdateSafeZones", "owner"),
    ("PickTeam",          "PickTeam for [%s] used beacon value [%d]", "single"),
    ("KickPlayer",        "Validation Failure: %s. kicking %s", "owner"),
    ("NotifyGameMemberAdded", "%s: Adding Player state with UniqueId: %s, in team: %d, and in squad: %d", "single"),
    ("EnterAircraft",     "EnterAircraft: [%s] is attempting to enter aircraft after having already exited.", "single"),
    ("FinishWorldInitialization", "Can't find a FortAthenaMapInfo placed in map.", "owner"),
    ("PickSupplyDropLocation", "PickSupplyDropLocation", "owner"),
    # ── abilities / inventory ────────────────────────────────────────
    ("GiveAbility",       "GiveAbilityAndActivateOnce called on ability %s on the client, not allowed!", "owner"),
    ("GiveAbilityAndActivateOnce", "GiveAbilityAndActivateOnce called on ability %s on the client, not allowed!", "owner"),
    ("InternalTryActivateAbility", "InternalTryActivateAbility called with invalid Handle! ASC: %s. AvatarActor: %s", "single"),
    ("InitializePlayerGameplayAbilities", "InitializePlayerGameplayAbilities with invalid PlayerStateOrProxy!", "single"),
    ("OnRep_ZiplineState", "ZIPLINES", "owner"),
    ("RemoveInventoryItem", "ServerRemoveInventoryItem", "owner"),
    ("RemoveInventoryStateValue", "ServerRemoveInventoryStateValue", "owner"),
    ("SetInventoryStateValue", "ServerSetInventoryStateValue", "owner"),
    # ── building ─────────────────────────────────────────────────────
    ("InitializeBuildingActor", "STAT_Fort_BuildingSMActorInitializeBuildingActor", "owner"),
    ("ReplaceBuildingActor", "STAT_Fort_BuildingSMActorReplaceBuildingActor", "owner"),
    ("CantBuild",         "CantBuild", "owner"),
    ("CanPlaceBuildableClassInStructuralGrid", "Invalid Structural Grid", "owner"),
    ("PayBuildableClassPlacementCost", "Failed to remove item %s during pay building costs, item duplicated!", "owner"),
    ("CanAffordToPlaceBuildableClass", "Resource not found! Resource Type is %i, might be invalid", "owner"),
    ("SelectAndSetupMyBuildingLevel", "ABuildingFoundation::SelectAndSetupMyBuildingLevel - Cannot get WorldManager!!", "owner"),
    ("StreamInMyBuilding", "%s.%s trying to load invalid level %s", "owner"),
    ("SpawnLoot",         "ABuildingContainer::SpawnLoot", "owner"),
    ("SetPickupTarget",   "Attempted to spawn non-world item %s!", "owner"),
    ("SetPickupItems",    "SetPickupItems", "owner"),
    # ── misc ─────────────────────────────────────────────────────────
    ("GetPlayerViewPoint", "%s failed to spawn a pawn", "owner"),
    ("SendRequestNow",    "MCP-Profile: Dispatching request to %s", "owner"),
    ("GameSessionPatch",  "Gamephase Step: %s", "owner"),
    ("EncryptionPatch",   "net.UseEncryption", "firstbl"),
    ("QueueStatEvent",    "UFortQuestManager::QueueStatEvent: %s tried to queue a stat event", "owner"),
    ("ApplyCharacterCustomization", "AFortPlayerState::ApplyCharacterCustomization - Failed initialization", "owner"),
    ("ActivatePhase",     "[ASpecialEventScript::ActivatePhase()]", "owner"),
    ("UnEquipVehicleWeapon", "UFortVehicleSeatWeaponComponent::UnEquipVehicleWeapon failed to cleanup", "owner"),
    ("StartStreamingAdditionalPlaylistLevel", "PLAYLIST: Failed to locate valid level to be streamed %s", "owner"),
    ("SetState",          "Time from Setup to InProgress: %6.2fms", "owner"),
    ("GetMaxTickRate",    "Hitching by request!", "owner"),
    ("SpawnDeco",         "AFortTrapTool::SpawnDeco World is tearing down", "owner"),
    ("ShouldAllowServerSpawnDeco", "Tried to place deco item %s %s that isn't actually in player inventory!", "owner"),
]

def resolve(name, anchor, pick):
    owners, xr, opcs = bake82.owner_of_string(anchor, window=0x40)
    if not owners:
        # no cold-fragment branches — the xref may be in the hot part
        if xr:
            f = h.find_func_start(xr[0])
            if f:
                owners = {(f, xr[0])}
        return {"name": name, "anchor": anchor, "xrefs": [hex(x) for x in xr],
                "owners": [], "addr": hex(f) if (f := (list(owners)[0][0] if owners else None)) else None,
                "method": "hot-direct", "pick": pick}
    olist = sorted(set(f for f, _ in owners))
    info = {"name": name, "anchor": anchor[:60],
            "xrefs": [hex(x) for x in xr], "pick": pick,
            "owners": [hex(f) for f in olist]}
    if pick == "single" and len(olist) == 1:
        info["addr"] = hex(olist[0]); info["method"] = "single-owner"
    elif pick == "first":
        info["addr"] = hex(olist[0]); info["method"] = "first-owner"
    elif pick == "owner":
        # owner closest below the xref (log usually late in function; owner
        # start is the function containing the branch)
        x0 = xr[0]
        below = [f for f in olist if f <= x0]
        info["addr"] = hex(max(below)) if below else hex(olist[0])
        info["method"] = "closest-owner"
    elif pick == "firstbl":
        f = olist[0] if olist else (h.find_func_start(xr[0]) if xr else None)
        if f:
            tgt = bake82.first_bl(f)
            info["addr"] = hex(tgt) if tgt else None
            info["anchor_func"] = hex(f)
            info["method"] = "first-bl"
        else:
            info["addr"] = None
    else:
        info["addr"] = None
    return info

def main():
    results = []
    for name, anchor, pick in ANCHORS:
        try:
            r = resolve(name, anchor, pick)
        except Exception as e:
            r = {"name": name, "error": str(e)}
        results.append(r)
        a = r.get("addr")
        print(f"{r['name']:38s} {'✓ '+a if a else '✗ no-addr':16s} owners={len(r.get('owners', []))}")
    json.dump(results, open('/home/z/Erbium/scripts/find82/results82.json', 'w'), indent=1)
    print("\nsaved results82.json")

if __name__ == "__main__":
    main()
