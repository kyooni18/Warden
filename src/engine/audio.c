#include "engine/audio.h"

/* Audio subsystem — stub implementation.
 * No audio hardware is used; all functions are no-ops.
 * Replace this file with a real implementation (e.g. SDL_mixer) when audio
 * support is added.  The header interface in include/engine/audio.h does not
 * need to change.
 */

bool engine_audio_init(const EngineAudioConfig *config)
{
    (void)config;
    return false; /* audio unavailable */
}

void engine_audio_deinit(void) {}

void engine_audio_play_sfx(const char *name)  { (void)name; }
void engine_audio_play_music(const char *name) { (void)name; }
void engine_audio_stop_music(void) {}
