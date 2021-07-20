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

#include <regex>
#include "Service.h"
#include "Receiver.h"

#include "spdlog/spdlog.h"
#include "gmime/gmime.h" 
#include "tinyxml2.h" 


OBECA::Service::Service(const libconfig::Config& cfg, const std::string& tmgi, const std::string& mcast, unsigned long long tsi, const std::string& iface, boost::asio::io_service& io_service)
  : _cfg(cfg)
  , _tmgi(tmgi)
  , _tsi(tsi)
  , _iface(iface)
  , _flute_thread{}
{
  size_t delim = mcast.find(':');
  if (delim == std::string::npos) {
    spdlog::error("Invalid multicast address {}", mcast);
    return;
  }
  std::string base_path = "/tmp/obeca";
  cfg.lookupValue("gw.cache.base_path", base_path);
  mkdir(base_path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); 
  _target_directory = base_path + "/" + tmgi;
  mkdir(_target_directory.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); 

  if (nullptr != std::getenv("GW_CACHE_LOCATION")) {
    _target_directory = std::getenv("GW_CACHE_LOCATION");
  }
  _mcast_addr = mcast.substr(0, delim);
  _mcast_port = mcast.substr(delim + 1);
  spdlog::info("Starting FLUTE receiver on {}:{} for TSI {}, target dir {}", _mcast_addr, _mcast_port, _tsi, _target_directory);
  _flute_thread = std::thread{[&](){
    _flute_receiver = std::make_unique<LibFlute::Receiver>(_iface, _mcast_addr, atoi(_mcast_port.c_str()), _tsi, io_service) ;
  }};
}

OBECA::Service::~Service() {
  spdlog::info("Closing service with TMGI {}", _tmgi);
  _flute_receiver.reset();
  if (_flute_thread.joinable()) {
    _flute_thread.join();
  }
}

auto OBECA::Service::fileList() -> std::vector<std::shared_ptr<LibFlute::File>>
{
  if (_flute_receiver) {
    return _flute_receiver->file_list();
  } else {
    return std::vector<std::shared_ptr<LibFlute::File>>();
  }
}

auto OBECA::Service::tryParseBootstrapFile(std::string str) -> void
{
  g_mime_init();

  // R&S header has no newlines, fix that
  str = std::regex_replace(str, std::regex(" Content-Type:"), "\nContent-Type:");
  str = std::regex_replace(str, std::regex(" Content-Description:"), "\nContent-Description:");

  auto stream = g_mime_stream_mem_new_with_buffer(str.c_str(), str.length());
  auto parser = g_mime_parser_new_with_stream(stream);
  g_object_unref(stream);

  auto mpart = g_mime_parser_construct_part(parser, nullptr);
  g_object_unref(parser);

  auto iter = g_mime_part_iter_new(mpart);
  auto ct = g_mime_object_get_content_type_parameter(mpart, "type");
  while (g_mime_part_iter_next(iter)) {
    GMimeObject *current = g_mime_part_iter_get_current (iter);
    GMimeObject *parent = g_mime_part_iter_get_parent (iter);

    if (GMIME_IS_MULTIPART (parent) && GMIME_IS_PART (current)) {
      auto type = std::string(g_mime_content_type_get_mime_type(g_mime_object_get_content_type(current)));
      auto options = g_mime_format_options_new();
      g_mime_format_options_add_hidden_header(options, "Content-Type");
      g_mime_format_options_add_hidden_header(options, "Content-Transfer-Encoding");
      g_mime_format_options_add_hidden_header(options, "Content-Location");

      if (type == "application/mbms-user-service-description+xml") {
        _service_description = g_mime_object_to_string(current, options);
      } else if (type == "application/sdp") {
        _sdp = g_mime_object_to_string(current, options);
        _sdp = std::regex_replace(_sdp, std::regex("^\n"), "");
      } else if (type == "application/vnd.apple.mpegurl") {
        _m3u = g_mime_object_to_string(current, options);
      }
    }
  }

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
      const std::regex sdp_line_regex("^([a-z])\=(.+)$");
      std::smatch match;
      if (std::regex_match(line, match, sdp_line_regex)) {
        if (match.size() == 3) {
          auto field = match[1].str();
          auto value = match[2].str();
          spdlog::debug("{}: {}", field, value);

          if (field == "c") {
            const std::regex value_regex("^IN (IP.) ([0-9\.]+).*$");
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
}
auto OBECA::Service::remove_expired_files(unsigned max_age) -> void
{
  if (_flute_receiver) {
    _flute_receiver->remove_expired_files(max_age);
  }
}
