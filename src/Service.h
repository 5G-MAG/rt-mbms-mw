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
#include <string>
#include <thread>
#include <libconfig.h++>
#include "cpprest/http_client.h"
#include "File.h"


#pragma once
namespace OBECA {
  class FluteReceiver;
class Service {
  public:
    Service(const libconfig::Config& cfg, const std::string& tmgi, const std::string& mcast, unsigned long long tsi, const std::string& iface);

    virtual ~Service();

    std::vector<OBECA::File> fileList();

    void setIsServiceAnnouncement(bool val) { _is_service_announcement = val; };
    bool isServiceAnnouncement() { return _is_service_announcement; };

    bool bootstrapped() { return _bootstrap_file_parsed; };

    void tryParseBootstrapFile();

    std::string streamType() const  { return _stream_type; };
    std::string streamTmgi() const { return _stream_tmgi; };
    void setStreamTmgi(std::string tmgi) { _stream_tmgi = tmgi; };
    std::string streamMcast() const { return _stream_mcast_addr + ":" + _stream_mcast_port; };
    unsigned long long streamFluteTsi() const { return _stream_flute_tsi; };

    std::string serviceDescription() const { return _service_description; };
    std::string serviceName() const { return _service_name; };
    std::string sdp() const { return _sdp; };
    std::string m3u() const { return _m3u; };

  private:
    const libconfig::Config& _cfg;

    std::string _iface;
    std::string _tmgi;
    std::string _mcast_addr;
    std::string _mcast_port;
    unsigned long long _tsi = 0;
    std::string _target_directory;
    std::thread _flute_thread;
    std::unique_ptr<OBECA::FluteReceiver> _flute_receiver;

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
