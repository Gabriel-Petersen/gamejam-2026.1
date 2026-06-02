#ifndef SOUND_H
#define SOUND_H

#include "audio.h"

#include "tururu.h"
#include "tema1.h"

static void sound_init(void)
{
    static int initialized = 0;

    if (initialized)
        return;

    initialized = 1;

    audio_init();

    audio_load_memory(
        "tururu",
        tururu_wav,
        tururu_wav_len
    );

    audio_load_memory(
        "tema1",
        tema1_wav,
        tema1_wav_len
    );
}

static void sound_shutdown(void)
{
    audio_shutdown();
}

#endif