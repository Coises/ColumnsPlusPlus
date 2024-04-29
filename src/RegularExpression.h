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

#pragma once

#include <string>

class ColumnsPlusPlusData;

class RegularExpressionInterface {
public:
    virtual ~RegularExpressionInterface() {}
    virtual bool         can_search(                                                 ) const = 0;
    virtual std::wstring find      (const std::wstring& s, bool caseSensitive = false)       = 0;
    virtual std::string  format    (const std::string& replacement                   ) const = 0;
    virtual void         invalidate(                                                 )       = 0;
    virtual intptr_t     length    (int n = 0                                        ) const = 0;
    virtual size_t       mark_count(                                                 ) const = 0;
    virtual intptr_t     position  (int n = 0                                        ) const = 0;
    virtual bool         search    (std::string_view s, size_t from = 0              )       = 0;
    virtual bool         search    (intptr_t from, intptr_t to, intptr_t start       )       = 0;
    virtual size_t       size      (                                                 ) const = 0;
    virtual std::string  str       (int n = 0                                        ) const = 0;
    virtual std::string  str       (std::string_view n                               ) const = 0;
};

class RegularExpression {
    RegularExpressionInterface* rex = 0;
public:
    RegularExpression(ColumnsPlusPlusData& data);
    ~RegularExpression() { if (rex) delete rex; }
    virtual bool         can_search(                                                 ) const {return rex->can_search(                );}
    virtual std::wstring find      (const std::wstring& s, bool caseSensitive = false)       {return rex->find      (s, caseSensitive);}
    virtual std::string  format    (const std::string& replacement                   ) const {return rex->format    (replacement     );}
    virtual void         invalidate(                                                 )       {       rex->invalidate(                );}
    virtual intptr_t     length    (int n = 0                                        ) const {return rex->length    (n               );}
    virtual size_t       mark_count(                                                 ) const {return rex->mark_count(                );}
    virtual intptr_t     position  (int n = 0                                        ) const {return rex->position  (n               );}
    virtual bool         search    (std::string_view s, size_t from = 0              )       {return rex->search    (s, from         );}
    virtual bool         search    (intptr_t from, intptr_t to, intptr_t start       )       {return rex->search    (from, to, start );}
    virtual size_t       size      (                                                 ) const {return rex->size      (                );}
    virtual std::string  str       (int n = 0                                        ) const {return rex->str       (n               );}
    virtual std::string  str       (std::string_view n                               ) const {return rex->str       (n               );}
};
