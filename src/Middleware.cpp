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
#include "Middleware.h"
#include "spdlog/spdlog.h"

/**
 *
 * @param io_service
 * @param cfg
 * @param api_url
 * @param iface
 */
MBMS_RT::Middleware::Middleware(boost::asio::io_service &io_service, const libconfig::Config &cfg,
                                const std::string &api_url,
                                const std::string &iface)
    : _rp(cfg), _control(cfg), _cache(cfg, io_service), _api(cfg, api_url, _cache, &_service_announcement, _services),
      _tick_interval(1), _timer(io_service, _tick_interval), _control_timer(io_service, _control_tick_interval),
      _cfg(cfg), _interface(iface), _io_service(io_service) {
  cfg.lookupValue("mw.seamless_switching.enabled", _seamless);
  if (_seamless) {
    spdlog::info("Seamless switching mode enabled");
  }
  cfg.lookupValue("mw.control_system.enabled", _control_system);
  if (_control_system) {
    int secs = 10;
    cfg.lookupValue("mw.control_system.interval", secs);
    _control_tick_interval = boost::posix_time::seconds(secs);
    spdlog::info("Control System API enabled");
  }

  _handle_local_service_announcement();
  _timer.async_wait(boost::bind(&Middleware::tick_handler, this)); //NOLINT
  _control_timer.async_wait(boost::bind(&Middleware::control_tick_handler, this)); //NOLINT
}

/**
 *
 * @return {bool} Whether a local SA file was used
 */
auto MBMS_RT::Middleware::_handle_local_service_announcement() -> bool {
  try {
    bool local_service_enabled = false;
    std::string local_bootstrap_file;

    _cfg.lookupValue("mw.local_service.enabled", local_service_enabled);
    _cfg.lookupValue("mw.local_service.bootstrap_file", local_bootstrap_file);

    if (local_service_enabled && local_bootstrap_file != "") {
      spdlog::info("Reading service announcement from file at {}", local_bootstrap_file);
      std::ifstream ifs(local_bootstrap_file);
      std::string sa_multipart((std::istreambuf_iterator<char>(ifs)),
                               (std::istreambuf_iterator<char>()));
      std::string tmgi = "";
      _cfg.lookupValue("mw.local_service.tmgi", tmgi);
      std::string mcast_address = "";
      _cfg.lookupValue("mw.local_service.mcast_address", mcast_address);

      _service_announcement = std::make_unique<MBMS_RT::ServiceAnnouncement>(_cfg, tmgi, mcast_address, 0, _interface,
                                                                             _io_service, _cache, _seamless,
                                                                             boost::bind(&Middleware::get_service, this,
                                                                                         _1),  //NOLINT
                                                                             boost::bind(&Middleware::set_service, this,
                                                                                         _1, _2)); //NOLINT
      _service_announcement->parse_bootstrap(sa_multipart);

      return true;
    }
    return false;
  } catch (...) {
    return false;
  }
}

/**
 *
 */
void MBMS_RT::Middleware::tick_handler() {
  auto mchs = _rp.getMchInfo();
  for (auto const &mch: mchs.as_array()) {
    for (auto const &mtch: mch.at("mtchs").as_array()) {
      auto tmgi = mtch.at("tmgi").as_string();
      auto dest = mtch.at("dest").as_string();
      auto is_service_announcement = std::stoul(tmgi.substr(0, 6), nullptr, 16) < 0xF;
      if (!dest.empty() && is_service_announcement && !_service_announcement) {
        // automatically start receiving the service announcement
        // 26.346 5.2.3.1.1 : the pre-defined TSI value shall be "0". 
        _service_announcement = std::make_unique<MBMS_RT::ServiceAnnouncement>(_cfg, tmgi, dest, 0 /*TSI*/, _interface,
                                                                               _io_service,
                                                                               _cache, _seamless,
                                                                               boost::bind(&Middleware::get_service,
                                                                                           this, _1),
                                                                               boost::bind(&Middleware::set_service,
                                                                                           this, _1, _2)); //NOLINT
        _service_announcement->start_flute_receiver(dest);
      }
    }
  }

  _cache.check_file_expiry_and_cache_size();

  _timer.expires_at(_timer.expires_at() + _tick_interval);
  _timer.async_wait(boost::bind(&Middleware::tick_handler, this)); //NOLINT
}

/**
 *
 */
void MBMS_RT::Middleware::control_tick_handler() {
  if (_control_system) {
    try {
      auto status = _rp.getStatus();
      auto cinr = status.at("cinr_db").as_double();
      spdlog::debug("CINR ist {}", cinr);

      std::vector<std::string> tmgis;
      auto services = _control.sendHello(cinr, tmgis);
      for (const auto &ctrl_service: services.as_array()) {
        spdlog::debug("control system sent service: {}", ctrl_service.serialize());
      }
    } catch (...) {}
  }


  _control_timer.expires_at(_control_timer.expires_at() + _control_tick_interval);
  _control_timer.async_wait(boost::bind(&Middleware::control_tick_handler, this)); //NOLINT
}

/**
 *
 * @param {string} service_id
 * @return
 */
auto MBMS_RT::Middleware::get_service(const std::string &service_id) -> std::shared_ptr<Service> {
  if (_services.find(service_id) != _services.end()) {
    return _services[service_id];
  } else {
    return nullptr;
  }
}
