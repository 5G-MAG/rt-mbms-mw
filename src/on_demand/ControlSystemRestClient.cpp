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

#include "ControlSystemRestClient.h"

#include <boost/algorithm/string.hpp>
#include "spdlog/spdlog.h"

using web::json::value;
using web::http::client::http_client;
using web::http::status_codes;
using web::http::methods;
using web::http::http_response;

MBMS_RT::ControlSystemRestClient::ControlSystemRestClient(const libconfig::Config& cfg)
{
  std::string ep = "";
  cfg.lookupValue("mw.control_system.endpoint", ep);
  if (ep != "") {
    _client = std::make_unique<http_client>(ep);
  }

  std::ifstream ifs("/etc/machine-id");
  std::string machine_id( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
  boost::trim(machine_id);
  _machine_id = machine_id;
}

auto MBMS_RT::ControlSystemRestClient::sendHello(double cinr, const std::vector<std::string>& service_tmgis) -> web::json::value
{
  auto res = web::json::value::array();
  if (!_client) return res;

  web::json::value req;
  req["sn"] = value(_machine_id);
  req["cinr"] = value(cinr);
  req["timestamp"] = value(std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count());
  req["lat"] = value(0);
  req["long"] = value(0);

  std::vector<web::json::value> services;
  for (const auto& tmgi : service_tmgis) {
    services.push_back(value(tmgi));
  }
  req["services"] = web::json::value::array(/*services*/);

  try {
    _client->request(methods::POST, "hello", req)
      .then([&res](http_response response) { // NOLINT
          if (response.status_code() == status_codes::OK) {
            res = response.extract_json().get();
            spdlog::debug("control system: {}", response.to_string());
          }
          else  {
            spdlog::error("Error from control system: {} - {}", response.status_code(), response.to_string());
          }
        }).wait();
  } catch (web::http::http_exception ex) { }
  return res;
}
