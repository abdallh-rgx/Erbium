#pragma once

struct FConfiguration
{
    static inline auto Playlist = L"/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo";
    static inline auto MaxTickRate = 30;
    static inline auto bLateGame = false;
    static inline auto LateGameZone = 3;          // starting zone
    static inline auto bLateGameLongZone = false; // zone doesnt close for a long time
    static inline auto bEnableCheats = true;
    static inline auto SiphonAmount = 50; // set to 0 to disable
    static inline auto bInfiniteMats = false;
    static inline auto bInfiniteAmmo = false;
    static inline auto bForceRespawns = false; // build your client with this too!
    static inline auto bJoinInProgress = false;
    static inline auto bAutoRestart = false;
    static inline auto bKeepInventory = false;
    static inline auto Port = 7777;
    static inline auto bEnableIris = true;
    // Listen-server mode (Android default): the local player is NEVER removed,
    // GIsClient stays true and the world reports NM_ListenServer once a
    // battle-royale terrain map is ticking — the host plays on the same screen
    // the server listens on. Hosting is armed by the world watcher the moment
    // the user travels (`open Artemis_Terrain`), NOT at boot.
    static inline auto bListenServer = false;
#ifdef __ANDROID__
    // Android: no ImGui/DX11 console, no SEH crash reporter (logcat tombstones),
    // no libcurl in the v1 port (webhook disabled).
    static inline constexpr auto bGUI = false;
    static inline constexpr auto bCustomCrashReporter = false;
    static inline constexpr auto bUseStdoutLog = false;
    static inline constexpr auto WebhookURL = "";
#else
    static inline constexpr auto bGUI = true;
    static inline constexpr auto bCustomCrashReporter = true;
    static inline constexpr auto bUseStdoutLog = false;
    static inline constexpr auto WebhookURL = ""; // fill in if you want status to send to a webhook
#endif
};
