#pragma once

#if defined(DISCORD_DYNAMIC_LIB)
#if defined(_WIN32)
#if defined(DISCORD_BUILDING_SDK)
#define DISCORD_EXPORT __declspec(dllexport)
#else
#define DISCORD_EXPORT __declspec(dllimport)
#endif
#else
#define DISCORD_EXPORT __attribute__((visibility("default")))
#endif
#else
#define DISCORD_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

DISCORD_EXPORT void Discord_Register(const char* applicationId, const char* command);
DISCORD_EXPORT void Discord_RegisterSteamGame(const char* applicationId, const char* steamId);

#ifdef __cplusplus
}
#endif
