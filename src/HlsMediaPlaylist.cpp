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

#include "HlsMediaPlaylist.h"

#include "spdlog/spdlog.h"
#include "boost/algorithm/string.hpp"
#include <sstream>

MBMS_RT::HlsMediaPlaylist::HlsMediaPlaylist(const std::string& content)
{
  spdlog::debug("Parsing HLS media playlist: {}", content);

  std::istringstream iss(content);
  int idx = 0;
  int seq_nr = 0;
  double extinf = -1;
  for (std::string line; std::getline(iss, line); idx++ )
  {
    boost::algorithm::trim(line);

    if (idx==0) {
      if ( line != "#EXTM3U") {
        throw("HLS playlist parsing failed: first line is not #EXTM3U");
      } else {
        continue;
      }
    }

    size_t cpos = line.rfind(':');

    if (line.rfind("#EXT-X-VERSION", 0) == 0) {
      if (_version != -1) {
        throw("HLS playlist parsing failed: duplicate #EXT-X-VERSION");
      }
      if (cpos != std::string::npos) {
        _version = atoi(line.substr(cpos+1).c_str());
      }
    } else if (line.rfind("#EXT-X-TARGETDURATION", 0) == 0) {
      if (cpos != std::string::npos) {
        _targetduration = atoi(line.substr(cpos+1).c_str());
      }
    } else if (line.rfind("#EXT-X-MEDIA-SEQUENCE", 0) == 0) {
      if (cpos != std::string::npos) {
        seq_nr = atoi(line.substr(cpos+1).c_str());
      }
    } else if (line.rfind("#EXTINF", 0) == 0) {
      if (cpos != std::string::npos) {
        extinf = atof(line.substr(cpos+1).c_str());
      }
    } else if (line.rfind('#', 0) == 0) {
        spdlog::debug("HLS playlist parser ignoring unhandled line {}", line);
    } else if (line.size() > 0) {
        _segments.push_back({line, seq_nr++, extinf});
    }
  }
}


auto MBMS_RT::HlsMediaPlaylist::to_string() const -> std::string
{
  std::stringstream pl;

  pl << "#EXTM3U" << std::endl;
  pl << "#EXT-X-VERSION:3" << std::endl;

  if (_segments.size() > 0) {
    pl << "#EXT-X-TARGETDURATION:" << _targetduration << std::endl;
    pl << "#EXT-X-MEDIA-SEQUENCE:" << _segments[0].seq << std::endl;
  }
  for (const auto& seg : _segments) {
    pl << "#EXTINF:" << seg.extinf << std::endl;
    pl << "/" << seg.uri << std::endl;
  }
  return std::move(pl.str());
}
