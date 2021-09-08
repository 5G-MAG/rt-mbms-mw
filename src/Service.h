// OBECA - Open Broadcast Edge Cache Appliance
// Gateway Process
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
#include "cpprest/http_client.h"
#include "File.h"
#include "Receiver.h"

namespace OBECA {
  class Service {
    public:
      Service(const libconfig::Config& cfg, std::string tmgi, const std::string& mcast, unsigned long long tsi, std::string iface, boost::asio::io_service& io_service);

      virtual ~Service();

      std::vector<std::shared_ptr<LibFlute::File>> fileList();

      void setIsServiceAnnouncement(bool val) { _is_service_announcement = val; };
      bool isServiceAnnouncement() { return _is_service_announcement; };

      bool bootstrapped() { return _bootstrap_file_parsed; };

      void tryParseBootstrapFile(std::string str);

      std::string streamType() const  { return _stream_type; };
      std::string streamTmgi() const { return _stream_tmgi; };
      void setStreamTmgi(std::string tmgi) { _stream_tmgi = tmgi; };
      std::string streamMcast() const { return _stream_mcast_addr + ":" + _stream_mcast_port; };
      unsigned long long streamFluteTsi() const { return _stream_flute_tsi; };

      std::string serviceDescription() const { return _service_description; };
      std::string serviceName() const { return _service_name; };
      std::string sdp() const { return _sdp; };
      std::string m3u() const { return _m3u; };

      void remove_expired_files(unsigned max_age);

    private:
      const libconfig::Config& _cfg;

      std::string _iface;
      std::string _tmgi;
      std::string _mcast_addr;
      std::string _mcast_port;
      unsigned long long _tsi = 0;
      std::thread _flute_thread;
      std::unique_ptr<LibFlute::Receiver> _flute_receiver;

      bool _is_service_announcement = false;
      bool _bootstrap_file_parsed = false;

      std::string _service_description = "";
      std::string _service_name = "";
      std::string _sdp = "";
      std::string _m3u = "";

      std::string _stream_tmgi = {};
      std::string _stream_type = "none";
      std::string _stream_mcast_addr = {};
      std::string _stream_mcast_port = {};
      unsigned long long _stream_flute_tsi = 0;
  };
}
