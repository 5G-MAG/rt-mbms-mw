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

#include "RestHandler.h"

#include <memory>
#include <utility>

#include "spdlog/spdlog.h"

using web::json::value;
using web::http::methods;
using web::http::uri;
using web::http::http_request;
using web::http::status_codes;
using web::http::experimental::listener::http_listener;
using web::http::experimental::listener::http_listener_config;

RestHandler::RestHandler(const libconfig::Config& cfg, const std::string& url, const std::map<std::string, OBECA::File>& downloaded_files, const unsigned& total_cache_size, const std::map<std::string, std::unique_ptr<OBECA::Service>>& services )
    : _cfg(cfg)
    , _files(downloaded_files) 
    , _services(services) 
    , _total_cache_size(total_cache_size) 
{
  _base_path = "/tmp/obeca";
  cfg.lookupValue("gw.cache.base_path", _base_path);

  http_listener_config server_config;
  if (url.rfind("https", 0) == 0) {
    server_config.set_ssl_context_callback(
        [&](boost::asio::ssl::context& ctx) {
          std::string cert_file = "/usr/share/obeca/cert.pem";
          cfg.lookupValue("gw.restful_api.cert", cert_file);

          std::string key_file = "/usr/share/obeca/key.pem";
          cfg.lookupValue("gw.restful_api.key", key_file);

          ctx.set_options(boost::asio::ssl::context::default_workarounds);
          ctx.use_certificate_chain_file(cert_file);
          ctx.use_private_key_file(key_file, boost::asio::ssl::context::pem);
        });
  }

  cfg.lookupValue("gw.restful_api.api_key.enabled", _require_bearer_token);
  if (_require_bearer_token) {
    _api_key = "106cd60-76c8-4c37-944c-df21aa690c1e";
    cfg.lookupValue("gw.restful_api.api_key.key", _api_key);
  }

  _listener = std::make_unique<http_listener>(
      url, server_config);

  _listener->support(methods::GET, std::bind(&RestHandler::get, this, std::placeholders::_1));  // NOLINT
  _listener->support(methods::PUT, std::bind(&RestHandler::put, this, std::placeholders::_1));  // NOLINT

  _listener->open().wait();
}

RestHandler::~RestHandler() = default;

void RestHandler::get(http_request message) {
  spdlog::debug("Received GET request {}", message.to_string() );
  auto paths = uri::split_path(uri::decode(message.relative_uri().path()));
  if (_require_bearer_token &&
    (message.headers()["Authorization"] != "Bearer " + _api_key)) {
    message.reply(status_codes::Unauthorized);
    return;
  }

  if (paths.empty()) {
    message.reply(status_codes::NotFound);
  } else {
    if (paths[0] == "files") {
      std::vector<value> files;
      for (const auto& [location, file] : _files) {
        value f;
        f["filename"] = value(file.location().filename().string());
        auto file_path = file.location().string();
        auto sp = file_path.find(_base_path);
        if (sp != std::string::npos) {
          file_path.erase(sp, _base_path.size());
        }
        f["location"] = value(file_path);
        f["content_length"] = value(file.content_length());
        f["received_at"] = value(file.received_at());
        f["age"] = value(time(nullptr) - file.received_at());
        files.push_back(f);
      }
      message.reply(status_codes::OK, value::array(files));
    } else if (paths[0] == "services") {
      std::vector<value> services;
      for (const auto& [tmgi, service] : _services) {
        if (!service->bootstrapped()) continue;
        value s;
        s["service_tmgi"] = value(tmgi);
        s["service_name"] = value(service->serviceName());
        s["service_description"] = value(service->serviceDescription());
        s["sdp"] = value(service->sdp());
        s["m3u"] = value(service->m3u());
        s["stream_tmgi"] = value(service->streamTmgi());
        s["stream_type"] = value(service->streamType());
        s["stream_mcast"] = value(service->streamMcast());
        services.push_back(s);
      }
      message.reply(status_codes::OK, value::array(services));
    } 
  }
}

void RestHandler::put(http_request message) {
  spdlog::debug("Received PUT request {}", message.to_string() );

  if (_require_bearer_token &&
    (message.headers()["Authorization"] != "Bearer " + _api_key)) {
    message.reply(status_codes::Unauthorized);
    return;
  }

  auto paths = uri::split_path(uri::decode(message.relative_uri().path()));
  if (paths.empty()) {
    message.reply(status_codes::NotFound);
  } else {
    // not yet implemented
  }
}

