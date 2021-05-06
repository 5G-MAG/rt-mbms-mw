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
#include "cpprest/containerstream.h"
#include "cpprest/producerconsumerstream.h"

#include "File.h"
#include "Service.h"

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
    RestHandler(const libconfig::Config& cfg, const std::string& url, const std::map<std::string, OBECA::File>& downloaded_files, const unsigned& total_cache_size, const std::map<std::string, std::unique_ptr<OBECA::Service>>& services );
    /**
     *  Default destructor.
     */
    virtual ~RestHandler();

  private:
    void get(web::http::http_request message);
    void put(web::http::http_request message);
    const libconfig::Config& _cfg;
    const std::map<std::string, OBECA::File>& _files;
    const std::map<std::string, std::unique_ptr<OBECA::Service>>& _services;
    unsigned _total_cache_size;

    std::unique_ptr<web::http::experimental::listener::http_listener> _listener;
    bool _require_bearer_token = false;
    std::string _api_key;
    std::string _base_path;
};

