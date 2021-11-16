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

#include "CdnClient.h"
#include "CdnFile.h"

#include "spdlog/spdlog.h"

using web::http::client::http_client;
using web::http::status_codes;
using web::http::methods;
using web::http::http_response;

MBMS_RT::CdnClient::CdnClient(const std::string& base_url)
{
  spdlog::debug("Cdn client constructed with base {}", base_url);
  _client = std::make_unique<http_client>(base_url);
}

auto MBMS_RT::CdnClient::get(const std::string& path, std::function<void(std::shared_ptr<CdnFile>)> completion_cb) -> void
{
  spdlog::debug("Cdn client requesting {}", path);
  try {
    _client->request(methods::GET, path)
      .then([completion_cb](http_response response) { // NOLINT
          if (response.status_code() == status_codes::OK) {
            Concurrency::streams::container_buffer<std::vector<uint8_t>> buf;
            response.body().read_to_end(buf)
              .then([buf, completion_cb](size_t bytes_read){
                spdlog::debug("Downloaded {} bytes", bytes_read);
                auto cdn_file = std::make_shared<CdnFile>(bytes_read);
                memcpy(cdn_file->buffer(), &(buf.collection())[0], bytes_read);
                if (completion_cb) {
                  completion_cb(cdn_file);
                }
            });
          } else {
                spdlog::debug("got status {}", response.status_code());
          }
        }).wait();
  } catch (web::http::http_exception ex) { }
}
