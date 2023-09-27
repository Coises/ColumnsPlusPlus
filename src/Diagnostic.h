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

#include <string>
#include <windows.h>

class Diagnostic {
	constexpr static int depth = 24;
	inline static std::wstring text[depth];
	inline static int n = 0;
public:
	static void trace(const std::wstring& next) {
		text[n] = next;
		n = n >= depth - 1 ? 0 : n + 1;
	}
	static void display(const std::wstring& title = L"Columns++: Error trace") {
		std::wstring msg = text[n];
		for (int i = n + 1; i < depth; ++i) msg += L'\n' + text[i];
		for (int i = 0; i < n; ++i) msg += L'\n' + text[i];
		MessageBox(0, msg.data(), title.data(), MB_ICONERROR);
	}
};