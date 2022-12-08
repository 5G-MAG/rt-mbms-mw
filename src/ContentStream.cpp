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

#include <regex>
#include "ContentStream.h"
#include "CacheItems.h"
#include "HlsPrimaryPlaylist.h"

#include "spdlog/spdlog.h"
#include "cpprest/base_uri.h"

MBMS_RT::ContentStream::ContentStream(std::string base, std::string flute_if, boost::asio::io_service &io_service,
                                      CacheManagement &cache, DeliveryProtocol protocol, const libconfig::Config &cfg)
    : _5gbc_stream_iface(std::move(flute_if)), _cfg(cfg), _delivery_protocol(protocol), _base(std::move(base)),
      _io_service(io_service), _cache(cache), _flute_thread{} {
}

MBMS_RT::ContentStream::~ContentStream() {
  spdlog::debug("Destroying content stream at base {}", _base);
  if (_flute_receiver) {
    _flute_receiver->stop();
    _flute_thread.join();
  }
}

auto MBMS_RT::ContentStream::configure_5gbc_delivery_from_sdp(const std::string &sdp) -> bool {
  spdlog::debug("ContentStream parsing SDP");
  std::istringstream iss(sdp);
  for (std::string line; std::getline(iss, line);) {
    const std::regex sdp_line_regex("^([a-z])\\=(.+)$");
    std::smatch match;
    if (std::regex_match(line, match, sdp_line_regex)) {
      if (match.size() == 3) {
        auto field = match[1].str();
        auto value = match[2].str();
        spdlog::debug("SDP line {}: {}", field, value);

        if (field == "c") {
          const std::regex value_regex("^IN (IP.) ([0-9\\.]+).*$");
          std::smatch cmatch;
          if (std::regex_match(value, cmatch, value_regex)) {
            if (cmatch.size() == 3) {
              _5gbc_stream_mcast_addr = cmatch[2].str();
            }
          }
        } else if (field == "m") {
          const std::regex value_regex("^application (.+) (.+)$");
          std::smatch cmatch;
          if (std::regex_match(value, cmatch, value_regex)) {
            if (cmatch.size() == 3) {
              _5gbc_stream_mcast_port = cmatch[1].str();
              _5gbc_stream_type = cmatch[2].str();
            }
          }
          const std::regex value_regex2("^application (.+) (.+) (.+)$");
          if (std::regex_match(value, cmatch, value_regex2)) {
            if (cmatch.size() == 4) {
              _5gbc_stream_mcast_port = cmatch[1].str();
              _5gbc_stream_type = cmatch[2].str();
            }
          }
        } else if (field == "a") {
          const std::regex value_regex("^flute-tsi:(.+)$");
          std::smatch cmatch;
          if (std::regex_match(value, cmatch, value_regex)) {
            if (cmatch.size() == 2) {
              _5gbc_stream_flute_tsi = stoul(cmatch[1].str());
            }
          }
        }
      }
    }
  }
  if (!_5gbc_stream_type.empty() && !_5gbc_stream_mcast_addr.empty() &&
      !_5gbc_stream_mcast_port.empty()) {
    spdlog::info("ContentStream SDP parsing complete. Stream type {}, TSI {}, MCast at {}:{}",
                 _5gbc_stream_type, _5gbc_stream_flute_tsi, _5gbc_stream_mcast_addr, _5gbc_stream_mcast_port);
    return true;
  }
  return false;
}

auto MBMS_RT::ContentStream::flute_file_received(std::shared_ptr<LibFlute::File> file) -> void {
  spdlog::info("ContentStream: {} (TOI {}, MIME type {}) has been received at {}",
               file->meta().content_location, file->meta().toi, file->meta().content_type, file->received_at());
  if (file->meta().content_location != "index.m3u8") { // ignore generated manifests
    std::string content_location = file->meta().content_location;
    // DASH: Add BaseURl to the content location of the received segments otherwise we do not have a cache match later
    if (_delivery_protocol == MBMS_RT::DeliveryProtocol::DASH) {
      content_location = _base_path + content_location;
    }

    //if this is an MPD also update our internal representation of the MPD
    if (file->meta().content_location.find(".mpd") != std::string::npos) {
      _cache.add_item(std::make_shared<CachedFile>(
          _base_path + "manifest.mpd", file->received_at(), std::move(file))
      );
    } else {
      _cache.add_item(std::make_shared<CachedFile>(
          content_location, file->received_at(), std::move(file))
      );
    }

  }
}

auto MBMS_RT::ContentStream::start() -> void {
  spdlog::info("ContentStream starting");
  if (_5gbc_stream_type == "FLUTE/UDP") {
    spdlog::info("Starting FLUTE receiver on {}:{} for TSI {}", _5gbc_stream_mcast_addr, _5gbc_stream_mcast_port,
                 _5gbc_stream_flute_tsi);
    _flute_thread = std::thread{[&]() {
      _flute_receiver = std::make_unique<LibFlute::Receiver>(_5gbc_stream_iface, _5gbc_stream_mcast_addr,
                                                             atoi(_5gbc_stream_mcast_port.c_str()),
                                                             _5gbc_stream_flute_tsi, _io_service);
      _flute_receiver->register_completion_callback(
          boost::bind(&ContentStream::flute_file_received, this, _1)); //NOLINT
    }};
  }
};

auto MBMS_RT::ContentStream::read_master_manifest(const std::string &manifest) -> void {
  if (_delivery_protocol == DeliveryProtocol::HLS) {
    auto pl = HlsPrimaryPlaylist(manifest, "");
    if (pl.streams().size() == 1) {
      auto stream = pl.streams()[0];
      _playlist_path = stream.uri;
    } else {
      spdlog::error("Error: HLS primary playlist for stream contains more than one stream definitions. Ignoring.");
    }
  }
}

auto MBMS_RT::ContentStream::flute_info() const -> std::string {
  if (_5gbc_stream_type == "none") {
    return "n/a";
  } else {
    return _5gbc_stream_type + ": " + _5gbc_stream_mcast_addr + ":" + _5gbc_stream_mcast_port +
           ", TSI " + std::to_string(_5gbc_stream_flute_tsi);
  }
}
