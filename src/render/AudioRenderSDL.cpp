#include "AudioRenderSDL.h"
#include <malloc.h>
#include "SDL/SDL.h"
#include "os_log.h"

static void read_data_cb(void * udata, uint8_t * data, int len)
{
    auto * audio_info = reinterpret_cast<audio_info_t *>(udata);
    if (nullptr == audio_info || 0 == audio_info->len)
    {
        return;
    }

    SDL_memset(data, 0, len);
    int tmp_len = len > audio_info->len ? audio_info->len : len;

    SDL_MixAudioFormat(data, audio_info->data + audio_info->pos, AUDIO_S16, tmp_len, audio_info->volume);
    audio_info->pos += tmp_len;
    audio_info->len -= tmp_len;
}

bool CAudioRendererSDL::create(int sample_rate, int channels, int frame_size, int volume)
{
    _cache = static_cast<audio_info_t *>(calloc(1, sizeof(audio_info_t)));
    if (nullptr == _cache)
    {
        log_msg_error("No enough memory");
        return false;
    }
    _cache->volume = volume;

    SDL_AudioSpec audio_player = {};
    audio_player.freq = sample_rate;
    audio_player.format = AUDIO_S16SYS;
    audio_player.channels = channels;
    audio_player.silence = 0;
    audio_player.samples = frame_size;
    audio_player.callback = read_data_cb;
    audio_player.userdata = _cache;
    if (SDL_OpenAudio(&audio_player, nullptr) < 0)
    {
        log_msg_warn("SDL_OpenAudio failed!");
        free(_cache);
        _cache = nullptr;
        return false;
    }
    SDL_PauseAudio(0);

    return true;
}

void CAudioRendererSDL::destroy()
{
    SDL_CloseAudio();

    if (nullptr != _cache)
    {
        free(_cache);
        _cache = nullptr;
    }
}

void CAudioRendererSDL::setVolume(const int volume)
{
    if (nullptr != _cache)
        _cache->volume = volume;
}

void CAudioRendererSDL::mute(const bool flag)
{
    SDL_PauseAudio(flag);
}

void CAudioRendererSDL::fillData(uint8_t * data, int len)
{
    if (nullptr != _cache)
    {
        _cache->data = data;
        _cache->len = len;
        _cache->pos = 0;
    }
}

bool CAudioRendererSDL::finished()
{
    if (nullptr != _cache)
        return _cache->len <= 0;
    return true;
}
