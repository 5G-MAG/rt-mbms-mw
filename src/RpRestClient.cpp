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

#include "RpRestClient.h"

#include "spdlog/spdlog.h"

using web::http::client::http_client;
using web::http::status_codes;
using web::http::methods;
using web::http::http_response;

OBECA::RpRestClient::RpRestClient(const libconfig::Config& cfg)
{
  std::string url = "http://localhost:3010/rp-api/";
//  cfg.lookupValue("rp.restful_api.uri", url);
  _client = std::make_unique<http_client>(url);
}

auto OBECA::RpRestClient::getMchInfo() -> web::json::value
{
  auto res = web::json::value::array();
  try {
    _client->request(methods::GET, "mch_info")
      .then([&res](http_response response) { // NOLINT
          if (response.status_code() == status_codes::OK) {
            res = response.extract_json().get();
          }
        }).wait();
  } catch (web::http::http_exception ex) { }
  return res;
}
