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
#include "File.h"
#include "Receiver.h"
#include "CacheManagement.h"
#include "DeliveryProtocols.h"

namespace MBMS_RT {
  class ContentStream {
    public:
      ContentStream(std::string base, std::string flute_if, boost::asio::io_service& io_service, CacheManagement& cache, DeliveryProtocol protocol, const libconfig::Config& cfg);
      virtual ~ContentStream();

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
