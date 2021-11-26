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
#include <filesystem>
#include <libconfig.h++>
#include <boost/asio.hpp>
#include "RpRestClient.h"
#include "Service.h"
#include "ServiceAnnouncement.h"
#include "File.h"
#include "RestHandler.h"
#include "CacheManagement.h"
#include "Service.h"
#include "on_demand/ControlSystemRestClient.h"

namespace MBMS_RT {
  class Middleware {
    public:
      Middleware( boost::asio::io_service& io_service, const libconfig::Config& cfg, const std::string& api_url, const std::string& iface);

      std::shared_ptr<Service> get_service(const std::string& service_id);
      void set_service(const std::string& service_id, std::shared_ptr<Service> service) { _services[service_id] = service; };

    private:
      void tick_handler();

      bool _seamless = false;
      bool _control_system = false;


      MBMS_RT::RpRestClient _rp;
      MBMS_RT::RestHandler _api;
      MBMS_RT::CacheManagement _cache;
      MBMS_RT::ControlSystemRestClient _control;

      std::unique_ptr<MBMS_RT::ServiceAnnouncement> _service_announcement = {nullptr};
      std::map<std::string, std::shared_ptr<Service>> _services;

      boost::posix_time::seconds _tick_interval;
      boost::asio::deadline_timer _timer;

      const libconfig::Config& _cfg;
      const std::string& _interface;
      boost::asio::io_service& _io_service;
    };
};
