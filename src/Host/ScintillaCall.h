// This file is part of Columns++ for Notepad++.
// Copyright 2023 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef SCINTILLACALL_H

#define Failure __unused__Failure

#include "original/ScintillaCall.h"

#undef Failure

#include <charconv>

namespace Scintilla {

struct Failure : std::exception {
    Scintilla::Status status;
    explicit Failure(Scintilla::Status status) noexcept : status(status) {}
    virtual const char* what() const noexcept override {
        static char text[37] = "Scintilla error; status code ";
        static auto tcr = std::to_chars(text + 29, text + 36, static_cast<int>(status));
        if (tcr.ec == std::errc()) *tcr.ptr = 0;
        else strcpy(text + 29, "unknown");
        return text;
    }
};

}

#endif