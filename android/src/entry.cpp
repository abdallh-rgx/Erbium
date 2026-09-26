// entry.cpp — liberbium.so entry point.
//
// Loaded via System.loadLibrary("erbium") from the patched onCreate() smali.
// The constructor fires while the process is still in Java land, so we spawn
// a worker thread that waits for the UE4 module, then hands control to the
// (to-be-ported) Erbium server bootstrap.
#include "alog.h"
#include "platform.h"

#include <chrono>
#include <cstring>
#include <thread>
#include <unistd.h>

namespace Erbium
{
    // Baked per-version offsets produced by scripts/analyze_libue4.py.
    // Regenerated for each game version in CI.
    // (Populated from android/Generated/<version>_offsets.h in a later phase.)
    static constexpr const char* kTargetModules[] = {
        "libUE4.so",     // Chapter 1-2 era Android builds
        "libUnreal.so",  // Chapter 3+ era Android builds
    };

    static void Main()
    {
        LOGI("=== Erbium Android bootstrap (build " __DATE__ " " __TIME__ ") ===");
        LOGI("pid=%d uid=%d", (int)getpid(), (int)getuid());

        // 1) Wait for the engine module (may take a while — UE loads after Java UI).
        uintptr_t engineBase = 0;
        const char* engineName = nullptr;
        for (const char* mod : kTargetModules)
        {
            LOGI("waiting for %s ...", mod);
            engineBase = WaitForModule(mod, 300);
            if (engineBase)
            {
                engineName = mod;
                break;
            }
        }

        if (!engineBase)
        {
            LOGE("FATAL: no UE module mapped after 300s — aborting bootstrap");
            return;
        }

        ModuleInfo info;
        if (GetModuleInfo(engineName, &info))
            LOGI("engine module: %s base=0x%lx size=0x%zx path=%s",
                 engineName, (unsigned long)info.base, info.size, info.path);

        // 2) TODO(phase-2): wait for engine ready (GObjects populated),
        //    then run the ported Erbium::Init():
        //      - SDK::Init (GObjects/GNames via baked offsets)
        //      - GIsClient=false / GIsServer=true flips
        //      - null/ret-true patches
        //      - TickFlush & NetDriver hooks via embedded Dobby
        //      - open Athena_Terrain / Artemis_Terrain per version
        LOGI("phase-1 skeleton reached end of bootstrap — core port lands next");
    }
}

__attribute__((constructor))
static void Erbium_OnLoad()
{
    // Spawn detached — must never block the classloader.
    std::thread(Erbium::Main).detach();
}
