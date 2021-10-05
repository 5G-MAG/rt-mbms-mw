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

#include "RpRestClient.h"

#include "spdlog/spdlog.h"

using web::http::client::http_client;
using web::http::status_codes;
using web::http::methods;
using web::http::http_response;

MBMS_RT::RpRestClient::RpRestClient(const libconfig::Config& cfg)
{
  std::string url = "http://localhost:3010/rp-api/";
//  cfg.lookupValue("rp.restful_api.uri", url);
  _client = std::make_unique<http_client>(url);
}

auto MBMS_RT::RpRestClient::getMchInfo() -> web::json::value
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
