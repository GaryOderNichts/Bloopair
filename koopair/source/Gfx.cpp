/*
 *   Copyright (C) 2024 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "Gfx.hpp"
#include "SDL_FontCache.h"
#include <SDL2_gfxPrimitives.h>
#include <SDL_image.h>
#include <map>
#include <cstdarg>

#include <coreinit/debug.h>
#include <coreinit/memory.h>

#include <ter-u32b_bdf.h>
#include <fa-solid-900_ttf.h>
#include <shell_png.h>

namespace
{

SDL_Window* window = nullptr;

SDL_Renderer* renderer = nullptr;

void* fontData = nullptr;

uint32_t fontSize = 0;

std::map<int, FC_Font*> fontMap;

FC_Font* monospaceFont = nullptr;

TTF_Font* iconFont = nullptr;

std::map<Uint16, SDL_Texture*> iconCache;

SDL_Texture* appIcon = nullptr;

FC_Font* GetFontForSize(int size)
{
    if (fontMap.contains(size)) {
        return fontMap[size];
    }

    FC_Font* font = FC_CreateFont();
    if (!font) {
        return font;
    }

    if (!FC_LoadFont_RW(font, renderer, SDL_RWFromMem(fontData, fontSize), 1, size, Gfx::COLOR_BLACK, TTF_STYLE_NORMAL)) {
        FC_FreeFont(font);
        return nullptr;
    }

    fontMap.insert({size, font});
    return font;
}

SDL_Texture* LoadIcon(Uint16 icon)
{
    if (icon == Gfx::APP_ICON) {
        return appIcon;
    }

    if (iconCache.contains(icon)) {
        return iconCache[icon];
    }

    SDL_Surface* iconSurface = TTF_RenderGlyph_Blended(iconFont, icon, Gfx::COLOR_WHITE);
    if (!iconSurface) {
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, iconSurface);
    SDL_FreeSurface(iconSurface);
    if (!texture) {
        return nullptr;
    }

    iconCache.insert({icon, texture});
    return texture;
}

}

namespace Gfx
{

bool Init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }

    window = SDL_CreateWindow("Koopair", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        OSReport("SDL_CreateWindow failed\n");
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        OSReport("SDL_CreateRenderer failed\n");
        SDL_DestroyWindow(window);
        window = nullptr;
        return false;
    }

    if (!OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, &fontData, &fontSize)) {
        OSReport("OSGetSharedData failed\n");
        return false;
    }

    TTF_Init();

    monospaceFont = FC_CreateFont();
    if (!monospaceFont) {
        return false;
    }

    if (!FC_LoadFont_RW(monospaceFont, renderer, SDL_RWFromConstMem(ter_u32b_bdf, ter_u32b_bdf_size), 1, 32, Gfx::COLOR_BLACK, TTF_STYLE_NORMAL)) {
        FC_FreeFont(monospaceFont);
        return false;
    }

    // icons @256 should be large enough for our needs
    iconFont = TTF_OpenFontRW(SDL_RWFromConstMem(fa_solid_900_ttf, fa_solid_900_ttf_size), 1, 256);
    if (!iconFont) {
        return false;
    }

    appIcon = IMG_LoadTextureTyped_RW(renderer, SDL_RWFromConstMem(shell_png, shell_png_size), 1, "PNG");
    if (!appIcon) {
        return false;
    }

    return true;
}

void Shutdown()
{
    for (const auto& [key, value] : fontMap) {
        FC_FreeFont(value);
    }

    for (const auto& [key, value] : iconCache) {
        SDL_DestroyTexture(value);
    }

    SDL_DestroyTexture(appIcon);
    FC_FreeFont(monospaceFont);
    TTF_CloseFont(iconFont);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Clear(SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer);
}

void Render()
{
    SDL_RenderPresent(renderer);
}

void DrawRectFilled(int x, int y, int w, int h, SDL_Color color)
{
    boxRGBA(renderer, x, y, x + w, y + h, color.r, color.g, color.b, color.a);
}

void DrawRect(int x, int y, int w, int h, int borderSize, SDL_Color color)
{
    DrawRectFilled(x, y, w, borderSize, color);
    DrawRectFilled(x, y + h - borderSize, w, borderSize, color);
    DrawRectFilled(x, y, borderSize, h, color);
    DrawRectFilled(x + w - borderSize, y, borderSize, h, color);
}

void DrawRectRoundedFilled(int x, int y, int w, int h, int radius, SDL_Color color)
{
    roundedBoxRGBA(renderer, x, y, x + w, y + h, radius, color.r, color.g, color.b, color.a);
}

void DrawCircleFilled(int x, int y, int radius, SDL_Color color)
{
    filledCircleRGBA(renderer, x, y, radius, color.r, color.g, color.b, color.a);
}

void DrawCircle(int x, int y, int radius, int borderSize, SDL_Color color)
{
    if (borderSize <= 0) {
        return;
    }

    // TODO eh this is not nice
    while (borderSize--) {
        circleRGBA(renderer, x, y, radius - borderSize, color.r, color.g, color.b, color.a);
    }
}

void DrawIcon(int x, int y, int size, SDL_Color color, Uint16 icon, AlignFlags align, double angle)
{
    SDL_Texture* iconTex = LoadIcon(icon);
    if (!iconTex) {
        return;
    }

    SDL_SetTextureColorMod(iconTex, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(iconTex, color.a);

    int w, h;
    SDL_QueryTexture(iconTex, nullptr, nullptr, &w, &h);

    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    // scale the width based on hight to keep AR
    rect.w = (int) (((float) w / h) * size);
    rect.h = size;

    if (align & ALIGN_RIGHT) {
        rect.x -= rect.w;
    } else if (align & ALIGN_HORIZONTAL) {
        rect.x -= rect.w / 2;
    }

    if (align & ALIGN_BOTTOM) {
        rect.y -= rect.h;
    } else if (align & ALIGN_VERTICAL) {
        rect.y -= rect.h / 2;
    }

    // draw the icon
    if (angle) {
        SDL_RenderCopyEx(renderer, iconTex, nullptr, &rect, angle, nullptr, SDL_FLIP_NONE);
    } else {
        SDL_RenderCopy(renderer, iconTex, nullptr, &rect);
    }
}

int GetIconWidth(int size, Uint16 icon)
{
    SDL_Texture* iconTex = LoadIcon(icon);
    if (!iconTex) {
        return 0;
    }

    int w, h;
    SDL_QueryTexture(iconTex, nullptr, nullptr, &w, &h);

    return (int) (((float) w / h) * size);
}

void Print(int x, int y, int size, SDL_Color color, std::string text, AlignFlags align, bool monospace)
{
    FC_Font* font = monospace ? monospaceFont : GetFontForSize(size);
    if (!font) {
        return;
    }

    FC_Effect effect;
    effect.color = color;

    // scale monospace font based on size
    if (monospace) {
        effect.scale = FC_MakeScale(size / 28.0f, size / 28.0f);
        // TODO figure out how to center this properly
        y += 5;
    } else {
        effect.scale = FC_MakeScale(1,1);
    }

    if (align & ALIGN_LEFT) {
        effect.alignment = FC_ALIGN_LEFT;
    } else if (align & ALIGN_RIGHT) {
        effect.alignment = FC_ALIGN_RIGHT;
    } else if (align & ALIGN_HORIZONTAL) {
        effect.alignment = FC_ALIGN_CENTER;
    } else {
        // left by default
        effect.alignment = FC_ALIGN_LEFT;
    }

    if (align & ALIGN_BOTTOM) {
        y -= GetTextHeight(size, text, monospace);
    } else if (align & ALIGN_VERTICAL) {
        y -= GetTextHeight(size, text, monospace) / 2;
    }

    FC_DrawEffect(font, renderer, x, y, effect, "%s", text.c_str());
}

int GetTextWidth(int size, std::string text, bool monospace)
{
    FC_Font* font = monospace ? monospaceFont : GetFontForSize(size);
    if (!font) {
        return 0;
    }

    float scale = monospace ? (size / 28.0f) : 1.0f;

    return FC_GetWidth(font, "%s", text.c_str()) * scale;
}

int GetTextHeight(int size, std::string text, bool monospace)
{
    // TODO this doesn't work nicely with monospace yet
    monospace = false;

    FC_Font* font = monospace ? monospaceFont : GetFontForSize(size);
    if (!font) {
        return 0;
    }

    float scale = monospace ? (size / 28.0f) : 1.0f;

    return FC_GetHeight(GetFontForSize(size), "%s", text.c_str()) * scale;
}

}

