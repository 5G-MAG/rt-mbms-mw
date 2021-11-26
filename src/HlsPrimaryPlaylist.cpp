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

#include "HlsPrimaryPlaylist.h"

#include "spdlog/spdlog.h"
#include "boost/algorithm/string.hpp"
#include <iomanip>
#include <sstream>

MBMS_RT::HlsPrimaryPlaylist::HlsPrimaryPlaylist(const std::string& content, const std::string& base_path)
{
  spdlog::debug("Parsing HLS primary playlist: {}", content);

  std::istringstream iss(content);
  int idx = 0;
  std::string resolution = "";
  std::string codecs = "";
  unsigned long bandwidth = 0;
  double frame_rate = 0;
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
    } else if (line.rfind("#EXT-X-STREAM-INF", 0) == 0) {
      if (cpos != std::string::npos) {
        std::string info = line.substr(cpos+1);
        auto params = parse_parameters(info);
        for (const auto& param : params) {
          if (param.first == "BANDWIDTH") {
            bandwidth = std::atoi(param.second.c_str()); 
          }
          else if (param.first == "FRAME-RATE") {
            frame_rate = std::atof(param.second.c_str());
          }
          else if (param.first == "RESOLUTION") {
            resolution = param.second;
          }
          else if (param.first == "CODECS") {
            codecs = param.second;
          }
        }
      }
    } else if (line.rfind('#', 0) == 0) {
        spdlog::debug("HLS playlist parser ignoring unhandled line {}", line);
    } else if (line.size() > 0) {
      std::string uri = base_path + line;
      _streams.push_back({uri, resolution, codecs, bandwidth, frame_rate});
      resolution = ""; codecs = ""; bandwidth = 0; frame_rate = 0;
    }
  }
}

auto MBMS_RT::HlsPrimaryPlaylist::parse_parameters(const std::string& line) const -> std::vector<std::pair<std::string, std::string>>
{
  std::vector<std::pair<std::string, std::string>> result;
  bool quoted = false;
  bool in_key = true;
  std::stringstream key;
  std::stringstream value;
  for(const char& c : line) {
    if (c == '"') {
      quoted = !quoted;
    } else if (c == ',' && !quoted) {
      result.emplace_back(std::make_pair(key.str(), value.str()));
      key.str(""); key.clear();
      value.str(""); value.clear();
      in_key = true;
    } else if (c == '=') {
      in_key = false;
    } else {
      if (in_key) {
        key << c;
      } else {
        value << c;
      }
    }
  }
  result.emplace_back(std::make_pair(key.str(), value.str()));
  return result;
}

auto MBMS_RT::HlsPrimaryPlaylist::to_string() const -> std::string
{
  std::stringstream pl;

  pl << "#EXTM3U" << std::endl;
  pl << "#EXT-X-VERSION:3" << std::endl;

  for (const auto& s : _streams) {
    pl << "#EXT-X-STREAM-INF:BANDWITH=" << s.bandwidth << 
      ",RESOLUTION=" << s.resolution <<
      ",FRAME-RATE=" << std::fixed << std::setprecision(3) << s.frame_rate <<
      ",CODECS=\"" << s.codecs << "\"" << std::endl;
    pl << s.uri << std::endl;
  }
  return std::move(pl.str());
}
