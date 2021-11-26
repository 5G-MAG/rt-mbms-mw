// 5G-MAG Reference Tools
// MBMS Middleware Process
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

#pragma once

#include <string>
#include <vector>

namespace MBMS_RT {
  class HlsPrimaryPlaylist {
    public:
      HlsPrimaryPlaylist(const std::string& content, const std::string& base_path);
      HlsPrimaryPlaylist() = default;
      ~HlsPrimaryPlaylist() = default;

      struct Stream {
        std::string uri;
        std::string resolution;
        std::string codecs;
        unsigned long bandwidth;
        double frame_rate;
      };
      const std::vector<Stream>& streams() const { return _streams; };
      void add_stream(Stream stream) { _streams.push_back(std::move(stream)); };

      std::string to_string() const;
    private:
      std::vector<std::pair<std::string, std::string>> parse_parameters(const std::string& line) const;
      int _version = -1;
      std::vector<Stream> _streams = {};
  };
}
