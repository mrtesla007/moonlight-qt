#include "sdl.h"

#include <Limelight.h>
#include <SDL.h>

#include <QtGlobal>

SdlAudioRenderer::SdlAudioRenderer()
    : m_AudioDevice(0)
{
    SDL_assert(!SDL_WasInit(SDL_INIT_AUDIO));
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s",
                     SDL_GetError());
        SDL_assert(SDL_WasInit(SDL_INIT_AUDIO));
    }
}

bool SdlAudioRenderer::prepareForPlayback(const OPUS_MULTISTREAM_CONFIGURATION* opusConfig)
{
    SDL_AudioSpec want, have;

    SDL_zero(want);
    want.freq = opusConfig->sampleRate;
    want.format = AUDIO_S16;
    want.channels = opusConfig->channelCount;

    // This is supposed to be a power of 2, but our
    // frames contain a non-power of 2 number of samples,
    // so the slop would require buffering another full frame.
    // Specifying non-Po2 seems to work for our supported platforms.
    want.samples = SAMPLES_PER_FRAME;

    m_AudioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (m_AudioDevice == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to open audio device: %s",
                     SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Desired audio buffer: %u samples (%lu bytes)",
                want.samples,
                want.samples * sizeof(short) * want.channels);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Obtained audio buffer: %u samples (%u bytes)",
                have.samples,
                have.size);

    // Start playback
    SDL_PauseAudioDevice(m_AudioDevice, 0);

    return true;
}

SdlAudioRenderer::~SdlAudioRenderer()
{
    if (m_AudioDevice != 0) {
        // Stop playback
        SDL_PauseAudioDevice(m_AudioDevice, 1);
        SDL_CloseAudioDevice(m_AudioDevice);
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    SDL_assert(!SDL_WasInit(SDL_INIT_AUDIO));
}

bool SdlAudioRenderer::submitAudio(short* audioBuffer, int audioSize)
{
    if (SDL_QueueAudio(m_AudioDevice, audioBuffer, audioSize) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to queue audio sample: %s",
                     SDL_GetError());
    }

    return true;
}
