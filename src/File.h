// OBECA - Open Broadcast Edge Cache Appliance
// Gateway Process
//
// Copyright (C) 2021 Klaus Kühnhammer (Österreichische Rundfunksender GmbH & Co KG)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
// 
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <string>
#include <filesystem>

#pragma once
namespace OBECA {
    class File {
      public:
        File(std::string l, size_t len, unsigned long timestamp)
          : _location(std::move(l))
            , _content_length(len)
            , _received_at(timestamp) {}
        std::filesystem::path location() const { return _location; };
        size_t content_length() const { return _content_length; };
        unsigned long received_at() const { return _received_at; };
      private:
        std::filesystem::path _location;
        size_t _content_length;
        unsigned long _received_at;

    };
}
