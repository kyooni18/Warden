#ifndef ENGINE_AUDIO_H
#define ENGINE_AUDIO_H

/*
 * engine/audio.h — Audio subsystem interface.
 *
 * Warden currently has no audio; this is a stub that defines the interface
 * so future implementations can be dropped in without touching game code.
 */

#include <stdbool.h>

typedef struct EngineAudioConfig {
    int master_volume;  /* 0–100 */
    bool muted;
} EngineAudioConfig;

/* Initialise the audio subsystem.  Returns false if unavailable (stub: always false). */
bool engine_audio_init(const EngineAudioConfig *config);

/* Shut down the audio subsystem and release resources. */
void engine_audio_deinit(void);

/* Play a named sound effect once (no-op stub). */
void engine_audio_play_sfx(const char *name);

/* Play/stop background music (no-op stub). */
void engine_audio_play_music(const char *name);
void engine_audio_stop_music(void);

#endif /* ENGINE_AUDIO_H */
