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
