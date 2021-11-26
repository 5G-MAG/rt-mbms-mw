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
#include <thread>
#include <libconfig.h++>
#include "HlsPrimaryPlaylist.h"
#include "File.h"
#include "Receiver.h"
#include "ContentStream.h"
#include "DeliveryProtocols.h"

namespace MBMS_RT {
  class Service {
    public:
      Service(CacheManagement& cache)
        : _cache(cache) {};
      virtual ~Service() = default;

      void add_name(const std::string& name, const std::string& lang);
      void add_and_start_content_stream(std::shared_ptr<ContentStream> s);
      void read_master_manifest(const std::string& manifest, const std::string& base_path);

      const std::map<std::string, std::string>& names() const { return _names; };
      const std::map<std::string, std::shared_ptr<ContentStream>>& content_streams() const { return _content_streams; };

      DeliveryProtocol delivery_protocol() const { return _delivery_protocol; };
      std::string delivery_protocol_string() const { return _delivery_protocol == DeliveryProtocol::HLS ? "HLS" :
        (_delivery_protocol == DeliveryProtocol::DASH ? "DASH" : "RTP"); };
      void set_delivery_protocol_from_mime_type(const std::string& mime_type);

      const std::string& manifest_path() const { return  _manifest_path; };

    private:
      CacheManagement& _cache;
      DeliveryProtocol _delivery_protocol;
      std::map<std::string, std::shared_ptr<ContentStream>> _content_streams;
      std::map<std::string, std::string> _names;

      HlsPrimaryPlaylist _hls_primary_playlist;
      std::string _manifest;
      std::string _manifest_path;
  };
}
