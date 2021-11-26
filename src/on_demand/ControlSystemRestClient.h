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
#include <string>
#include <libconfig.h++>
#include "cpprest/http_client.h"

#pragma once
namespace MBMS_RT {
class ControlSystemRestClient {
  public:
    ControlSystemRestClient(const libconfig::Config& cfg);
    virtual ~ControlSystemRestClient() = default;

    web::json::value sendHello(double cinr, const std::vector<std::string>& service_tmgis);

  private:
    std::unique_ptr<web::http::client::http_client> _client;
    std::string _machine_id = {};
};
}
