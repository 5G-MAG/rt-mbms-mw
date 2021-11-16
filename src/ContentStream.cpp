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
#include "ContentStream.h"
#include "CdnClient.h"
#include "CacheItems.h"

#include "spdlog/spdlog.h"
#include "cpprest/base_uri.h"

using DeliveryProtocol = MBMS_RT::ContentStream::DeliveryProtocol;

MBMS_RT::ContentStream::ContentStream(std::string flute_if, boost::asio::io_service& io_service, CacheManagement& cache)
  : _5gbc_stream_iface( flute_if )
  , _io_service(io_service)
  , _cache(cache)
  , _flute_thread{}
{
  spdlog::debug("ContentStream created");
}

MBMS_RT::ContentStream::~ContentStream() {
  spdlog::debug("ContentStream destroyed");
}

auto MBMS_RT::ContentStream::configure_5gbc_delivery_from_sdp(const std::string& sdp) -> bool {
  spdlog::debug("ContentStream parsing SDP");
  std::istringstream iss(sdp);
  for (std::string line; std::getline(iss, line); )
  {
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


auto MBMS_RT::ContentStream::start() -> void {
  spdlog::info("ContentStream starting");
  if (_5gbc_stream_type == "FLUTE/UDP") {
    spdlog::info("Starting FLUTE receiver on {}:{} for TSI {}", _5gbc_stream_mcast_addr, _5gbc_stream_mcast_port, _5gbc_stream_flute_tsi);
    _flute_thread = std::thread{[&](){
      _flute_receiver = std::make_unique<LibFlute::Receiver>(_5gbc_stream_iface, _5gbc_stream_mcast_addr, 
          atoi(_5gbc_stream_mcast_port.c_str()), _5gbc_stream_flute_tsi, _io_service) ;

      _flute_receiver->register_completion_callback(
          [&](std::shared_ptr<LibFlute::File> file) {
          spdlog::info("ContentStream: {} (TOI {}, MIME type {}) has been received",
              file->meta().content_location, file->meta().toi, file->meta().content_type);

          if (file->meta().content_location == _playlist_path) {
            spdlog::info("ContentStream: got PLAYLIST at {}", file->meta().content_location);
            _playlist = std::string(file->buffer(), file->length());
          } else if (file->meta().content_location == "index.m3u8") {
           if (!_manifest.size()) {
            _manifest = std::string(file->buffer(), file->length());
            spdlog::info("ContentStream: got MANIFEST at {}:\n{}", file->meta().content_location, _manifest);
            _cache.add_item( std::make_shared<CachedPlaylist>(
                  "index.m3u8",
                  0,
                  [&]() -> const std::string& {
                    spdlog::info("ContentStream: manifest requested");
                    return _manifest;
                  }
                  ));
            }
          } else {
            spdlog::info("ContentStream: got SEGMENT at {}", file->meta().content_location);
            auto segment =
              std::make_shared<Segment>( 
                  file->meta().content_location,
                  file );

            if (_cdn_client) {
              segment->set_cdn_client(_cdn_client);
            }
            _segments[file->meta().content_location] = segment;

            _cache.add_item( std::make_shared<CachedSegment>(
                  file->meta().content_location, file->received_at(), segment )
                );
          }
      });

    }};
  }
}

auto MBMS_RT::ContentStream::set_cdn_endpoint(const std::string& cdn_ept) -> void
{
  web::uri uri(cdn_ept);
  web::uri_builder cdn_base(cdn_ept);

  std::string path = uri.path();
  _playlist_path = path.erase(0,1) + "?" + uri.query();
  spdlog::info("ContentStream: playlist location is {}", _playlist_path);

  cdn_base.set_path("");
  cdn_base.set_query("");
  _cdn_endpoint = cdn_base.to_string(); 
//  _cdn_endpoint = "http://192.168.188.120/demo-segments/";
  spdlog::info("ContentStream: setting CDN ept to {}", _cdn_endpoint);
  
  _cdn_client = std::make_shared<CdnClient>(_cdn_endpoint);

  _cache.add_item( std::make_shared<CachedPlaylist>(
        _playlist_path,
        0,
        [&]() -> const std::string& {
          spdlog::info("ContentStream: {} playlist requested", _playlist_path);
          return _playlist;
        }
        ));
};

auto MBMS_RT::ContentStream::set_delivery_protocol_from_sdp_mime_type(const std::string& mime_type) -> void
{
  _delivery_protocol = 
    ( mime_type == "application/vnd.apple.mpegurl" ? DeliveryProtocol::HLS : 
      ( mime_type == "application/dash+xml" ? DeliveryProtocol::DASH : DeliveryProtocol::RTP ) );
};
