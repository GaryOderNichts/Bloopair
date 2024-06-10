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

#include <utility>
#include <vector>
#include <string>
#include <functional>

#include "Controller.hpp"

class MessageBox {
public:
    struct Option {
        uint16_t icon;
        std::string text;
        std::function<void(void)> callback;
    };

public:
    MessageBox(const std::string& title, const std::string& message, const std::vector<Option> options);
    virtual ~MessageBox();

    virtual void Draw();

    virtual bool Update(const CombinedInputController& input);

protected:
    std::string mTitle;
    std::string mMessage;
    std::vector<Option> mOptions;

    size_t mSelected;
};
