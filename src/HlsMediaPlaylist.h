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
  class HlsMediaPlaylist {
    public:
      HlsMediaPlaylist(const std::string& content);
      HlsMediaPlaylist() = default;
      ~HlsMediaPlaylist() = default;

      struct Segment {
        std::string uri;
        int seq;
        double extinf;
      };
      const std::vector<Segment>& segments() const { return _segments; };
      void add_segment(Segment segment) { _segments.push_back(std::move(segment)); };

      std::string to_string() const;

      void set_target_duration(int duration) { _targetduration = duration; };
      int target_duration() const { return _targetduration; };

    private:
      int _version = -1;
      int _targetduration;
      std::vector<Segment> _segments = {};
  };
}
