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
