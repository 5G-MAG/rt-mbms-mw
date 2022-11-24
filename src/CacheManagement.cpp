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

#include "CacheManagement.h"
#include "spdlog/spdlog.h"

MBMS_RT::CacheManagement::CacheManagement(const libconfig::Config& cfg, boost::asio::io_service& io_service)
  : _io_service(io_service)
{
  cfg.lookupValue("mw.cache.max_total_size", _max_cache_size);
  _max_cache_size *= 1024 * 1024;
  cfg.lookupValue("mw.cache.max_file_age", _max_cache_file_age);
}

auto MBMS_RT::CacheManagement::check_file_expiry_and_cache_size() -> void
{
  std::multimap<unsigned, std::string> items_by_age;
  for (auto it = _cache_items.cbegin(); it != _cache_items.cend();) {
    spdlog::debug("checking {}", it->second->content_location());
    if (it->second->received_at() != 0) {
      auto age = time(nullptr) - it->second->received_at();
      if (age > _max_cache_file_age) {
        spdlog::info("Cache management deleting expired item at {} after {} seconds",
            it->second->content_location(), age);
        it = _cache_items.erase(it);
      } else {
        items_by_age.insert(std::make_pair(age, it->first));
        ++it;
      }
    } else {
      ++it;
    }
  }
  uint32_t total_size = 0;
  for (const auto& it : items_by_age) {
    total_size += _cache_items[it.second]->content_length();
    if (total_size > _max_cache_size) {
        spdlog::info("Cache management deleting item at {} (aged {} secs) due to cache size limit",
            it.second, it.first);
        _cache_items.erase(it.second);
    }
  }
}
