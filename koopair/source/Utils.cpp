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
#include "Utils.hpp"
#include <cstring>

namespace Utils
{

std::string ToHexString(const void* data, size_t size, bool upperCase)
{
    std::string str;
    for (size_t i = 0; i < size; ++i)
        str += Utils::sprintf(upperCase ? "%02X" : "%02x", ((const uint8_t*) data)[i]);
    
    return str;
}

}
