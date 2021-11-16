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
#include "File.h"
#include "Receiver.h"
#include "CacheManagement.h"
#include "CdnClient.h"
#include "Segment.h"

namespace MBMS_RT {
  class ContentStream {
    public:
      ContentStream(std::string flute_if, boost::asio::io_service& io_service, CacheManagement& cache);
      virtual ~ContentStream();

      bool configure_5gbc_delivery_from_sdp(const std::string& sdp);

      void set_cdn_endpoint(const std::string& cdn_ept);

      void start();

      enum class DeliveryProtocol { HLS, DASH, RTP };
      DeliveryProtocol delivery_protocol() const { return _delivery_protocol; };
      void set_delivery_protocol_from_sdp_mime_type(const std::string& mime_type);
      
    private:
      DeliveryProtocol _delivery_protocol;
      std::string _5gbc_stream_iface;
      std::string _5gbc_stream_type = "none";
      std::string _5gbc_stream_mcast_addr = {};
      std::string _5gbc_stream_mcast_port = {};
      unsigned long long _5gbc_stream_flute_tsi = 0;
      std::thread _flute_thread;
      std::unique_ptr<LibFlute::Receiver> _flute_receiver;

      std::string _cdn_endpoint = "none";
      std::string _playlist_path;

      boost::asio::io_service& _io_service;
      CacheManagement& _cache;

      std::map<std::string, std::shared_ptr<Segment>> _segments;

      std::shared_ptr<CdnClient> _cdn_client;

      std::string _playlist = "none";
      std::string _manifest;
  };
}
