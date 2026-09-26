// entry.cpp — liberbium.so entry point.
//
// Loaded via System.loadLibrary("erbium") from the patched onCreate() smali.
// System.loadLibrary calls JNI_OnLoad, which captures the JavaVM — that's our
// ticket back into Java-land for the console-command bridge. A worker thread
// then:
//   1. waits for libUnreal.so (or libUE4.so) to be mapped,
//   2. waits for the engine singleton (GEngine) to exist,
//   3. flips GIsClient=false / GIsServer=true  (the Erbium core trick),
//   4. invokes the game's own exported nativeConsoleCommand() with
//      "open Artemis_Terrain" — no pattern-scanning, no UObject SDK needed
//      for the v1 map-hosting bring-up.
//
// All game addresses come from android/Generated/offsets-<version>.h, baked
// offline by scripts/analyze_libue4.py against the exact APK shipped in CI.
#include "offsets.h"
#include "alog.h"
#include "owen.h"
#include "platform.h"

#include <chrono>
#include <cstring>
#include <dlfcn.h>
#include <jni.h>
#include <thread>
#include <unistd.h>
namespace Erbium
{
    static constexpr const char* kTargetModules[] = {
        "libUE4.so",     // Chapter 1-2 era Android builds
        "libUnreal.so",  // Chapter 3+ era Android builds
    };

    static JavaVM* g_vm = nullptr;

    // Backend (Voltronite) URL for the Owen.c redirect. On an emulator the
    // host loopback is 10.0.2.2 — the CI runs the backend on the runner.
    static const char* BackendUrl()
    {
#ifdef ERBIUM_EMULATOR
        return "http://10.0.2.2:3551";
#else
        return "http://127.0.0.1:3551";
#endif
    }

    // Console command to run once the engine is up (env-tunable for CI).
    // 21.30 is Chapter 3 → Artemis_Terrain. Erbium windows picks the map by
    // version; keep that logic here once more versions get baked offsets.
    static const char* ConsoleCommandForVersion()
    {
        if (strstr(Baked::kRelease, "Release-19.") || strstr(Baked::kRelease, "Release-20."))
            return "open Artemis_Terrain";
        if (strstr(Baked::kRelease, "Release-21.") || strstr(Baked::kRelease, "Release-22."))
            return "open Artemis_Terrain";
        if (strstr(Baked::kRelease, "Release-23.") || strstr(Baked::kRelease, "Release-27."))
            return "open Asteria_Terrain";
        if (strstr(Baked::kRelease, "Release-28."))
            return "open Helios_Terrain";
        if (strstr(Baked::kRelease, "Release-1") && !strstr(Baked::kRelease, "Release-19")
            && !strstr(Baked::kRelease, "Release-18"))
            return "open Apollo_Terrain";  // Chapter 2 (11.x-14.x are Apollo-era)
        return "open Artemis_Terrain";     // default for the 21.30 bake
    }

    // Wait for *(void**)(base + kGEngine) to become non-null — the engine
    // singleton is constructed during FEngineLoop::Init, before any world.
    static void* WaitForEngineSingleton(uintptr_t base, int timeoutSeconds)
    {
        volatile void** slot = (volatile void**)(base + Baked::kGEngine);
        for (int i = 0; i < timeoutSeconds * 10; ++i)
        {
            void* engine = (void*)*slot;
            if (engine)
                return engine;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return nullptr;
    }

    // Extra settling delay after GEngine appears before we flip flags / open
    // the map. The frontend world needs a few seconds to come up. Bumped or
    // rebuilt per CI run when needed — frontend login flow lands in phase-3.
    static int SettleDelaySeconds()
    {
        return 25;
    }

    // Invoke the game's exported JNI console-command entry directly.
    // Signature (from 21.30 disasm): void fn(JNIEnv*, jobject /*unused*/, jstring)
    static bool RunConsoleCommand(JNIEnv* env, void* libHandle, const char* command)
    {
        using NativeConsoleCommandFn = void (*)(JNIEnv*, jobject, jstring);
        auto fn = (NativeConsoleCommandFn)dlsym(libHandle, Baked::kConsoleCommandSymbol);
        if (!fn)
        {
            LOGE("dlsym(%s) failed: %s", Baked::kConsoleCommandSymbol, dlerror());
            return false;
        }

        jstring str = env->NewStringUTF(command);
        if (!str)
        {
            LOGE("NewStringUTF failed");
            return false;
        }
        LOGI("dispatching console command: %s", command);
        fn(env, nullptr, str);
        env->DeleteLocalRef((jobject)str);
        return true;
    }

    static void Main()
    {
        LOGI("=== Erbium Android bootstrap (build " __DATE__ " " __TIME__ ") ===");
        LOGI("pid=%d uid=%d baked=%s", (int)getpid(), (int)getuid(), Baked::kRelease);

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

#ifdef ERBIUM_HAS_OWEN
        // Install the backend redirect BEFORE any Epic HTTP request fires —
        // login happens while the frontend loads, long before GEngine exists.
        {
            const char* versionStr = Baked::kRelease;
            int rc = owen_install(engineBase, BackendUrl(),
                                  (uint32_t)Baked::kOwenProcessRequest,
                                  (uint16_t)Baked::kOwenGetUrlField,
                                  (uint32_t)Baked::kOwenSetUrl,
                                  0, 0, 0, // EOS hooks: 21.30 doesn't need them
                                  versionStr);
            LOGI("owen_install rc=%d (backend=%s)", rc, BackendUrl());
        }
#endif

        // Validate the bake matches the loaded lib before touching anything.
        {
            const char* releasePtr = (const char*)(engineBase + Baked::kReleaseString);
            char probe[64] = {0};
            // The release string lives in .rodata; read defensively via memcpy
            // (could straddle a page boundary in theory).
            memcpy(probe, releasePtr, sizeof(probe) - 1);
            if (strncmp(probe, "++Fortnite+Release-", 19) != 0)
            {
                LOGE("FATAL: release-string probe mismatch (got %.20s) — baked offsets "
                     "do not belong to this build; refusing to patch", probe);
                return;
            }
            LOGI("release probe OK: %.48s", probe);
        }

        // 2) Wait for the engine singleton.
        void* engine = nullptr;
        LOGI("waiting for GEngine (slot 0x%llx)...", (unsigned long long)Baked::kGEngine);
        engine = WaitForEngineSingleton(engineBase, 2400);
        if (!engine)
        {
            LOGE("GEngine never appeared after 40min — aborting (translation too slow or init stuck)");
            return;
        }
        LOGI("GEngine = %p", engine);

        // Grab a JNI env for this thread (we were spawned from JNI_OnLoad).
        if (!g_vm)
        {
            LOGE("no JavaVM captured — was JNI_OnLoad called?");
            return;
        }
        JNIEnv* env = nullptr;
        JavaVMAttachArgs attachArgs{JNI_VERSION_1_6, (char*)"ErbiumBootstrap", nullptr};
        if (g_vm->AttachCurrentThread(&env, &attachArgs) != JNI_OK || !env)
        {
            LOGE("AttachCurrentThread failed");
            return;
        }

        // Existing handle to the already-loaded engine lib (no reload).
        void* libHandle = dlopen(info.path[0] ? info.path : engineName, RTLD_NOW | RTLD_NOLOAD);
        if (!libHandle)
        {
            LOGE("dlopen(NORELOAD) %s failed: %s", info.path, dlerror());
            return;
        }

        // 3) Let the frontend settle, then flip the client/server flags.
        //    .bss is writable — no mprotect dance needed for data flips.
        const int delay = SettleDelaySeconds();
        LOGI("engine up — settling %ds before flag flip...", delay);
        std::this_thread::sleep_for(std::chrono::seconds(delay));

        volatile bool* isClient = (volatile bool*)(engineBase + Baked::kGIsClient);
        volatile bool* isServer = (volatile bool*)(engineBase + Baked::kGIsServer);
        LOGI("pre-flip: GIsClient=%d GIsServer=%d", (int)*isClient, (int)*isServer);
        *isClient = false;
        *isServer = true;
        LOGI("POST-FLIP: GIsClient=%d GIsServer=%d  → this process is now a game server",
             (int)(bool)*isClient, (int)(bool)*isServer);

        // 4) Open the battle-royale terrain map as a listen server.
        const char* cmd = ConsoleCommandForVersion();
        if (!RunConsoleCommand(env, libHandle, cmd))
        {
            LOGE("console command dispatch failed — engine may still accept it later");
        }

        // v1 milestone: map-hosting bring-up. Phase-3 adds:
        //  - LocalPlayers removal (GameInstance->LocalPlayers.Remove(0))
        //  - NetDriver/InitListen hooking for LAN discovery
        //  - Owen.c-style backend redirect for the login flow
        LOGI("Erbium v1 bootstrap complete — watching for map load...");
    }
} // namespace Erbium

// System.loadLibrary("erbium") calls this — captures the JavaVM.
extern "C" jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    Erbium::g_vm = vm;
    LOGI("JNI_OnLoad: JavaVM captured (%p) — spawning bootstrap", (void*)vm);
    std::thread(Erbium::Main).detach();
    return JNI_VERSION_1_6;
}
