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

#include <libconfig.h++>
#include <boost/asio.hpp>
#include "Segment.h"
#include "CacheItems.h"

namespace MBMS_RT {
  class CacheManagement {
    public:
      CacheManagement(const libconfig::Config& cfg, boost::asio::io_service& io_service);
      virtual ~CacheManagement();

      void add_item(std::shared_ptr<CacheItem> item) { _cache_items[item->content_location()] = item; };
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
