// 5G-MAG Reference Tools
// MBMS Middleware Process
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

#include <regex>
#include <iostream>     // std::cin, std::cout
#include <iomanip>      // std::get_time
#include <ctime>        // struct std::tm
#include <boost/algorithm/string/trim.hpp>
#include "ServiceAnnouncement.h"
#include "Service.h"
#include "Receiver.h"
#include "seamless/SeamlessContentStream.h"

#include "spdlog/spdlog.h"
#include "gmime/gmime.h" 
#include "tinyxml2.h" 
#include "cpprest/base_uri.h"


MBMS_RT::ServiceAnnouncement::ServiceAnnouncement(const libconfig::Config& cfg, std::string tmgi, const std::string& mcast, 
    unsigned long long tsi, std::string iface, boost::asio::io_service& io_service, CacheManagement& cache, bool seamless_switching,
    get_service_callback_t get_service, set_service_callback_t set_service)
  : _cfg(cfg)
  , _tmgi(std::move(tmgi))
  , _tsi(tsi)
  , _iface(std::move(iface))
  , _io_service(io_service)
  , _cache(cache)
  , _flute_thread{}
  , _seamless(seamless_switching)
  , _get_service( std::move(get_service) )
  , _set_service( std::move(set_service) )
{
  size_t delim = mcast.find(':');
  if (delim == std::string::npos) {
    spdlog::error("Invalid multicast address {}", mcast);
    return;
  }
  _mcast_addr = mcast.substr(0, delim);
  _mcast_port = mcast.substr(delim + 1);
  spdlog::info("Starting FLUTE receiver on {}:{} for TSI {}", _mcast_addr, _mcast_port, _tsi); 
  _flute_thread = std::thread{[&](){
    _flute_receiver = std::make_unique<LibFlute::Receiver>(_iface, _mcast_addr, atoi(_mcast_port.c_str()), _tsi, io_service) ;

    /*
    _flute_receiver->register_progress_callback(
      [&](std::shared_ptr<LibFlute::File> file) {
        spdlog::info("{} (TOI {}) progress, completion: {}",
          file->meta().content_location, file->meta().toi, file->percent_complete());
      });
*/

    _flute_receiver->register_completion_callback(
      [&](std::shared_ptr<LibFlute::File> file) { //NOLINT
        spdlog::info("{} (TOI {}) has been received",
          file->meta().content_location, file->meta().toi);
        if (file->meta().content_location == "bootstrap.multipart" && (!_bootstrapped || _toi != file->meta().toi)) {
          _toi = file->meta().toi;
          _raw_content = std::string(file->buffer());
          parse_bootstrap(file->buffer());
        }
      });
  }};
}

MBMS_RT::ServiceAnnouncement::ServiceAnnouncement(const libconfig::Config& cfg, std::string iface, boost::asio::io_service& io_service, CacheManagement& cache, bool seamless_switching, get_service_callback_t get_service, set_service_callback_t set_service)
  : _cfg(cfg)
  , _cache(cache)
  , _io_service(io_service)
  , _iface(std::move(iface))
  , _flute_thread{}
  , _seamless(seamless_switching)
  , _get_service( std::move(get_service) )
  , _set_service( std::move(set_service) )
{
}

MBMS_RT::ServiceAnnouncement::~ServiceAnnouncement() {
  spdlog::info("Closing service announcement session with TMGI {}", _tmgi);
  _flute_receiver.reset();
  if (_flute_thread.joinable()) {
    _flute_thread.join();
  }
}

auto MBMS_RT::ServiceAnnouncement::parse_bootstrap(const std::string& str) -> void
{
  g_mime_init();
  auto stream = g_mime_stream_mem_new_with_buffer(str.c_str(), str.length());
  auto parser = g_mime_parser_new_with_stream(stream);
  g_object_unref(stream);

  auto mpart = g_mime_parser_construct_part(parser, nullptr);
  g_object_unref(parser);

  auto iter = g_mime_part_iter_new(mpart);
  do {
    GMimeObject *current = g_mime_part_iter_get_current (iter);
    GMimeObject *parent = g_mime_part_iter_get_parent (iter);

    if ( GMIME_IS_PART (current)) {
      auto type = std::string(g_mime_content_type_get_mime_type(g_mime_object_get_content_type(current)));
      std::string location = "";
      if ( g_mime_object_get_header(current, "Content-Location")) {
        location = std::string(g_mime_object_get_header(current, "Content-Location"));
      }
      auto options = g_mime_format_options_new();
      g_mime_format_options_add_hidden_header(options, "Content-Type");
      g_mime_format_options_add_hidden_header(options, "Content-Transfer-Encoding");
      g_mime_format_options_add_hidden_header(options, "Content-Location");
      std::string content = g_mime_object_to_string(current, options);
      boost::algorithm::trim_left(content);

      if (location != "") {
        _items.emplace_back(Item{
            type,
            location,
            0, 0, 0,
            content
            });
      }
    }
  } while (g_mime_part_iter_next(iter));

  // Parse MBMS envelope
  for (const auto& item : _items) {
    if (item.content_type == "application/mbms-envelope+xml") {
      try {
        tinyxml2::XMLDocument doc;
        doc.Parse(item.content.c_str());
        auto envelope = doc.FirstChildElement("metadataEnvelope");
        for(auto* i = envelope->FirstChildElement("item"); i != nullptr; i = i->NextSiblingElement("item")) {
          spdlog::debug("uri: {}", i->Attribute("metadataURI"));
          for (auto& ir : _items) {
            if (ir.uri == i->Attribute("metadataURI") ) {
              std::stringstream ss_from(i->Attribute("validFrom"));
              struct std::tm from;
              ss_from >> std::get_time(&from, "%Y-%m-%dT%H:%M:%S.%fZ");
              ir.valid_from = mktime(&from);
              std::stringstream ss_until(i->Attribute("validUntil"));
              struct std::tm until;
              ss_until >> std::get_time(&until, "%Y-%m-%dT%H:%M:%S.%fZ");
              ir.valid_until = mktime(&until);
              ir.version = atoi(i->Attribute("version"));
            }
          }
        }
      } catch (std::exception e) {
        spdlog::warn("MBMS envelope parsing failed: {}", e.what());
      }
    }
  }

  // Parse MBMS user service description bundle
  for (const auto& item : _items) {
    if (item.content_type == "application/mbms-user-service-description+xml") {
      try {
        tinyxml2::XMLDocument doc;
        doc.Parse(item.content.c_str());

        auto bundle = doc.FirstChildElement("bundleDescription");
        for(auto* usd = bundle->FirstChildElement("userServiceDescription"); 
            usd != nullptr; 
            usd = usd->NextSiblingElement("userServiceDescription")) {
          auto service_id = usd->Attribute("serviceId");

          bool is_new_service = false;
          auto service = _get_service(service_id);
          if (service == nullptr) {
            service = std::make_shared<Service>(_cache);
            is_new_service = true;
          }

          // read the names
          for(auto* name = usd->FirstChildElement("name"); name != nullptr; name = name->NextSiblingElement("name")) {
            auto lang = name->Attribute("lang");
            auto namestr = name->GetText();
            service->add_name(namestr, lang);
          }

          // parse the appService element
          auto app_service = usd->FirstChildElement("r12:appService");
          service->set_delivery_protocol_from_mime_type(app_service->Attribute("mimeType"));

          for (const auto& item : _items) {
            if (item.uri == app_service->Attribute("appServiceDescriptionURI")) {
              web::uri uri(item.uri);

              // remove file, leave only dir
              const std::string& path = uri.path();
              size_t spos = path.rfind('/');
              auto base_path = path.substr(0, spos+1);

              // make relative path: remove leading /
              if (base_path[0] == '/') {
                base_path.erase(0,1);
              }
              service->read_master_manifest(item.content, base_path);
            }
          }
          auto alternative_content = app_service->FirstChildElement("r12:alternativeContent");
          for (auto* base_pattern = alternative_content->FirstChildElement("r12:basePattern"); 
               base_pattern != nullptr; 
               base_pattern = base_pattern->NextSiblingElement("r12:basePattern")) {
            std::string base = base_pattern->GetText();

            // create a content stream
            std::shared_ptr<ContentStream> cs;
            if (_seamless) {
              cs = std::make_shared<SeamlessContentStream>(base, _iface, _io_service, _cache, service->delivery_protocol(), _cfg);
            } else { 
              cs = std::make_shared<ContentStream>(base, _iface, _io_service, _cache, service->delivery_protocol(), _cfg);
            }
            bool have_delivery_method = false;

            // Check for 5GBC delivery method elements
            for(auto* delivery_method = usd->FirstChildElement("deliveryMethod"); 
                delivery_method != nullptr; 
                delivery_method = delivery_method->NextSiblingElement("deliveryMethod")) {
              auto sdp_uri = delivery_method->Attribute("sessionDescriptionURI");
              auto broadcast_app_service = delivery_method->FirstChildElement("r12:broadcastAppService");
              std::string broadcast_base_pattern = broadcast_app_service->FirstChildElement("r12:basePattern")->GetText();

              if (broadcast_base_pattern == base) {
                for (const auto& item : _items) {
                  if (item.uri == broadcast_base_pattern) {
                    cs->read_master_manifest(item.content);
                  }
                  if (item.content_type == "application/sdp" &&
                      item.uri == sdp_uri) {
                    have_delivery_method = cs->configure_5gbc_delivery_from_sdp(item.content);
                  }
                }
              }
            }

            if (_seamless) {
              if (!have_delivery_method) {
                // No 5G broadcast available. Assume the base pattern is a CDN endpoint.
                std::dynamic_pointer_cast<SeamlessContentStream>(cs)->set_cdn_endpoint(base);
                have_delivery_method = true;
              } else {
                // Check for identical content entries to find a CDN base pattern
                for(auto* identical_content = app_service->FirstChildElement("r12:identicalContent"); 
                    identical_content != nullptr; 
                    identical_content = identical_content->NextSiblingElement("r12:identicalContent")) {

                  bool base_matched = false;
                  std::string found_identical_base;
                  for(auto* base_pattern = identical_content->FirstChildElement("r12:basePattern"); 
                      base_pattern != nullptr; 
                      base_pattern = base_pattern->NextSiblingElement("r12:basePattern")) {
                    std::string identical_base = base_pattern->GetText();
                    if (base == identical_base) {
                      base_matched = true;
                    } else {
                      found_identical_base = identical_base;
                    }
                  }

                  if (base_matched && found_identical_base.length()) {
                    std::dynamic_pointer_cast<SeamlessContentStream>(cs)->set_cdn_endpoint(found_identical_base);
                  }
                }
              }
            }

            if (have_delivery_method) {
              service->add_and_start_content_stream(cs);
            }
          }

          if (is_new_service && service->content_streams().size() > 0) {
            _set_service(service_id, service);
          }
        }
      } catch (std::exception e) {
        spdlog::warn("MBMS user service desription parsing failed: {}", e.what());
      }
    }
  }
}
