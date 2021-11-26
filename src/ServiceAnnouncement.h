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
#include "cpprest/http_client.h"
#include "File.h"
#include "Receiver.h"
#include "Service.h"
#include "CacheManagement.h"

namespace MBMS_RT {
  class ServiceAnnouncement {
    public:
      typedef std::function<std::shared_ptr<Service>(const std::string& service_id)> get_service_callback_t;
      typedef std::function<void(const std::string& service_id, std::shared_ptr<Service>)> set_service_callback_t;

      ServiceAnnouncement(const libconfig::Config& cfg, std::string tmgi, const std::string& mcast, unsigned long long tsi, 
          std::string iface, boost::asio::io_service& io_service, CacheManagement& cache, bool seamless_switching,
          get_service_callback_t get_service, set_service_callback_t set_service);

      ServiceAnnouncement(const libconfig::Config& cfg, std::string iface, boost::asio::io_service& io_service, CacheManagement& cache, bool seamless_switching,
          get_service_callback_t get_service, set_service_callback_t set_service);
      virtual ~ServiceAnnouncement();

      struct Item {
        std::string content_type;
        std::string uri;
        time_t valid_from;
        time_t valid_until;
        unsigned version;
        std::string content;
      };

      const std::vector<Item>& items() const { return _items; };
      const std::string& content() const { return _raw_content; };

      uint32_t toi() const { return _toi; };
      void parse_bootstrap(const std::string& str);

    private:

      get_service_callback_t _get_service;
      set_service_callback_t _set_service;

      bool _seamless = false;

      std::vector<Item> _items;

      const libconfig::Config& _cfg;

      bool _bootstrapped = false;

      uint32_t _toi = {};
      std::string _raw_content;
      std::string _iface;
      std::string _tmgi;
      std::string _mcast_addr;
      std::string _mcast_port;
      unsigned long long _tsi = 0;
      std::thread _flute_thread;
      std::unique_ptr<LibFlute::Receiver> _flute_receiver;

      boost::asio::io_service& _io_service;
      CacheManagement& _cache;
  };
}
