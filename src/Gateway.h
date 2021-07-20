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

#pragma once

#include <string>
#include <filesystem>
#include <libconfig.h++>
#include <boost/asio.hpp>
#include "RpRestClient.h"
#include "Service.h"
#include "File.h"
#include "RestHandler.h"

namespace OBECA {
  class Gateway {
    public:
      Gateway( boost::asio::io_service& io_service, const libconfig::Config& cfg, const std::string& api_url, const std::string& iface);
    private:
      void tick_handler();

      unsigned _max_cache_size = 512;
      unsigned _total_cache_size = 0;
      unsigned _max_cache_file_age = 30;

      OBECA::RpRestClient _rp;
      OBECA::RestHandler _api;
      std::map<std::string, std::string> _available_services;
      std::map<std::string, std::unique_ptr<OBECA::Service>> _services;
      std::map<std::string, std::unique_ptr<OBECA::Service>> _payload_flute_streams;

      boost::posix_time::seconds _tick_interval;
      boost::asio::deadline_timer _timer;

      const libconfig::Config& _cfg;
      const std::string& _interface;
      boost::asio::io_service& _io_service;
    };
};
