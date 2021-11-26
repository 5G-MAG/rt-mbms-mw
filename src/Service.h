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
