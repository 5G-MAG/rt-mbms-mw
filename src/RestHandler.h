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
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <libconfig.h++>

#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#include "cpprest/filestream.h"
#include "cpprest/rawptrstream.h"
#include "cpprest/containerstream.h"
#include "cpprest/producerconsumerstream.h"

#include "File.h"
#include "Service.h"
#include "ServiceAnnouncement.h"
#include "CacheManagement.h"

namespace MBMS_RT {
  /**
   *  The RESTful API handler. Supports GET and PUT verbs for SDR parameters, and GET for reception info
   */
  class RestHandler {
    public:
      /**
       *  Default constructor.
       *
       *  @param cfg Config singleton reference
       *  @param url URL to open the server on
       */
      RestHandler(const libconfig::Config& cfg, const std::string& url, const CacheManagement& cache,
          const std::unique_ptr<MBMS_RT::ServiceAnnouncement>* service_announcement,
          const std::map<std::string, std::unique_ptr<MBMS_RT::Service>>& services );
      /**
       *  Default destructor.
       */
      virtual ~RestHandler();

    private:

      const CacheManagement& _cache;
      void get(web::http::http_request message);
      void put(web::http::http_request message);
      const libconfig::Config& _cfg;
   //   const std::map<std::string, LibFlute::File>& _files;
      const std::map<std::string, std::unique_ptr<MBMS_RT::Service>>& _services;
      const std::unique_ptr<MBMS_RT::ServiceAnnouncement>* _service_announcement_h = {};
      unsigned _total_cache_size;

      std::unique_ptr<web::http::experimental::listener::http_listener> _listener;
      bool _require_bearer_token = false;
      std::string _api_key;
      std::string _api_path;
  };
};
