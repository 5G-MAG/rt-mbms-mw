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

#include <libconfig.h++>
#include <boost/asio.hpp>
#include "CacheItems.h"

namespace MBMS_RT {
  class CacheManagement {
    public:
      CacheManagement(const libconfig::Config& cfg, boost::asio::io_service& io_service);
      virtual ~CacheManagement() = default;

      void add_item(std::shared_ptr<CacheItem> item) { _cache_items[item->content_location()] = item; };
      void remove_item(const std::string& location) { _cache_items.erase(location); };
      const std::map<std::string, std::shared_ptr<CacheItem>>& item_map() const { return _cache_items; };

      void check_file_expiry_and_cache_size();


    private:
      std::map<std::string, std::shared_ptr<CacheItem>> _cache_items; 
      unsigned _max_cache_size = 512;
      unsigned _total_cache_size = 0;
      unsigned _max_cache_file_age = 30;
      boost::asio::io_service& _io_service;
  };
}
