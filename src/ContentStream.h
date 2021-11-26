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
#include "DeliveryProtocols.h"

namespace MBMS_RT {
  class ContentStream {
    public:
      ContentStream(std::string base, std::string flute_if, boost::asio::io_service& io_service, CacheManagement& cache, DeliveryProtocol protocol, const libconfig::Config& cfg);
      virtual ~ContentStream() = default;

      enum class StreamType {
        Basic,
        SeamlessSwitching
      };
      virtual StreamType stream_type() const { return StreamType::Basic; };
      virtual std::string stream_type_string() const { return "Basic"; };

      bool configure_5gbc_delivery_from_sdp(const std::string& sdp);
      void read_master_manifest(const std::string& manifest);
      void start();

      virtual void flute_file_received(std::shared_ptr<LibFlute::File> file);

      const std::string& base() const { return _base; };
      const std::string& playlist_path() const { return  _playlist_path; };

      std::string flute_info() const;

      DeliveryProtocol delivery_protocol() const { return _delivery_protocol; };
      std::string delivery_protocol_string() const { return _delivery_protocol == DeliveryProtocol::HLS ? "HLS" :
        (_delivery_protocol == DeliveryProtocol::DASH ? "DASH" : "RTP"); };


      const std::string& resolution() const { return _resolution; };
      void set_resolution(std::string r) { _resolution = std::move(r); };

      const std::string& codecs() const { return _codecs; }
      void set_codecs(std::string c) { _codecs = std::move(c); };
      
      unsigned long bandwidth() const { return _bandwidth; }
      void set_bandwidth(unsigned long b) { _bandwidth = b; };
      
      double frame_rate() const { return _frame_rate; }
      void set_frame_rate(double f) { _frame_rate = f; };

    protected:
      const libconfig::Config& _cfg;
      DeliveryProtocol _delivery_protocol;
      std::string _base = "";
      std::string _playlist_path;
      std::string _5gbc_stream_iface;
      std::string _5gbc_stream_type = "none";
      std::string _5gbc_stream_mcast_addr = {};
      std::string _5gbc_stream_mcast_port = {};
      unsigned long long _5gbc_stream_flute_tsi = 0;
      std::thread _flute_thread;
      std::unique_ptr<LibFlute::Receiver> _flute_receiver;

      boost::asio::io_service& _io_service;
      CacheManagement& _cache;

      std::string _resolution;
      std::string _codecs;
      unsigned long _bandwidth;
      double _frame_rate;
  };
}
