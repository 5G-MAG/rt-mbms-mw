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

#include "Gateway.h" 

OBECA::Gateway::Gateway( boost::asio::io_service& io_service, const libconfig::Config& cfg, const std::string& api_url, const std::string& iface)
  : _rp(cfg)
  , _api(cfg, api_url, _total_cache_size, _services)
  , _tick_interval(1)
  , _timer(io_service, _tick_interval)
  , _cfg(cfg)
  , _interface(iface)
  , _io_service(io_service)
{
  _cfg.lookupValue("gw.cache.max_total_size", _max_cache_size);
  _max_cache_size *= 1024 * 1024;
  _cfg.lookupValue("gw.cache.max_file_age", _max_cache_file_age);
  _timer.async_wait(boost::bind(&Gateway::tick_handler, this));
}

void OBECA::Gateway::tick_handler()
{
  auto mchs = _rp.getMchInfo();
  _available_services.clear();
  for (auto const &mch : mchs.as_array()) {
    for (auto const &mtch : mch.at("mtchs").as_array()) {
      auto tmgi = mtch.at("tmgi").as_string();
      auto dest = mtch.at("dest").as_string();
      _available_services[tmgi] = dest;
      auto is_service_announcement = std::stoul(tmgi.substr(0,6), nullptr, 16) < 0xF;
      if (!dest.empty() && is_service_announcement && _services.find(tmgi) == _services.end()) {
        // automatically start receiving the service announcement
        // 26.346 5.2.3.1.1 : the pre-defined TSI value shall be "0". 
        _services[tmgi] = std::make_unique<OBECA::Service>(_cfg, tmgi, dest, 0 /*TSI*/, _interface, _io_service);
        _services[tmgi]->setIsServiceAnnouncement(true);
      }
    }
  }

  for (auto it = _services.cbegin(); it != _services.cend();)
  {
    if (_available_services.find(it->first) == _available_services.end()) {
      it = _services.erase(it);
    } else {
      ++it;
    }
  }

  for (auto const& [tmgi, service] : _services) {
    // read file lists
    auto files = service->fileList();
    for (auto file : files) {
      //_downloaded_files.insert_or_assign(file.location(), file);
      if (file->meta().content_location == "bootstrap.multipart" && service->isServiceAnnouncement()) {
        if (!service->bootstrapped()) {
          service->tryParseBootstrapFile(file->buffer());
        }
      }
    }

    if (service->bootstrapped() && service->streamType() == "FLUTE/UDP") {
      if (_payload_flute_streams.count(service->streamMcast()) == 0) {
        auto mcast_target = service->streamMcast();
        auto service_available = std::find_if(_available_services.begin(), _available_services.end(),
            [&mcast_target](const std::pair<std::string, std::string> &p) {
            return p.second == mcast_target;
            });

        if (service_available != _available_services.end()) {
          if (_services.find(service_available->first) == _services.end()) {
            service->setStreamTmgi(service_available->first);
            _services[service_available->first] = std::make_unique<OBECA::Service>(_cfg, service_available->first, service->streamMcast(), service->streamFluteTsi(), _interface, _io_service);
          }
        }
      }
    }
    service->remove_expired_files(_max_cache_file_age);
  }

  _timer.expires_at(_timer.expires_at() + _tick_interval);
  _timer.async_wait(boost::bind(&Gateway::tick_handler, this));
}
