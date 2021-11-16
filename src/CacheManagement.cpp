
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

#include "CacheManagement.h"
#include "spdlog/spdlog.h"

MBMS_RT::CacheManagement::CacheManagement(const libconfig::Config& cfg, boost::asio::io_service& io_service)
  : _io_service(io_service)
{
  spdlog::debug("CacheManagement created");
  cfg.lookupValue("mw.cache.max_total_size", _max_cache_size);
  _max_cache_size *= 1024 * 1024;
  cfg.lookupValue("mw.cache.max_file_age", _max_cache_file_age);
}

MBMS_RT::CacheManagement::~CacheManagement() 
{
  spdlog::debug("CacheManagement destroyed");
}


auto MBMS_RT::CacheManagement::check_file_expiry_and_cache_size() -> void
{
#if 0
  std::vector<std::pair<unsigned, const CacheItem&> items_by_age;
  for (auto it = _cache_items.cbegin(); it != _files.cend();)
    auto age = time(nullptr) - it->second.received_at;
    if (age > _max_cache_file_age) {
      it = _cache_items.erase(it);
    } else {
      items_by_age.push_back(std::make_pair(age, item));
    }
  }
#endif
}
