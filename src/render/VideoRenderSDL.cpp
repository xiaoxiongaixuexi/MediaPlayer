#include "VideoRenderSDL.h"
#include "os_log.h"

bool CVideoRendererSDL::create(const void * wnd, int width, int height)
{
    if (nullptr == wnd)
    {
        log_msg_warn("Input window handle is invalid!");
        return false;
    }

    if (width <= 0 || height <= 0)
    {
        log_msg_warn("Player width: %d or(and) height: %d is invalid!", width, height);
        return false;
    }

    // 创建窗口
    _wnd = SDL_CreateWindowFrom(wnd);
    if (nullptr == _wnd)
    {
        log_msg_warn("SDL_CreateWindowFrom failed.");
        return false;
    }
    // 创建渲染器
    _renderer = SDL_CreateRenderer(_wnd, -1, SDL_RENDERER_TARGETTEXTURE);
    if (nullptr == _renderer)
    {
        log_msg_warn("SDL_CreateRenderer failed.");
        destroy();
        return false;
    }
    // 创建纹理器
    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (nullptr == _texture)
    {
        log_msg_warn("SDL_CreateTexture failed!");
        destroy();
        return false;
    }

    _width = width;
    _height = height;

    return true;
}

void CVideoRendererSDL::destroy()
{
    if (nullptr != _texture)
    {
        SDL_DestroyTexture(_texture);
        _texture = nullptr;
    }
    if (nullptr != _renderer)
    {
        SDL_RenderClear(_renderer);
        SDL_DestroyRenderer(_renderer);
        _renderer = nullptr;
    }
    if (nullptr != _wnd)
    {
        SDL_DestroyWindow(_wnd);
        _wnd = nullptr;
    }

    _width = 0;
    _height = 0;
}

bool CVideoRendererSDL::setData(uint8_t * data[8], int linesize[8])
{
    if (nullptr == data[0] || linesize[0] <= 0)
    {
        log_msg_warn("Input param is invalid");
        return false;
    }

    int ret = SDL_UpdateYUVTexture(_texture, &_rect, data[0], linesize[0], data[1], linesize[1], data[2], linesize[2]);
    if (0 != ret)
    {
        log_msg_warn("SDL_UpdateYUVTexture failed");
    }

    _rect = { 0, 0, _width, _height };

    ret = SDL_RenderClear(_renderer);
    if (0 != ret)
    {
        log_msg_warn("SDL_RenderClear failed");
    }

    ret = SDL_RenderCopy(_renderer, _texture, nullptr, &_rect);
    if (0 != ret)
    {
        log_msg_warn("SDL_RenderCopy failed");
    }
    SDL_RenderPresent(_renderer);

    return true;
}

void CVideoRendererSDL::setCurPos(int x, int y)
{
}

void CVideoRendererSDL::setLastPos(int x, int y)
{
}

bool CVideoRendererSDL::opened()
{
    std::unique_lock<std::mutex> lck(_mtx);
    return (nullptr == _wnd);
}
