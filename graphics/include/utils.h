#pragma once

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

namespace SpaceRogueLite {

inline SDL_Surface* rotateSurface90CCW(SDL_Surface* src) {
    if (!src) return nullptr;

    int w = src->w;
    int h = src->h;

    SDL_Surface* dst = SDL_CreateSurface(h, w, src->format);
    if (!dst) {
        spdlog::error("Failed to create rotated surface: {}", SDL_GetError());
        return nullptr;
    }

    if (!SDL_LockSurface(src)) {
        SDL_DestroySurface(dst);
        return nullptr;
    }
    if (!SDL_LockSurface(dst)) {
        SDL_UnlockSurface(src);
        SDL_DestroySurface(dst);
        return nullptr;
    }

    uint8_t* srcPixels = static_cast<uint8_t*>(src->pixels);
    uint8_t* dstPixels = static_cast<uint8_t*>(dst->pixels);
    int bytesPerPixel = SDL_BYTESPERPIXEL(src->format);

    // Rotate 90° CCW: src(x, y) -> dst(y, w-1-x)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int srcIdx = y * src->pitch + x * bytesPerPixel;
            int dstX = y;
            int dstY = w - 1 - x;
            int dstIdx = dstY * dst->pitch + dstX * bytesPerPixel;

            for (int b = 0; b < bytesPerPixel; b++) {
                dstPixels[dstIdx + b] = srcPixels[srcIdx + b];
            }
        }
    }

    SDL_UnlockSurface(src);
    SDL_UnlockSurface(dst);

    return dst;
}

}  // namespace SpaceRogueLite
