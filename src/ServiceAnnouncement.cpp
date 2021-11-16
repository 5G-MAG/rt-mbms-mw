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

#include "spdlog/spdlog.h"
#include "gmime/gmime.h" 
#include "tinyxml2.h" 


MBMS_RT::ServiceAnnouncement::ServiceAnnouncement(const libconfig::Config& cfg, std::string tmgi, const std::string& mcast, unsigned long long tsi, std::string iface, boost::asio::io_service& io_service, CacheManagement& cache)
  : _cfg(cfg)
  , _tmgi(std::move(tmgi))
  , _tsi(tsi)
  , _iface(std::move(iface))
  , _io_service(io_service)
  , _cache(cache)
  , _flute_thread{}
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
      [&](std::shared_ptr<LibFlute::File> file) {
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
      /*
      if (type == "application/mbms-user-service-description+xml") {
        _service_description = g_mime_object_to_string(current, options);
      } else if (type == "application/sdp" && _sdp.empty()) {
        _sdp = g_mime_object_to_string(current, options);
        _sdp = std::regex_replace(_sdp, std::regex("^\n"), "");
      } else if (type == "application/vnd.apple.mpegurl") {
        _m3u = g_mime_object_to_string(current, options);
      }
      */
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
          auto& service = _services[service_id];

          // read the names
          for(auto* name = usd->FirstChildElement("name"); name != nullptr; name = name->NextSiblingElement("name")) {
            auto lang = name->Attribute("lang");
            auto namestr = name->GetText();
            service.add_name(namestr, lang);
          }

          // parse the appService element
          auto app_service = usd->FirstChildElement("r12:appService");
          auto alternative_content = app_service->FirstChildElement("r12:alternativeContent");
          for (auto* base_pattern = alternative_content->FirstChildElement("r12:basePattern"); 
               base_pattern != nullptr; 
               base_pattern = base_pattern->NextSiblingElement("r12:basePattern")) {
            std::string base = base_pattern->GetText();
            spdlog::warn("Alternative content at base pattern {}", base);

            // create a content stream
            auto cs = std::make_shared<ContentStream>(_iface, _io_service, _cache);
            bool have_delivery_method = false;

            // Check for 5GBC delivery method elements
            for(auto* delivery_method = usd->FirstChildElement("deliveryMethod"); 
                delivery_method != nullptr; 
                delivery_method = delivery_method->NextSiblingElement("deliveryMethod")) {
              auto sdp_uri = delivery_method->Attribute("sessionDescriptionURI");
              auto broadcast_app_service = delivery_method->FirstChildElement("r12:broadcastAppService");
              std::string broadcast_base_pattern = broadcast_app_service->FirstChildElement("r12:basePattern")->GetText();

              if (broadcast_base_pattern == base) {
                spdlog::warn("MATCHED Delivery method with SDP {} for broadcast base pattern {}", sdp_uri, broadcast_base_pattern);
                for (const auto& item : _items) {
                  if (item.content_type == "application/sdp" &&
                      item.uri == sdp_uri) {
                    cs->set_delivery_protocol_from_sdp_mime_type(item.content_type);
                    have_delivery_method = cs->configure_5gbc_delivery_from_sdp(item.content);
                  }
                }
              }
            }

            if (!have_delivery_method) {
              // assume base pattern is a CDN endpoint
              cs->set_cdn_endpoint(base);
            }

            // Check for identical content entries
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
                cs->set_cdn_endpoint(found_identical_base);
              }
            }

            service.add_and_start_content_stream(cs);
          }
        }
      } catch (std::exception e) {
        spdlog::warn("MBMS user service desription parsing failed: {}", e.what());
      }
    }
  }

#if 0
  if (!_service_description.empty()) {
    tinyxml2::XMLDocument doc;
    doc.Parse(_service_description.c_str());
    auto bundle = doc.FirstChildElement("bundleDescription");
    if (bundle) {
      auto usd = bundle->FirstChildElement("userServiceDescription");
      if (usd) {
        _service_name = usd->FirstChildElement("name")->GetText();
        spdlog::debug("Service Name: {}", _service_name);
      }
    }
  }

  if (!_sdp.empty()) {
    std::istringstream iss(_sdp);
    for (std::string line; std::getline(iss, line); )
    {
      const std::regex sdp_line_regex("^([a-z])\\=(.+)$");
      std::smatch match;
      if (std::regex_match(line, match, sdp_line_regex)) {
        if (match.size() == 3) {
          auto field = match[1].str();
          auto value = match[2].str();
          spdlog::debug("{}: {}", field, value);

          if (field == "c") {
            const std::regex value_regex("^IN (IP.) ([0-9\\.]+).*$");
            std::smatch cmatch;
            if (std::regex_match(value, cmatch, value_regex)) {
              if (cmatch.size() == 3) {
                _stream_mcast_addr = cmatch[2].str();
              }
            }
          } else if (field == "m") {
            const std::regex value_regex("^application (.+) (.+)$");
            std::smatch cmatch;
            if (std::regex_match(value, cmatch, value_regex)) {
              if (cmatch.size() == 3) {
                _stream_mcast_port = cmatch[1].str();
                _stream_type = cmatch[2].str();
              }
            }
            const std::regex value_regex2("^application (.+) (.+) (.+)$");
            if (std::regex_match(value, cmatch, value_regex2)) {
              if (cmatch.size() == 4) {
                _stream_mcast_port = cmatch[1].str();
                _stream_type = cmatch[2].str();
              }
            }
          } else if (field == "a") {
            const std::regex value_regex("^flute-tsi:(.+)$");
            std::smatch cmatch;
            if (std::regex_match(value, cmatch, value_regex)) {
              if (cmatch.size() == 2) {
                _stream_flute_tsi = stoul(cmatch[1].str());
              }
            }
          }
        }
      }
    }

    if (_stream_type != "none") {
      _bootstrap_file_parsed = true;
    }
  }
#endif
}
