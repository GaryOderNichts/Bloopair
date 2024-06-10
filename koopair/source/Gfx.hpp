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
#pragma once

#include <SDL.h>
#include <string>

namespace Gfx
{

constexpr uint32_t SCREEN_WIDTH            = 1920;
constexpr uint32_t SCREEN_HEIGHT           = 1080;

constexpr SDL_Color COLOR_BLACK            = { 0x00, 0x00, 0x00, 0xff };
constexpr SDL_Color COLOR_WHITE            = { 0xff, 0xff, 0xff, 0xff };
constexpr SDL_Color COLOR_GRAY             = { 0x80, 0x80, 0x80, 0xff };
constexpr SDL_Color COLOR_BACKGROUND       = { 0x0c, 0x12, 0x15, 0xff };
constexpr SDL_Color COLOR_ALT_BACKGROUND   = { 0x24, 0x2f, 0x36, 0xff };
constexpr SDL_Color COLOR_HIGHLIGHTED      = { 0x00, 0x91, 0xea, 0xff };
constexpr SDL_Color COLOR_TEXT             = { 0xf8, 0xf8, 0xf8, 0xff };
constexpr SDL_Color COLOR_ALT_TEXT         = { 0xb0, 0xb0, 0xb0, 0xff };
constexpr SDL_Color COLOR_ACCENT           = COLOR_TEXT;//{ 0x09, 0x00, 0x88, 0xff };
constexpr SDL_Color COLOR_ALT_ACCENT       = { 0x07, 0x84, 0xb5, 0xff };
constexpr SDL_Color COLOR_BARS             = { 0x00, 0x22, 0x42, 0xff };
constexpr SDL_Color COLOR_ERROR            = { 0xff, 0x33, 0x33, 0xff };
constexpr SDL_Color COLOR_WIIU             = { 0x00, 0x95, 0xc7, 0xff };

enum AlignFlags {
    ALIGN_LEFT            =   1 << 0,
    ALIGN_RIGHT           =   1 << 1,
    ALIGN_HORIZONTAL      =   1 << 2,
    ALIGN_TOP             =   1 << 3,
    ALIGN_BOTTOM          =   1 << 4,
    ALIGN_VERTICAL        =   1 << 5,
    ALIGN_CENTER          =   ALIGN_HORIZONTAL | ALIGN_VERTICAL,
};

static constexpr inline AlignFlags operator|(AlignFlags lhs, AlignFlags rhs) {
    return static_cast<AlignFlags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

bool Init();

void Shutdown();

void Clear(SDL_Color color);

void Render();

void DrawRectFilled(int x, int y, int w, int h, SDL_Color color);

void DrawRect(int x, int y, int w, int h, int borderSize, SDL_Color color);

void DrawRectRoundedFilled(int x, int y, int w, int h, int radius, SDL_Color color);

void DrawCircleFilled(int x, int y, int radius, SDL_Color color);

void DrawCircle(int x, int y, int radius, int borderSize, SDL_Color color);

void DrawIcon(int x, int y, int size, SDL_Color color, Uint16 icon, AlignFlags align = ALIGN_CENTER, double angle = 0.0);

int GetIconWidth(int size, Uint16 icon);

static inline int GetIconHeight(int size, Uint16 icon) { return size; }

void Print(int x, int y, int size, SDL_Color color, std::string text, AlignFlags align = ALIGN_LEFT | ALIGN_TOP, bool monospace = false);

int GetTextWidth(int size, std::string text, bool monospace = false);

int GetTextHeight(int size, std::string text, bool monospace = false);

}
