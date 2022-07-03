#pragma once

#include <cstdio>
#include <cstdbool>
#include <mutex>
#include "SDL/SDL.h"

class CVideoRendererSDL
{
public:
    CVideoRendererSDL() = default;
    ~CVideoRendererSDL() = default;

    // 创建
    bool create(const void * wnd, int width, int height);
    // 销毁
    void destroy();
    // 设置数据
    bool setData(uint8_t * data[8], int linesize[8]);
    // 设置当前位置
    void setCurPos(int x, int y);
    // 设置最后一次位置
    void setLastPos(int x, int y);

protected:
    // 是否已打开
    bool opened();

private:
    // 宽度
    int _width = 0;
    // 高度
    int _height = 0;

    // 位置
    SDL_Rect _rect = { 0 };
    // 播放器句柄
    SDL_Window * _wnd = nullptr;
    // 渲染器
    SDL_Renderer * _renderer = nullptr;
    // 色泽纹理
    SDL_Texture * _texture = nullptr;

    std::mutex _mtx;
};
