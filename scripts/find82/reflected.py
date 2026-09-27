#!/usr/bin/env python3
"""reflected-native resolver: exact UFunction name → registration pair →
thunk → native function address."""
import sys
sys.path.insert(0, '/home/z/Erbium/scripts/find82')
import h

TEXT_LO, TEXT_HI = h.TEXT_LO, h.TEXT_HI

def find_ascii_exact(s):
    pat = s.encode() + b'\x00'
    i = h.raw.find(pat)
    # restrict to .rodata (name table region)
    rod = h.elf.section('.rodata')
    j = h.raw.find(pat, rod['offset'], rod['offset'] + rod['size'])
    return rod['addr'] + (j - rod['offset']) if j >= 0 else None

def thunk_target(f):
    """native address from a registration thunk: scan ≤6 instrs for the
    unconditional tail-jump `b imm` (thunks = counter inc + b native)."""
    i0 = (f - TEXT_LO) // 4
    for i in range(i0, i0 + 6):
        if i >= len(h.st_arr):
            return None
        w = h.st_arr[i]
        if (w & 0xFC000000) == 0x14000000:  # b imm
            imm = w & 0x3FFFFFF
            if imm & 0x2000000: imm -= 0x4000000
            tgt = TEXT_LO + i*4 + imm*4
            # sanity: target must be a different address, in .text
            if TEXT_LO <= tgt < TEXT_HI:
                return tgt
    return None

def resolve_native(name):
    a = find_ascii_exact(name)
    if a is None:
        return {"name": name, "error": "name not found"}
    out = {"name": name, "name_addr": hex(a)}
    # relro slots pointing at the name string
    slots = [s for s, t in h.RELRO.items() if t == a]
    if not slots:
        out["error"] = "no relro pointer to name"
        return out
    results = []
    for s in slots:
        # neighbor slots: prev and next — pick .text targets
        for d in (-8, 8):
            fn = h.RELRO.get(s + d)
            if fn and TEXT_LO <= fn < TEXT_HI:
                entry = {"pair_slot": hex(s + d), "thunk": hex(fn)}
                tgt = thunk_target(fn)
                if tgt and TEXT_LO <= tgt < TEXT_HI:
                    entry["native"] = hex(tgt)
                results.append(entry)
    out["candidates"] = results
    return out

if __name__ == "__main__":
    names = [
        "OnRep_ZiplineState", "ServerRemoveInventoryItem",
        "ServerRemoveInventoryStateValue", "ServerSetInventoryStateValue",
        "UpdateSafeZonesPhase", "PickSupplyDropLocation", "SpawnLoot",
        "PickTeam", "EnterAircraft", "NotifyGameMemberAdded",
        "HandlePostSafeZonePhaseChanged", "SpawnInitialSafeZone",
        "StartAircraftPhase", "ServerCheat", "GiveAbility",
        "KickedFromGameSession", "OnItemInstanceAdded",
        "SpawnDeco", "ShouldAllowServerSpawnDeco", "RemoveFromAlivePlayers",
        "HandleMatchHasStarted", "SelectAndSetupMyBuildingLevel",
        "StreamInMyBuilding", "StartStreamingAdditionalPlaylistLevel",
        "UnEquipVehicleWeapon", "QueueStatEvent", "CantBuild",
        "InitializePlayerGameplayAbilities", "ApplyCharacterCustomization",
        "ActivatePhase", "SetState", "SetPickupTarget", "SetPickupItems",
        "GetNetMode", "AttemptDeriveFromURL", "SendClientAdjustment",
        "CallPreReplication", "ClientHasInitializedLevelFor",
        "SetChannelActor", "CreateChannel", "ReplicateActor",
        "CloseActorChannel", "SendDestructionInfo", "StartBecomingDormant",
        "FlushDormancy", "IsNetReady", "GetPlayerViewPoint",
        "InitializeBuildingActor", "ReplaceBuildingActor",
        "CanPlaceBuildableClassInStructuralGrid", "PayBuildableClassPlacementCost",
        "CanAffordToPlaceBuildableClass", "InternalTryActivateAbility",
        "GiveAbilityAndActivateOnce", "ClearAbility", "ConstructAbilitySpec",
        "FinishWorldInitialization", "InitializeFlightPath",
        "SetGamePhase", "Reset", "GetMaxTickRate", "SendRequestNow",
    ]
    for n in names:
        r = resolve_native(n)
        if "error" in r:
            print(f"{n:42s} ✗ {r['error']}")
        else:
            cands = r["candidates"]
            natives = {c.get("native") for c in cands if c.get("native")}
            thunks = {c["thunk"] for c in cands}
            print(f"{n:42s} name@{r['name_addr']} thunks={len(thunks)} natives={natives}")
