// 5G-MAG Reference Tools
// MBMS Middleware Process
//
// Copyright (C) 2021 Klaus Kühnhammer (Österreichische Rundfunksender GmbH & Co KG)
//
// Licensed under the License terms and conditions for use, reproduction, and
// distribution of 5G-MAG software (the “License”).  You may not use this file
// except in compliance with the License.  You may obtain a copy of the License at
// https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
// agreed to in writing, software distributed under the License is distributed on
// an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.
// 
// See the License for the specific language governing permissions and limitations
// under the License.
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
